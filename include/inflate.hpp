#ifndef __INFLATE_HPP
#define __INFLATE_HPP

#include <cstring>

#include <inflate/platform.hpp>
#include <inflate/exception.hpp>
#include <inflate/bitstream.hpp>
#include <inflate/utility.hpp>

namespace inflate
{
   enum InflateLevel
   {
      INFLATE_NOOP = 0,
      INFLATE_1BIT,
      INFLATE_2BIT,
      INFLATE_3BIT,
      INFLATE_4BIT,
      INFLATE_5BIT,
      INFLATE_6BIT,
      INFLATE_7BIT
      //INFLATE_RANDOM_1BIT,
   };

   PACK(1)
   struct InflateHeader
   {
      char magic[4];
      std::uint8_t level;
      std::uint64_t inflated;
      std::uint64_t deflated;
      std::uint32_t checksum;
      std::uint32_t reserved;
      std::uint8_t data[1];
   };
   UNPACK()

   #define INFLATE_MAGIC "NFL8"
      
   EXPORT ByteVec inflate(const void *ptr, std::size_t size, InflateLevel level=InflateLevel::INFLATE_3BIT);
   EXPORT ByteVec inflate(const ByteVec &vec, InflateLevel level=InflateLevel::INFLATE_3BIT);
   EXPORT ByteVec deflate(const void *ptr, std::size_t size);
   EXPORT ByteVec deflate(const ByteVec &vec);
}

#endif
