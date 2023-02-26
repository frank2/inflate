#include <inflate.hpp>

using namespace inflate;

BitVec inflate::to_bitvec(const ByteVec &byte_vec) {
   BitVec result;

   for (std::size_t i=0; i<byte_vec.size()*8; ++i)
   {
      auto byte_index = i / 8;
      auto bit_index = i % 8;
      
      result.push_back(static_cast<bool>((byte_vec[byte_index] >> bit_index) & 1));
   }

   return result;
}

ByteVec inflate::to_bytevec(const BitVec &bit_vec) {
   auto bytes = bit_vec.size()/8 + static_cast<std::size_t>(bit_vec.size() % 8 != 0);
   ByteVec result(bytes);

   for (std::size_t i=0; i<bit_vec.size(); ++i)
   {
      auto byte_offset = i / 8;
      auto bit_offset = i % 8;
      
      result[byte_offset] |= static_cast<std::uint8_t>(bit_vec[i]) << bit_offset;
   }

   return result;
}

BitstreamPtr::reference BitstreamPtr::operator[](std::size_t index)
{
   if (index >= this->bit_size())
      throw exception::OutOfBounds(index, this->bit_size());
   
   return BitstreamPtr::reference(this, index);
}

bool BitstreamPtr::operator[](std::size_t index) const
{
   if (index >= this->bit_size())
      throw exception::OutOfBounds(index, this->bit_size());
   
   return this->get_bit(index);
}

bool BitstreamPtr::get_bit(std::size_t index) const {
   if (this->_data == nullptr)
      throw exception::NullPointer();
   
   if (index >= this->bit_size())
      throw exception::OutOfBounds(index, this->bit_size());

   auto byte_offset = index / 8;
   auto bit_offset = index % 8;

   return static_cast<bool>((this->_data[byte_offset] >> bit_offset) & 1);
}

void BitstreamPtr::set_bit(std::size_t index, bool bit) {
   if (this->_data == nullptr)
      throw exception::NullPointer();

   if (index >= this->bit_size())
      throw exception::OutOfBounds(index, this->bit_size());

   auto byte_offset = index / 8;
   auto bit_offset = index % 8;
   std::uint8_t bit_mask = (1 << bit_offset) ^ 0xFF;

   this->_data[byte_offset] = (this->_data[byte_offset] & bit_mask) | (static_cast<std::uint8_t>(bit) << bit_offset);
}

std::uint8_t &BitstreamPtr::get_byte(std::size_t index) const {
   if (this->_data == nullptr)
      throw exception::NullPointer();

   if (index >= this->byte_size())
      throw exception::OutOfBounds(index, this->byte_size());

   return this->_data[index];
}

void BitstreamPtr::set_byte(std::size_t index, std::uint8_t byte) {
   if (this->_data == nullptr)
      throw exception::NullPointer();

   if (index >= this->byte_size())
      throw exception::OutOfBounds(index, this->byte_size());

   this->_data[index] = byte;
}

std::size_t BitstreamPtr::bit_size() const { return this->_size; }
std::size_t BitstreamPtr::byte_size() const { return this->_size / 8 + static_cast<std::size_t>(this->_size % 8 != 0); }

std::uint8_t *BitstreamPtr::data() { return this->_data; }
const std::uint8_t *BitstreamPtr::data() const { return this->_data; }

void BitstreamPtr::set_data(std::uint8_t *data, std::size_t size) {
   this->_data = data;
   this->_size = size;
}

BitVec BitstreamPtr::read_bits(std::size_t index, std::size_t size) const {
   BitVec result;

   if (index+size > this->bit_size())
      throw exception::OutOfBounds(index+size, this->bit_size());

   for (std::size_t i=index; i<index+size; ++i)
      result.push_back(this->get_bit(i));

   return result;
}

void BitstreamPtr::write_bits(std::size_t index, const BitVec &bits) {
   if (index+bits.size() > this->bit_size())
      throw exception::OutOfBounds(index+bits.size(), this->bit_size());

   for (std::size_t i=index; i<index+bits.size(); ++i)
      this->set_bit(i, bits[i-index]);
}

void BitstreamPtr::write_bits(std::size_t index, const BitstreamPtr &bits) {
   if (index+bits.bit_size() > this->bit_size())
      throw exception::OutOfBounds(index+bits.bit_size(), this->bit_size());

   for (std::size_t i=index; i<index+bits.bit_size(); ++i)
      this->set_bit(i, bits[i-index]);
}

BitVec BitstreamPtr::to_bitvec() const { return this->read_bits(0, this->_size); }
ByteVec BitstreamPtr::to_bytevec() const {
   if (this->_data == nullptr)
      throw exception::NullPointer();
   
   return ByteVec(this->_data, this->_data+this->byte_size());
}

void BitstreamVec::resize(std::size_t bits) {
   std::intptr_t delta = static_cast<std::intptr_t>(bits) - this->_size;
   auto old = this->_size;

   if (delta < 0 && bits % 8 != 0)
   {
      auto trailing_bits = 8 - bits % 8;
      auto trailing_mask = 0xFF >> trailing_bits;
      auto byte_offset = bits / 8;
      this->_vec[byte_offset] &= trailing_mask;
   }
   
   this->_size = bits;

   if (this->byte_size() != this->_vec.size())
   {
      this->_vec.resize(this->byte_size());
      this->set_data(this->_vec.data(), this->_size);
   }

   if (delta > 0)
      this->write_bits(old, BitVec(delta, false));
}

void BitstreamVec::push_bit(bool bit) {
   auto index = this->_size;
   this->resize(this->_size+1);
   
   this->set_bit(index, bit);
}

void BitstreamVec::push_bits(const BitVec &bits) {
   auto index = this->_size;
   this->resize(this->_size+bits.size());

   this->write_bits(index, bits);
}

bool BitstreamVec::pop_bit() {
   if (this->bit_size() == 0)
      throw exception::NoBits();

   auto bit = this->get_bit(this->_size-1);
   this->resize(this->_size-1);

   return bit;
}

BitVec BitstreamVec::pop_bits(std::size_t bits) {
   if (bits > this->bit_size())
      throw exception::OutOfBounds(bits, this->bit_size());

   auto result = this->read_bits(this->bit_size()-bits, bits);
   std::reverse(result.begin(), result.end());

   this->resize(this->bit_size() - bits);

   return result;
}

void BitstreamVec::insert_bit(std::size_t index, bool bit) {
   BitVec vec; vec.push_back(bit);
   this->insert_bits(index, vec);
}

void BitstreamVec::insert_bits(std::size_t index, const BitVec &bits) {
   if (index > this->_size)
      throw exception::OutOfBounds(index, this->_size);
   
   if (index == this->_size)
   {
      this->push_bits(bits);
      return;
   }

   auto rest = this->_size - index;
   auto end_bits = this->read_bits(index, rest);
   
   this->resize(this->_size + bits.size());
   this->write_bits(index, bits);
   this->write_bits(index+bits.size(), end_bits);
}

void BitstreamVec::erase_bit(std::size_t index) {
   this->erase_bits(index, 1);
}

void BitstreamVec::erase_bits(std::size_t index, std::size_t size) {
   if (index+size > this->bit_size())
      throw exception::OutOfBounds(index+size, this->bit_size());

   if (index+size == this->bit_size())
   {
      this->resize(this->bit_size()-size);
      return;
   }

   auto end_offset = index + size;
   auto rest = this->_size - end_offset;
   auto bits = this->read_bits(end_offset, rest);

   this->resize(this->bit_size()-size);
   this->write_bits(index, bits);
}
