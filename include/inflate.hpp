#ifndef __INFLATE_HPP
#define __INFLATE_HPP

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <set>
#include <utility>
#include <iostream>

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
      INFLATE_7BIT,

      INFLATE_RNG_PARTIAL_1BIT,
      INFLATE_RNG_PARTIAL_2BIT,
      INFLATE_RNG_PARTIAL_3BIT,
      INFLATE_RNG_PARTIAL_4BIT,
      INFLATE_RNG_PARTIAL_5BIT,
      INFLATE_RNG_PARTIAL_6BIT,
      INFLATE_RNG_PARTIAL_7BIT,

      INFLATE_RNG_FULL_1BIT,
      INFLATE_RNG_FULL_2BIT,
      INFLATE_RNG_FULL_3BIT,
      INFLATE_RNG_FULL_4BIT,
      INFLATE_RNG_FULL_5BIT,
      INFLATE_RNG_FULL_6BIT,
      INFLATE_RNG_FULL_7BIT,
   };

   PACK(1)
   struct InflateHeader
   {
      std::uint8_t level;
      std::uint64_t inflated;
      std::uint64_t deflated;
      std::uint32_t checksum;
      std::uint32_t seed;
   };
   UNPACK()

   #define INFLATE_MAGIC "NFL8"

   EXPORT std::pair<ByteVec, InflateHeader> inflate_memory(const void *ptr,
                                                           std::uint64_t size,
                                                           InflateLevel level=InflateLevel::INFLATE_3BIT,
                                                           std::optional<std::uint32_t> seed=std::nullopt);
   EXPORT std::pair<ByteVec, InflateHeader> inflate_memory(const ByteVec &vec,
                                                           InflateLevel level=InflateLevel::INFLATE_3BIT,
                                                           std::optional<std::uint32_t> seed=std::nullopt);
   EXPORT ByteVec deflate_memory(const void *ptr,
                                 std::uint64_t size,
                                 const InflateHeader &header,
                                 bool validate=true);
   EXPORT ByteVec deflate_memory(const ByteVec &vec,
                                 const InflateHeader &header,
                                 bool validate=true);
   
   EXPORT ByteVec inflate_disk(const void *ptr,
                               std::uint64_t size,
                               InflateLevel level=InflateLevel::INFLATE_3BIT,
                               std::optional<std::uint32_t> seed=std::nullopt);
   EXPORT ByteVec inflate_disk(const ByteVec &vec,
                               InflateLevel level=InflateLevel::INFLATE_3BIT,
                               std::optional<std::uint32_t> seed=std::nullopt);
   EXPORT ByteVec deflate_disk(const void *ptr, std::uint64_t size);
   EXPORT ByteVec deflate_disk(const ByteVec &vec);
}

#endif
