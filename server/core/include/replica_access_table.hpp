#ifndef IRODS_REPLICA_ACCESS_TABLE_HPP
#define IRODS_REPLICA_ACCESS_TABLE_HPP

/// \file

#include <string>
#include <string_view>
#include <stdexcept>
#include <optional>

#include <sys/types.h>
#include <unistd.h>

namespace irods::experimental
{
    /// The primary exception class for replica_access_table.
    ///
    /// \since 4.2.9
    class replica_access_table_error
        : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    /// A utility class that enables tracking of replicas and PIDs.
    ///
    /// This class provides access to a singleton object that is meant to be used by
    /// the iRODS streaming operations and voting mechanisms to control multi-process
    /// write access to a single replica.
    ///
    /// \since 4.2.9
    class replica_access_table
    {
    public:
        // clang-format off
        using replica_token_type      = std::string;
        using replica_token_view_type = std::string_view;
        using data_id_type            = std::uint64_t;
        using replica_number_type     = std::uint32_t;
        // clang-format on

        /// A class that is used to restore previously removed entries.
        ///
        /// \see erase_pid(replica_token_view_type, pid_t)
        class restorable_entry
        {
        public:
            const replica_token_type token;
            const data_id_type data_id;
            const replica_number_type replica_number;
            const pid_t pid;

        private:
            restorable_entry(replica_token_view_type _token,
                             data_id_type _data_id,
                             replica_number_type _replica_number,
                             pid_t _pid)
                : token{_token}
                , data_id{_data_id}
                , replica_number{_replica_number}
                , pid{_pid}
            {
            }

            friend replica_access_table;
        }; // class restorable_entry

        /// Initializes the replica access table.
        ///
        /// While calling deinit() is preferred, it is not necessary since this function
        /// will delete any memory used by a previous instance of the process.
        static auto init() noexcept -> void;

        /// Cleans up any memory previously allocated by calling init().
        static auto deinit() noexcept -> void;

        /// Returns a reference to the global replica access table.
        static auto instance() noexcept -> replica_access_table&;

        replica_access_table(const replica_access_table&) = delete;
        auto operator=(const replica_access_table&) -> replica_access_table& = delete;

        /// Creates a new entry in the table.
        ///
        /// \param[in] _data_id        The data id of the data object.
        /// \param[in] _replica_number The replica number of the targeted replica.
        /// \param[in] _pid            The process id of the agent handling the request.
        ///
        /// \throws replica_access_table_error If an entry exists matching the arguments.
        ///
        /// \return A newly generated replica token.
        auto create_new_entry(data_id_type _data_id,
                              replica_number_type _replica_number,
                              pid_t _pid) -> replica_token_type;

        /// Appends a PID to the entry's PID list.
        ///
        /// \param[in] _token          The replica token mapped to the replica.
        /// \param[in] _data_id        The data id of the data object.
        /// \param[in] _replica_number The replica number of the targeted replica.
        /// \param[in] _pid            The PID to append.
        ///
        /// \throws replica_access_table_error If the token is invalid.
        /// \throws replica_access_table_error If the data id and/or replica number are invalid.
        auto append_pid(replica_token_view_type _token,
                        data_id_type _data_id,
                        replica_number_type _replica_number,
                        pid_t _pid) -> void;

        /// Returns whether the (data id, replica number) tuple exists.
        ///
        /// \param[in] _data_id        The data id of the replica.
        /// \param[in] _replica_number The replica number of the replica.
        ///
        /// \return A boolean.
        /// \retval true  If the tuple exists.
        /// \retval false Otherwise.
        auto contains(data_id_type _data_id, replica_number_type _replica_number) -> bool;

        /// Returns whether the (replica token, data id, replica number) tuple exists.
        ///
        /// \param[in] _token          The replica token mapped to the replica.
        /// \param[in] _data_id        The data id of the replica.
        /// \param[in] _replica_number The replica number of the replica.
        ///
        /// \return A boolean.
        /// \retval true  If the tuple exists.
        /// \retval false Otherwise.
        auto contains(replica_token_view_type _token,
                      data_id_type _data_id,
                      replica_number_type _replica_number) -> bool;

        /// Removes a PID from the entry's PID list.
        ///
        /// \param[in] _token The replica token mapped to the replica.
        /// \param[in] _pid   The PID to remove.
        ///
        /// \returns A \ref restorable_entry containing all information about the removed PID.
        auto erase_pid(replica_token_view_type _token, pid_t _pid) -> std::optional<restorable_entry>;

        /// Permanently removes a PID from all entries.
        ///
        /// This function is provided in cases where the agent exits or terminates in an
        /// unusual manner. This function should only be used by the agent factory.
        ///
        /// \param[in] _pid The PID to remove.
        auto erase_pid(pid_t _pid) -> void;

        /// Restores a previously removed entry.
        ///
        /// The entry will appear as if it was never removed.
        ///
        /// \param[in] _entry A reference to a previously erased entry.
        auto restore(const restorable_entry& _entry) -> void;

    private:
        replica_access_table() = default;
    };
} // namespace irods::experimental

#endif // IRODS_REPLICA_ACCESS_TABLE_HPP
