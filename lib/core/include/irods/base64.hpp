#ifndef IRODS_BASE64_HPP
#define IRODS_BASE64_HPP

/// \file

namespace irods
{
    /// Base64 encode a buffer (NUL terminated)
    ///
    /// \param[in]      The input buffer to encode
    /// \param[in]      The length of the input buffer
    /// \param[out]     The destination of the base64 encoded data
    /// \param[in,out]  The max size and resulting size
    ///
    /// \return 0 if successful
    ///
    /// \since 4.3.1
    int base64_encode(const unsigned char* in, unsigned long inlen, unsigned char* out, unsigned long* outlen);

    /// Base64 decode a block of memory
    ///
    /// \param[in]      The base64 data to decode
    /// \param[in]      The length of the base64 data
    /// \param[out]     The destination of the binary decoded data
    /// \param[in,out]  The max size and resulting size of the decoded data
    //
    /// \return 0 if successful
    ///
    /// \since 4.3.1
    int base64_decode(const unsigned char* in, unsigned long inlen, unsigned char* out, unsigned long* outlen);
} // namespace irods

#endif //IRODS_BASE64_HPP
