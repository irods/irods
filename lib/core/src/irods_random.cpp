#include "irods/irods_random.hpp"

#include "irods/irods_exception.hpp"
#include "irods/rodsErrorTable.h"

#include <boost/random.hpp>
#include <fmt/format.h>
#include <openssl/rand.h>

#include <cctype>
#include <ctime>
#include <random>
#include <string>

namespace
{
    auto generate_random_alphanumeric_character() -> char
    {
        constexpr char min_char = '0';
        constexpr char max_char = 'z';

        static std::random_device rd;
        static std::default_random_engine e{rd()};
        static std::uniform_int_distribution<> d{min_char, max_char};

        char c{};
        while (!std::isalnum(static_cast<unsigned char>(c))) {
            c = static_cast<char>(d(e));
        }
        return c;
    } // generate_random_alphanumeric_character
} // anonymous namespace

void irods::getRandomBytes( void * buf, int bytes ) {
    if ( RAND_bytes( ( unsigned char * )buf, bytes ) != 1 ) {
        static boost::mt19937 generator( std::time( 0 ) ^ ( getpid() << 16 ) );
        static boost::uniform_int<unsigned char> byte_range( 0, 0xff );
        static boost::variate_generator<boost::mt19937, boost::uniform_int<unsigned char> > random_byte( generator, byte_range );
        for ( int i = 0; i < bytes; i++ ) {
            ( ( unsigned char * ) buf )[i] = random_byte();
        }
    }
}

auto irods::generate_random_alphanumeric_string(std::int16_t _length) -> std::string
{
    if (_length < 0) {
        THROW(SYS_INVALID_INPUT_PARAM, fmt::format("{}: _length must be non-negative.", __func__));
    }

    try {
        std::string s;
        s.reserve(_length);

        for (std::int16_t i = 0; i < _length; ++i) {
            s += generate_random_alphanumeric_character();
        }

        return s;
    }
    catch (const std::length_error& e) {
        THROW(SYS_INVALID_INPUT_PARAM, fmt::format("{}: Invalid length. [{}]", __func__, e.what()));
    }
    catch (const std::exception& e) {
        THROW(SYS_LIBRARY_ERROR, fmt::format("{}: Failed to generate string. [{}]", __func__, e.what()));
    }
    catch (...) {
        THROW(SYS_UNKNOWN_ERROR, "Failed to generate string: [Unknown error]");
    }
} // generate_random_alphanumeric_string
