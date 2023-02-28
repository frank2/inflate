#include <inflate.hpp>

using namespace inflate;

std::pair<ByteVec, InflateHeader> inflate::inflate_memory(const void *ptr, std::uint64_t size, InflateLevel level, std::optional<std::uint32_t> seed) {
   auto u8_ptr = reinterpret_cast<const std::uint8_t *>(ptr);

   BitVec padding_bits;
   std::uint64_t modulus = 0;
   std::uint64_t inflate_size;

   if (!seed.has_value())
   {
      std::srand(std::time(nullptr));
      seed = static_cast<std::uint32_t>(std::rand());
   }
   
   if (level <= InflateLevel::INFLATE_7BIT)
   {
      modulus = 8 - level;
      padding_bits = BitVec(8 - modulus, false);
      inflate_size = (size * 8) + ((size * 8) / modulus) * padding_bits.size();
   }
   else
   {
      if (level <= InflateLevel::INFLATE_RNG_PARTIAL_7BIT)
         modulus = 8 - (level - InflateLevel::INFLATE_7BIT);
      else
         modulus = 8 - (level - InflateLevel::INFLATE_RNG_PARTIAL_7BIT);

      padding_bits = BitVec(8 - modulus, false);
      inflate_size = (size * 8) + ((size * 8) / modulus) * padding_bits.size();

      if (inflate_size % 8 != 0)
         inflate_size += 8 - inflate_size % 8;
   }

   auto deflate_stream = BitstreamPtr(u8_ptr, size*8);
   auto inflate_stream = BitstreamVec(inflate_size);

   if (level == InflateLevel::INFLATE_NOOP)
   {
      inflate_stream = BitstreamVec(deflate_stream.data(), deflate_stream.bit_size());
   }
   else if (level <= InflateLevel::INFLATE_7BIT)
   {
      std::uint64_t inflate_offset = 0;
      
      for (std::uint64_t i=0; i<deflate_stream.bit_size(); i+=modulus)
      {
         auto read_size = (deflate_stream.bit_size() - i > modulus) ? modulus : deflate_stream.bit_size() - i;

         for (std::size_t j=0; j<read_size; ++j)
            inflate_stream.set_bit(inflate_offset++, deflate_stream.get_bit(i+j));
         
         if (read_size == modulus)
            for (std::size_t j=0; j<padding_bits.size(); ++j)
               inflate_stream.set_bit(inflate_offset++, padding_bits[j]);
      }
   }
   else if (level <= InflateLevel::INFLATE_RNG_PARTIAL_7BIT)
   {
      auto lfsr = ShiftRegister(*seed);
      std::uint64_t inflate_offset = 0;
      
      for (std::uint64_t i=0; i<deflate_stream.bit_size(); i+=modulus)
      {
         auto read_size = (deflate_stream.bit_size() - i > modulus) ? modulus : deflate_stream.bit_size() - i;
         auto inject_index = *lfsr % read_size;

         for (std::size_t j=0; j<read_size; ++j)
         {
            if (j == inject_index)
            {
               inflate_stream.write_bits(inflate_offset, padding_bits);
               inflate_offset += padding_bits.size();
            }
            
            inflate_stream.set_bit(inflate_offset++, deflate_stream.get_bit(i+j));
         }
      }
   }
   else
   {
      auto lfsr = ShiftRegister(*seed);
      std::uint64_t inflate_offset = 0;
      
      for (std::uint64_t i=0; i<deflate_stream.bit_size(); i+=modulus)
      {
         auto read_size = (deflate_stream.bit_size() - i > modulus) ? modulus : deflate_stream.bit_size() - i;
         bool relevant_bits[8] = { false, false, false, false, false, false, false, false };
         std::size_t target_bits = 0;

         while (target_bits < read_size)
         {
            auto index = *lfsr % 8;

            if (relevant_bits[index])
               continue;

            relevant_bits[index] = true;
            ++target_bits;
         }

         std::size_t bit_offset = 0;

         for (std::size_t j=0; j<8; ++j)
         {
            if (relevant_bits[j])
               inflate_stream.set_bit(inflate_offset++, deflate_stream.get_bit(i+bit_offset++));
            else
               inflate_stream.set_bit(inflate_offset++, false);
         }
      }
   }
   
   InflateHeader header;

   header.level = level;
   header.inflated = inflate_stream.bit_size();
   header.deflated = deflate_stream.bit_size();
   header.checksum = crc32(ptr, size);
   header.seed = *seed;

   return std::make_pair(inflate_stream.to_bytevec(), header);
}

std::pair<ByteVec, InflateHeader> inflate::inflate_memory(const ByteVec &vec, InflateLevel level, std::optional<std::uint32_t> seed) {
   return inflate_memory(vec.data(), vec.size(), level, seed);
}

ByteVec inflate::deflate_memory(const void *ptr, std::size_t size, const InflateHeader &header, bool validate) {
   auto inflated_bytes = header.inflated / 8 + static_cast<std::size_t>(header.inflated % 8 != 0);
   auto deflated_bytes = header.deflated / 8;

   if (size != inflated_bytes)
      throw exception::InsufficientSize(size, inflated_bytes);

   auto inflate_stream = BitstreamPtr(reinterpret_cast<const std::uint8_t *>(ptr), header.inflated);
   auto deflate_stream = BitstreamVec(header.deflated);

   switch (header.level)
   {
   case INFLATE_NOOP:
      deflate_stream.write_bits(0, inflate_stream.to_bitvec());
      break;

   case INFLATE_1BIT:
   case INFLATE_2BIT:
   case INFLATE_3BIT:
   case INFLATE_4BIT:
   case INFLATE_5BIT:
   case INFLATE_6BIT:
   case INFLATE_7BIT:
   {
      auto modulus = 8 - header.level;
      std::size_t deflate_offset = 0;

      for (std::size_t inflate_offset=0; inflate_offset<header.inflated; inflate_offset+=8)
      {
         std::size_t read_size = ((header.inflated - inflate_offset > modulus) ? modulus : header.inflated - inflate_offset);

         for (std::size_t i=0; i<read_size; ++i)
            deflate_stream.set_bit(deflate_offset++, inflate_stream.get_bit(inflate_offset+i));
      }

      break;
   }

   case INFLATE_RNG_PARTIAL_1BIT:
   case INFLATE_RNG_PARTIAL_2BIT:
   case INFLATE_RNG_PARTIAL_3BIT:
   case INFLATE_RNG_PARTIAL_4BIT:
   case INFLATE_RNG_PARTIAL_5BIT:
   case INFLATE_RNG_PARTIAL_6BIT:
   case INFLATE_RNG_PARTIAL_7BIT:
   {
      auto modulus = 8 - (header.level - InflateLevel::INFLATE_7BIT);
      auto padding = 8 - modulus;
      auto lfsr = ShiftRegister(header.seed);
      std::size_t deflate_offset = 0;
      
      for (std::size_t inflate_offset=0; inflate_offset<inflate_stream.bit_size(); inflate_offset+=8)
      {
         auto read_size = (header.deflated - deflate_offset > modulus) ? modulus : header.deflated - deflate_offset;
         auto inject_index = *lfsr % read_size;
       
         for (std::size_t i=0; i<read_size+padding; ++i)
         {
            if (i >= inject_index && i < inject_index+padding)
               continue;
            
            deflate_stream.set_bit(deflate_offset++, inflate_stream.get_bit(inflate_offset+i));
         }
      }

      break;
   }

   case INFLATE_RNG_FULL_1BIT:
   case INFLATE_RNG_FULL_2BIT:
   case INFLATE_RNG_FULL_3BIT:
   case INFLATE_RNG_FULL_4BIT:
   case INFLATE_RNG_FULL_5BIT:
   case INFLATE_RNG_FULL_6BIT:
   case INFLATE_RNG_FULL_7BIT:
   {
      auto modulus = 8 - (header.level - InflateLevel::INFLATE_RNG_PARTIAL_7BIT);
      auto lfsr = ShiftRegister(header.seed);
      std::size_t deflate_offset = 0;

      for (std::size_t inflate_offset=0; inflate_offset<header.inflated; inflate_offset+=8)
      {
         std::size_t read_size = (header.deflated - deflate_offset > modulus) ? modulus : header.deflated - deflate_offset;
         bool relevant_bits[8] = { false, false, false, false, false, false, false, false };
         std::size_t target_bits = 0;
         
         while (target_bits < read_size)
         {
            auto index = *lfsr % 8;

            if (relevant_bits[index])
               continue;

            relevant_bits[index] = true;
            ++target_bits;
         }

         for (std::size_t i=0; i<8; ++i)
         {
            if (!relevant_bits[i])
               continue;

            deflate_stream.set_bit(deflate_offset++, inflate_stream.get_bit(inflate_offset+i));
         }
      }

      break;
   }

   default:
      throw exception::UnsupportedInflateLevel(header.level);
   }

   if (validate)
   {
      auto crc = crc32(deflate_stream.data(), deflate_stream.byte_size());

      if (crc != header.checksum)
         throw exception::BadCRC(crc, header.checksum);
   }

   return deflate_stream.to_bytevec();
}
      
ByteVec inflate::deflate_memory(const ByteVec &vec, const InflateHeader &header, bool validate) {
   return inflate::deflate_memory(vec.data(), vec.size(), header, validate);
}

ByteVec inflate::inflate_disk(const void *ptr, std::uint64_t size, InflateLevel level, std::optional<std::uint32_t> seed) {
   auto mem = inflate::inflate_memory(ptr, size, level, seed);
   auto header = mem.second;
   auto magic = INFLATE_MAGIC;
   ByteVec inflate_vec;

   inflate_vec.insert(inflate_vec.end(),
                      magic,
                      magic+std::strlen(magic));
   inflate_vec.insert(inflate_vec.end(),
                      reinterpret_cast<std::uint8_t *>(&header),
                      reinterpret_cast<std::uint8_t *>(&header)+(sizeof(InflateHeader)));
   inflate_vec.insert(inflate_vec.end(),
                      mem.first.data(),
                      mem.first.data()+mem.first.size());

   return inflate_vec;
}

ByteVec inflate::inflate_disk(const ByteVec &vec, InflateLevel level, std::optional<std::uint32_t> seed) {
   return inflate::inflate_disk(vec.data(), vec.size(), level, seed);
}

ByteVec inflate::deflate_disk(const void *ptr, std::uint64_t size) {
   if (size < sizeof(InflateHeader)-1)
      throw exception::InsufficientSize(size, sizeof(InflateHeader));

   if (std::memcmp(ptr, INFLATE_MAGIC, std::strlen(INFLATE_MAGIC)) != 0)
      throw exception::BadHeaderMagic();

   auto u8_ptr = reinterpret_cast<const std::uint8_t *>(ptr);
   auto header = reinterpret_cast<const InflateHeader *>(u8_ptr+std::strlen(INFLATE_MAGIC));

   return inflate::deflate_memory(u8_ptr+std::strlen(INFLATE_MAGIC)+sizeof(InflateHeader),
                                  size-std::strlen(INFLATE_MAGIC)-sizeof(InflateHeader),
                                  *header);
}

ByteVec inflate::deflate_disk(const ByteVec &vec) {
   return inflate::deflate_disk(vec.data(), vec.size());
}
