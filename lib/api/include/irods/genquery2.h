#ifndef IRODS_GENQUERY2_H
#define IRODS_GENQUERY2_H

/// \file

struct RcComm;

/// The input data type used to invoke #rc_genquery2.
///
/// \note This data structure is part of an experimental API endpoint and may change in the future.
///
/// \since 4.3.2
typedef struct GenQuery2Input // NOLINT(modernize-use-using)
{
    /// The GenQuery2 query string to execute.
    ///
    /// This member variable MUST be non-empty.
    ///
    /// \since 4.3.2
    char* query_string;

    /// The zone to execute the query string against.
    ///
    /// This member variable is allowed to be set to NULL.
    ///
    /// \since 4.3.2
    char* zone;

    /// Controls whether the SQL derived from the query string is
    /// executed or returned to the caller.
    ///
    /// When set to 0, the generated SQL will be executed.
    /// When set to 1, the generated SQL will be returned to the caller. The SQL will not be executed.
    ///
    /// \since 4.3.2
    int sql_only;
} genQuery2Inp_t;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GenQuery2Input_PI "str *query_string; str *zone; int sql_only;"

#ifdef __cplusplus
extern "C" {
#endif

/// Query the catalog using the GenQuery2 parser.
///
/// \note This is an experimental API endpoint and may change in the future.
///
/// The GenQuery2 parser supports the following:
/// - Enforces the iRODS permission model
/// - Logical AND, OR, and NOT
/// - Grouping via parentheses
/// - SQL CAST
/// - SQL GROUP BY
/// - SQL aggregate functions (e.g. count, sum, avg, etc)
/// - Per-column sorting via ORDER BY [ASC|DESC]
/// - SQL FETCH FIRST <i>N</i> ROWS ONLY (LIMIT offered as an alias)
/// - Metadata queries involving different iRODS entities (i.e. data objects, collections, users, and resources)
/// - Operators: =, !=, <, <=, >, >=, LIKE, BETWEEN, IS [NOT] NULL
/// - SQL keywords are case-insensitive
/// - Federation
/// - Escaping of single quotes
/// - Bytes encoded as hexadecimal (e.g. \x21)
///
/// Limitations:
/// - Groups are not yet fully supported
/// - Cannot resolve tickets to data objects and collections using a single query
/// - Integer values must be treated as strings, except when used for OFFSET, LIMIT, FETCH FIRST <i>N</i> ROWS ONLY
///
/// \param[in]     _comm   A pointer to a RcComm.
/// \param[in]     _input  A pointer to a GenQuery2Input.
/// \param[in,out] _output \parblock A pointer that will hold the results of the operation.
/// On success, the pointer will either hold a JSON string or a string representing the SQL derived from
/// the GenQuery2 query string. See GenQuery2Input::sql_only for details.
///
/// The string is always heap-allocated and must be free'd by the caller using free().
///
/// On failure, the pointer will be NULL.
/// \endparblock
///
/// \return An integer.
/// \retval  0 On success.
/// \retval <0 On failure.
///
/// \b Example
/// \code{.cpp}
/// RcComm* comm = // Our iRODS connection.
///
/// // Configure the input object for the API call.
/// struct GenQuery2Input input;
/// memset(&input, 0, sizeof(struct GenQuery2Input));
///
/// // This is the query that will be executed (i.e. input.sql_only is set to 0).
/// input.query_string = strdup("select COLL_NAME, DATA_NAME where RESC_ID = '10016'");
///
/// char* output = NULL;
/// const int ec = rc_genquery2(comm, &input, &output);
///
/// // Handle error.
/// if (ec < 0) {
///     free(input.query_string);
///     return;
/// }
///
/// // At this point, "output" should represent a JSON string.
/// // Parse the string as JSON and inspect the results. 
/// // If "input.sql_only" was set to 1, "output" would hold the SQL derived from the GenQuery2 syntax.
/// \endcode
///
/// \since 4.3.2
int rc_genquery2(struct RcComm* _comm, struct GenQuery2Input* _input, char** _output);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_GENQUERY2_H
