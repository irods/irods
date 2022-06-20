#ifndef IRODS_METADATA_HPP
#define IRODS_METADATA_HPP

#include "irods/entity.hpp"

#include <string>
#include <vector>
#include <map>

#ifdef IRODS_METADATA_ENABLE_SERVER_SIDE_API
struct RsComm;
using rxComm_t = RsComm;
#else
struct RcComm;
using rxComm_t = RcComm;
#endif // IRODS_METADATA_ENABLE_SERVER_SIDE_API

namespace irods::experimental::metadata
{
    using entity_type = irods::experimental::entity::entity_type;

    struct avu
    {
        std::string attribute;
        std::string value;
        std::string units;
    };

    bool operator==(const avu& _lhs, const avu& _rhs) noexcept;

    /// \returns irods::experimental::entity::entity_type
    /// \retval An entity_type enumeration
    /// \brief Converts an entity string token into an entity enumerated type
    /// \since 4.2.9
    entity_type to_entity_type(const std::string& _t);

    /// \returns std::string
    /// \retval A rxModAVUMetadata entity token string
    /// \brief Converts an entity type enumeration to a string token
    /// \since 4.2.9
    std::string to_entity_token(entity_type _t);

    /// \returns std::string
    /// \retval A rxModAVUMetadata entity token string
    /// \brief Converts an entity type enumeration to a string token
    /// \since 4.2.9
    std::string to_entity_string(entity_type _t);

    /// \brief Set a metadata attribute, value and unit for a given entity type
    ///        note that this will possibly overwrite an existing metadata entry
    /// \since 4.2.9
    void set(rxComm_t& _comm, const avu& _md, entity_type _et, const std::string& _tgt);

    /// \brief Add a metadata triple to a given entity type without the possibility of an overwrite
    /// \since 4.2.9
    void add(rxComm_t& _comm, const avu& _md, entity_type _et, const std::string& _tgt);

    /// \brief Remove a metadata triple from a given entity type
    /// \since 4.2.9
    void remove(rxComm_t& _comm, const avu& _md, entity_type _et, const std::string& _tgt);

    /// \brief Modify a given metadata triple with new values
    /// \since 4.2.9
    void modify(rxComm_t& _comm, const avu& _md, const avu& _dmd, entity_type _et, const std::string& _tgt);

    /// \returns A vector of metadata triples
    /// \retval std::vector<avu>
    /// \brief Fetch all metadata triples for a given entity type from the catalog
    /// \since 4.2.9
    std::vector<avu> get(rxComm_t& _comm, entity_type _et, const std::string& _tgt);
} // namespace irods::experimental::metadata

#endif // IRODS_METADATA_HPP
