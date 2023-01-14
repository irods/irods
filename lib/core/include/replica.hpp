#ifndef IRODS_REPLICA_HPP
#define IRODS_REPLICA_HPP

/// \file

#ifdef IRODS_REPLICA_ENABLE_SERVER_SIDE_API
    #define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
    #define IRODS_QUERY_ENABLE_SERVER_SIDE_API

    #include "rsDataObjChksum.hpp"
    #include "rsModDataObjMeta.hpp"
    #include "rsFileStat.hpp"

    #define rxFileStat rsFileStat
#else
    #undef IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
    #undef IRODS_QUERY_ENABLE_SERVER_SIDE_API

    #include "dataObjChksum.h"
    #include "modDataObjMeta.h"
    #include "fileStat.h"

    #define rxFileStat rcFileStat
#endif

#include "filesystem.hpp"
#include "filesystem/path_utilities.hpp"
#include "irods_at_scope_exit.hpp"
#include "irods_exception.hpp"
#include "key_value_proxy.hpp"
#include "objInfo.h"
#include "query_builder.hpp"
#include "rcConnect.h"
#include "rcMisc.h"

#include "fmt/format.h"

#include <chrono>
#include <iomanip>
#include <optional>
#include <string_view>
#include <variant>

struct DataObjInfo;

namespace irods::experimental::replica
{
    using replica_number_type = int;

    using leaf_resource_name_type = std::string_view;

    using query_value_type = std::vector<std::vector<std::string>>;

    /// \brief Describes whether the catalog should be updated when calculating a replica's checksum
    ///
    /// \since 4.2.9
    enum class verification_calculation
    {
        if_empty,
        always
    };

    /// \brief GenQuery columns which represent a replica
    ///
    /// \since 4.2.9
    enum genquery_column_index : std::size_t
    {
        DATA_ID,
        DATA_COLL_ID,
        DATA_NAME,
        DATA_REPL_NUM,
        DATA_VERSION,
        DATA_TYPE_NAME,
        DATA_SIZE,
        DATA_RESC_NAME,
        DATA_PATH,
        DATA_OWNER_NAME,
        DATA_OWNER_ZONE,
        DATA_REPL_STATUS,
        DATA_STATUS,
        DATA_CHECKSUM,
        DATA_EXPIRY,
        DATA_MAP_ID,
        DATA_COMMENTS,
        DATA_CREATE_TIME,
        DATA_MODIFY_TIME,
        DATA_MODE,
        DATA_RESC_HIER,
        DATA_RESC_ID,
        COLL_NAME
    };

    namespace detail
    {
        /// \param[in] _comm connection object
        /// \param[in] _logical_path
        ///
        /// \throws irods::exception if the path does not refer to a data object
        ///
        /// \since 4.2.9
        template<typename rxComm>
        inline auto throw_if_path_is_not_a_data_object(
            rxComm& _comm,
            const irods::experimental::filesystem::path& _logical_path) -> void
        {
            if (!irods::experimental::filesystem::NAMESPACE_IMPL::is_data_object(_comm, _logical_path)) {
                THROW(SYS_INVALID_INPUT_PARAM, "path does not point to a data object");
            }
        } // throw_if_path_is_not_a_data_object

        /// \brief Enforce valid replica number for replica operation
        ///
        /// \param[in] _replica_number
        ///
        /// \throws irods::exception if the path does not refer to a data object
        ///
        /// \since 4.2.9
        inline auto throw_if_replica_number_is_invalid(const replica_number_type _replica_number) -> void
        {
            if (_replica_number < 0) {
                THROW(SYS_INVALID_INPUT_PARAM, "invalid replica number");
            }
        } // throw_if_replica_number_is_invalid

        /// \brief Enforce valid path for replica operation
        ///
        /// \param[in] _comm connection object
        /// \param[in] _logical_path
        ///
        /// \throws filesystem_error if the path is empty or too long
        /// \throws irods::exception if the path does not refer to a data object or replica number is invalid
        ///
        /// \since 4.2.9
        template<typename rxComm>
        inline auto throw_if_replica_logical_path_is_invalid(
            rxComm& _comm,
            const irods::experimental::filesystem::path& _logical_path) -> void
        {
            throw_if_path_is_empty(_logical_path);

            throw_if_path_length_exceeds_limit(_logical_path);

            throw_if_path_is_not_a_data_object(_comm, _logical_path);
        } // throw_if_replica_logical_path_is_invalid

        /// \brief Enforce valid resource name for replica operation
        ///
        /// \param[in] _resource_name
        ///
        /// \throws irods::exception If the provided resource name is invalid
        ///
        /// \since 4.2.9
        inline auto throw_if_replica_resource_name_is_invalid(const leaf_resource_name_type& _resource_name) -> void
        {
            if (_resource_name.size() <= 0) {
                THROW(SYS_INVALID_INPUT_PARAM, "resource name cannot be empty");
            }
        } // throw_if_replica_resource_name_is_invalid

        /// \brief Gets a row from r_data_main using irods::query
        ///
        /// Passing an empty string for the _query_condition_string input will result in all replicas
        /// being returned.
        ///
        /// \param[in] _comm connection object
        /// \param[in] _logical_path
        /// \param[in] _query_condition_string Additional constraints for selection of replica information
        ///
        /// \throws irods::exception If no replica information is found or query fails
        ///
        /// \retval Set of results from the query
        ///
        /// \since 4.2.9
        template<typename rxComm>
        auto get_data_object_info_impl(
            rxComm& _comm,
            const irods::experimental::filesystem::path& _logical_path,
            std::string_view _query_condition_string = "") -> query_value_type
        {
            detail::throw_if_replica_logical_path_is_invalid(_comm, _logical_path);

            query_builder qb;

            if (const auto zone = irods::experimental::filesystem::zone_name(_logical_path); zone) {
                qb.zone_hint(*zone);
            }

            std::string qstr = fmt::format(
                "SELECT "
                "DATA_ID, "
                "DATA_COLL_ID, "
                "DATA_NAME, "
                "DATA_REPL_NUM, "
                "DATA_VERSION,"
                "DATA_TYPE_NAME, "
                "DATA_SIZE, "
                "DATA_RESC_NAME, "
                "DATA_PATH, "
                "DATA_OWNER_NAME, "
                "DATA_OWNER_ZONE, "
                "DATA_REPL_STATUS, "
                "DATA_STATUS, "
                "DATA_CHECKSUM, "
                "DATA_EXPIRY, "
                "DATA_MAP_ID, "
                "DATA_COMMENTS, "
                "DATA_CREATE_TIME, "
                "DATA_MODIFY_TIME, "
                "DATA_MODE, "
                "DATA_RESC_HIER, "
                "DATA_RESC_ID, "
                "COLL_NAME"
                " WHERE DATA_NAME = '{}' AND COLL_NAME = '{}'",
                _logical_path.object_name().c_str(), _logical_path.parent_path().c_str());

            if (!_query_condition_string.empty()) {
                qstr += fmt::format(" AND {}", _query_condition_string.data());
            }

            auto q = qb.build<rxComm>(_comm, qstr);
            if (q.size() <= 0) {
                THROW(CAT_NO_ROWS_FOUND, "no replica information found");
            }

            query_value_type output;
            for (auto&& result : q) {
                output.push_back(result);
            }

            return output;
        } // get_data_object_info_impl

        /// \brief Calculate checksum for a replica
        ///
        /// The verification_calculation helps determine whether the calculated checksum will be registered in the catalog
        ///
        /// \param[in] _comm connection object
        /// \param[in] _data_object_input
        /// \param[in] _logical_path
        /// \param[in] _calculation mode for catalog update
        ///
        /// \throws filesystem_error if the path is empty or too long
        /// \throws irods::exception if the path does not refer to a data object or replica input is invalid
        ///
        /// \returns std::string
        /// \retval value of calculated checksum
        ///
        /// \since 4.2.9
        template<typename rxComm>
        auto replica_checksum_impl(
            rxComm& _comm,
            dataObjInp_t& _data_object_input,
            const irods::experimental::filesystem::path& _logical_path,
            const verification_calculation _calculation = verification_calculation::if_empty) -> std::string
        {
            detail::throw_if_replica_logical_path_is_invalid(_comm, _logical_path);

            std::snprintf(_data_object_input.objPath, sizeof(_data_object_input.objPath), "%s", _logical_path.c_str());

            if (verification_calculation::always == _calculation) {
                auto cond_input = irods::experimental::make_key_value_proxy(_data_object_input.condInput);
                cond_input[FORCE_CHKSUM_KW] = "";
            }

            char* checksum{};
            at_scope_exit free_checksum{[&checksum] {
                if (checksum) {
                    std::free(checksum);
                }
            }};

            if constexpr (std::is_same_v<rxComm, rsComm_t>) {
                if (const auto ec = rsDataObjChksum(&_comm, &_data_object_input, &checksum); ec < 0) {
                    THROW(ec, fmt::format("cannot calculate checksum for [{}]", _logical_path.c_str()));
                }
            }
            else {
                if (const auto ec = rcDataObjChksum(&_comm, &_data_object_input, &checksum); ec < 0) {
                    THROW(ec, fmt::format("cannot calculate checksum for [{}]", _logical_path.c_str()));
                }
            }

            return checksum ? checksum : std::string{};
        } // replica_checksum_impl

        /// \brief Sets value of the timestamp of last time this replica was written to
        ///
        /// \param[in] _comm connection object
        /// \param[in] _logical_path
        /// \param[in] _resource_hierarchy
        /// \param[in] _new_time timestamp to use as last_write_time
        ///
        /// \throws irods::filesystem::filesystem_error if the path is empty or too long
        /// \throws irods::exception if the path does not refer to a data object or replica input is invalid
        ///
        /// \since 4.2.9
        template<typename rxComm>
        auto last_write_time_impl(
            rxComm& _comm,
            const irods::experimental::filesystem::path& _logical_path,
            std::string_view _resource_hierarchy,
            const irods::experimental::filesystem::object_time_type _new_time) -> void
        {
            const auto seconds = _new_time.time_since_epoch();
            std::stringstream new_time;
            new_time << std::setfill('0') << std::setw(11) << std::to_string(seconds.count());

            DataObjInfo info{};
            std::snprintf(info.objPath, sizeof(info.objPath), "%s", _logical_path.c_str());
            std::snprintf(info.rescHier, sizeof(info.rescHier), "%s", _resource_hierarchy.data());

            keyValPair_t kvp{};
            at_scope_exit free_memory{[&kvp] { clearKeyVal(&kvp); }};
            auto reg_params = make_key_value_proxy(kvp);
            reg_params[DATA_MODIFY_KW] = new_time.str();

            modDataObjMeta_t input{};
            input.dataObjInfo = &info;
            input.regParam = reg_params.get();

            const auto mod_obj_info_fcn = [](rxComm& _comm, modDataObjMeta_t& _inp)
            {
                if constexpr (std::is_same_v<rxComm, rsComm_t>) {
                    return rsModDataObjMeta(&_comm, &_inp);
                }
                else {
                    return rcModDataObjMeta(&_comm, &_inp);
                }
            };

            if (const auto ec = mod_obj_info_fcn(_comm, input); ec != 0) {
                THROW(ec, fmt::format("cannot set mtime for [{}]", _logical_path.c_str()));
            }
        } // last_write_time_impl

        /// \brief Gets size of the replica's physical representation from storage
        ///
        /// \param[in] _comm connection object
        /// \param[in] _logical_path
        /// \param[in] _resource_hierarchy
        /// \param[in] _physical_path
        ///
        /// \throws irods::filesystem::filesystem_error if the path is empty or too long
        /// \throws irods::exception if the path does not refer to a data object or replica input is invalid
        ///
        /// \since 4.2.9
        template<typename rxComm>
        auto get_replica_size_from_storage_impl(
            rxComm& _comm,
            const irods::experimental::filesystem::path& _logical_path,
            std::string_view _resource_hierarchy,
            std::string_view _physical_path) -> rodsLong_t
        {
            fileStatInp_t stat_inp{};
            rstrcpy(stat_inp.objPath,  _logical_path.c_str(),      sizeof(stat_inp.objPath));
            rstrcpy(stat_inp.rescHier, _resource_hierarchy.data(), sizeof(stat_inp.rescHier));
            rstrcpy(stat_inp.fileName, _physical_path.data(),      sizeof(stat_inp.fileName));

            rodsStat *stat_out{};
            irods::at_scope_exit free_stat_out{[&stat_out] { free(stat_out); }};

            if (const auto status_rxFileStat = rxFileStat(&_comm, &stat_inp, &stat_out); status_rxFileStat < 0) {
                THROW(status_rxFileStat, fmt::format(
                    "rxFileStat of objPath [{}] rescHier [{}] fileName [{}] failed with [{}]",
                    stat_inp.objPath, stat_inp.rescHier, stat_inp.fileName, status_rxFileStat));
            }

            return stat_out->st_size;
        } // get_replica_size_from_storage_impl
    } // namespace detail

    /// \brief Gets a row from r_data_main using irods::query
    ///
    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _replica_number
    ///
    /// \throws irods::exception If no replica information is found or query fails
    ///
    /// \retval Set of results from the query
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto get_data_object_info(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const replica_number_type _replica_number) -> query_value_type
    {
        detail::throw_if_replica_number_is_invalid(_replica_number);

        const std::string qstr = fmt::format("DATA_REPL_NUM = '{}'", _replica_number);

        return detail::get_data_object_info_impl(_comm, _logical_path, qstr);
    } // get_data_object_info

    /// \brief Gets a row from r_data_main using irods::query
    ///
    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _leaf_resource_name
    ///
    /// \throws irods::exception If no replica information is found or query fails
    ///
    /// \retval Set of results from the query
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto get_data_object_info(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const leaf_resource_name_type& _leaf_resource_name) -> query_value_type
    {
        detail::throw_if_replica_resource_name_is_invalid(_leaf_resource_name);

        const std::string qstr = fmt::format("DATA_RESC_NAME = '{}'", _leaf_resource_name);

        return detail::get_data_object_info_impl(_comm, _logical_path, qstr);
    } // get_data_object_info

    /// \brief Gets a row from r_data_main using irods::query
    ///
    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    ///
    /// \throws irods::exception If no replica information is found or query fails
    ///
    /// \retval Set of results from the query
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto get_data_object_info(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path) -> query_value_type
    {
        return detail::get_data_object_info_impl(_comm, _logical_path);
    } // get_data_object_info

    /// \brief Gets a row from r_data_main using irods::query
    ///
    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    ///
    /// \throws irods::exception If no replica information is found or query fails
    ///
    /// \retval Set of results from the query
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto get_data_object_info(
        rxComm& _comm,
        const rodsLong_t _data_id) -> query_value_type
    {
        namespace fs = irods::experimental::filesystem;

        const std::string qstr = fmt::format("SELECT COLL_NAME, DATA_NAME where DATA_ID = '{}'", _data_id);

        query_builder qb;

        auto q = qb.build<rxComm>(_comm, qstr);
        if (q.size() <= 0) {
            THROW(CAT_NO_ROWS_FOUND, "no replica information found");
        }

        const auto& result = q.front();
        const auto& coll_name = result[0];
        const auto& data_name = result[1];
        const auto logical_path = fs::path{coll_name} / data_name;

        return detail::get_data_object_info_impl(_comm, logical_path);
    } // get_data_object_info

    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _replica_number
    ///
    /// \throws filesystem_error if the path is empty or too long
    /// \throws irods::exception if the path does not refer to a data object or replica number is invalid
    ///
    /// \returns std::uintmax_t
    /// \retval size of specified replica in bytes
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto replica_size(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const replica_number_type _replica_number) -> std::uintmax_t
    {
        const auto result = get_data_object_info(_comm, _logical_path, _replica_number).front();

        std::string_view size = result[genquery_column_index::DATA_SIZE];

        return static_cast<std::uintmax_t>(std::stoull(size.data()));
    } // replica_size

    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _leaf_resource_name
    ///
    /// \throws filesystem_error if the path is empty or too long
    /// \throws irods::exception if the path does not refer to a data object or leaf resource is invalid
    ///
    /// \returns std::uintmax_t
    /// \retval size of specified replica in bytes
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto replica_size(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const leaf_resource_name_type& _leaf_resource_name) -> std::uintmax_t
    {
        const auto result = get_data_object_info(_comm, _logical_path, _leaf_resource_name).front();

        std::string_view size = result[genquery_column_index::DATA_SIZE];

        return static_cast<std::uintmax_t>(std::stoull(size.data()));
    } // replica_size

    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _replica_number
    ///
    /// \throws filesystem_error if the path is empty or too long
    /// \throws irods::exception if the path does not refer to a data object or replica number is invalid
    ///
    /// \returns bool
    /// \retval true if replica size is 0; otherwise, false
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto is_replica_empty(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const replica_number_type _replica_number) -> bool
    {
        return replica_size(_comm, _logical_path, _replica_number) == 0;
    } // is_replica_empty

    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _leaf_resource_name
    ///
    /// \throws filesystem_error if the path is empty or too long
    /// \throws irods::exception if the path does not refer to a data object or leaf resource is invalid
    ///
    /// \returns bool
    /// \retval true if replica size is 0; otherwise, false
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto is_replica_empty(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const leaf_resource_name_type& _leaf_resource_name) -> bool
    {
        return replica_size(_comm, _logical_path, _leaf_resource_name) == 0;
    } // is_replica_empty

    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _replica_number
    ///
    /// \throws filesystem_error if the path is empty or too long
    /// \throws irods::exception if the path does not refer to a data object or replica number is invalid
    ///
    /// \returns std::string
    /// \retval value of calculated checksum
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto replica_checksum(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const replica_number_type _replica_number,
        const verification_calculation _calculation = verification_calculation::if_empty) -> std::string
    {
        detail::throw_if_replica_number_is_invalid(_replica_number);

        dataObjInp_t input{};
        at_scope_exit free_memory{[&input] { clearKeyVal(&input.condInput); }};
        auto cond_input = make_key_value_proxy(input.condInput);
        cond_input[REPL_NUM_KW] = std::to_string(_replica_number);

        return detail::replica_checksum_impl(_comm, input, _logical_path, _calculation);
    } // replica_checksum

    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _leaf_resource_name
    ///
    /// \throws filesystem_error if the path is empty or too long
    /// \throws irods::exception if the path does not refer to a data object or resource is invalid
    ///
    /// \returns std::string
    /// \retval value of calculated checksum
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto replica_checksum(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const leaf_resource_name_type& _leaf_resource_name,
        const verification_calculation _calculation = verification_calculation::if_empty) -> std::string
    {
        detail::throw_if_replica_resource_name_is_invalid(_leaf_resource_name);

        dataObjInp_t input{};
        at_scope_exit free_memory{[&input] { clearKeyVal(&input.condInput); }};
        auto cond_input = make_key_value_proxy(input.condInput);
        cond_input[LEAF_RESOURCE_NAME_KW] = _leaf_resource_name;

        return detail::replica_checksum_impl(_comm, input, _logical_path, _calculation);
    } // replica_checksum

    /// \brief Returns timestamp of last time this replica was written to
    ///
    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _replica_number
    ///
    /// \throws filesystem_error if the path is empty or too long
    /// \throws irods::exception if the path does not refer to a data object or replica number is invalid
    ///
    /// \returns irods::filesystem::object_time_type
    /// \retval Timestamp of the last time the replica was written to
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto last_write_time(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const replica_number_type _replica_number) -> irods::experimental::filesystem::object_time_type
    {
        using object_time_type = irods::experimental::filesystem::object_time_type;

        const auto result = get_data_object_info(_comm, _logical_path, _replica_number).front();

        std::string_view mtime = result[genquery_column_index::DATA_MODIFY_TIME];

        return object_time_type{std::chrono::seconds{std::stoull(mtime.data())}};
    } // last_write_time

    /// \brief Returns timestamp of last time this replica was written to
    ///
    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _leaf_resource_name
    ///
    /// \throws filesystem_error if the path is empty or too long
    /// \throws irods::exception if the path does not refer to a data object or leaf resource is invalid
    ///
    /// \returns irods::filesystem::object_time_type
    /// \retval Timestamp of the last time the replica was written to
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto last_write_time(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const leaf_resource_name_type& _leaf_resource_name) -> irods::experimental::filesystem::object_time_type
    {
        using object_time_type = irods::experimental::filesystem::object_time_type;

        const auto result = get_data_object_info(_comm, _logical_path, _leaf_resource_name).front();

        std::string_view mtime = result[genquery_column_index::DATA_MODIFY_TIME];

        return object_time_type{std::chrono::seconds{std::stoull(mtime.data())}};
    } // last_write_time

    /// \brief Sets value of the timestamp of last time this replica was written to
    ///
    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _replica_number
    /// \param[in] _new_time timestamp to use as last_write_time
    ///
    /// \throws irods::filesystem::filesystem_error if the path is empty or too long
    /// \throws irods::exception if the path does not refer to a data object or replica number is invalid
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto last_write_time(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const replica_number_type _replica_number,
        const irods::experimental::filesystem::object_time_type _new_time) -> void
    {
        const auto replica_info = get_data_object_info(_comm, _logical_path, _replica_number).front();

        std::string_view resource_hierarchy = replica_info[genquery_column_index::DATA_RESC_HIER];

        return detail::last_write_time_impl(_comm, _logical_path, resource_hierarchy, _new_time);
    } // last_write_time

    /// \brief Sets value of the timestamp of last time this replica was written to
    ///
    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _leaf_resource_name
    /// \param[in] _new_time timestamp to use as last_write_time
    ///
    /// \throws irods::filesystem::filesystem_error if the path is empty or too long
    /// \throws irods::exception if the path does not refer to a data object or leaf resource is invalid
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto last_write_time(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const leaf_resource_name_type& _leaf_resource_name,
        const irods::experimental::filesystem::object_time_type _new_time) -> void
    {
        const auto replica_info = get_data_object_info(_comm, _logical_path, _leaf_resource_name).front();

        std::string_view resource_hierarchy = replica_info[genquery_column_index::DATA_RESC_HIER];

        return detail::last_write_time_impl(_comm, _logical_path, resource_hierarchy, _new_time);
    } // last_write_time

    /// \brief Get the leaf resource name for a replica based on the provided replica number.
    ///
    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _replica_number
    ///
    /// \throws irods::exception If query fails
    ///
    /// \retval std::string containing the resource name which hosts the specified replica
    /// \retval std::nullopt if no replica by the specified replica number is found
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto to_leaf_resource(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const replica_number_type _replica_number) -> std::optional<std::string>
    {
        try {
            const auto replica_info = get_data_object_info(_comm, _logical_path, _replica_number).front();

            return replica_info[genquery_column_index::DATA_RESC_NAME];
        }
        catch (const irods::exception& e) {
            if (CAT_NO_ROWS_FOUND == e.code()) {
                return std::nullopt;
            }
            throw;
        }
    } // to_leaf_resource

    /// \brief Get the replica number for a replica based on the provided leaf resource name.
    ///
    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _leaf_resource_name
    ///
    /// \throws irods::exception If query fails
    ///
    /// \retval std::string containing the resource name which hosts the specified replica
    /// \retval std::nullopt if no replica by the specified leaf resource name is found
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto to_replica_number(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const leaf_resource_name_type& _leaf_resource_name) -> std::optional<replica_number_type>
    {
        try {
            const auto replica_info = get_data_object_info(_comm, _logical_path, _leaf_resource_name).front();

            return std::stoi(replica_info[genquery_column_index::DATA_REPL_NUM]);
        }
        catch (const irods::exception& e) {
            if (CAT_NO_ROWS_FOUND == e.code()) {
                return std::nullopt;
            }
            throw;
        }
    } // to_replica_number

    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _leaf_resource_name
    ///
    /// \throws irods::exception If query fails
    ///
    /// \retval true if qb.build returns results
    /// \retval false if qb.build returns no results
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto replica_exists(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const leaf_resource_name_type& _leaf_resource_name) -> bool
    {
        detail::throw_if_replica_logical_path_is_invalid(_comm, _logical_path);
        detail::throw_if_replica_resource_name_is_invalid(_leaf_resource_name);

        query_builder qb;

        if (const auto zone = irods::experimental::filesystem::zone_name(_logical_path); zone) {
            qb.zone_hint(*zone);
        }

        const std::string qstr = fmt::format(
            "SELECT DATA_ID WHERE DATA_NAME = '{}' AND COLL_NAME = '{}' AND DATA_RESC_NAME = '{}'",
            _logical_path.object_name().c_str(), _logical_path.parent_path().c_str(), _leaf_resource_name.data());

        return 0 != qb.build<rxComm>(_comm, qstr).size();
    } // replica_exists

    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _replica_number
    ///
    /// \throws irods::exception If query fails
    ///
    /// \retval true if qb.build returns results
    /// \retval false if qb.build returns no results
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto replica_exists(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const replica_number_type& _replica_number) -> bool
    {
        detail::throw_if_replica_logical_path_is_invalid(_comm, _logical_path);
        detail::throw_if_replica_number_is_invalid(_replica_number);

        query_builder qb;

        if (const auto zone = irods::experimental::filesystem::zone_name(_logical_path); zone) {
            qb.zone_hint(*zone);
        }

        const std::string qstr = fmt::format(
            "SELECT DATA_ID WHERE DATA_NAME = '{}' AND COLL_NAME = '{}' AND DATA_REPL_NUM = '{}'",
            _logical_path.object_name().c_str(), _logical_path.parent_path().c_str(), _replica_number);

        return 0 != qb.build<rxComm>(_comm, qstr).size();
    } // replica_exists

    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _replica_number
    ///
    /// \throws filesystem_error if the path is empty or too long
    /// \throws irods::exception if the path does not refer to a data object or replica number is invalid
    ///
    /// \returns int
    /// \retval replica status
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto replica_status(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const replica_number_type _replica_number) -> int
    {
        const auto result = get_data_object_info(_comm, _logical_path, _replica_number).front();

        std::string_view status = result[genquery_column_index::DATA_REPL_STATUS];

        return static_cast<int>(std::stoi(status.data()));
    } // replica_status

    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _leaf_resource_name
    ///
    /// \throws filesystem_error if the path is empty or too long
    /// \throws irods::exception if the path does not refer to a data object or leaf resource is invalid
    ///
    /// \returns int
    /// \retval replica status
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto replica_status(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const leaf_resource_name_type& _leaf_resource_name) -> int
    {
        const auto result = get_data_object_info(_comm, _logical_path, _leaf_resource_name).front();

        std::string_view status = result[genquery_column_index::DATA_REPL_STATUS];

        return static_cast<int>(std::stoi(status.data()));
    } // replica_status

    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _replica_number
    ///
    /// \throws filesystem_error if the path is empty or too long
    /// \throws irods::exception if the path does not refer to a data object or replica number is invalid
    ///
    /// \returns std::uintmax_t
    /// \retval size of the data in storage which specified replica represents
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto get_replica_size_from_storage(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const replica_number_type _replica_number) -> rodsLong_t
    {
        const auto result = get_data_object_info(_comm, _logical_path, _replica_number).front();

        std::string_view hierarchy = result[genquery_column_index::DATA_RESC_HIER];
        std::string_view physical_path = result[genquery_column_index::DATA_PATH];

        return detail::get_replica_size_from_storage_impl(_comm, _logical_path, hierarchy, physical_path);
    } // get_replica_size_from_storage

    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _leaf_resource_name
    ///
    /// \throws filesystem_error if the path is empty or too long
    /// \throws irods::exception if the path does not refer to a data object or leaf resource is invalid
    ///
    /// \returns std::uintmax_t
    /// \retval size of the data in storage which specified replica represents
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto get_replica_size_from_storage(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        const leaf_resource_name_type& _leaf_resource_name) -> rodsLong_t
    {
        const auto result = get_data_object_info(_comm, _logical_path, _leaf_resource_name).front();

        std::string_view hierarchy = result[genquery_column_index::DATA_RESC_HIER];
        std::string_view physical_path = result[genquery_column_index::DATA_PATH];

        return detail::get_replica_size_from_storage_impl(_comm, _logical_path, hierarchy, physical_path);
    } // get_replica_size_from_storage

    /// \param[in] _comm connection object
    /// \param[in] _logical_path
    /// \param[in] _resource_hierarchy
    /// \param[in] _physical_path
    ///
    /// \throws filesystem_error if the path is empty or too long
    /// \throws irods::exception if the path does not refer to a data object or leaf resource is invalid
    ///
    /// \returns std::uintmax_t
    /// \retval size of the data in storage which specified replica represents
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto get_replica_size_from_storage(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _logical_path,
        std::string_view _resource_hierarchy,
        std::string_view _physical_path) -> rodsLong_t
    {
        return detail::get_replica_size_from_storage_impl(_comm, _logical_path, _resource_hierarchy, _physical_path);
    } // get_replica_size_from_storage

} // namespace irods::experimental::replica

#endif // #ifndef IRODS_REPLICA_HPP
