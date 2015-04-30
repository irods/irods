#include "Hasher.hpp"

#include <iostream>
#include <algorithm>
#include <boost/algorithm/string.hpp>

#include "rodsErrorTable.h"
#include "irods_stacktrace.hpp"

namespace irods {

    error
    Hasher::init( const HashStrategy* _strategy_in ) {
        _strategy = _strategy_in;
        _stored_error = SUCCESS();
        _stored_digest.clear();

        return PASS( _strategy->init( _context ) );
    }

    error
    Hasher::update( const std::string& _data ) {
        if ( NULL == _strategy ) {
            return ERROR( SYS_UNINITIALIZED, "Update called on a hasher that has not been initialized" );
        }
        if ( !_stored_digest.empty() ) {
            return ERROR( SYS_HASH_IMMUTABLE, "Update called on a hasher that has already generated a digest" );
        }
        error ret = _strategy->update( _data, _context );

        return PASS( ret );
    }

    error
    Hasher::digest( std::string& _messageDigest ) {
        if ( NULL == _strategy ) {
            return ERROR( SYS_UNINITIALIZED, "Digest called on a hasher that has not been initialized" );
        }
        if ( _stored_digest.empty() ) {
            _stored_error = _strategy->digest( _stored_digest, _context );
        }

        _messageDigest = _stored_digest;
        return PASS( _stored_error );
    }

}; //namespace irods
