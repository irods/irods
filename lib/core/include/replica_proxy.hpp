#ifndef IRODS_REPLICA_PROXY_HPP
#define IRODS_REPLICA_PROXY_HPP

#include "filesystem.hpp"
#include "irods_exception.hpp"
#include "key_value_proxy.hpp"
#include "objInfo.h"

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

        /// \returns std::string_view
        /// \since 4.2.9
        auto logical_path()     const noexcept -> std::string_view { return doi_->objPath; }

        /// \returns std::string_view
        /// \since 4.2.9
        auto resource()         const noexcept -> std::string_view { return doi_->rescName; }

        /// \returns std::string_view
        /// \since 4.2.9
        auto hierarchy()        const noexcept -> std::string_view { return doi_->rescHier; }

        /// \returns std::string_view
        /// \since 4.2.9
        auto type()             const noexcept -> std::string_view { return doi_->dataType; }

        /// \returns rodsLong_t
        /// \since 4.2.9
        auto size()             const noexcept -> rodsLong_t { return doi_->dataSize; }

        /// \returns std::string_view
        /// \since 4.2.9
        auto checksum()         const noexcept -> std::string_view { return doi_->chksum; }

        /// \returns std::string_view
        /// \since 4.2.9
        auto version()          const noexcept -> std::string_view { return doi_->version; }

        /// \returns std::string_view
        /// \since 4.2.9
        auto physical_path()    const noexcept -> std::string_view { return doi_->filePath; }

        /// \returns std::string_view
        /// \since 4.2.9
        auto owner_user_name()  const noexcept -> std::string_view { return doi_->dataOwnerName; }

        /// \returns std::string_view
        /// \since 4.2.9
        auto owner_zone_name()  const noexcept -> std::string_view { return doi_->dataOwnerZone; }

        /// \returns int
        /// \since 4.2.9
        auto replica_number()   const noexcept -> int { return doi_->replNum; }

        /// \returns int
        /// \since 4.2.9
        auto replica_status()   const noexcept -> int { return doi_->replStatus; }

        /// \returns rodsLong_t
        /// \since 4.2.9
        auto data_id()          const noexcept -> rodsLong_t { return doi_->dataId; }

        /// \returns rodsLong_t
        /// \since 4.2.9
        auto collection_id()    const noexcept -> rodsLong_t { return doi_->collId; }

        /// \returns rodsLong_t
        /// \since 4.2.9
        auto resource_id()      const noexcept -> rodsLong_t { return doi_->rescId; }

        /// \returns std::string_view
        /// \since 4.2.9
        auto comments()         const noexcept -> std::string_view { return doi_->dataComments; }

        /// \returns std::string_view
        /// \since 4.2.9
        auto ctime()            const noexcept -> std::string_view { return doi_->dataCreate; }

        /// \returns std::string_view
        /// \since 4.2.9
        auto mtime()            const noexcept -> std::string_view { return doi_->dataModify; }

        /// \returns std::string_view
        /// \since 4.2.9
        auto in_pdmo()          const noexcept -> std::string_view { return doi_->in_pdmo; }

        /// \returns std::string_view
        /// \since 4.2.9
        auto status()           const noexcept -> std::string_view { return doi_->statusString; }

        /// \returns key_value_proxy
        /// \retval condInput for the dataObjInfo_t node as a key_value_proxy
        /// \since 4.2.9
        auto cond_input()       const -> key_value_proxy<const keyValPair_t>
        {
            return make_key_value_proxy(doi_->condInput);
        }

        /// \returns const specColl_t*
        /// \retval specColl pointer for the dataObjInfo_t node
        /// \since 4.2.9
        auto special_collection_info() const noexcept -> const specColl_t*
        {
            return doi_->specColl;
        }

        /// \returns std::string_view
        /// \since 4.2.9
        auto mode()             const noexcept -> std::string_view { return doi_->dataMode; }

        /// \returns const doi_pointer_type
        /// \retval Pointer to the underlying struct
        /// \since 4.2.9
        auto get() const noexcept -> const doi_pointer_type { return doi_; }

        // mutators

        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto logical_path(std::string_view _lp) -> void
        {
            set_string_property(doi_->objPath, _lp, sizeof(doi_->objPath));
        }

        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto resource(std::string_view _r) -> void
        {
            set_string_property(doi_->rescName, _r, sizeof(doi_->rescName));
        }

        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto hierarchy(std::string_view _h) -> void
        {
            set_string_property(doi_->rescHier, _h, sizeof(doi_->rescHier));
        }

        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto type(std::string_view _t) -> void
        {
            set_string_property(doi_->dataType, _t, sizeof(doi_->dataType));
        }

        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto size(const rodsLong_t _s)              -> void { doi_->dataSize = _s; }

        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto checksum(std::string_view _cs) -> void
        {
            set_string_property(doi_->chksum, _cs, sizeof(doi_->chksum));
        }

        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto version(std::string_view _v) -> void
        {
            set_string_property(doi_->version, _v, sizeof(doi_->version));
        }

        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto physical_path(std::string_view _p) -> void
        {
            set_string_property(doi_->filePath, _p, sizeof(doi_->filePath));
        }

        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto owner_user_name(std::string_view _oun) -> void
        {
            set_string_property(doi_->dataOwnerName, _oun, sizeof(doi_->ownerZone));
        }

        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto owner_zone_name(std::string_view _ozn) -> void
        {
            set_string_property(doi_->dataOwnerZone, _ozn, sizeof(doi_->ownerZone));
        }

        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto replica_number(const int _rn)          -> void { doi_->replNum = _rn; }

        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto replica_status(const int _s)           -> void { doi_->replStatus = _s; }

        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto data_id(const rodsLong_t _id)          -> void { doi_->dataId = _id; }

        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto collection_id(const rodsLong_t _id)    -> void { doi_->collId = _id; }

        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto resource_id(const rodsLong_t _id)      -> void { doi_->rescId = _id; }

        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto comments(std::string_view _c) -> void
        {
            set_string_property(doi_->dataComments, _c, sizeof(doi_->dataComments));
        }

        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto ctime(std::string_view _ct) -> void
        {
            set_string_property(doi_->dataCreate, _ct, sizeof(doi_->dataCreate));
        }

        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto mtime(std::string_view _mt) -> void
        {
            set_string_property(doi_->dataModify, _mt, sizeof(doi_->dataModify));
        }

        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto in_pdmo(std::string_view _p) -> void
        {
            set_string_property(doi_->in_pdmo, _p, sizeof(doi_->in_pdmo));
        }

        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto status(std::string_view _s) -> void
        {
            set_string_property(doi_->statusString, _s, sizeof(doi_->statusString));
        }

        /// \returns key_value_proxy
        /// \retval condInput for the dataObjInfo_t node as a key_value_proxy
        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto cond_input() -> key_value_proxy<keyValPair_t>
        {
            return make_key_value_proxy(doi_->condInput);
        }

        /// \returns specColl_t*
        /// \retval specColl pointer for the dataObjInfo_t node
        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto special_collection_info() -> specColl_t*
        {
            return doi_->specColl;
        }

        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto mode(std::string_view _m)              -> void
        {
            set_string_property(doi_->dataMode, _m, sizeof(doi_->dataMode));
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

    /// \brief Factory method for making a replica proxy object for a dataObjInfo_t
    /// \param[in] _doi - Pre-existing doi_type which will be wrapped by the returned proxy.
    /// \return replica_proxy
    /// \since 4.2.9
    template<typename doi_type>
    static auto make_replica_proxy(doi_type& _doi) -> replica_proxy<doi_type>
    {
        return replica_proxy{_doi};
    } // make_replica_proxy

    /// \brief Factory method for making a replica proxy object for a dataObjInfo_t
    /// Allocates a new dataObjInfo_t and wraps the struct in a proxy and lifetime_manager
    /// \return std::pair<replica_proxy<dataObjInfo_t>, lifetime_manager<dataObjInfo_t>>
    /// \retval replica_proxy and lifetime_manager for managing a new dataObjInfo_t
    /// \since 4.2.9
    static auto make_replica_proxy() -> std::pair<replica_proxy<dataObjInfo_t>, lifetime_manager<dataObjInfo_t>>
    {
        dataObjInfo_t* doi = (dataObjInfo_t*)std::malloc(sizeof(dataObjInfo_t));
        std::memset(doi, 0, sizeof(dataObjInfo_t));
        return {replica_proxy{*doi}, lifetime_manager{*doi}};
    } // make_replica_proxy

} // namespace irods::experimental::replica

#endif // #ifndef IRODS_REPLICA_PROXY_HPP
