


#ifndef __EIRODS_BUFFER_ENCRYPTION_H__
#define __EIRODS_BUFFER_ENCRYPTION_H__

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_error.h"

// =-=-=-=-=-=-=-
// stl includes
#include <string>

// =-=-=-=-=-=-=-
// ssl includes
#include <openssl/evp.h>

namespace eirods {

    /// =-=-=-=-=-=-=-
    /// @brief functor which manages buffer encryption
    ///        used for parallel transfers.  based on 
    ///        SSL EVP library
    class buffer_crypt {
    public:
        // =-=-=-=-=-=-=-
        // con/de structors
        buffer_crypt();
        buffer_crypt( 
            int,           // key size in bytes
            int,           // salt size in bytes
            int,           // num hash rounds
            const char* ); // algorithm
        ~buffer_crypt();
        
        /// =-=-=-=-=-=-=-
        /// @brief given a string, encrypt it
        eirods::error encrypt( 
            const std::string&, // key
            const std::string&, // initialization vector
            const std::string&, // plaintext buffer
            std::string& );     // encrypted buffer

        /// =-=-=-=-=-=-=-
        /// @brief given a string, decrypt it
        eirods::error decrypt( 
            const std::string&, // key
            const std::string&, // initialization vector
            const std::string&, // encrypted buffer
            std::string& );     // plaintext buffer

        /// =-=-=-=-=-=-=-
        /// @brief given a key, create a hashed key and IV
        eirods::error initialization_vector(
            const std::string&, // plain text key
            std::string&,       // hashed key
            std::string& );     // intialization vector

        /// =-=-=-=-=-=-=-
        /// @brief generate a random byte key
        eirods::error generate_key(
            std::string& ); // random byte key
        
        /// =-=-=-=-=-=-=-
        /// @brief accessors for attributes
        int         key_size()        { return key_size_;        };
        int         salt_size()       { return salt_size_;       };
        int         num_hash_rounds() { return num_hash_rounds_; };
        std::string algorithm()       { return algorithm_;       };

    private:
        // =-=-=-=-=-=-=-
        // attributes
        int         key_size_;
        int         salt_size_;
        int         num_hash_rounds_;
        std::string algorithm_;

    }; // class buffer_crypt

}; // namespace eirods

#endif // __EIRODS_BUFFER_ENCRYPTION_H__



