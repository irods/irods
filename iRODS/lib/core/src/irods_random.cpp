#include <boost/random.hpp>
#include <openssl/rand.h>
#include "irods_random.hpp"
#include <ctime>

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
