#ifndef IRODS_METADATA_HPP
#define IRODS_METADATA_HPP

#include <string>
#include <vector>
#include <map>

#include "entity.hpp"

#ifdef IRODS_METADATA_ENABLE_SERVER_SIDE_API
    #include "rsModAVUMetadata.hpp"
    using rxComm_t = rsComm_t;
    #define rxModAVUMetadata rsModAVUMetadata
    #define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
    #define IRODS_QUERY_ENABLE_SERVER_SIDE_API
#else
    #include "modAVUMetadata.h"
    using rxComm_t = rcComm_t;
    #define rxModAVUMetadata rcModAVUMetadata
#endif

#include "irods_query.hpp"
#include "filesystem.hpp"
#include "fmt/format.h"

namespace irods::experimental::metadata {
    /// \brief This library provides an interface for metadata interaction
    /// \since 4.2.9



    /// \brief A structure for representing iRODS metadata triples
    /// iRODS provides metadata triples within the catalog known as attribute, value and units
    /// \since 4.2.9
    struct avu {
        std::string attribute;
        std::string value;
        std::string units;
    };

    auto operator==(const avu& _lhs, const avu& _rhs) noexcept {
        return _lhs.attribute == _rhs.attribute &&
               _lhs.value     == _rhs.value &&
               _lhs.units     == _rhs.units;
    }

    namespace {

        /// \brief Tokens used for representing entity type for rxModAVUMetadata
        /// \since 4.2.9
        namespace tokens {
            const std::string collection{"-C"};
            const std::string data_object{"-d"};
            const std::string user{"-u"};
            const std::string resource{"-R"};
        }

        using entity_type = irods::experimental::entity::entity_type;

        /// \brief Map which converts an entity type to a string representation
        /// \since 4.2.9
        const std::map<entity_type, std::string> type_to_string {
            {entity_type::collection,  "collection"},
            {entity_type::data_object, "data_object"},
            {entity_type::user,        "user"},
            {entity_type::resource,    "resource"}
        };

        /// \brief Map which converts an entity type to a rxModAVUMetadata token
        /// \since 4.2.9
        const std::map<entity_type, std::string> type_to_token {
            {entity_type::collection,  tokens::collection},
            {entity_type::data_object, tokens::data_object},
            {entity_type::user,        tokens::user},
            {entity_type::resource,    tokens::resource}
        };

        /// \brief Map which converts a rxModAVUMetadata token to an entity type
        /// \since 4.2.9
        const std::map<std::string, entity_type> token_to_type {
            {tokens::collection,  entity_type::collection},
            {tokens::data_object, entity_type::data_object},
            {tokens::user,        entity_type::user},
            {tokens::resource,    entity_type::resource}
        };

        /// \brief convenience function for invoking rxModAVUMetadata
        /// \since 4.2.9
        auto mod_avu_meta(rxComm_t& _comm, const avu& _md, const avu& _dmd, entity_type _et, const std::string& _tgt, const std::string& _op)
        {
            modAVUMetadataInp_t inp{
                const_cast<char*>(_op.c_str()),
                const_cast<char*>(type_to_token.at(_et).c_str()),
                const_cast<char*>(_tgt.c_str()),
                const_cast<char*>(_md.attribute.c_str()),
                const_cast<char*>(_md.value.c_str()),
                const_cast<char*>(_md.units.c_str()),
                const_cast<char*>(_dmd.attribute.c_str()),
                const_cast<char*>(_dmd.value.c_str()),
                const_cast<char*>(_dmd.units.c_str()),
                nullptr, // unused arg9
                KeyValPair{}
            };

            return rxModAVUMetadata(&_comm, &inp);
        } // mod_avu_meta

        template<typename M_T, typename T_T>
        auto throw_if_invalid(const M_T& _m, const T_T& _t)
        {
            if(_m.find(_t) == _m.end()) {
                THROW(SYS_INVALID_INPUT_PARAM, "token or entity not found");
            }
        }

    } // namespace


    /// \returns irods::experimental::entity::entity_type
    /// \retval An entity_type enumeration
    /// \brief Converts an entity string token into an entity enumerated type
    /// \since 4.2.9
    auto to_entity_type(const std::string& _t)
    {
        throw_if_invalid(token_to_type, _t);
        return token_to_type.at(_t);

    } // to_entity_type


    /// \returns std::string
    /// \retval A rxModAVUMetadata entity token string
    /// \brief Converts an entity type enumeration to a string token
    /// \since 4.2.9
    auto to_entity_token(entity_type _t)
    {
        throw_if_invalid(type_to_token, _t);
        return type_to_token.at(_t);

    } // to_entity_token


    /// \returns std::string
    /// \retval A rxModAVUMetadata entity token string
    /// \brief Converts an entity type enumeration to a string token
    /// \since 4.2.9
    auto to_entity_string(entity_type _t)
    {
        throw_if_invalid(type_to_string, _t);
        return type_to_string.at(_t);

    } // to_entity_string


    /// \brief Set a metadata attribute, value and unit for a given entity type
    ///        note that this will possibly overwrite an existing metadata entry
    /// \since 4.2.9
    void set(rxComm_t& _comm, const avu& _md, entity_type _et, const std::string& _tgt)
    {
        if(const auto ec = mod_avu_meta(_comm, _md, {}, _et, _tgt, "set"); ec < 0) {
            THROW(ec, fmt::format("metadata set failed for [{}]", _tgt));
        }

    } // set


    /// \brief Add a metadata triple to a given entity type without the possibility of an overwrite
    /// \since 4.2.9
    void add(rxComm_t& _comm, const avu& _md, entity_type _et, const std::string& _tgt)
    {

        if(const auto ec = mod_avu_meta(_comm, _md, {}, _et, _tgt, "add"); ec < 0) {
            THROW(ec, fmt::format("metadata add failed for [{}]", _tgt));
        }

    } // add


    /// \brief Remove a metadata triple from a given entity type
    /// \since 4.2.9
    void remove(rxComm_t& _comm, const avu& _md, entity_type _et, const std::string& _tgt)
    {

        if(const auto ec = mod_avu_meta(_comm, _md, {}, _et, _tgt, "rm"); ec < 0) {
            THROW(ec, fmt::format("metadata remove failed for [{}]", _tgt));
        }

    } // remove


    /// \brief Modify a given metadata triple with new values
    /// \since 4.2.9
    void modify(rxComm_t& _comm, const avu& _md, const avu& _dmd, entity_type _et, const std::string& _tgt)
    {

        avu tmp{
              std::string{"n:"} + _dmd.attribute
            , std::string{"v:"} + _dmd.value
            , std::string{"u:"} + _dmd.units
        };

        if(const auto ec = mod_avu_meta(_comm, _md, tmp, _et, _tgt, "mod"); ec < 0) {
            THROW(ec, fmt::format("metadata modify failed for [{}]", _tgt));
        }

    } // modify


    /// \returns A vector of metadata triples
    /// \retval std::vector<avu>
    /// \brief Fetch all metadata triples for a given entity type from the catalog
    /// \since 4.2.9
    auto get(rxComm_t& _comm, entity_type _et, const std::string& _tgt) -> std::vector<avu>
    {
        namespace fs = irods::experimental::filesystem;

        auto temp = std::string{"SELECT META_{}_ATTR_NAME, META_{}_ATTR_VALUE, META_{}_ATTR_UNITS WHERE {}"};

        const std::map<entity_type, std::string> type_to_string {
            {entity_type::collection,  "COLL"},
            {entity_type::data_object, "DATA"},
            {entity_type::user,        "USER"},
            {entity_type::resource,    "RESC"}
        };

        const std::map<entity_type, std::string> type_to_query {
            {entity_type::collection,  "COLL_NAME = '{}'"},
            {entity_type::data_object, "COLL_NAME = '{}' AND DATA_NAME = '{}'"},
            {entity_type::user,        "USER_NAME = '{}'"},
            {entity_type::resource,    "RESC_NAME = '{}'"}
        };

        auto where = type_to_query.at(_et);

        std::string where_fmt;
        if(entity_type::data_object == _et) {
            fs::path p{_tgt};
            auto obj  = p.object_name().string();
            auto coll = p.parent_path().string();

            where_fmt = fmt::format(where, coll, obj);
        }
        else {
            where_fmt = fmt::format(where, _tgt);
        }

        auto type_str  = type_to_string.at(_et);
        auto query_str = fmt::format(temp, type_str, type_str, type_str, where_fmt);

        auto res = std::vector<avu>{};

        try {
            for(auto&& r : query(&_comm, query_str)) {
                res.push_back(avu{r[0], r[1], r[2]});
            }
        }
        catch(const irods::exception& e) {
            if(e.code() != CAT_NO_ROWS_FOUND) {
                THROW(e.code(), e.what());
            }
        }

        return res;

    } // get

} // namespace irods::experimental::metadata

#endif // IRODS_METADATA_HPP
