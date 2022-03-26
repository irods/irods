#ifndef IRODS_SERVER_UTILITIES_HPP
#define IRODS_SERVER_UTILITIES_HPP

/// \file

#include <sys/types.h>

#include <string_view>
#include <optional>

struct RsComm;
struct DataObjInp;
struct BytesBuf;

namespace irods
{
    /// The name of the PID file used for the main server.
    ///
    /// \since 4.3.0
    extern const std::string_view PID_FILENAME_MAIN_SERVER;

    /// The name of the PID file used for the delay server.
    ///
    /// \since 4.3.0
    extern const std::string_view PID_FILENAME_DELAY_SERVER;

    /// A utility function primarily meant to be used with ::rsDataObjPut and ::rsDataObjCopy.
    ///
    /// \param[in] _comm  A reference to the communication object.
    /// \param[in] _input A reference to the ::DataObjInp containing the ::KeyValPair.
    ///
    /// \return A boolean value indicating whether the FORCE_FLAG_KW is required or not.
    /// \retval true  If the keyword is required.
    /// \retval false If the keyword is not required.
    ///
    /// \since 4.2.9
    auto is_force_flag_required(RsComm& _comm, const DataObjInp& _input) -> bool;

    /// Checks if the provided rule text contains session variables.
    ///
    /// This function will reject any text that contains a session variable. This includes text
    /// that contains session variables in comments.
    ///
    /// \param[in] _rule_text The rule text to check.
    ///
    /// \return A boolean value.
    /// \retval true  If the rule text contains session variables.
    /// \retval false Otherwise.
    ///
    /// \since 4.2.9
    auto contains_session_variables(const std::string_view _rule_text) -> bool;

    /// Converts provided string to a new BytesBuf structure (caller owns the memory).
    ///
    /// \param[in] _s The string to copy into the buffer.
    ///
    /// \return A pointer to the newly allocated BytesBuf structure.
    ///
    /// \since 4.2.11
    auto to_bytes_buffer(const std::string_view _s) -> BytesBuf*;

    /// Creates a PID file that allows only one instance of a process to run at a time.
    ///
    /// \param[in] _pid_filename The name of the PID file (e.g. "irods.pid").
    ///
    /// \return An integer representing success or failure.
    /// \retval  0 On success.
    /// \retval -1 On failure.
    ///
    /// \since 4.3.0
    auto create_pid_file(const std::string_view _pid_filename) -> int;

    /// Returns the PID stored in the file if available.
    ///
    /// This function will return a \p std::nullopt if the filename does not refer to a file
    /// under the temp directory or the PID is not a child of the calling process.
    ///
    /// \param[in] _pid_filename The name of the file to which contains a PID value (just the
    ///                          filename, not the absolute path).
    ///
    /// \return The PID stored in the file.
    ///
    /// \since 4.3.0
    auto get_pid_from_file(const std::string_view _pid_filename) noexcept -> std::optional<pid_t>;
} // namespace irods

#endif // IRODS_SERVER_UTILITIES_HPP

