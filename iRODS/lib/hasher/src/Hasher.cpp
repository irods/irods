#include "Hasher.hpp"
#include <iostream>
#include <algorithm>

#include "irods_stacktrace.hpp"

namespace irods {

    Hasher::
    Hasher( void ) {
        // empty
    }

    Hasher::
    ~Hasher( void ) {
        for ( std::vector<HashStrategy*>::iterator it = _strategies.begin(); it != _strategies.end(); ++it ) {
            HashStrategy* strategy = *it;
            delete strategy;
        }
    }

    unsigned int Hasher::
    listStrategies(
        std::vector<std::string>& strategies ) const {
        unsigned int result = 0;
        for ( std::vector<HashStrategy*>::const_iterator it = _strategies.begin();
                it != _strategies.end(); ++it ) {
            HashStrategy* strategy = *it;
            strategies.push_back( strategy->name() );
        }
        return result;
    }

    unsigned int Hasher::
    init( const std::string& _hasher ) {
        unsigned int result = 0;
        _requested_hasher.clear();

        std::string scheme = _hasher;
        std::transform(
            scheme.begin(),
            scheme.end(),
            scheme.begin(),
            ::tolower );

        for ( std::vector<HashStrategy*>::iterator it = _strategies.begin();
                result == 0 && it != _strategies.end();
                ++it ) {
            if ( scheme == ( *it )->name() ) {
                _requested_hasher = scheme;
                HashStrategy* strategy = *it;
                result = strategy->init();
                break;
            }
        }
        if ( _requested_hasher.empty() ) {
            std::cout << "Hasher::init - strategy not found ["
                      << scheme
                      << "]"
                      << std::endl;

            return -1;
        }
        else {

            return result;
        }
    }

    unsigned int Hasher::
    update(
        char const* data,
        unsigned int size ) {
        if ( _requested_hasher.empty() ) {
            std::cout << "Hasher::update - not initialized" << std::endl;
            return -1;
        }

        bool found = false;
        unsigned int result = 0;
        for ( std::vector<HashStrategy*>::iterator it = _strategies.begin();
                result == 0 && it != _strategies.end();
                ++it ) {
            if ( _requested_hasher == ( *it )->name() ) {
                HashStrategy* strategy = *it;
                result = strategy->update( data, size );
                found = true;
                break;
            }
        }
        if ( !found ) {
            std::cout << "Hasher::init - strategy not found ["
                      << _requested_hasher
                      << "]"
                      << std::endl;

            result = -1;             // TODO - should be an enum or string
            // table value here
        }
        return result;
    }

    unsigned int Hasher::
    digest(
        std::string& messageDigest ) {
        if ( _requested_hasher.empty() ) {
            std::cout << "Hasher::update - not initialized" << std::endl;
            return -1;
        }

        unsigned result = 0;
        bool found = false;
        for ( std::vector<HashStrategy*>::iterator it = _strategies.begin();
                !found && it != _strategies.end();
                ++it ) {
            HashStrategy* strategy = *it;
            if ( _requested_hasher == strategy->name() ) {
                found = true;
                result = strategy->digest( messageDigest );
                break;
            }
        }
        if ( !found ) {
            std::cout << "Hasher::init - strategy not found ["
                      << _requested_hasher
                      << "]"
                      << std::endl;

            result = -1;             // TODO - should be an enum or string
            // table value here
        }
        return result;
    }

}; //namespace irods
