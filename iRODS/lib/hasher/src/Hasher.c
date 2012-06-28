/* -*- mode: c++; fill-column: 72; c-basic-offset: 4; indent-tabs-mode: nil -*- */


#include "Hasher.h"

namespace eirods {
    
    Hasher::
    Hasher(void)
    {
        // empty
    }

    Hasher::
    ~Hasher(void)
    {
        for(std::vector<HashStrategy*>::iterator it = _strategies.begin(); it != _strategies.end(); ++it) {
            HashStrategy* strategy = *it;
            delete strategy;
        }
    }

    unsigned int Hasher::
    listStrategies(
        std::vector<std::string>& strategies) const
    {
        unsigned int result = 0;
        for(std::vector<HashStrategy*>::const_iterator it = _strategies.begin(); it != _strategies.end(); ++it) {
            HashStrategy* strategy = *it;
            strategies.push_back(strategy->name());
        }
        return result;
    }

    unsigned int Hasher::
    init(void)
    {
        unsigned int result = 0;
        for(std::vector<HashStrategy*>::iterator it = _strategies.begin();
            result == 0 && it != _strategies.end();
            ++it) {
            HashStrategy* strategy = *it;
            result = strategy->init();
        }
        return result;
    }

    unsigned int Hasher::
    update(
        char const* data,
        unsigned int size)
    {
        unsigned int result = 0;
        for(std::vector<HashStrategy*>::iterator it = _strategies.begin();
            result == 0 && it != _strategies.end();
            ++it) {
            HashStrategy* strategy = *it;
            result = strategy->update(data, size);
        }
        return result;
    }

    unsigned int Hasher::
    digest(
        const std::string& name,
        std::string& messageDigest)
    {
        unsigned result = 0;
        bool found = false;
        for(std::vector<HashStrategy*>::iterator it = _strategies.begin();
            !found && it != _strategies.end();
            ++it) {
            HashStrategy* strategy = *it;
            if(name == strategy->name()) {
                found = true;
                result = strategy->digest(messageDigest);
            }
        }
        if(!found) {
            result = 1;             // TODO - should be an enum or string
            // table value here
        }
        return result;
    }

    unsigned int Hasher::
    catDigest(
        std::string& messageDigest)
    {
        unsigned int result = 0;
        for(std::vector<HashStrategy*>::iterator it = _strategies.begin();
            result == 0 && it != _strategies.end();
            ++it) {
            HashStrategy* strategy = *it;
            std::string tmpDigest;
            result = strategy->digest(tmpDigest);
            if(result == 0) {
                messageDigest += tmpDigest;
            }
        }
        return result;
    }
}; //namespace eirods
