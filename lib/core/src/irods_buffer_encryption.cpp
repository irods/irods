// =-=-=-=-=-=-=-
#include "irods_buffer_encryption.hpp"
#include "irods_log.hpp"

// =-=-=-=-=-=-=-
// ssl includes
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/aes.h>
#include <openssl/md5.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <memory>

namespace irods {
    class evp_lifetime_mgr {
    public:
        evp_lifetime_mgr() {
            OpenSSL_add_all_algorithms();
        }

        ~evp_lifetime_mgr() {
            EVP_cleanup();
        }
    };
    static const evp_lifetime_mgr global_evp_lifetime_mgr_;

    std::string buffer_crypt::gen_hash(
        unsigned char* _buf,
        int            _sz ) {
        MD5_CTX ctx;
        MD5_Init( &ctx );
        MD5_Update( &ctx, _buf, _sz );
        unsigned char hash[16];
        MD5_Final( hash, &ctx );

        std::stringstream ss;
        for ( int i = 0; i < 16; ++i ) {
            ss << std::setfill( '0' ) << std::setw( 2 ) << std::hex << ( int )hash[i];
        }

        return ss.str();
    }

// =-=-=-=-=-=-=-
// public - constructor
    buffer_crypt::buffer_crypt() :
        key_size_( 32 ),
        salt_size_( 8 ),
        num_hash_rounds_( 16 ),
        algorithm_( "aes-256-cbc" ) {
    }

    buffer_crypt::buffer_crypt(
        int         _key_sz,
        int         _salt_sz,
        int         _num_rnds,
        const char* _algo ) :
        key_size_( _key_sz ),
        salt_size_( _salt_sz ),
        num_hash_rounds_( _num_rnds ),
        algorithm_( _algo ) {

        std::transform(
            algorithm_.begin(),
            algorithm_.end(),
            algorithm_.begin(),
            ::tolower );

        // =-=-=-=-=-=-=-
        // select some sane defaults
        if ( 0 == key_size_ ) {
            key_size_ = 32;
        }

        if ( 0 == salt_size_ ) {
            salt_size_ = 8;
        }

        if ( 0 == num_hash_rounds_ ) {
            num_hash_rounds_ = 16;
        }

        if ( algorithm_.empty() ) {
            algorithm_ = "aes-256-cbc";
        }
    } // ctor

// =-=-=-=-=-=-=-
// public - destructor
    buffer_crypt::~buffer_crypt() {
    } // dtor

// =-=-=-=-=-=-=-
// public static - generate a random key
    irods::error buffer_crypt::generate_key(
        array_t& _out_key,
        int _key_size
    ) {
        // =-=-=-=-=-=-=-
        // generate random bytes
        _out_key.resize( _key_size );
        const int rnd_err = RAND_bytes( &_out_key[0], _key_size );
        if ( 1 != rnd_err ) {
            char err[ 256 ];
            ERR_error_string_n( ERR_get_error(), err, sizeof( err ) );
            const std::string msg = std::string( "failed in RAND_bytes - " ) + err;
            return ERROR( ERR_get_error(), msg );
        }

        return SUCCESS();

    } // buffer_crypt::generate_key

// =-=-=-=-=-=-=-
// public static - hex encode byte array
    irods::error buffer_crypt::hex_encode(
        const array_t& _in_buf,
        std::string& _out_str ) {
        std::stringstream ss;
        for ( irods::buffer_crypt::array_t::size_type i = 0; i < _in_buf.size(); ++i ) {
            ss << std::setfill( '0' ) << std::setw( 2 ) << std::hex << static_cast<unsigned int>( _in_buf[i] );
        }

        _out_str = ss.str();

        return SUCCESS();
    } // buffer_crypt::hex_encode

// =-=-=-=-=-=-=-
// public - create a hashed key and initialization vector
    irods::error buffer_crypt::initialization_vector(
        array_t& _out_iv ) {
        // =-=-=-=-=-=-=-
        // generate a random initialization vector
        unsigned char* iv = new unsigned char[ key_size_ ];
        int rnd_err = RAND_bytes(
                          iv,
                          key_size_ );
        if ( 1 != rnd_err ) {
            delete [] iv;
            char err[ 256 ];
            ERR_error_string_n( ERR_get_error(), err, 256 );
            std::string msg( "failed in RAND_bytes - " );
            msg += err;
            return ERROR( ERR_get_error(), msg );

        }

        // =-=-=-=-=-=-=-
        // copy the iv to the out variable
        _out_iv.assign(
            &iv[0],
            &iv[ key_size_ ] );

        // =-=-=-=-=-=-=-
        // clean up
        delete [] iv;

        return SUCCESS();

    } // buffer_crypt::initialization_vector

// =-=-=-=-=-=-=-
// public - encryptor
    irods::error buffer_crypt::encrypt(
        const array_t& _key,
        const array_t& _iv,
        const array_t& _in_buf,
        array_t&       _out_buf ) {

        // =-=-=-=-=-=-=-
        // create an encryption context
        auto* context = EVP_CIPHER_CTX_new();

        auto* algo = EVP_get_cipherbyname( algorithm_.c_str() );
        if ( !algo ) {
            rodsLog(
                LOG_NOTICE,
                "buffer_crypt::encrypt - algorithm not supported [%s]",
                algorithm_.c_str() );
            // default to aes 256 cbc
            algo = EVP_aes_256_cbc();
        }

        int ret = EVP_EncryptInit_ex(
                      context,
                      algo,
                      NULL,
                      &_key[0],
                      &_iv[0] );
        if ( 0 == ret ) {
            char err[ 256 ];
            ERR_error_string_n( ERR_get_error(), err, 256 );
            std::string msg( "failed in EVP_EncryptInit_ex - " );
            msg += err;
            return ERROR( ERR_get_error(), msg );
        }


        // =-=-=-=-=-=-=-
        // max ciphertext len for a n bytes of plaintext is n + AES_BLOCK_SIZE -1 bytes
        int            cipher_len  = _in_buf.size() + AES_BLOCK_SIZE;
        unsigned char* cipher_text = new unsigned char[ cipher_len ] ;
        // =-=-=-=-=-=-=-
        // update ciphertext, cipher_len is filled with the length of ciphertext generated,
        ret = EVP_EncryptUpdate(
                  context,
                  cipher_text,
                  &cipher_len,
                  &_in_buf[0],
                  _in_buf.size() );
        if ( 0 == ret ) {
            delete [] cipher_text;
            char err[ 256 ];
            ERR_error_string_n( ERR_get_error(), err, 256 );
            std::string msg( "failed in EVP_EncryptUpdate - " );
            msg += err;
            return ERROR( ERR_get_error(), msg );
        }

        // =-=-=-=-=-=-=-
        // update ciphertext with the final remaining bytes
        int final_len = 0;
        ret = EVP_EncryptFinal_ex(
                  context,
                  cipher_text + cipher_len,
                  &final_len );
        if ( 0 == ret ) {
            delete [] cipher_text;
            char err[ 256 ];
            ERR_error_string_n( ERR_get_error(), err, 256 );
            std::string msg( "failed in EVP_EncryptFinal_ex - " );
            msg += err;
            return ERROR( ERR_get_error(), msg );
        }

        // =-=-=-=-=-=-=-
        // clean up and assign out variables before exit
        _out_buf.resize( cipher_len + final_len );

        // =-=-=-=-=-=-=-
        // copy the iv to the out variable
        _out_buf.assign(
            &cipher_text[0],
            &cipher_text[ cipher_len + final_len ] );

        delete [] cipher_text;

        EVP_CIPHER_CTX_free( context );

        return SUCCESS();

    } // encrypt

// =-=-=-=-=-=-=-
// public - decryptor
    irods::error buffer_crypt::decrypt(
        const array_t& _key,
        const array_t& _iv,
        const array_t& _in_buf,
        array_t&       _out_buf ) {
        // =-=-=-=-=-=-=-
        // create an decryption context
        auto* context = EVP_CIPHER_CTX_new();

        auto* algo = EVP_get_cipherbyname( algorithm_.c_str() );
        if ( !algo ) {
            rodsLog(
                LOG_NOTICE,
                "buffer_crypt::decrypt - algorithm not supported [%s]",
                algorithm_.c_str() );
            // default to aes 256 cbc
            algo = EVP_aes_256_cbc();
        }

        int ret = EVP_DecryptInit_ex(
                      context,
                      algo,
                      NULL,
                      &_key[0],
                      &_iv [0] );
        if ( 0 == ret ) {
            char err[ 256 ];
            ERR_error_string_n( ERR_get_error(), err, 256 );
            std::string msg( "failed in EVP_DecryptInit_ex - " );
            msg += err;
            return ERROR( ERR_get_error(), msg );
        }

        // =-=-=-=-=-=-=-
        // allocate a plain text buffer
        // because we have padding ON, we must allocate an extra cipher block size of memory
        int            plain_len  = 0;
        auto plain_text = std::make_unique<unsigned char[]>(_in_buf.size() + AES_BLOCK_SIZE);

        // =-=-=-=-=-=-=-
        // update the plain text, plain_len is filled with the length of the plain text
        ret = EVP_DecryptUpdate(
                  context,
                  plain_text.get(),
                  &plain_len,
                  &_in_buf[0],
                  _in_buf.size() );
        if ( 0 == ret ) {
            char err[ 256 ];
            ERR_error_string_n( ERR_get_error(), err, 256 );
            std::string msg( "failed in EVP_DecryptUpdate - " );
            msg += err;
            return ERROR( ERR_get_error(), msg );
        }

        // =-=-=-=-=-=-=-
        // finalize the plain text, final_len is filled with the resulting length of the plain text
        int final_len = 0;
        ret = EVP_DecryptFinal_ex(
                  context,
                  plain_text.get() + plain_len,
                  &final_len );
        if ( 0 == ret ) {
            char err[ 256 ];
            ERR_error_string_n( ERR_get_error(), err, 256 );
            std::string msg( "failed in EVP_DecryptFinal_ex - " );
            msg += err;
            return ERROR( ERR_get_error(), msg );
        }

        // =-=-=-=-=-=-=-
        // assign the plain text to the outvariable and clean up
        _out_buf.resize( plain_len + final_len );

        // =-=-=-=-=-=-=-
        // copy the iv to the out variable
        _out_buf.assign(
            plain_text.get(),
            plain_text.get() + plain_len + final_len );

        EVP_CIPHER_CTX_free( context );

        return SUCCESS();

    } // decrypt

}; // namespace irods
