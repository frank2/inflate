#include <inflate.hpp>

using namespace inflate;

ByteVec inflate::inflate(const void *ptr, std::size_t size, InflateLevel level) {
   auto u8_ptr = reinterpret_cast<const std::uint8_t *>(ptr);

   BitVec padding_bits;
   std::size_t modulus = 0;
   
   if (level <= InflateLevel::INFLATE_7BIT)
   {
      padding_bits = BitVec(level, false);
      modulus = 8 - level;
   }

   auto deflate_stream = BitstreamPtr(const_cast<std::uint8_t *>(u8_ptr), size*8);
   auto inflate_stream = BitstreamVec();

   if (level == InflateLevel::INFLATE_NOOP)
   {
      inflate_stream = BitstreamVec(deflate_stream.data(), deflate_stream.bit_size());
   }
   else
   {
      for (std::size_t i=0; i<deflate_stream.bit_size(); i+=modulus)
      {
         auto read_size = (deflate_stream.bit_size() - i > modulus) ? modulus : deflate_stream.bit_size() - i;
         inflate_stream.push_bits(deflate_stream.read_bits(i, read_size));

         if (read_size == modulus)
            inflate_stream.push_bits(padding_bits);
      }
   }

   ByteVec inflate_data;
   InflateHeader header;

   std::memcpy(&header.magic, INFLATE_MAGIC, 4);
   header.level = level;
   header.inflated = inflate_stream.bit_size();
   header.deflated = deflate_stream.bit_size();
   header.checksum = crc32(ptr, size);
   // TODO: implement random insertions with this initial seed
   header.reserved = 0;

   inflate_data.insert(inflate_data.end(),
                       reinterpret_cast<std::uint8_t *>(&header),
                       reinterpret_cast<std::uint8_t *>(&header)+(sizeof(InflateHeader)-1));
   inflate_data.insert(inflate_data.end(),
                       inflate_stream.data(),
                       inflate_stream.data()+inflate_stream.byte_size());

   return inflate_data;
}

ByteVec inflate::inflate(const ByteVec &vec, InflateLevel level) {
   return inflate(vec.data(), vec.size(), level);
}

ByteVec inflate::deflate(const void *ptr, std::size_t size) {
   if (size < sizeof(InflateHeader)-1)
      throw exception::InsufficientSize(size, sizeof(InflateHeader));

   auto header = reinterpret_cast<const InflateHeader *>(ptr);

   if (std::memcmp(&header->magic, INFLATE_MAGIC, 4) != 0)
      throw exception::BadHeaderMagic();

   auto inflated_bytes = header->inflated / 8 + static_cast<std::size_t>(header->inflated % 8 != 0);
   auto deflated_bytes = header->deflated / 8;
   auto expected_size = sizeof(InflateHeader) - 1 + inflated_bytes;

   if (size != expected_size)
      throw exception::InsufficientSize(size, expected_size);

   auto inflate_stream = BitstreamPtr(const_cast<std::uint8_t *>(&header->data[0]), header->inflated);
   auto deflate_stream = BitstreamVec();

   switch (header->level)
   {
   case INFLATE_NOOP:
      return inflate_stream.to_bytevec();

   case INFLATE_1BIT:
   case INFLATE_2BIT:
   case INFLATE_3BIT:
   case INFLATE_4BIT:
   case INFLATE_5BIT:
   case INFLATE_6BIT:
   case INFLATE_7BIT:
   {
      auto modulus = 8 - header->level;
      std::size_t offset = 0;

      while (offset < header->inflated)
      {
         std::size_t read_size = ((header->inflated - offset > modulus) ? modulus : header->inflated - offset);
         deflate_stream.push_bits(inflate_stream.read_bits(offset, read_size));
         offset += 8;
      }

      auto crc = crc32(deflate_stream.data(), deflate_stream.byte_size());

      if (crc != header->checksum)
         throw exception::BadCRC(crc, header->checksum);

      return deflate_stream.to_bytevec();
   }

   default:
      throw exception::UnsupportedInflateLevel(header->level);
   }
}
      
