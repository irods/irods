/* -*- mode: c++; fill-column: 72; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _HashStrategy_H_
#define _HashStrategy_H_

#include <string>

namespace irods {
    
    class HashStrategy
    {
    public:

        virtual ~HashStrategy(void) {};

        virtual std::string name(void) const = 0;
        virtual unsigned int init(void) = 0;
        virtual unsigned int update(char const* data, unsigned int size) = 0;
        virtual unsigned int digest(std::string& messageDigest) = 0;
    };
}; // namespace irods

#endif // _HashStrategy_H_
