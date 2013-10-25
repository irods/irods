


// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_buffer_encryption.h"

// =-=-=-=-=-=-=-
// ssl includes
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/aes.h>

#include <iostream>

namespace eirods {

    // =-=-=-=-=-=-=-
    // public - constructor
    buffer_crypt::buffer_crypt() :
            key_size_( 32 ),
            salt_size_( 8 ),
            num_hash_rounds_( 16 ),
            algorithm_( "AES-256-CBC" ) {
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
        // =-=-=-=-=-=-=-
        // select some sane defaults
        if( 0 == key_size_ ) {
            key_size_ = 32;
        }
        
        if( 0 == salt_size_ ) {
            salt_size_ = 8;
        }
        
        if( 0 == num_hash_rounds_ ) {
            num_hash_rounds_ = 16;
        }

        if( algorithm_.empty() ) {
            algorithm_ = "AES-256-CBC";

        }
 
        if( !EVP_get_cipherbyname( algorithm_.c_str() ) ) {
            algorithm_ = "AES-256-CBC";

        }
       
    } // ctor

    // =-=-=-=-=-=-=-
    // public - destructor
    buffer_crypt::~buffer_crypt() {

    } // dtor
    
    // =-=-=-=-=-=-=-
    // public - generate a random 32 byte key
    eirods::error buffer_crypt::generate_key( 
        std::string& _out_key ) {
        // =-=-=-=-=-=-=-
        // generate 32 random bytes
        unsigned char* key = new unsigned char[ key_size_ ];
        int rnd_err = RAND_bytes( key, key_size_ );
        if( 1 != rnd_err ) {
            delete [] key;
            char err[ 256 ];
            ERR_error_string_n( ERR_get_error(), err, 256 );
            std::string msg( "failed in RAND_bytes - " );
            msg += err;
            return ERROR( ERR_get_error(), msg );

        }

        // =-=-=-=-=-=-=-
        // assign the key to the out variable
        _out_key.assign( 
            reinterpret_cast< const char* >( key ), 
            key_size_ );

        delete [] key;

        return SUCCESS();

    } // buffer_crypt::generate_key
     
    // =-=-=-=-=-=-=-
    // public - create a hashed key and initialization vector
    eirods::error buffer_crypt::initialization_vector( 
        const std::string& _key,
        std::string&       _out_key,
        std::string&       _out_iv ) {
        // =-=-=-=-=-=-=-
        // generate a random salt
        unsigned char* salty = new unsigned char[ salt_size_ ];
        int rnd_err = RAND_bytes( salty, salt_size_ );
        if( 1 != rnd_err ) {
            delete [] salty;
            char err[ 256 ];
            ERR_error_string_n( ERR_get_error(), err, 256 );
            std::string msg( "failed in RAND_bytes - " );
            msg += err;
            return ERROR( ERR_get_error(), msg );
                       
        }

        // =-=-=-=-=-=-=-
        // build the initialization vector
        int num_rounds = 2;
        unsigned char* init_vec = new unsigned char[ _key.size() ];
        unsigned char* key_hash = new unsigned char[ _key.size() ];
        unsigned char* key_buff = new unsigned char[ _key.size() ];
        memcpy( 
            key_buff, 
            _key.c_str(),
            _key.size()*sizeof(unsigned char) );
        size_t sz = EVP_BytesToKey( 
                        EVP_get_cipherbyname( algorithm_.c_str() ),
                        EVP_sha1(), 
                        salty, 
                        key_buff,
                        _key.size(), 
                        num_rounds, 
                        key_hash, 
                        init_vec );
        if( _key.size() != sz ) {
            delete [] key_buff;
            delete [] salty;
            delete [] init_vec;
            delete [] key_hash;
            
            char err[ 256 ];
            ERR_error_string_n( ERR_get_error(), err, 256 );
            std::string msg( "failed in EVP_BytesToKey - " );
            msg += err;
            return ERROR( ERR_get_error(), msg );
        }

        _out_key.assign( reinterpret_cast< const char* >( key_hash ), _key.size() );
        _out_iv.assign( reinterpret_cast< const char* >( init_vec ), _key.size() );
            
        delete [] key_buff;
        delete [] salty;
        delete [] init_vec;
        delete [] key_hash;

        return SUCCESS();

    } // buffer_crypt::initialization_vector

    // =-=-=-=-=-=-=-
    // public - encryptor
    eirods::error buffer_crypt::encrypt( 
        const std::string& _key,
        const std::string& _iv,
        const std::string& _in_buf,
        std::string&       _out_buf ) {
        // =-=-=-=-=-=-=-
        // create an encryption context
        unsigned char* key_buff = reinterpret_cast< unsigned char* >( 
                                      const_cast< char* >( _key.c_str() ) );
        unsigned char* init_vec = reinterpret_cast< unsigned char* >( 
                                      const_cast< char* >( _iv.c_str() ) );
        EVP_CIPHER_CTX context;
        EVP_CIPHER_CTX_init( &context );
        int ret = EVP_EncryptInit_ex(
                    &context, 
                    EVP_get_cipherbyname( algorithm_.c_str() ),
                    NULL, 
                    key_buff, 
                    init_vec );
         if( 0 == ret ) {
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
        unsigned char* txt_buff    = reinterpret_cast< unsigned char* >( 
                                      const_cast< char* >( _in_buf.c_str() ) );

        // =-=-=-=-=-=-=-
        // update ciphertext, cipher_len is filled with the length of ciphertext generated,
        ret = EVP_EncryptUpdate( 
                      &context, 
                      cipher_text, 
                      &cipher_len, 
                      txt_buff,
                      _in_buf.size() );
        if( 0 == ret ) {
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
                  &context, 
                  cipher_text+cipher_len, 
                  &final_len ); 
        if( 0 == ret ) {
            char err[ 256 ];
            ERR_error_string_n( ERR_get_error(), err, 256 );
            std::string msg( "failed in EVP_EncryptFinal_ex - " );
            msg += err;
            return ERROR( ERR_get_error(), msg );
        }

        // =-=-=-=-=-=-=-
        // clean up and assign out variables before exit
        _out_buf.assign( 
            reinterpret_cast< const char* >( cipher_text ), 
            cipher_len+final_len );

        delete [] cipher_text;
 
        return SUCCESS();

    } // encrypt 
 
    // =-=-=-=-=-=-=-
    // public - decryptor
    eirods::error buffer_crypt::decrypt( 
        const std::string& _key,
        const std::string& _iv,
        const std::string& _in_buf,
        std::string&       _out_buf ) {
        // =-=-=-=-=-=-=-
        // create an decryption context
        unsigned char* key_buff = reinterpret_cast< unsigned char* >( 
                                      const_cast< char* >( _key.c_str() ) );
        unsigned char* iv_buff  = reinterpret_cast< unsigned char* >( 
                                      const_cast< char* >( _iv.c_str() ) );
        EVP_CIPHER_CTX context;
        EVP_CIPHER_CTX_init( &context );
        int ret = EVP_DecryptInit_ex( 
                    &context, 
                    EVP_get_cipherbyname( algorithm_.c_str() ),
                    NULL, 
                    key_buff,
                    iv_buff );
         if( 0 == ret ) {
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
        unsigned char* plain_text = new unsigned char[ _in_buf.size() + AES_BLOCK_SIZE ];
        unsigned char* txt_buff   = reinterpret_cast< unsigned char* >( 
                                        const_cast< char* >( _in_buf.c_str() ) );
        // =-=-=-=-=-=-=-
        // update the plain text, plain_len is filled with the length of the plain text
        ret = EVP_DecryptUpdate(
                      &context, 
                      plain_text, 
                      &plain_len, 
                      txt_buff, 
                      _in_buf.size() );
         if( 0 == ret ) {
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
                  &context, 
                  plain_text+plain_len, 
                  &final_len );
        if( 0 == ret ) {
            char err[ 256 ];
            ERR_error_string_n( ERR_get_error(), err, 256 );
            std::string msg( "failed in EVP_DecryptFinal_ex - " );
            msg += err;
            return ERROR( ERR_get_error(), msg );
        }

        // =-=-=-=-=-=-=-
        // assign the plain text to the outvariable and clean up 
        _out_buf.assign( 
            reinterpret_cast< const char* >( plain_text ), 
            plain_len+final_len );

        delete [] plain_text;

        return SUCCESS();

    } // decrypt 

}; // namespace eirods



