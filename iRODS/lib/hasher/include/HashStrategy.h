/* -*- mode: c++; fill-column: 72; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _HashStrategy_H_
#define _HashStrategy_H_

#include <string>

class HashStrategy
{
public:

    virtual ~HashStrategy(void) {};

    virtual std::string name(void) const = 0;
    virtual unsigned int init(void) = 0;
    virtual unsigned int update(const std::string& data) = 0;
    virtual unsigned int digest(std::string& messageDigest) = 0;
};

#endif // _HashStrategy_H_
