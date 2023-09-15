#ifndef IRODS_RANDOM_HPP
#define IRODS_RANDOM_HPP

#include <string>

namespace irods {
    void getRandomBytes( void * buf, int bytes );

    /// \brief Generate a string of size \p _length consisting of randomly generated alphanumeric characters.
    ///
    /// \param[in] _length Desired length of the generated string.
    ///
    /// \throws irods::exception If \p _length is negative.
    ///
    /// \since 4.3.1
    auto generate_random_alphanumeric_string(std::int16_t _length) -> std::string;

    template <typename T>
    T
    getRandom() {
        T random;
        getRandomBytes( &random, sizeof(random) );
        return random;
    }
}
#endif // IRODS_RANDOM_HPP
