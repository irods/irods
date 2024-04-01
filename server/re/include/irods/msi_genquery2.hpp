#ifndef IRODS_MSI_GENQUERY2_HPP
#define IRODS_MSI_GENQUERY2_HPP

/// \file

struct MsParam;
struct RuleExecInfo;

/// Query the catalog using the GenQuery2 parser.
///
/// \warning This microservice is experimental and may change in the future.
/// \warning This microservice is NOT thread-safe.
///
/// GenQuery2 handles are only available to the agent which produced them.
///
/// The lifetime of a handle and its resultset is tied to the lifetime of the agent.
/// The value of the handle MUST NOT be viewed as having any meaning. Policy MUST NOT rely on
/// its structure for any reason as it may change across releases.
///
/// \param[out] _handle       The output parameter that will hold the handle to the resultset.
/// \param[in]  _query_string The GenQuery2 string to execute.
/// \param[in]  _rei          This parameter is special and should be ignored.
///
/// \return An integer.
/// \retval  0 On success.
/// \retval <0 On failure.
///
/// \see genquery2.h for features supported by GenQuery2.
///
/// \since 4.3.2
auto msi_genquery2_execute(MsParam* _handle, MsParam* _query_string, RuleExecInfo* _rei) -> int;

/// Moves the cursor forward by one row.
///
/// \warning This microservice is experimental and may change in the future.
/// \warning This microservice is NOT thread-safe.
///
/// GenQuery2 handles are only available to the agent which produced them.
///
/// \param[in] _handle The GenQuery2 handle.
/// \param[in] _rei    This parameter is special and should be ignored.
///
/// \return An integer.
/// \retval       0 If data can be read from the new row.
/// \retval -408000 If the end of the resultset has been reached.
/// \retval      <0 If an error occurred.
///
/// \b Example
/// \code{.py}
/// # A global constant representing the value which indicates the end of
/// # the resultset has been reached.
/// END_OF_RESULTSET = -408000;
///
/// iterating_over_genquery2_results()
/// {
///     # Execute a query. The results are stored in the Rule Engine Plugin.
///     msi_genquery2_execute(*handle, "select COLL_NAME, DATA_NAME order by DATA_NAME desc limit 1");
///
///     # Iterate over the results.
///     while (true) {
///         *ec = errorcode(msi_genquery2_next_row(*handle));
///
///         if (*ec < 0) {
///             if (END_OF_RESULTSET == *ec) {
///                 break;
///             }
///
///             failmsg(*ec, "Unexpected error while iterating over GenQuery2 resultset.");
///         }
///
///         msi_genquery2_column(*handle, '0', *coll_name); # Copy the COLL_NAME into *coll_name.
///         msi_genquery2_column(*handle, '1', *data_name); # Copy the DATA_NAME into *data_name.
///         writeLine("stdout", "logical path => [*coll_name/*data_name]");
///     }
///
///     # Free any resources used. This is handled for you when the agent is shut down as well.
///     msi_genquery2_free(*handle);
/// }
/// \endcode
///
/// \since 4.3.2
auto msi_genquery2_next_row(MsParam* _handle, RuleExecInfo* _rei) -> int;

/// Reads the value of a column from a row within a GenQuery2 resultset.
///
/// \warning This microservice is experimental and may change in the future.
/// \warning This microservice is NOT thread-safe.
///
/// GenQuery2 handles are only available to the agent which produced them.
///
/// \param[in]  _handle       The GenQuery2 handle.
/// \param[in]  _column_index The index of the column to read. The index must be passed as a string.
/// \param[out] _column_value The variable to write the value of the column to.
/// \param[in]  _rei          This parameter is special and should be ignored.
///
/// \return An integer.
/// \retval  0 On success.
/// \retval <0 On failure.
///
/// \since 4.3.2
auto msi_genquery2_column(MsParam* _handle, MsParam* _column_index, MsParam* _column_value, RuleExecInfo* _rei) -> int;

/// Frees all resources associated with a GenQuery2 handle.
///
/// \warning This microservice is experimental and may change in the future.
/// \warning This microservice is NOT thread-safe.
///
/// GenQuery2 handles are only available to the agent which produced them.
///
/// Users are expected to call this microservice when a GenQuery2 resultset is no longer needed.
///
/// \warning Failing to follow this rule can result in memory leaks. However, the handle and the resultset
/// it is mapped to will be free'd on agent teardown.
///
/// \param[in] _handle The GenQuery2 handle.
/// \param[in] _rei    This parameter is special and should be ignored.
///
/// \return An integer.
/// \retval  0 On success.
/// \retval <0 On failure.
///
/// \since 4.3.2
auto msi_genquery2_free(MsParam* _handle, RuleExecInfo* _rei) -> int;

#endif // IRODS_MSI_GENQUERY2_HPP
