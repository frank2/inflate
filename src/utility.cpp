#include <inflate.hpp>

using namespace inflate;

std::uint32_t inflate::crc32(const void *ptr, std::size_t size, std::uint32_t init_crc) {
   auto crc = init_crc ^ 0xFFFFFFFF;
   auto u8_ptr = reinterpret_cast<const std::uint8_t *>(ptr);

   for (std::size_t i=0; i<size; ++i)
      crc = CRC32_TABLE[(crc ^ u8_ptr[i]) & 0xFF] ^ (crc >> 8);

   return crc ^ 0xFFFFFFFF;
}

std::uint32_t inflate::crc32(const std::vector<std::uint8_t> &vec, std::uint32_t init_crc) {
   return inflate::crc32(vec.data(), vec.size(), init_crc);
}
