#ifndef __INFLATE_BITSTREAM_HPP
#define __INFLATE_BITSTREAM_HPP

#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include <iostream>

#include <inflate/platform.hpp>
#include <inflate/exception.hpp>

namespace inflate
{
   using BitVec = std::vector<bool>;
   using ByteVec = std::vector<std::uint8_t>;

   EXPORT BitVec to_bitvec(const ByteVec &byte_vec);
   EXPORT ByteVec to_bytevec(const BitVec &bit_vec);

   class BitstreamVec;
   
   EXPORT
   class BitstreamPtr
   {
   protected:
      union {
         std::uint8_t *m;
         const std::uint8_t *c;
      } _data;
      
      std::uint64_t _size;

      bool _const;

   public:
      class reference {
         friend BitstreamPtr;
         
         BitstreamPtr *_stream;
         std::uint64_t _index;

         reference(BitstreamPtr *stream, std::uint64_t index) : _stream(stream), _index(index) {}

      public:
         reference() : _stream(nullptr), _index(0) {}
         reference(const reference &other) : _stream(other._stream), _index(other._index) {}

         operator bool() const {
            if (this->is_null())
               throw exception::NullPointer();
         
            return this->_stream->get_bit(this->_index);
         }
         reference &operator=(bool value) {
            if (this->is_null())
               throw exception::NullPointer();
            
            this->_stream->set_bit(this->_index, value);
            return *this;
         }
         reference &operator=(const reference &ref) { return *this = bool(ref); }
         bool operator==(const reference &other) const { return bool(*this) == bool(other); }
         bool operator<(const reference &other) const { return !bool(*this) && bool(other); }
         reference &operator++() { this->_index++; return *this; }
         reference &operator--() { this->_index--; return *this; }
         reference operator++(int) { auto tmp = *this; ++(*this); return tmp; }
         reference operator--(int) { auto tmp = *this; --(*this); return tmp; }

         bool is_null() const { return this->_stream == nullptr; }
         std::uint64_t index() const { return this->_index; }
      };

      class iterator {
         friend BitstreamPtr;
         
         BitstreamPtr *_stream;
         std::uint64_t _index;

         iterator(BitstreamPtr *stream, std::uint64_t index) : _stream(stream), _index(index) {}
         
      public:
         using iterator_category = std::forward_iterator_tag;
         using difference_type = std::ptrdiff_t;
         using value_type = bool;
         using pointer = BitstreamPtr::reference *;
         using reference = BitstreamPtr::reference;

         iterator() : _stream(nullptr), _index(0) {}
         iterator(const iterator &other) : _stream(other._stream), _index(other._index) {}

         iterator &operator=(const iterator &other) { this->_stream = other._stream; this->_index = other._index; }
         bool operator==(const iterator &other) const { return this->_stream == other._stream && this->_index == other._index; }
         bool operator!=(const iterator &other) const { return !(*this == other); }

         iterator &operator++() { this->_index++; return *this; }
         iterator operator++(int) { auto tmp = *this; ++(*this); return tmp; }

         iterator &operator--() { this->_index--; return *this; }
         iterator operator--(int) { auto tmp = *this; --(*this); return tmp; }

         reference operator*() {
            if (this->_stream == nullptr)
               throw exception::NullPointer();
            
            return this->_stream->operator[](this->_index);
         }
         const reference operator*() const {
            if (this->_stream == nullptr)
               throw exception::NullPointer();
            
            return this->_stream->operator[](this->_index);
         }
      };

      class const_iterator {
         friend BitstreamPtr;
         
         const BitstreamPtr *_stream;
         std::uint64_t _index;

         const_iterator(const BitstreamPtr *stream, std::uint64_t index) : _stream(stream), _index(index) {}
         
      public:
         using iterator_category = std::forward_iterator_tag;
         using difference_type = std::ptrdiff_t;
         using value_type = bool;
         using pointer = bool *;
         using reference = bool;

         const_iterator() : _stream(nullptr), _index(0) {}
         const_iterator(const const_iterator &other) : _stream(other._stream), _index(other._index) {}

         const_iterator &operator=(const const_iterator &other) { this->_stream = other._stream; this->_index = other._index; }
         bool operator==(const const_iterator &other) const { return this->_stream == other._stream && this->_index == other._index; }
         bool operator!=(const const_iterator &other) const { return !(*this == other); }
         
         const_iterator &operator++() { this->_index++; return *this; }
         const_iterator operator++(int) { auto tmp = *this; ++(*this); return tmp; }
         
         const_iterator &operator--() { this->_index--; return *this; }
         const_iterator operator--(int) { auto tmp = *this; --(*this); return tmp; }

         const reference operator*() const {
            if (this->_stream == nullptr)
               throw exception::NullPointer();
            
            return this->_stream->get_bit(this->_index);
         }
      };

      BitstreamPtr() : _size(0), _const(false) { this->_data.m = nullptr; }
      BitstreamPtr(std::uint8_t *data, std::uint64_t size) : _size(size), _const(false) { this->_data.m = data; }
      BitstreamPtr(const std::uint8_t *data, std::uint64_t size) : _size(size), _const(true) { this->_data.c = data; }
      BitstreamPtr(const BitstreamPtr &other) : _data(other._data), _size(other._size), _const(other._const) {}

      BitstreamPtr &operator=(const BitstreamPtr &other);
      reference operator[](std::uint64_t index);
      bool operator[](std::uint64_t index) const;
      bool operator==(const BitstreamPtr &other) const;
      bool operator!=(const BitstreamPtr &other) const { return !(*this == other); }
      bool operator==(const BitVec &other) const;
      bool operator!=(const BitVec &other) const { return !(*this == other); }

      iterator begin() { return iterator(this, 0); }
      iterator end() { return iterator(this, this->_size); }

      const_iterator cbegin() const { return const_iterator(this, 0); }
      const_iterator cend() const { return const_iterator(this, this->_size); }

      bool get_bit(std::uint64_t index) const;
      void set_bit(std::uint64_t index, bool bit);

      std::uint8_t get_byte(std::uint64_t index) const;
      void set_byte(std::uint64_t index, std::uint8_t byte);

      std::uint64_t bit_size() const;
      std::uint64_t byte_size() const;

      const std::uint8_t *data() const;
      void set_data(std::uint8_t *data, std::uint64_t size);
      void set_data(const std::uint8_t *data, std::uint64_t size);

      BitstreamVec read_bits(std::uint64_t index, std::uint64_t size) const;
      void write_bits(std::uint64_t index, const BitVec &bits);
      void write_bits(std::uint64_t index, const BitstreamPtr &bits);

      BitVec to_bitvec() const;
      ByteVec to_bytevec() const;
      BitstreamVec to_vec() const;
   };

   EXPORT
   class BitstreamVec : public BitstreamPtr
   {
   protected:
      ByteVec _vec;
      
   public:
      BitstreamVec() : _vec(ByteVec()), BitstreamPtr() { this->set_data(this->_vec.data(), this->_vec.size()*8); }
      BitstreamVec(std::uint64_t size) : BitstreamPtr() {
         this->_vec = ByteVec(size/8 + static_cast<std::uint64_t>(size % 8 != 0), 0);
         this->set_data(this->_vec.data(), size);
      }
      BitstreamVec(const std::uint8_t *data, std::uint64_t size)
         : _vec(data, data+(size/8+static_cast<std::uint64_t>(size%8!=0))),
           BitstreamPtr()
      {
         this->set_data(this->_vec.data(), size);
      }
      BitstreamVec(const ByteVec &vec, std::uint64_t size) : _vec(vec), BitstreamPtr() { this->set_data(this->_vec.data(), size); }
      BitstreamVec(const BitstreamVec &other) : _vec(other._vec), BitstreamPtr(other) { this->set_data(this->_vec.data(), this->_size); }

      BitstreamVec &operator=(const BitstreamVec &other);

      void resize(std::uint64_t bits);

      void push_bit(bool bit);
      void push_bits(const BitVec &bits);

      bool pop_bit();
      BitstreamVec pop_bits(std::uint64_t bits);

      void insert_bit(std::uint64_t index, bool bit);
      void insert_bits(std::uint64_t index, const BitVec &bits);

      void erase_bit(std::uint64_t index);
      void erase_bits(std::uint64_t index, std::uint64_t size);
   };
}

#endif
