/* -*- mode: c++; fill-column: 72; c-basic-offset: 2; indent-tabs-mode: nil -*- */

#ifndef _HashStrategy_H_
#define _HashStrategy_H_

#include <string>

using namespace std;

class HashStrategy
{
public:

  virtual ~HashStrategy(void) {};

  virtual string name(void) const = 0;
  virtual unsigned int init(void) = 0;
  virtual unsigned int update(const string& data) = 0;
  virtual unsigned int digest(string& messageDigest) = 0;
};

#endif // _HashStrategy_H_
