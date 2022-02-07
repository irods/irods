#ifndef IRODS_HASH_HPP
#define IRODS_HASH_HPP

#include "irods_stacktrace.hpp"
#include "rodsLog.h"

#include <boost/unordered_map.hpp>

#define HASH_TYPE boost::unordered_map

#include <string>
#include <iostream>

namespace irods
{
    struct irods_string_hash
    {
        enum
        {
            // parameters for hash table
            bucket_size = 4, // 0 < bucket_size
            min_buckets = 8
        }; // min_buckets = 2 ^^ N, 0 < N

        ~irods_string_hash() = default;

        std::size_t operator()(const std::string& s1) const
        {
            if (s1.empty()) {
                std::cerr << irods::stacktrace().dump();
                rodsLog(LOG_NOTICE, "irods_string_hash - empty string value");
                return 0;
            }

            const unsigned char *p = reinterpret_cast<const unsigned char*>(s1.c_str());
            std::size_t hashval = 0;

            while (*p != '\0') {
                hashval = 31 * hashval + *p++;
            }

            return hashval;
        }

        bool operator()(const std::string& s1, const std::string& s2) const
        {
            return s1 < s2;
        }
    }; // struct irods_string_hash
} // namespace irods

#endif // IRODS_HASH_HPP
