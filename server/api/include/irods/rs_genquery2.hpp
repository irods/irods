#ifndef IRODS_RS_GENQUERY2_HPP
#define IRODS_RS_GENQUERY2_HPP

/// \file

#include "irods/genquery2.h"

struct RsComm;

/// Query the catalog using the GenQuery2 parser.
///
/// \warning This API is experimental and may change in the future.
///
/// The GenQuery2 parser supports the following:
/// - Enforces the iRODS permission model
/// - Logical AND, OR, and NOT
/// - Grouping via parentheses
/// - SQL CAST
/// - SQL GROUP BY
/// - SQL aggregate functions (e.g. count, sum, avg, etc)
/// - Per-column sorting via ORDER BY [ASC|DESC]
/// - SQL FETCH FIRST \a N ROWS ONLY (LIMIT offered as an alias)
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
/// - Integer values must be treated as strings, except when used for OFFSET, LIMIT, FETCH FIRST \a N ROWS ONLY
///
/// When the query does not include the FETCH FIRST \a N ROWS ONLY or LIMIT clause, the API will clamp the
/// size of the resultset to 256 rows.
///
/// The API does not provide any built-in support for pagination due to it being application-dependent.
/// However, the API does provide the necessary facilities for implementing various forms of pagination.
///
/// The column mappings between GenQuery2 and the catalog can be obtained via the API. See the Genquery2Input
/// structure for details.
///
/// \param[in]     _comm   A pointer to a RsComm.
/// \param[in]     _input  A pointer to a Genquery2Input.
/// \param[in,out] _output \parblock A pointer that will hold the results of the operation.
/// On success, the pointer will either hold a JSON string or a string representing the SQL derived from
/// the GenQuery2 query string. See Genquery2Input::sql_only for details.
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
/// RsComm* comm = // Our iRODS connection.
///
/// // Configure the input object for the API call.
/// struct Genquery2Input input;
/// memset(&input, 0, sizeof(struct Genquery2Input));
///
/// // This is the query that will be executed (i.e. input.sql_only is set to 0).
/// input.query_string = strdup("select COLL_NAME, DATA_NAME where RESC_ID = '10016'");
///
/// char* output = NULL;
/// const int ec = rs_genquery2(comm, &input, &output);
///
/// if (ec < 0) {
///     // Only the query string needs to be free'd.
///     // The "output" pointer will always be NULL on error.
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
int rs_genquery2(RsComm* _comm, Genquery2Input* _input, char** _output);

#endif // IRODS_RS_GENQUERY2_HPP
