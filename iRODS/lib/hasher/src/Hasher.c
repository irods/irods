/* -*- mode: c++; fill-column: 72; c-basic-offset: 2; indent-tabs-mode: nil -*- */


#include "Hasher.h"

Hasher::
Hasher(void)
{
  // empty
}

Hasher::
~Hasher(void)
{
  for(vector<HashStrategy*>::iterator it = _strategies.begin(); it != _strategies.end(); ++it) {
    HashStrategy* strategy = *it;
    delete strategy;
  }
}

unsigned int Hasher::
listStrategies(
  vector<string>& strategies) const
{
  unsigned int result = 0;
  for(vector<HashStrategy*>::const_iterator it = _strategies.begin(); it != _strategies.end(); ++it) {
    HashStrategy* strategy = *it;
    strategies.push_back(strategy->name());
  }
  return result;
}

unsigned int Hasher::
init(void)
{
  unsigned int result = 0;
  for(vector<HashStrategy*>::iterator it = _strategies.begin();
      result == 0 && it != _strategies.end();
      ++it) {
    HashStrategy* strategy = *it;
    result = strategy->init();
  }
  return result;
}

unsigned int Hasher::
update(
  const string& data)
{
  unsigned int result = 0;
  for(vector<HashStrategy*>::iterator it = _strategies.begin();
      result == 0 && it != _strategies.end();
      ++it) {
    HashStrategy* strategy = *it;
    result = strategy->update(data);
  }
  return result;
}

unsigned int Hasher::
digest(
  const string& name,
  string& messageDigest)
{
  unsigned result = 0;
  bool found = false;
  for(vector<HashStrategy*>::iterator it = _strategies.begin();
      !found && it != _strategies.end();
      ++it) {
    HashStrategy* strategy = *it;
    if(name == strategy->name()) {
      found = true;
      result = strategy->digest(messageDigest);
    }
  }
  return result;
}

unsigned int Hasher::
catDigest(
  string& messageDigest)
{
  unsigned int result = 0;
  for(vector<HashStrategy*>::iterator it = _strategies.begin();
      result == 0 && it != _strategies.end();
      ++it) {
    HashStrategy* strategy = *it;
    string tmpDigest;
    result = strategy->digest(tmpDigest);
    if(result == 0) {
      messageDigest += tmpDigest;
    }
  }
  return result;
}
