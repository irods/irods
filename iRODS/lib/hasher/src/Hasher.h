/* -*- mode: c++; fill-column: 72; c-basic-offset: 2; indent-tabs-mode: nil -*- */

#ifndef _Hasher_H_
#define _Hasher_H_

#include "HashStrategy.h"

#include <string>
#include <vector>

class Hasher
{
public:
  Hasher(void);
  virtual ~Hasher(void);

  unsigned int addStrategy(HashStrategy* strategy) { _strategies.push_back(strategy); return 0; }
  unsigned int listStrategies(vector<string>& strategies) const;
  
  unsigned int init(void);
  unsigned int update(const string& data);
  unsigned int digest(const string& name, string& messageDigest);
  unsigned int catDigest(string& messageDigest);
  
private:
  vector<HashStrategy*> _strategies;
};

#endif // _Hasher_H_
