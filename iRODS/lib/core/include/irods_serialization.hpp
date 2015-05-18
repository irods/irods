#ifndef IRODS_SERIALIZATION_HPP__
#define IRODS_SERIALIZATION_HPP__

#ifdef __cplusplus
#include <string>
#include <vector>

namespace irods {
    std::string serialize_list( const std::vector<std::string>& metadata );
    std::vector<std::string> deserialize_list( const std::string& metadata );
    std::string serialize_metadata( const std::vector<std::string>& metadata );
    std::vector<std::string> deserialize_metadata( const std::string& metadata );
    std::string serialize_acl( const std::vector<std::vector<std::string> >& acl );
    std::vector<std::vector<std::string> > deserialize_acl( const std::string& acl );
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

char* serialize_list_c( const char** list, size_t list_len );
char* serialize_metadata_c( const char** metadata, size_t metadata_len );
char* serialize_acl_c( const char** acl, size_t acl_len );

#ifdef __cplusplus
}
#endif

#endif
