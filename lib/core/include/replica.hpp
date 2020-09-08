#ifndef IRODS_REPLICA_HPP
#define IRODS_REPLICA_HPP

#ifdef IRODS_REPLICA_ENABLE_SERVER_SIDE_API
    #define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
    #include "rsDataObjChksum.hpp"
#else
    #undef IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
    #include "dataObjChksum.h"
#endif

#include "filesystem.hpp"
#include "irods_exception.hpp"
#include "key_value_proxy.hpp"
#include "modDataObjMeta.h"
#include "objInfo.h"
#include "query_builder.hpp"
#include "rcConnect.h"

#include "fmt/format.h"

#include <chrono>
#include <iomanip>
#include <string_view>

namespace irods::experimental::replica
{
    using replica_number_type = int;

    /// \brief Describes whether the catalog should be updated when calculating a replica's checksum
    /// \since 4.2.9
    enum class verification_calculation
    {
        if_empty,
        always
    };

    namespace detail
    {
        /// \brief GenQuery columns which represent a replica
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
            DATA_RESC_ID
        };

        /// \param[in] _comm connection object
        /// \param[in] _p path to inspect
        ///
        /// \throws irods::exception if the path does not refer to a data object
        ///
        /// \since 4.2.9
        inline auto throw_if_path_is_not_a_data_object(
            rxComm& _comm,
            const irods::experimental::filesystem::path& _p) -> void
        {
            if (!irods::experimental::filesystem::NAMESPACE_IMPL::is_data_object(_comm, _p)) {
                THROW(SYS_INVALID_INPUT_PARAM, "path does not point to a data object");
            }
        } // throw_if_path_is_not_a_data_object

        /// \param[in] _rn replica number to inspect
        ///
        /// \throws irods::exception if the path does not refer to a data object
        ///
        /// \since 4.2.9
        inline auto throw_if_replica_number_is_invalid(const replica_number_type _rn) -> void
        {
            if (_rn < 0) {
                THROW(SYS_INVALID_INPUT_PARAM, "invalid replica number");
            }
        } // throw_if_replica_number_is_invalid

        /// \brief Enforce valid path and replica number for replica operation
        /// \param[in] _comm connection object
        /// \param[in] _p path to inspect
        /// \param[in] _rn replica number to inspect
        /// \throws filesystem_error if the path is empty or too long
        /// \throws irods::exception if the path does not refer to a data object or replica number is invalid
        /// \since 4.2.9
        template<typename rxComm>
        inline auto throw_if_replica_input_is_invalid(
            rxComm& _comm,
            const irods::experimental::filesystem::path& _p,
            const replica_number_type _rn) -> void
        {
            irods::experimental::filesystem::detail::throw_if_path_is_empty(_p);

            irods::experimental::filesystem::detail::throw_if_path_length_exceeds_limit(_p);

            throw_if_path_is_not_a_data_object(_comm, _p);

            throw_if_replica_number_is_invalid(_rn);
        } // throw_if_replica_input_is_invalid

    } // namespace detail

    // TODO: this may disappear in the future
    /// \brief Gets a row from r_data_main using irods::query
    ///
    /// \param[in] _comm connection object
    /// \param[in] _p logical path
    /// \param[in] _rn replica number
    ///
    /// \throws irods::exception If no replica information is found or query fails
    ///
    /// \returns std::vector<std::string>
    /// \retval Set of results from the query
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto get_replica_info(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _p,
        const replica_number_type _rn) -> std::vector<std::string>
    {
        query_builder qb;
        qb.zone_hint(*irods::experimental::filesystem::zone_name(_p));

        const std::string qstr = fmt::format(
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
            "DATA_RESC_ID "
            "WHERE DATA_NAME = '{}' AND COLL_NAME = '{}' AND DATA_REPL_NUM = '{}'",
            _p.object_name().c_str(), _p.parent_path().c_str(), _rn);

        const auto q = qb.build<rxComm>(_comm, qstr);
        if (q.size() <= 0) {
            THROW(CAT_NO_ROWS_FOUND, "no replica information found");
        }
        return q.front();
    } // get_replica_info

    /// \param[in] _comm connection object
    /// \param[in] _p logical path
    /// \param[in] _rn replica number
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
        const irods::experimental::filesystem::path& _p,
        const replica_number_type _rn) -> std::uintmax_t
    {
        detail::throw_if_replica_input_is_invalid(_comm, _p, _rn);

        const auto result = get_replica_info(_comm, _p, _rn);

        return static_cast<std::uintmax_t>(std::stoull(result[detail::genquery_column_index::DATA_SIZE]));
    } // replica_size

    /// \param[in] _comm connection object
    /// \param[in] _p logical path
    /// \param[in] _rn replica number
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
        const irods::experimental::filesystem::path& _p,
        const replica_number_type _rn) -> bool
    {
        return replica_size(_comm, _p, _rn) == 0;
    } // is_replica_empty

    /// \brief Calculate checksum for a replica
    ///
    /// The verification_calculation helps determine whether the calculated checksum will be registered in the catalog
    ///
    /// \param[in] _comm connection object
    /// \param[in] _p logical path
    /// \param[in] _rn replica number
    /// \param[in] _calculation mode for catalog update
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
        const irods::experimental::filesystem::path& _p,
        const replica_number_type _rn,
        verification_calculation _calculation = verification_calculation::if_empty) -> std::string
    {
        detail::throw_if_replica_input_is_invalid(_comm, _p, _rn);

        dataObjInp_t input{};
        std::string replica_number_string;
        auto cond_input = make_key_value_proxy(input.condInput);

        cond_input[REPL_NUM_KW] = std::to_string(_rn);

        std::snprintf(input.objPath, sizeof(input.objPath), "%s", _p.c_str());

        if (verification_calculation::always == _calculation) {
            cond_input[FORCE_CHKSUM_KW] = "";
        }

        char* checksum{};

        if constexpr (std::is_same_v<rxComm, rsComm_t>) {
            if (const auto ec = rsDataObjChksum(&_comm, &input, &checksum); ec < 0) {
                THROW(ec, fmt::format("cannot calculate checksum for [{}] (replica [{}])", _p.string(), _rn));
            }
        }
        else {
            if (const auto ec = rcDataObjChksum(&_comm, &input, &checksum); ec < 0) {
                THROW(ec, fmt::format("cannot calculate checksum for [{}] (replica [{}])", _p.string(), _rn));
            }
        }

        return checksum ? checksum : std::string{};
    } // replica_checksum

    /// \brief Returns timestamp of last time this replica was written to
    ///
    /// \param[in] _comm connection object
    /// \param[in] _p logical path
    /// \param[in] _rn replica number
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
        const irods::experimental::filesystem::path& _p,
        const replica_number_type _rn) -> irods::experimental::filesystem::object_time_type
    {
        using object_time_type = irods::experimental::filesystem::object_time_type;

        detail::throw_if_replica_input_is_invalid(_comm, _p, _rn);

        const auto result = get_replica_info(_comm, _p, _rn);
        const auto& mtime = result[detail::genquery_column_index::DATA_MODIFY_TIME];

        return object_time_type{std::chrono::seconds{std::stoull(mtime)}};
    } // last_write_time

    /// \brief Sets value of the timestamp of last time this replica was written to
    ///
    /// \param[in] _comm connection object
    /// \param[in] _p logical path
    /// \param[in] _rn replica number
    /// \param[in] _new_time timestamp to use as last_write_time
    ///
    /// \throws irods::filesystem::filesystem_error if the path is empty or too long
    /// \throws irods::exception if the path does not refer to a data object or replica number is invalid
    ///
    /// \since 4.2.9
    template<typename rxComm>
    auto last_write_time(
        rxComm& _comm,
        const irods::experimental::filesystem::path& _p,
        const replica_number_type _rn,
        const irods::experimental::filesystem::object_time_type _new_time) -> void
    {
        detail::throw_if_replica_input_is_invalid(_comm, _p, _rn);

        const auto seconds = _new_time.time_since_epoch();
        std::stringstream new_time;
        new_time << std::setfill('0') << std::setw(11) << std::to_string(seconds.count());

        dataObjInfo_t info{};
        std::snprintf(info.objPath, sizeof(info.objPath), "%s", _p.c_str());

        keyValPair_t kvp{};
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
            THROW(ec, fmt::format("cannot set mtime for [{}] (replica [{}])", _p.c_str(), _rn));
        }
    } // last_write_time
} // namespace irods::experimental::replica

#endif // #ifndef IRODS_REPLICA_HPP
