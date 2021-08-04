#ifndef IRODS_DATA_OBJECT_PROXY_HPP
#define IRODS_DATA_OBJECT_PROXY_HPP

#include "dataObjInpOut.h"
#include "rcConnect.h"
#include "replica_proxy.hpp"

#include <algorithm>
#include <optional>
#include <string_view>
#include <vector>

#include "json.hpp"

namespace irods::experimental::data_object
{
    /// \brief Presents a data object-level interface to a DataObjInfo legacy iRODS struct.
    ///
    /// Holds a pointer to a DataObjInfo whose lifetime is managed outside of the proxy object.
    ///
    /// The data_object_proxy is essentially a wrapper around the linked list of a DataObjInfo struct.
    /// This is meant to be used as an interface to the logical representation of data, which has physical
    /// representations in the form of replicas. The data_object_proxy has a vector of replica_proxy
    /// objects to interact with individual physical representations of data.
    ///
    /// \since 4.2.9
    template<
        typename I,
        typename = std::enable_if_t<
            std::is_same_v<DataObjInfo, typename std::remove_const_t<I>>
        >
    >
    class data_object_proxy {
    public:
        // Aliases for various types used in data_object_proxy
        using doi_type = I;
        using doi_pointer_type = doi_type*;
        using replica_list = std::vector<replica::replica_proxy<doi_type>>;
        using size_type = int;

        /// \brief Constructs proxy using an existing doi_type
        /// \since 4.2.9
        explicit data_object_proxy(doi_type& _doi)
            : data_obj_info_{&_doi}
            , replica_list_{fill_replica_list(_doi)}
        {
        }

        // accessors

        /// \returns rodsLong_t
        /// \retval id of the data object from the catalog
        /// \since 4.2.9
        auto data_id() const noexcept -> rodsLong_t { return data_obj_info_->dataId; }

        /// \returns rodsLong_t
        /// \retval id of the collection containing this data object
        /// \since 4.2.9
        auto collection_id() const noexcept -> rodsLong_t { return data_obj_info_->collId; }

        /// \returns std::string_view
        /// \retval Logical path of the data object
        /// \since 4.2.9
        auto logical_path() const noexcept -> std::string_view { return data_obj_info_->objPath; }

        /// \returns std::string_view
        /// \retval Owner user name of the data object
        /// \since 4.2.9
        auto owner_user_name() const noexcept -> std::string_view { return data_obj_info_->dataOwnerName; }

        /// \returns std::string_view
        /// \retval Owner zone name of the data object
        /// \since 4.2.9
        auto owner_zone_name() const noexcept -> std::string_view { return data_obj_info_->dataOwnerZone; }

        /// \returns size_type
        /// \retval Number of replicas for this data object
        /// \since 4.2.9
        auto replica_count() const noexcept -> size_type { return replica_list_.size(); }

        /// \returns const replica_list&
        /// \retval Reference to the list of replica_proxy objects
        /// \since 4.2.9
        auto replicas() const noexcept -> const replica_list& { return replica_list_; }

        /// \returns const doi_pointer_type
        /// \retval Pointer to the underlying struct
        /// \since 4.2.9
        auto get() const noexcept -> const doi_pointer_type { return data_obj_info_; }

        auto locked() const noexcept -> bool
        {
            for (const auto& r : replica_list_) {
                if (r.locked()) {
                    return true;
                }
            }

            return false;
        }

        // mutators

        /// \brief Set the data id for the data object
        /// \param[in] _did - New value for data id
        /// \returns void
        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto data_id(const rodsLong_t _did) -> void
        {
            for (auto& r : replica_list_) {
                r.data_id(_did);
            }
        }

        /// \brief Set the logical path and collection id for the data object (i.e. all replicas)
        ///
        /// \param[in] _collection_id - New value for collection id
        /// \param[in] _logical_path - New value for logical path
        ///
        /// \returns void
        ///
        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto rename(const rodsLong_t _collection_id, std::string_view _logical_path) -> void
        {
            for (auto& r : replica_list_) {
                r.collection_id(_collection_id);
                r.logical_path(_logical_path);
            }
        }

        /// \brief Set the owner user name for the data object (i.e. all replicas)
        /// \param[in] _un - New value for owner user name
        /// \returns void
        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto owner_user_name(std::string_view _un) -> void
        {
            for (auto& r : replica_list_) {
                r.owner_user_name(_un);
            }
        }

        /// \brief Set the owner zone name for the data object (i.e. all replicas)
        /// \param[in] _zn - New value for owner zone name
        /// \returns void
        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto owner_zone_name(std::string_view _zn) -> void
        {
            for (auto& r : replica_list_) {
                r.owner_zone_name(_zn);
            }
        }

        /// \brief Get reference to replica list
        /// \returns replica_list&
        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto replicas() noexcept -> replica_list& { return replica_list_; }

        /// \brief Get pointer to the underlying DataObjInfo struct
        /// \returns doi_pointer_type
        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto get() noexcept -> doi_pointer_type { return data_obj_info_; }

        /// \brief Add a replica to the linked list and to the vector of replicas
        ///
        /// \param[in] _repl New replica to be added to the list
        ///
        /// \since 4.2.9
        template<
            typename P = doi_type,
            typename = std::enable_if_t<!std::is_const_v<P>>>
        auto add_replica(doi_type& _repl) -> void
        {
            if (replica_count() > 0) {
                auto& replica = replica_list_.back();
                replica.get()->next = &_repl;
            }
            replica_list_.push_back(replica::replica_proxy{_repl});
        } // add_replica

        operator bool() const noexcept { return static_cast<bool>(data_obj_info_); }

    private:
        /// \brief Pointer to underlying doi_type
        /// \since 4.2.9
        doi_pointer_type data_obj_info_;

        /// \brief List of objects representing physical replicas
        /// \since 4.2.9
        replica_list replica_list_;

        /// \brief Generates replica proxy objects from doi_type's linked list and stores them in a list
        /// \since 4.2.9
        inline auto fill_replica_list(doi_type& _doi) -> replica_list
        {
            replica_list r;
            for (doi_type* d = &_doi; d; d = d->next) {
                r.push_back(replica::replica_proxy{*d});
            }
            return r;
        } // fill_replica_list
    }; // data_object_proxy

    /// \brief Helper type for data_object_proxy using a non-const DataObjInfo
    ///
    /// \since 4.2.9
    using data_object_proxy_t = data_object_proxy<DataObjInfo>;

    /// \brief Helper type for a JSON representation of a data object.
    ///
    /// \since 4.2.11
    using json_repr_t = std::vector<const nlohmann::json*>;

    /// \brief Wraps an existing doi_type with a proxy object.
    /// \param[in] _doi - Pre-existing doi_type which will be wrapped by the returned proxy.
    /// \return data_object_proxy
    /// \since 4.2.9
    template<typename doi_type>
    static auto make_data_object_proxy(doi_type& _doi) -> data_object_proxy<doi_type>
    {
        return data_object_proxy{_doi};
    } // make_data_object_proxy

    namespace detail
    {
        /// \brief Generates a new doi_type from the catalog and wraps with a proxy.
        ///
        /// \param[in] _comm
        /// \param[in] _query_results Result set from the catalog containing data object information
        ///
        /// \throws irods::exception If no information is found for the logical path or list is empty at the end
        ///
        /// \returns data_object_proxy and lifetime_manager for underlying struct
        ///
        /// \since 4.2.9
        template<typename rxComm>
        static auto make_data_object_proxy_impl(rxComm& _comm, const irods::experimental::replica::query_value_type& _query_results)
            -> std::pair<data_object_proxy_t, lifetime_manager<DataObjInfo>>
        {
            namespace replica = irods::experimental::replica;

            DataObjInfo* head{};
            DataObjInfo* prev{};

            for (const auto& row : _query_results) {
                // Create a new DataObjInfo to represent this replica
                DataObjInfo* curr = static_cast<DataObjInfo*>(std::malloc(sizeof(DataObjInfo)));
                std::memset(curr, 0, sizeof(DataObjInfo));

                // Populate the new struct
                replica::detail::populate_struct_from_results(*curr, row);

                // Make sure the structure used for the head is populated
                if (!head) {
                    head = curr;
                }
                else {
                    prev->next = curr;
                }
                prev = curr;
            }

            if (!head) {
                THROW(SYS_INTERNAL_ERR, "list remains unpopulated");
            }

            return {data_object_proxy{*head}, lifetime_manager{*head}};
        } // make_data_object_proxy_impl
    };

    /// \brief Generates a new DataObjInfo from the catalog and wraps with a proxy.
    ///
    /// \param[in] _comm
    /// \param[in] _logical_path
    ///
    /// \throws irods::exception If no information is found for the logical path or list is empty at the end
    ///
    /// \returns data_object_proxy and lifetime_manager for underlying struct
    ///
    /// \since 4.2.9
    template<typename rxComm>
    static auto make_data_object_proxy(rxComm& _comm, const irods::experimental::filesystem::path& _logical_path)
        -> std::pair<data_object_proxy_t, lifetime_manager<DataObjInfo>>
    {
        namespace replica = irods::experimental::replica;

        const auto data_obj_info = replica::get_data_object_info(_comm, _logical_path);

        return detail::make_data_object_proxy_impl(_comm, data_obj_info);
    } // make_data_object_proxy

    /// \brief Generates a new DataObjInfo from the catalog and wraps with a proxy.
    ///
    /// \param[in] _comm
    /// \param[in] _data_id
    ///
    /// \throws irods::exception If no information is found for the data id or list is empty at the end
    ///
    /// \returns data_object_proxy and lifetime_manager for underlying struct
    ///
    /// \since 4.2.9
    template<typename rxComm>
    static auto make_data_object_proxy(rxComm& _comm, const rodsLong_t _data_id)
        -> std::pair<data_object_proxy_t, lifetime_manager<DataObjInfo>>
    {
        namespace replica = irods::experimental::replica;

        const auto data_obj_info = replica::get_data_object_info(_comm, _data_id);

        return detail::make_data_object_proxy_impl(_comm, data_obj_info);
    } // make_data_object_proxy

    /// \brief Creates a data_object_proxy from a list of replicas described in JSON.
    ///
    /// \param[in] _logical_path Full logical path of the data object being described.
    /// \param[in] _replicas JSON list of replicas which conform to irods::experimental::replica::to_json.
    ///
    /// \throws irods::exception If _replicas is empty or the head pointer is not populated for some reason
    /// \throws json::exception If _replicas is not a JSON array
    ///
    /// \returns data_object_proxy and lifetime_manager for underlying struct
    ///
    /// \since 4.2.11
    static auto make_data_object_proxy(const std::string_view _logical_path, const json_repr_t& _replicas)
        -> std::pair<data_object_proxy_t, lifetime_manager<DataObjInfo>>
    {
        namespace ir = irods::experimental::replica;

        if (_replicas.empty()) {
            THROW(SYS_INVALID_INPUT_PARAM, "replica list is empty");
        }

        DataObjInfo* head{};
        DataObjInfo* prev{};

        for (const auto* replica_json : _replicas) {
            if (!replica_json) {
                THROW(SYS_INTERNAL_NULL_INPUT_ERR, "json pointer is null");
            }

            // Populate the new struct
            auto [curr, curr_lm] = ir::make_replica_proxy(_logical_path.data(), *replica_json);

            // Make sure the structure used for the head is populated
            if (!head) {
                head = curr.get();
            }
            else {
                prev->next = curr.get();
            }
            prev = curr_lm.release();
        }

        if (!head) {
            THROW(SYS_INTERNAL_ERR, "list remains unpopulated");
        }

        return {data_object_proxy{*head}, lifetime_manager{*head}};
    } // make_data_object_proxy

    /// \brief Takes an existing data_object_proxy and duplicates the underlying struct.
    ///
    /// \param[in] _obj data object to duplicate
    ///
    /// \returns data_object_proxy and lifetime_manager for underlying struct
    ///
    /// \since 4.2.9
    static auto duplicate_data_object(const DataObjInfo& _obj)
        -> std::pair<data_object_proxy_t, lifetime_manager<DataObjInfo>>
    {
        DataObjInfo* head{};
        DataObjInfo* prev{};

        for (const DataObjInfo* tmp = &_obj; tmp; tmp = tmp->next) {
            DataObjInfo* curr = static_cast<DataObjInfo*>(std::malloc(sizeof(DataObjInfo)));
            std::memset(curr, 0, sizeof(DataObjInfo));
            std::memcpy(curr, tmp, sizeof(DataObjInfo));

            // Do not maintain the linked list - muy peligroso
            curr->next = nullptr;

            replKeyVal(&tmp->condInput, &curr->condInput);

            if (tmp->specColl) {
                SpecColl* tmp_sc = static_cast<SpecColl*>(std::malloc(sizeof(SpecColl)));
                std::memset(tmp_sc, 0, sizeof(SpecColl));
                std::memcpy(tmp_sc, tmp->specColl, sizeof(SpecColl));
                curr->specColl = tmp_sc;
            }

            if (!head) {
                head = curr;
            }
            else {
                prev->next = curr;
            }
            prev = curr;
        }

        if (!head) {
            THROW(SYS_INTERNAL_ERR, "list remains unpopulated");
        }

        return {data_object_proxy{*head}, lifetime_manager{*head}};
    } // duplicate_data_object

    /// \brief Takes an existing data_object_proxy and duplicates the underlying struct.
    ///
    /// \param[in] _comm
    /// \param[in] _logical_path
    ///
    /// \throws irods::exception If no information is found for the logical path or list is empty at the end
    ///
    /// \returns data_object_proxy and lifetime_manager for underlying struct
    ///
    /// \since 4.2.9
    static auto duplicate_data_object(const data_object_proxy_t& _obj)
        -> std::pair<data_object_proxy_t, lifetime_manager<DataObjInfo>>
    {
        if (!_obj) {
            THROW(USER__NULL_INPUT_ERR, "object information is null");
        }

        return duplicate_data_object(*_obj.get());
    } //duplicate_data_object

    /// \brief Finds a replica in the list based on resource hierarchy
    ///
    /// \param[in] _obj data_object_proxy to search in
    /// \param[in] _hierarchy resource hierarchy to find
    ///
    /// \retval replica_proxy if found
    /// \retval std::nullopt if no replica is found with the provided resource hierarchy
    ///
    /// \since 4.2.9
    template<typename doi_type>
    static auto find_replica(data_object_proxy<doi_type>& _obj, std::string_view _hierarchy)
        -> std::optional<replica::replica_proxy<doi_type>>
    {
        auto itr = std::find_if(
            std::begin(_obj.replicas()), std::end(_obj.replicas()),
            [&_hierarchy](const auto& _r)
            {
                return _r.hierarchy() == _hierarchy;
            });

        if (std::end(_obj.replicas()) != itr) {
            return *itr;
        }

        return std::nullopt;
    } // find_replica

    template<typename doi_type>
    static auto find_replica(const data_object_proxy<doi_type>& _obj, std::string_view _hierarchy)
        -> std::optional<const replica::replica_proxy<doi_type>>
    {
        auto itr = std::find_if(
            std::begin(_obj.replicas()), std::end(_obj.replicas()),
            [&_hierarchy](const auto& _r)
            {
                return _r.hierarchy() == _hierarchy;
            });

        if (std::end(_obj.replicas()) != itr) {
            return *itr;
        }

        return std::nullopt;
    } // find_replica

    /// \brief Finds a replica in the list based on replica number
    ///
    /// \param[in] _obj data_object_proxy to search in
    /// \param[in] _replica_number
    ///
    /// \retval replica_proxy if found
    /// \retval std::nullopt if no replica is found with the provided replica number
    ///
    /// \since 4.2.9
    template<typename doi_type>
    static auto find_replica(data_object_proxy<doi_type>& _obj, const int _replica_number)
        -> std::optional<replica::replica_proxy<doi_type>>
    {
        auto itr = std::find_if(
            std::cbegin(_obj.replicas()), std::cend(_obj.replicas()),
            [&_replica_number](const auto& _r)
            {
                return _r.replica_number() == _replica_number;
            });

        if (std::end(_obj.replicas()) != itr) {
            return *itr;
        }

        return std::nullopt;
    } // find_replica

    /// \brief Finds a replica in the list based on replica number
    ///
    /// \param[in] _obj data_object_proxy to search in
    /// \param[in] _replica_number
    ///
    /// \retval replica_proxy if found
    /// \retval std::nullopt if no replica is found with the provided replica number
    ///
    /// \since 4.2.9
    template<typename doi_type>
    static auto find_replica(const data_object_proxy<doi_type>& _obj, const int _replica_number)
        -> std::optional<replica::replica_proxy<doi_type>>
    {
        auto itr = std::find_if(
            std::cbegin(_obj.replicas()), std::cend(_obj.replicas()),
            [&_replica_number](const auto& _r)
            {
                return _r.replica_number() == _replica_number;
            });

        if (std::end(_obj.replicas()) != itr) {
            return *itr;
        }

        return std::nullopt;
    } // find_replica
} // namespace irods::experimental::data_object

#endif // #ifndef IRODS_DATA_OBJECT_PROXY_HPP
