#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <map>

#include <framework.hpp>
#include <inflate.hpp>

using namespace inflate;

int
test_bitstream()
{
   INIT();

   auto data = "\xc0\x1d\xc0\xff\xee";
   auto vec_data = ByteVec(data, data+std::strlen(data));
   auto stream = BitstreamVec(vec_data, vec_data.size()*8);

   ASSERT(stream[0] == false);
   ASSERT(stream[7] == true);
   ASSERT(stream[35] == true);
   ASSERT(stream.pop_bits(4) == BitVec({ true, true, true, false }));
   ASSERT(stream.pop_bit());
   ASSERT(stream.pop_bit());
   ASSERT(stream.pop_bit());
   ASSERT(!stream.pop_bit());
   ASSERT(stream.pop_bits(10) == BitVec(10, true));
   ASSERT(stream.pop_bits(6) == BitVec(6, false));

   auto abad1dea = "\xab\xad\x1d\xea";
   auto abad1dea_vec = ByteVec(abad1dea, abad1dea+std::strlen(abad1dea));
   auto abad1dea_bits = to_bitvec(abad1dea_vec);

   ASSERT_SUCCESS(stream.push_bits(abad1dea_bits));
   ASSERT(*reinterpret_cast<std::uint32_t *>(stream.data()) == 0xADAB1DC0);
   ASSERT(*reinterpret_cast<std::uint16_t *>(stream.data()+4) == 0xEA1D);

   auto beef = "\xbe\xef";
   auto beef_vec = ByteVec(beef, beef+std::strlen(beef));
   auto beef_bits = to_bitvec(beef_vec);

   ASSERT_SUCCESS(stream.insert_bits(20, beef_bits));
   ASSERT(*reinterpret_cast<std::uint64_t *>(stream.data()) == 0xEA1DADAEFBEB1DC0);
   
   ASSERT_SUCCESS(stream.erase_bits(20, beef_bits.size()));
   ASSERT(*reinterpret_cast<std::uint32_t *>(stream.data()) == 0xADAB1DC0);
   ASSERT(*reinterpret_cast<std::uint16_t *>(stream.data()+4) == 0xEA1D);

   COMPLETE();
}

double entropy(const std::uint8_t *ptr, std::size_t size)
{
   std::map<std::uint8_t,std::size_t> occurrences;

   for (std::size_t i=0; i<size; ++i)
      ++occurrences[ptr[i]];

   double result = 0.0;

   for (auto entry : occurrences)
   {
      auto weight = entry.second;
      auto p_x = static_cast<double>(weight) / size;

      if (p_x == 0) { continue; }

      result -= p_x * std::log2(p_x);
   }

   return std::abs(result);
}

int
test_inflate()
{
   INIT();

   std::srand(std::time(nullptr));
   ByteVec random_bytes;

   for (std::size_t i=0; i<1024*256; ++i)
      random_bytes.push_back(std::rand() % 256);

   auto init_entropy = entropy(random_bytes.data(), random_bytes.size());
   LOG_INFO("Entropy (deflated): " << init_entropy);

   auto inflated = inflate::inflate(random_bytes.data(), random_bytes.size());
   auto inflate_entropy = entropy(inflated.data(), inflated.size());
   ASSERT(inflate_entropy < init_entropy);
   LOG_INFO("Entropy (inflated): " << inflate_entropy);

   auto deflated = inflate::deflate(inflated.data(), inflated.size());
   ASSERT(deflated == random_bytes);
   
   COMPLETE();
}

int
main
(int argc, char *argv[])
{
   INIT();

   LOG_INFO("Testing Bitstream objects.");
   PROCESS_RESULT(test_bitstream);

   LOG_INFO("Testing inflate functions.");
   PROCESS_RESULT(test_inflate);

   COMPLETE();
}
