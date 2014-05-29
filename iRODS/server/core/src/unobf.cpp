
#include <iostream>
#include "string.h"

extern "C" int obfGetPw( char* );


int main( int _argc, char* _argv[] ) {

    if( _argc != 2 ) {
       std::cout << "usage: unobf scrambled_password" 
                 << std::endl; 
       return -1;
    }

    if( !_argv[1] ) {
       std::cout << "usage: unobf scrambled_password" 
                 << std::endl; 
       return -1;
    }

    char pw[1024];
    strcpy( pw, _argv[1] );

    int ret = obfGetPw( pw );

    std::cout << pw;

    return ret;

} // main



