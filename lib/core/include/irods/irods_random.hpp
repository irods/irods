#ifndef IRODS_RANDOM_HPP
#define IRODS_RANDOM_HPP

namespace irods {
    void getRandomBytes( void * buf, int bytes );

    template <typename T>
    T
    getRandom() {
        T random;
        getRandomBytes( &random, sizeof(random) );
        return random;
    }
}
#endif // IRODS_RANDOM_HPP
