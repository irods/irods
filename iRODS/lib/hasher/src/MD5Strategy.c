/* -*- mode: c++; fill-column: 72; c-basic-offset: 4; indent-tabs-mode: nil -*- */


#include "MD5Strategy.h"

#include <string>

std::string MD5Strategy::_name = "MD5";

MD5Strategy::
MD5Strategy(void)
{
    _finalized = false;
}

MD5Strategy::
~MD5Strategy()
{
    // TODO - stub
}

unsigned int MD5Strategy::
init(void)
{
    unsigned int result = 0;
    MD5Init(&_context);
    _finalized = false;
    return result;
}

unsigned int MD5Strategy::
update(
    const std::string& data)
{
    unsigned int result = 0;
    unsigned char* charData = new unsigned char[data.length()];
    MD5Update(&_context, charData, data.length());
    return result;
}

unsigned int MD5Strategy::
digest(
    std::string& messageDigest)
{
    unsigned int result = 0;
    if(!_finalized) {
        unsigned char buffer[17];
        MD5Final(buffer, &_context);
        buffer[16] = '\0';
        _digest = std::string(reinterpret_cast<char*>(buffer));
    }
    messageDigest = _digest;
    return result;
}
