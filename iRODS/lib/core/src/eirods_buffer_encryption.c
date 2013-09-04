


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
    buffer_crypt::buffer_crypt() {

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
        unsigned char key[ 32 ];
        int rnd_err = RAND_bytes( key, 32 );
        if( 1 != rnd_err ) {
            return ERROR( 
                       ERR_get_error(), 
                       "error generating random key" );
        }

        // =-=-=-=-=-=-=-
        // assign the key to the out variable
        _out_key.assign( 
            reinterpret_cast< const char* >( key ), 
            32 );

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
        unsigned char salty[ 64 ];
        int rnd_err = RAND_bytes( salty, 64 );
        if( 1 != rnd_err ) {
            return ERROR( 
                       ERR_get_error(), 
                       "error generating random salt" );
        }

        // =-=-=-=-=-=-=-
        // build the initialization vector
        int num_rounds = 2;
        unsigned char* init_vec = new unsigned char[ _key.size() ];
        unsigned char* key_hash = new unsigned char[ _key.size() ];
        unsigned char* key_buff = reinterpret_cast< unsigned char* >( 
                                      const_cast< char* >( _key.c_str() ) );
        size_t sz = EVP_BytesToKey( 
                    EVP_aes_256_cbc(), 
                        EVP_sha1(), 
                        salty, 
                        key_buff,
                        _key.size(), 
                        num_rounds, 
                        key_hash, 
                        init_vec );
        if( _key.size() != sz ) {
            delete [] init_vec;
            delete [] key_hash;
            return ERROR(
                       ERR_get_error(),
                       "key size mismatch on key gen" );
        }

        _out_key.assign( reinterpret_cast< const char* >( key_hash ), _key.size() );
        _out_iv.assign( reinterpret_cast< const char* >( init_vec ), _key.size() );
            
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
        EVP_EncryptInit_ex(
            &context, 
            EVP_aes_256_cbc(), 
            NULL, 
            key_buff, 
            init_vec );
        
        // =-=-=-=-=-=-=-
        // max ciphertext len for a n bytes of plaintext is n + AES_BLOCK_SIZE -1 bytes 
        int            cipher_len  = _in_buf.size() + AES_BLOCK_SIZE;
        unsigned char* cipher_text = new unsigned char[ cipher_len ] ;
        unsigned char* txt_buff    = reinterpret_cast< unsigned char* >( 
                                      const_cast< char* >( _in_buf.c_str() ) );

        // =-=-=-=-=-=-=-
        // update ciphertext, cipher_len is filled with the length of ciphertext generated,
        EVP_EncryptUpdate( 
            &context, 
            cipher_text, 
            &cipher_len, 
            txt_buff,
            _in_buf.size() );

        // =-=-=-=-=-=-=-
        // update ciphertext with the final remaining bytes 
        int final_len = 0;
        EVP_EncryptFinal_ex( 
            &context, 
            cipher_text+cipher_len, 
            &final_len ); 

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
        EVP_DecryptInit_ex( 
            &context, 
            EVP_aes_256_cbc(), 
            NULL, 
            key_buff,
            iv_buff );

        // =-=-=-=-=-=-=-
        // allocate a plain text buffer
        // because we have padding ON, we must allocate an extra cipher block size of memory 
        int            plain_len  = _in_buf.size();
        unsigned char* plain_text = new unsigned char[ plain_len + AES_BLOCK_SIZE ];
        unsigned char* txt_buff   = reinterpret_cast< unsigned char* >( 
                                        const_cast< char* >( _in_buf.c_str() ) );
        // =-=-=-=-=-=-=-
        // update the plain text, plain_len is filled with the length of the plain text
        EVP_DecryptUpdate(
            &context, 
            plain_text, 
            &plain_len, 
            txt_buff, 
            _in_buf.size() );
        
        // =-=-=-=-=-=-=-
        // finalize the plain text, final_len is filled with the resulting length of the plain text
        int final_len = 0;
        EVP_DecryptFinal_ex(
            &context, 
            plain_text+plain_len, 
            &final_len );

        // =-=-=-=-=-=-=-
        // assign the plain text to the outvariable and clean up 
        _out_buf.assign( 
            reinterpret_cast< const char* >( plain_text ), 
            plain_len+final_len );

        delete [] plain_text;

        return SUCCESS();

    } // decrypt 

}; // namespace eirods



