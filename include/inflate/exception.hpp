#ifndef __INFLATE_EXCEPTION_HPP
#define __INFLATE_EXCEPTION_HPP

#include <cstdint>
#include <exception>
#include <sstream>
#include <string>

namespace inflate
{
namespace exception
{
   class Exception : public std::exception
   {
   public:
      std::string error;

      Exception() : std::exception() {}
      Exception(const std::string &error) : error(error), std::exception() {}
      Exception(const Exception &other) : error(other.error) {}

      const char *what() const noexcept {
         return this->error.c_str();
      }
   };

   class NullPointer : public Exception
   {
   public:
      NullPointer() : Exception("Null pointer: An unexpected null pointer was encountered.") {}
   };

   class OutOfBounds : public Exception
   {
   public:
      std::size_t given;
      std::size_t boundary;

      OutOfBounds(std::size_t given, std::size_t boundary) : given(given), boundary(boundary), Exception() {
         std::stringstream stream;

         stream << "Out of bounds: the given boundary was " << given
                << ", but the expected boundary was " << boundary;

         this->error = stream.str();
      }
      OutOfBounds(const OutOfBounds &other) : given(other.given), boundary(other.boundary), Exception(other) {}
   };

   class NoBits : public Exception
   {
   public:
      NoBits() : Exception("No bits: there are no bits present in the bitstream.") {}
   };

   class InsufficientSize : public Exception
   {
   public:
      std::size_t given;
      std::size_t needed;

      InsufficientSize(std::size_t given, std::size_t needed) : given(given), needed(needed), Exception() {
         std::stringstream stream;

         stream << "Insufficient size: was given a size of " << given
                << ", but needed at least " << needed;

         this->error = stream.str();
      }
   };

   class BadHeaderMagic : public Exception
   {
   public:
      BadHeaderMagic() : Exception("Bad header magic: the magic bytes in the header of the inflate stream was not 'NFL8'.") {}
   };

   class BadCRC : public Exception
   {
   public:
      std::uint32_t given;
      std::uint32_t expected;

      BadCRC(std::uint32_t given, std::uint32_t expected) : given(given), expected(expected), Exception() {
         std::stringstream stream;

         stream << "Bad CRC: the given CRC calculated to "
                << std::showbase << std::hex << given
                << ", but was expecting " << expected;

         this->error = stream.str();
      }
   };

   class UnsupportedInflateLevel : public Exception
   {
   public:
      std::uint8_t level;

      UnsupportedInflateLevel(std::uint8_t level) : level(level), Exception() {
         std::stringstream stream;

         stream << "Unsupported inflate level: the given level " << level << " is unsupported.";

         this->error = stream.str();
      }
   };
}}

#endif
