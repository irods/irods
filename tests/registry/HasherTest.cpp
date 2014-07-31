#include "Hasher.hpp"
#include "MD5Strategy.hpp"
#include "SHA256Strategy.hpp"
#include "irods_hasher_factory.hpp"

#include <string>
#include <iostream>
#include <fstream>

using namespace std;
using namespace irods;

void
generateHash(
    const char* filename ) {
    // Create hasher
    Hasher md5_hasher;
    Hasher sha_hasher;
    getHasher( MD5_NAME, md5_hasher );
    getHasher( SHA256_NAME, sha_hasher );

    // Read the file and hash it
    ifstream input( filename, ios_base::in | ios_base::binary );
    if ( input.is_open() ) {
        char buffer[1024];
        while ( !input.eof() ) {
            input.read( buffer, 1023 );
            int numRead = input.gcount();
            md5_hasher.update( std::string( buffer, numRead ) );
            sha_hasher.update( std::string( buffer, numRead ) );
        }
        input.close();
        string messageDigest;
        md5_hasher.digest( messageDigest );
        cout << messageDigest << endl;
        sha_hasher.digest( messageDigest );
        cout << messageDigest << endl;
    }
    else {
        cerr << "ERROR: Unable to open file: " << filename << endl;
    }
}

int
main(
    int argc,
    char* argv[] ) {
    int result = 0;
    if ( argc != 2 ) {
        cerr << "ERROR: wrong number of arguments: " << argc << endl;
        cerr << "ERROR: filename must be specified" << endl;
        result = 1;
    }
    else {
        generateHash( argv[1] );
    }
    return result;
}
