


#ifndef __IRODS_HASH_HPP__
#define __IRODS_HASH_HPP__

// =-=-=-=-=-=-=-
// hash_map include
#include <boost/unordered_map.hpp>
#define HASH_TYPE boost::unordered_map

#include <string.h>
#include <string>

namespace irods
{

	struct irods_char_str_hash
	{
		enum
		{	// parameters for hash table
			bucket_size = 4, // 0 < bucket_size
			min_buckets = 8

		}; // min_buckets = 2 ^^ N, 0 < N

		size_t operator()(const char *s1) const
		{
			// hash string s1 to size_t value
			const unsigned char *p = (const unsigned char *)s1;
			size_t hashval = 0;

			while (*p != '\0')
				hashval = 31 * hashval + (*p++); // or whatever

			return (hashval);
		}

		bool operator()(const char* s1, const char* s2 ) const
		{
			return ( strcmp( s1, s2 ) < 0 );
		}

	}; // struct irods_char_str_hash

	struct irods_string_hash
	{
		enum
		{	// parameters for hash table
			bucket_size = 4, // 0 < bucket_size
			min_buckets = 8

		}; // min_buckets = 2 ^^ N, 0 < N

		size_t operator()(const std::string s1) const
		{
			// hash string s1 to size_t value
			const unsigned char *p = (const unsigned char *)s1.c_str();
			size_t hashval = 0;

			while (*p != '\0')
				hashval = 31 * hashval + (*p++); // or whatever

			return (hashval);
		}

		bool operator()(const std::string s1, const std::string s2 ) const
		{
			return ( s1 < s2 );
		}

	}; // struct irods_string_hash

}; // namespace irods

#endif // __IRODS_HASH_HPP__
