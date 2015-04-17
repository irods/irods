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
}
#endif

extern "C" {
    char* serialize_list_c( const char** metadata, size_t metadata_len );
}
#endif
