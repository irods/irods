#ifndef IRODS_REPLICA_PROXY_HPP
#define IRODS_REPLICA_PROXY_HPP

#include "filesystem.hpp"
#include "irods_exception.hpp"
#include "key_value_proxy.hpp"
#include "objInfo.h"
#include "replica.hpp"

#include "json.hpp"

#include <string_view>

namespace irods::experimental::replica
{
    /// \brief Presents a replica-level interface to a dataObjInfo_t legacy iRODS struct.
    ///
    /// Holds a pointer to a dataObjInfo_t whose lifetime is managed outside of the proxy object.
    ///
    /// The replica_proxy is essentially a wrapper around a node of the linked list of a dataObjInfo_t struct.
    /// This is meant to be used as an interface to the physical representation of a data object, also known
    /// as a replica. All members of the dataObjInfo_t struct are accessible through this interface except for
    /// the next pointer because the object has no concept of a "next" replica.
    ///
    /// \since 4.2.9
    template<
        typename I,
        typename = std::enable_if_t<
            std::is_same_v<dataObjInfo_t, typename std::remove_const_t<I>>
        >
    >
    class replica_proxy
    {
    public:
        // Aliases for various types used in data_object_proxy
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
        /// \retval condInput for the dataObjInfo_t node as a key_value_proxy
        ///
        /// \since 4.2.9
        auto cond_input()       const -> key_value_proxy<const keyValPair_t>
        {
            return make_key_value_proxy(doi_->condInput);
        }

        /// \returns const specColl_t*
        ///
        /// \retval specColl pointer for the dataObjInfo_t node
        ///
        /// \since 4.2.9
        auto special_collection_info() const noexcept -> const specColl_t*
        {
            return doi_->specColl;
        }

        /// \returns const doi_pointer_type
        ///
        /// \retval Pointer to the underlying struct
        ///
        /// \since 4.2.9
        auto get() const noexcept -> const doi_pointer_type { return doi_; }

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
        /// \retval condInput for the dataObjInfo_t node as a key_value_proxy
        ///
        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto cond_input() -> key_value_proxy<keyValPair_t>
        {
            return make_key_value_proxy(doi_->condInput);
        }

        /// \returns specColl_t*
        ///
        /// \retval specColl pointer for the dataObjInfo_t node
        ///
        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto special_collection_info() -> specColl_t*
        {
            return doi_->specColl;
        }

        /// \returns doi_pointer_type
        /// \retval Pointer to the underlying struct
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

    namespace detail
    {
        auto populate_struct_from_results(
            dataObjInfo_t& _doi,
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

    /// \brief Factory method for making a replica proxy object for a dataObjInfo_t
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

    /// \brief Factory method for making a replica proxy object for a dataObjInfo_t
    ///
    /// Allocates a new dataObjInfo_t and wraps the struct in a proxy and lifetime_manager
    ///
    /// \return std::pair<replica_proxy<dataObjInfo_t>, lifetime_manager<dataObjInfo_t>>
    /// \retval replica_proxy and lifetime_manager for managing a new dataObjInfo_t
    ///
    /// \since 4.2.9
    static auto make_replica_proxy() -> std::pair<replica_proxy<dataObjInfo_t>, lifetime_manager<dataObjInfo_t>>
    {
        dataObjInfo_t* doi = (dataObjInfo_t*)std::malloc(sizeof(dataObjInfo_t));
        std::memset(doi, 0, sizeof(dataObjInfo_t));
        return {replica_proxy{*doi}, lifetime_manager{*doi}};
    } // make_replica_proxy

    /// \brief Fetch information from the catalog and create a new replica proxy with it
    ///
    /// Allocates a new dataObjInfo_t and wraps the struct in a proxy and lifetime_manager
    ///
    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _replica_number
    ///
    /// \return std::pair<replica_proxy<dataObjInfo_t>, lifetime_manager<dataObjInfo_t>>
    /// \retval replica_proxy and lifetime_manager for managing a new dataObjInfo_t
    ///
    /// \since 4.2.9
    template<typename rxComm>
    static auto make_replica_proxy(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const replica_number_type _replica_number)
        -> std::pair<replica_proxy<dataObjInfo_t>, lifetime_manager<dataObjInfo_t>>
    {
        dataObjInfo_t* doi = (dataObjInfo_t*)std::malloc(sizeof(dataObjInfo_t));
        std::memset(doi, 0, sizeof(dataObjInfo_t));

        const auto result = get_data_object_info(_comm, _logical_path, _replica_number).front();

        detail::populate_struct_from_results(*doi, result);

        return {replica_proxy{*doi}, lifetime_manager{*doi}};
    } // make_replica_proxy

    /// \brief Fetch information from the catalog and create a new replica proxy with it
    ///
    /// Allocates a new dataObjInfo_t and wraps the struct in a proxy and lifetime_manager
    ///
    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _leaf_resource_name
    ///
    /// \return std::pair<replica_proxy<dataObjInfo_t>, lifetime_manager<dataObjInfo_t>>
    /// \retval replica_proxy and lifetime_manager for managing a new dataObjInfo_t
    ///
    /// \since 4.2.9
    template<typename rxComm>
    static auto make_replica_proxy(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const leaf_resource_name_type _leaf_resource_name)
        -> std::pair<replica_proxy<dataObjInfo_t>, lifetime_manager<dataObjInfo_t>>
    {
        dataObjInfo_t* doi = (dataObjInfo_t*)std::malloc(sizeof(dataObjInfo_t));
        std::memset(doi, 0, sizeof(dataObjInfo_t));

        const auto result = get_data_object_info(_comm, _logical_path, _leaf_resource_name).front();

        detail::populate_struct_from_results(*doi, result);

        return {replica_proxy{*doi}, lifetime_manager{*doi}};
    } // make_replica_proxy

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
