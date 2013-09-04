


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
        ~buffer_crypt();
        
        /// =-=-=-=-=-=-=-
        /// @brief given a string, encrypt it
        eirods::error encrypt( 
            const std::string&, // hashed key
            const std::string&, // initialization vector
            const std::string&, // plaintext buffer
            std::string& );     // encrypted buffer

        /// =-=-=-=-=-=-=-
        /// @brief given a string, decrypt it
        eirods::error decrypt( 
            const std::string&, // hashed key
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
        /// @brief generate a random 32 byte key
        eirods::error generate_key(
            std::string& ); // random 32 byte key

    private:
        // =-=-=-=-=-=-=-
        // attributes

    }; // class buffer_crypt

}; // namespace eirods

#endif // __EIRODS_BUFFER_ENCRYPTION_H__



