
#ifndef __IRODS_STRING_TOKENIZE_HPP__
#define __IRODS_STRING_TOKENIZE_HPP__

#include <string>
#include <vector>

namespace irods {
// =-=-=-=-=-=-=-
/// @brief helper function to break up string into tokens for easy handling with a default delim of " "
    void string_tokenize(
        const std::string&,            // incoming string to tokenize
        const std::string&,            // delimiter for separating tokens
        std::vector< std::string >& ); // vector of tokenized symbols
}; // namespace irods

#endif // __IRODS_STRING_TOKENIZE_HPP__




