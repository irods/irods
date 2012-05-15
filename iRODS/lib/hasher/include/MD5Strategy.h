/* -*- mode: c++; fill-column: 72; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _MD5Strategy_H_
#define _MD5Strategy_H_

#include "HashStrategy.h"
#include "global.h"             // cause md5.h needs it
#include "md5.h"

class MD5Strategy : public HashStrategy
{
public:
    MD5Strategy(void);
    virtual ~MD5Strategy(void);

    virtual std::string name(void) const { return _name; }
    virtual unsigned int init(void);
    virtual unsigned int update(const std::string& data);
    virtual unsigned int digest(std::string& messageDigest);

private:
    static std::string _name;

    MD5_CTX _context;
    bool _finalized;
    std::string _digest;
};

#endif // _MD5Strategy_H_
