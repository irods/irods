/* -*- mode: c++; fill-column: 72; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _SHA256Strategy_H_
#define _SHA256Strategy_H_

#include "HashStrategy.h"

#include <string>

#include <openssl/sha.h>

class SHA256Strategy : public HashStrategy
{
public:
    SHA256Strategy(void);
    virtual ~SHA256Strategy(void);

    virtual std::string name(void) const { return _name; }
    virtual unsigned int init(void);
    virtual unsigned int update(const std::string& data);
    virtual unsigned int digest(std::string& messageDigest);

private:
    static std::string _name;

    SHA256_CTX _context;
    bool _finalized;
    std::string _digest;
};

#endif // _SHA256Strategy_H_
