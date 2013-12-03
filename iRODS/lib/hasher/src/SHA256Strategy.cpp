/* -*- mode: c++; fill-column: 72; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include "SHA256Strategy.hpp"

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

#include <string.h>

namespace irods {
    
    std::string SHA256Strategy::_name = "SHA256";

    SHA256Strategy::
    SHA256Strategy(void)
    {
        _finalized = false;
    }

    SHA256Strategy::
    ~SHA256Strategy(void)
    {
        // TODO - stub
    }

    unsigned int SHA256Strategy::
    init(void)
    {
        unsigned int result = 0;
        SHA256_Init(&_context);
        _finalized = false;
        return result;
    }

    unsigned int SHA256Strategy::
    update(
        char const* data,
        unsigned int size)
    {
        unsigned int result = 0;
        if(!_finalized) {
            unsigned char* charData = new unsigned char[size];
            memcpy(charData, data, size);
            SHA256_Update(&_context, charData, size);
            delete [] charData;
        } else {
            result = 1;             // TODO - should be an enum or string
            // table value here
        }
        return result;
    }

    unsigned int SHA256Strategy::
    digest(
        std::string& messageDigest)
    {
        unsigned int result = 0;
        if(!_finalized) {
            unsigned char buffer[SHA256_DIGEST_LENGTH];
            SHA256_Final(buffer, &_context);
            std::stringstream ins;
            for(int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
                ins << std::setfill('0') << std::setw(2) << std::hex << (int)buffer[i];
            }
            _digest = ins.str();
        }
        messageDigest = _digest;
        return result;
    }
}; // namespace irods
