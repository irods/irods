/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef __EIRODS_STRING_TOKENIZE_HPP__
#define __EIRODS_STRING_TOKENIZE_HPP__

#include <string>
#include <vector>

namespace eirods {
    // =-=-=-=-=-=-=-
    /// @brienf helper fcn to break up string into tokens for easy handling with a default delim of " "
    void string_tokenize( 
             const std::string&,            // incoming string to tokenize
             const std::string&,            // delimiter for separating tokens
             std::vector< std::string >& ); // vector of tokenized symbols
}; // namespace eirods

#endif // __EIRODS_STRING_TOKENIZE_HPP__




