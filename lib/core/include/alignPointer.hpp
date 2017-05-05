#include <cstdlib>
#include "rodsType.h"

template<typename T>
T alignAddrToBoundary( T ptr, std::size_t boundary ) {
#if defined(_LP64) || defined(__LP64__)
    rodsLong_t b, m;
    b = ( rodsLong_t ) ptr;

    m = b % boundary;

    if ( m == 0 ) {
        return ptr;
    }

    /* rodsLong_t is signed */
    if ( m < 0 ) {
        m = boundary + m;
    }

    return ( char * )ptr + boundary - m;

#else
    uint b, m;
    b = ( uint ) ptr;

    m = b % boundary;

    if ( m == 0 ) {
        return ptr;
    }

    return ( char * )ptr + boundary - m;

#endif
}

template<typename T>
T alignInt( T ptr ) {
    return alignAddrToBoundary( ptr, 4 );
}
template<typename T>
T alignInt16( T ptr ) {
    return alignAddrToBoundary( ptr, 2 );
}
template<typename T>
T alignDouble( T ptr ) {
#if defined(linux_platform) || defined(windows_platform)
    // no need align at 64 bit boundary for linux
    // Windows 32-bit OS is aligned with 4.

#if defined(_LP64) || defined(__LP64__)
    return alignAddrToBoundary( ptr, 8 );
#else
    return alignAddrToBoundary( ptr, 4 );
#endif // LP64

#else   /* non-linux_platform || non-windows*/
    return alignAddrToBoundary( ptr, 8 );
#endif  /* linux_platform | windows */
}
template<typename T>
T ialignAddr( T ptr ) {
#if defined(_LP64) || defined(__LP64__)
    return alignDouble( ptr );
#else
    return alignInt( ptr );
#endif
}
