#ifndef IRODS_REPLICA_PROXY_HPP
#define IRODS_REPLICA_PROXY_HPP

/// \file

#include "filesystem.hpp"
#include "irods_exception.hpp"
#include "key_value_proxy.hpp"
#include "objInfo.h"
#include "replica.hpp"

#include "json.hpp"

#include <string_view>

namespace irods::experimental::replica
{
    /// \brief Presents a replica-level interface to a DataObjInfo legacy iRODS struct.
    ///
    /// Holds a pointer to a DataObjInfo whose lifetime is managed outside of the proxy object.
    ///
    /// The replica_proxy is essentially a wrapper around a node of the linked list of a DataObjInfo struct.
    /// This is meant to be used as an interface to the physical representation of a data object, also known
    /// as a replica. All members of the DataObjInfo struct are accessible through this interface except for
    /// the next pointer because the object has no concept of a "next" replica.
    ///
    /// \since 4.2.9
    template<
        typename I,
        typename = std::enable_if_t<
            std::is_same_v<DataObjInfo, typename std::remove_const_t<I>>
        >
    >
    class replica_proxy
    {
    public:
        // Aliases for various types used in replica_proxy
        using doi_type = I;
        using doi_pointer_type = doi_type*;

        /// \brief Constructs proxy using an existing doi_type
        /// \since 4.2.9
        explicit replica_proxy(doi_type& _doi)
            : doi_{&_doi}
        {
        }

        // clang-format off
        auto logical_path()     const noexcept -> std::string_view { return doi_->objPath; }
        auto resource()         const noexcept -> std::string_view { return doi_->rescName; }
        auto hierarchy()        const noexcept -> std::string_view { return doi_->rescHier; }
        auto type()             const noexcept -> std::string_view { return doi_->dataType; }
        auto size()             const noexcept -> rodsLong_t       { return doi_->dataSize; }
        auto checksum()         const noexcept -> std::string_view { return doi_->chksum; }
        auto version()          const noexcept -> std::string_view { return doi_->version; }
        auto physical_path()    const noexcept -> std::string_view { return doi_->filePath; }
        auto owner_user_name()  const noexcept -> std::string_view { return doi_->dataOwnerName; }
        auto owner_zone_name()  const noexcept -> std::string_view { return doi_->dataOwnerZone; }
        auto replica_number()   const noexcept -> int              { return doi_->replNum; }
        auto replica_status()   const noexcept -> int              { return doi_->replStatus; }
        auto data_id()          const noexcept -> rodsLong_t       { return doi_->dataId; }
        auto collection_id()    const noexcept -> rodsLong_t       { return doi_->collId; }
        auto resource_id()      const noexcept -> rodsLong_t       { return doi_->rescId; }
        auto comments()         const noexcept -> std::string_view { return doi_->dataComments; }
        auto ctime()            const noexcept -> std::string_view { return doi_->dataCreate; }
        auto mtime()            const noexcept -> std::string_view { return doi_->dataModify; }
        auto in_pdmo()          const noexcept -> std::string_view { return doi_->in_pdmo; }
        auto status()           const noexcept -> std::string_view { return doi_->statusString; }
        auto mode()             const noexcept -> std::string_view { return doi_->dataMode; }
        auto data_expiry()      const noexcept -> std::string_view { return doi_->dataExpiry; }
        auto map_id()           const noexcept -> int              { return doi_->dataMapId; }
        // clang-format on

        /// \returns key_value_proxy
        ///
        /// \returns condInput for the DataObjInfo node as a key_value_proxy
        ///
        /// \since 4.2.9
        auto cond_input()       const -> key_value_proxy<const KeyValPair>
        {
            return make_key_value_proxy<const KeyValPair>(doi_->condInput);
        }

        /// \returns const SpecColl*
        ///
        /// \returns specColl pointer for the DataObjInfo node
        ///
        /// \since 4.2.9
        auto special_collection_info() const noexcept -> const SpecColl*
        {
            return doi_->specColl;
        }

        /// \returns const doi_pointer_type
        ///
        /// \returns Pointer to the underlying struct
        ///
        /// \since 4.2.9
        auto get() const noexcept -> const doi_pointer_type { return doi_; }

        /// \returns Whether the replica is considered at rest
        /// \retval true If the passed replica status is considered at rest
        /// \retval false If the passed replica status is not considered at rest
        ///
        /// \since 4.2.9
        auto at_rest() const -> bool { return INTERMEDIATE_REPLICA != replica_status(); }

        /// \returns Whether the replica is locked at the logical level
        /// \retval true If the passed replica status is locked at the logical level
        /// \retval false If the passed replica status is not locked at the logical level
        ///
        /// \since 4.2.9
        auto locked() const -> bool
        {
            return READ_LOCKED  == replica_status() ||
                   WRITE_LOCKED == replica_status();
        }

        // mutators

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto logical_path(std::string_view _lp) -> void
        {
            set_string_property(doi_->objPath, _lp, sizeof(doi_->objPath));
        }

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto resource(std::string_view _r) -> void
        {
            set_string_property(doi_->rescName, _r, sizeof(doi_->rescName));
        }

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto hierarchy(std::string_view _h) -> void
        {
            set_string_property(doi_->rescHier, _h, sizeof(doi_->rescHier));
        }

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto type(std::string_view _t) -> void
        {
            set_string_property(doi_->dataType, _t, sizeof(doi_->dataType));
        }

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto size(const rodsLong_t _s)              -> void { doi_->dataSize = _s; }

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto checksum(std::string_view _cs) -> void
        {
            set_string_property(doi_->chksum, _cs, sizeof(doi_->chksum));
        }

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto version(std::string_view _v) -> void
        {
            set_string_property(doi_->version, _v, sizeof(doi_->version));
        }

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto physical_path(std::string_view _p) -> void
        {
            set_string_property(doi_->filePath, _p, sizeof(doi_->filePath));
        }

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto owner_user_name(std::string_view _oun) -> void
        {
            set_string_property(doi_->dataOwnerName, _oun, sizeof(doi_->dataOwnerName));
        }

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto owner_zone_name(std::string_view _ozn) -> void
        {
            set_string_property(doi_->dataOwnerZone, _ozn, sizeof(doi_->dataOwnerZone));
        }

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto replica_number(const int _rn)          -> void { doi_->replNum = _rn; }

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto replica_status(const int _s)           -> void { doi_->replStatus = _s; }

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto data_id(const rodsLong_t _id)          -> void { doi_->dataId = _id; }

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto collection_id(const rodsLong_t _id)    -> void { doi_->collId = _id; }

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto resource_id(const rodsLong_t _id)      -> void { doi_->rescId = _id; }

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto comments(std::string_view _c) -> void
        {
            set_string_property(doi_->dataComments, _c, sizeof(doi_->dataComments));
        }

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto ctime(std::string_view _ct) -> void
        {
            set_string_property(doi_->dataCreate, _ct, sizeof(doi_->dataCreate));
        }

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto mtime(std::string_view _mt) -> void
        {
            set_string_property(doi_->dataModify, _mt, sizeof(doi_->dataModify));
        }

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto in_pdmo(std::string_view _p) -> void
        {
            set_string_property(doi_->in_pdmo, _p, sizeof(doi_->in_pdmo));
        }

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto status(std::string_view _s) -> void
        {
            set_string_property(doi_->statusString, _s, sizeof(doi_->statusString));
        }

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto mode(std::string_view _m) -> void
        {
            set_string_property(doi_->dataMode, _m, sizeof(doi_->dataMode));
        }

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto data_expiry(std::string_view _e) -> void
        {
            set_string_property(doi_->dataExpiry, _e, sizeof(doi_->dataExpiry));
        }

        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto map_id(const int _m) -> void { doi_->dataMapId = _m; }

        /// \returns key_value_proxy
        ///
        /// \returns condInput for the DataObjInfo node as a key_value_proxy
        ///
        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto cond_input() -> key_value_proxy<KeyValPair>
        {
            return make_key_value_proxy(doi_->condInput);
        }

        /// \returns SpecColl*
        ///
        /// \returns specColl pointer for the DataObjInfo node
        ///
        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto special_collection_info() -> SpecColl*
        {
            return doi_->specColl;
        }

        /// \returns Pointer to the underlying struct
        ///
        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto get() -> doi_pointer_type { return doi_; }

    private:
        /// \brief Pointer to underlying doi_type
        /// \since 4.2.9
        doi_pointer_type doi_;

        /// \brief Sets a fixed-sized string property in the underlying struct
        /// \param[out] _dst - Destination buffer
        /// \param[in] _src - Source string
        /// \param[in] _dst_size - Destination buffer size
        /// \since 4.2.9
        inline auto set_string_property(
            char* _dst,
            std::string_view _src,
            const std::size_t _dst_size) -> void
        {
            if (_src.size() >= _dst_size) {
                THROW(USER_STRLEN_TOOLONG, "source length exceeds destination buffer length");
            }
            std::memset(_dst, 0, _dst_size);
            std::snprintf(_dst, _dst_size, "%s", _src.data());
        } // set_string_property
    }; // class replica_proxy

    /// \brief Helper type for replica_proxy using a non-const DataObjInfo
    ///
    /// \since 4.2.9
    using replica_proxy_t = replica_proxy<DataObjInfo>;

    namespace detail
    {
        static auto populate_struct_from_results(
            DataObjInfo& _doi,
            const std::vector<std::string>& _info) -> void
        {
            namespace fs = irods::experimental::filesystem;
            using index = detail::genquery_column_index;

            try {
                auto proxy = replica_proxy{_doi};

                proxy.data_id(std::stoul(_info[index::DATA_ID]));
                proxy.collection_id(std::stoul(_info[index::DATA_COLL_ID]));
                proxy.replica_number(std::stoi(_info[index::DATA_REPL_NUM]));
                proxy.version(_info[index::DATA_VERSION]);
                proxy.type(_info[index::DATA_TYPE_NAME]);
                proxy.size(std::stoul(_info[index::DATA_SIZE]));
                proxy.physical_path(_info[index::DATA_PATH]);
                proxy.owner_user_name(_info[index::DATA_OWNER_NAME]);
                proxy.owner_zone_name(_info[index::DATA_OWNER_ZONE]);
                proxy.replica_status(std::stoi(_info[index::DATA_REPL_STATUS]));
                proxy.status(_info[index::DATA_STATUS]);
                proxy.checksum(_info[index::DATA_CHECKSUM]);
                proxy.mode(_info[index::DATA_MODE]);
                proxy.comments(_info[index::DATA_COMMENTS]);
                proxy.ctime(_info[index::DATA_CREATE_TIME]);
                proxy.mtime(_info[index::DATA_MODIFY_TIME]);
                proxy.resource_id(std::stoul(_info[index::DATA_RESC_ID]));
                proxy.data_expiry(_info[index::DATA_EXPIRY]);
                proxy.map_id(std::stoi(_info[index::DATA_MAP_ID]));

                proxy.resource(_info[index::DATA_RESC_NAME]);
                proxy.hierarchy(_info[index::DATA_RESC_HIER]);

                const auto lp = fs::path{_info[index::COLL_NAME]} / _info[index::DATA_NAME];
                proxy.logical_path(lp.c_str());
            }
            catch (const std::exception& e)
            {
                THROW(SYS_LIBRARY_ERROR, e.what());
            }
        } // populate_struct_from_results
    } // namespace detail

    /// \brief Factory method for making a replica proxy object for a DataObjInfo
    ///
    /// \param[in] _doi - Pre-existing doi_type which will be wrapped by the returned proxy.
    ///
    /// \return replica_proxy
    ///
    /// \since 4.2.9
    template<typename doi_type>
    static auto make_replica_proxy(doi_type& _doi) -> replica_proxy<doi_type>
    {
        return replica_proxy{_doi};
    } // make_replica_proxy

    /// \brief Factory method for making a replica proxy object for a DataObjInfo
    ///
    /// Allocates a new DataObjInfo and wraps the struct in a proxy and lifetime_manager
    ///
    /// \returns replica_proxy and lifetime_manager for managing a new DataObjInfo
    ///
    /// \since 4.2.9
    static auto make_replica_proxy() -> std::pair<replica_proxy_t, lifetime_manager<DataObjInfo>>
    {
        DataObjInfo* doi = static_cast<DataObjInfo*>(std::malloc(sizeof(DataObjInfo)));
        std::memset(doi, 0, sizeof(DataObjInfo));
        return {replica_proxy{*doi}, lifetime_manager{*doi}};
    } // make_replica_proxy

    /// \brief Fetch information from the catalog and create a new replica proxy with it
    ///
    /// Allocates a new DataObjInfo and wraps the struct in a proxy and lifetime_manager
    ///
    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _replica_number
    ///
    /// \returns replica_proxy and lifetime_manager for managing a new DataObjInfo
    ///
    /// \since 4.2.9
    template<typename rxComm>
    static auto make_replica_proxy(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const replica_number_type _replica_number)
        -> std::pair<replica_proxy_t, lifetime_manager<DataObjInfo>>
    {
        DataObjInfo* doi = static_cast<DataObjInfo*>(std::malloc(sizeof(DataObjInfo)));
        std::memset(doi, 0, sizeof(DataObjInfo));

        const auto result = get_data_object_info(_comm, _logical_path, _replica_number).front();

        detail::populate_struct_from_results(*doi, result);

        return {replica_proxy{*doi}, lifetime_manager{*doi}};
    } // make_replica_proxy

    /// \brief Fetch information from the catalog and create a new replica proxy with it
    ///
    /// Allocates a new DataObjInfo and wraps the struct in a proxy and lifetime_manager
    ///
    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _leaf_resource_name
    ///
    /// \returns replica_proxy and lifetime_manager for managing a new DataObjInfo
    ///
    /// \since 4.2.9
    template<typename rxComm>
    static auto make_replica_proxy(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const leaf_resource_name_type _leaf_resource_name)
        -> std::pair<replica_proxy_t, lifetime_manager<DataObjInfo>>
    {
        DataObjInfo* doi = static_cast<DataObjInfo*>(std::malloc(sizeof(DataObjInfo)));
        std::memset(doi, 0, sizeof(DataObjInfo));

        const auto result = get_data_object_info(_comm, _logical_path, _leaf_resource_name).front();

        detail::populate_struct_from_results(*doi, result);

        return {replica_proxy{*doi}, lifetime_manager{*doi}};
    } // make_replica_proxy

    /// \brief Takes an existing replica_proxy and duplicates the underlying struct.
    ///
    /// \param[in] _replica replica to duplicate
    ///
    /// \returns replica_proxy and lifetime_manager for underlying struct
    ///
    /// \since 4.2.9
    static auto duplicate_replica(const DataObjInfo& _replica)
        -> std::pair<replica_proxy_t, lifetime_manager<DataObjInfo>>
    {
        DataObjInfo* curr = static_cast<DataObjInfo*>(std::malloc(sizeof(DataObjInfo)));
        std::memset(curr, 0, sizeof(DataObjInfo));
        std::memcpy(curr, &_replica, sizeof(DataObjInfo));

        // Do not maintain the linked list - muy peligroso
        curr->next = nullptr;

        replKeyVal(&_replica.condInput, &curr->condInput);

        if (_replica.specColl) {
            SpecColl* tmp_sc = static_cast<SpecColl*>(std::malloc(sizeof(SpecColl)));
            std::memset(tmp_sc, 0, sizeof(SpecColl));
            std::memcpy(tmp_sc, _replica.specColl, sizeof(SpecColl));
            curr->specColl = tmp_sc;
        }

        if (!curr) {
            THROW(SYS_INTERNAL_ERR, "list remains unpopulated");
        }

        return {replica_proxy{*curr}, lifetime_manager{*curr}};
    } // duplicate_replica

    /// \brief Takes an existing replica_proxy and duplicates the underlying struct.
    ///
    /// \param[in] _replica replica to duplicate
    ///
    /// \returns replica_proxy and lifetime_manager for underlying struct
    ///
    /// \since 4.2.9
    static auto duplicate_replica(const replica_proxy_t& _replica)
        -> std::pair<replica_proxy_t, lifetime_manager<DataObjInfo>>
    {
        return duplicate_replica(*_replica.get());
    } // duplicate_replica

    /// \brief Takes a structured JSON input and creates a replica proxy
    ///
    /// \param[in] _logical_path The DataObjInfo holds an objPath, but the catalog only holds the data name and collection ID
    /// \param[in] _input \parblock
    /// Structured JSON of the following format (order is unimportant, but all fields must be included):
    /// \code{.js}
    ///     {
    ///         "data_id": <string>,
    ///         "coll_id": <string>,
    ///         "data_repl_num": <string>,
    ///         "data_version": <string>,
    ///         "data_type_name": <string>,
    ///         "data_size": <string>,
    ///         "data_path": <string>,
    ///         "data_owner_name": <string>,
    ///         "data_owner_zone": <string>,
    ///         "data_is_dirty": <string>,
    ///         "data_status": <string>,
    ///         "data_checksum": <string>,
    ///         "data_expiry_ts": <string>,
    ///         "data_map_id": <string>,
    ///         "data_mode": <string>,
    ///         "r_comment": <string>,
    ///         "create_ts": <string>,
    ///         "modify_ts": <string>,
    ///         "resc_id": <string>
    ///     }
    /// \endcode
    /// \endparblock
    ///
    /// \returns replica_proxy and lifetime_manager for underlying struct
    ///
    /// \since 4.2.9
    static auto make_replica_proxy(const std::string_view _logical_path, const nlohmann::json& _input)
        -> std::pair<replica_proxy_t, lifetime_manager<DataObjInfo>>
    {
        auto proxy_lm_pair = make_replica_proxy();
        auto& proxy = proxy_lm_pair.first;

        proxy.data_id(std::stoul(_input.at("data_id").get<std::string>()));
        proxy.collection_id(std::stoul(_input.at("coll_id").get<std::string>()));
        proxy.replica_number(std::stoi(_input.at("data_repl_num").get<std::string>()));
        proxy.version(_input.at("data_version").get<std::string>());
        proxy.type(_input.at("data_type_name").get<std::string>());
        proxy.size(std::stoul(_input.at("data_size").get<std::string>()));
        proxy.physical_path(_input.at("data_path").get<std::string>());
        proxy.owner_user_name(_input.at("data_owner_name").get<std::string>());
        proxy.owner_zone_name(_input.at("data_owner_zone").get<std::string>());
        proxy.replica_status(std::stoi(_input.at("data_is_dirty").get<std::string>()));
        proxy.status(_input.at("data_status").get<std::string>());
        proxy.checksum(_input.at("data_checksum").get<std::string>());
        proxy.data_expiry(_input.at("data_expiry_ts").get<std::string>());
        proxy.map_id(std::stoi(_input.at("data_map_id").get<std::string>()));
        proxy.mode(_input.at("data_mode").get<std::string>());
        proxy.comments(_input.at("r_comment").get<std::string>());
        proxy.ctime(_input.at("create_ts").get<std::string>());
        proxy.mtime(_input.at("modify_ts").get<std::string>());
        proxy.resource_id(std::stoul(_input.at("resc_id").get<std::string>()));

        // TODO: resource() and hierarchy() are not being populated

        // TODO: not part of r_data_main, only tracks data_name and coll_id
        proxy.logical_path(_logical_path);

        return proxy_lm_pair;
    } // make_replica_proxy

    /// \brief Takes a replica proxy and generates a structured JSON object
    ///
    /// \param[in] _proxy replica_proxy containing data object information
    ///
    /// \returns Structured JSON of the following format: \parblock
    /// \code{.js}
    ///     {
    ///         "data_id": <string>,
    ///         "coll_id": <string>,
    ///         "data_repl_num": <string>,
    ///         "data_version": <string>,
    ///         "data_type_name": <string>,
    ///         "data_size": <string>,
    ///         "data_path": <string>,
    ///         "data_owner_name": <string>,
    ///         "data_owner_zone": <string>,
    ///         "data_is_dirty": <string>,
    ///         "data_status": <string>,
    ///         "data_checksum": <string>,
    ///         "data_expiry_ts": <string>,
    ///         "data_map_id": <string>,
    ///         "data_mode": <string>,
    ///         "r_comment": <string>,
    ///         "create_ts": <string>,
    ///         "modify_ts": <string>,
    ///         "resc_id": <string>
    ///     }
    /// \endcode
    /// \endparblock
    ///
    /// \since 4.2.9
    template<typename doi_type>
    static auto to_json(const replica_proxy<doi_type>& _proxy) -> nlohmann::json
    {
        namespace fs = irods::experimental::filesystem;

        return nlohmann::json{
            {"data_id",         std::to_string(_proxy.data_id())},
            {"coll_id",         std::to_string(_proxy.collection_id())},
            {"data_name",       fs::path{_proxy.logical_path().data()}.object_name().c_str()},
            {"data_repl_num",   std::to_string(_proxy.replica_number())},
            {"data_version",    _proxy.version()},
            {"data_type_name",  _proxy.type()},
            {"data_size",       std::to_string(_proxy.size())},
            {"data_path",       _proxy.physical_path()},
            {"data_owner_name", _proxy.owner_user_name()},
            {"data_owner_zone", _proxy.owner_zone_name()},
            {"data_is_dirty",   std::to_string(_proxy.replica_status())},
            {"data_status",     _proxy.status()},
            {"data_checksum",   _proxy.checksum()},
            {"data_expiry_ts",  _proxy.get()->dataExpiry},
            {"data_map_id",     std::to_string(_proxy.get()->dataMapId)},
            {"data_mode",       _proxy.mode()},
            {"r_comment",       _proxy.comments()},
            {"create_ts",       _proxy.ctime()},
            {"modify_ts",       _proxy.mtime()},
            {"resc_id",         std::to_string(_proxy.resource_id())}
        };
    } // to_json
} // namespace irods::experimental::replica

#endif // #ifndef IRODS_REPLICA_PROXY_HPP
