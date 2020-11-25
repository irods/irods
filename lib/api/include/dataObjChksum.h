#ifndef DATA_OBJ_CHKSUM_H__
#define DATA_OBJ_CHKSUM_H__

/// \file

struct RcComm;
struct DataObjInp;

#ifdef __cplusplus
extern "C" {
#endif

/// Computes and stores a checksum for a data object.
///
/// The checksum algorithm used by this function is determined by the client and server's configuration,
/// but defaults to SHA256.
///
/// This API function supports two modes of operation:
/// - \p Lookup/Update (default): Deals with computation and updates of checksum information.
/// - \p Verification:            Deals with verifying the checksum information. This mode does not perform catalog updates.
///
/// Operations that target multiple replicas will only affect replicas that are marked good. This means intermediate,
/// locked, and stale replicas will be ignored.
///
/// Operations that target a specific replica are allowed to operate on stale replicas.
///
/// \param[in]  comm             The communication object.
/// \param[in]  dataObjChksumInp \parblock
/// The bundle of input arguments that dictate what happens.
///
/// The relevant member variables of this structure are:
/// - \p objPath:   The absolute path to a data object.
/// - \p condInput: A list of keywords and values that instruct the server on how to carry out the operation.
/// 
/// Keywords supported by \p condInput:
/// - \p FORCE_CHKSUM_KW:
///     - Instructs the server to compute and update the checksum.
///     - Must be set to an empty string.
/// - \p REPL_NUM_KW:
///     - Identifies a specific replica by the replica number.
///     - Accepts the replica number as a string.
///     - Incompatible with \p RESC_NAME_KW.
///     - Incompatible with \p CHKSUM_ALL_KW.
/// - \p RESC_NAME_KW:
///     - Identifies a specific replica by the leaf resource name.
///     - Accepts the resource name as a string.
///     - Incompatible with \p REPL_NUM_KW.
///     - Incompatible with \p CHKSUM_ALL_KW.
/// - \p CHKSUM_ALL_KW:
///     - Instructs the server to operate on all replicas.
///     - Must be set to an empty string.
///     - In \p lookup/update mode, reports if the replicas do not share identical checksums if no errors occur.
///     - Incompatible with \p RESC_NAME_KW.
///     - Incompatible with \p REPL_NUM_KW.
/// - \p ADMIN_KW:
///     - Instructs the server to execute the operation as an administrator.
///     - Must be set to an empty string.
/// - \p VERIFY_CHKSUM_KW:
///     - Instructs the server to verify the checksum information (enables verification mode).
///     - Must be set to an empty string.
///     - Operates on all replicas unless a specific replica is targeted via \p REPL_NAME_KW or \p RESC_NAME_KW.
///     - No checksum is returned to the client when in verification mode.
///     - The following operations are performed:
///         1. Reports replicas with mismatched size information (physical vs catalog).
///         2. Reports replicas that are missing checksums to the client.
///         3. Reports replicas with mismatched checksums (computed vs catalog).
///         4. Reports if the replicas do not share identical checksums.
///     - If \p NO_COMPUTE_KW is set, step 3 will not be performed.
///     - If a specific replica is targeted, step 4 will not be performed.
///     - Verification results are reported to the client via the RcComm::rError object.
/// - \p NO_COMPUTE_KW:
///     - Instructs the server to not compute a checksum in verification mode potentially leading to a performance boost.
///     - Must be set to an empty string.
///     - A modifier for \p VERIFY_CHKSUM_KW.
/// \endparblock
/// \param[out] outChksum        Holds the returned checksum if available.
///
/// \returns An integer.
/// \retval 0        On success.
/// \retval Non-zero On failure.
///
/// \b Example
/// \code{.cpp}
/// DataObjInp input;
/// memset(&input, 0, sizeof(DataObjInp));
///
/// // Set the absolute path to the data object.
/// rstrcpy(input.objPath, "/tempZone/home/rods/data_object", MAX_NAME_LEN);
///
/// // Verify the checksum information for replica number 2.
/// addKeyVal(&input.condInput, VERIFY_CHKSUM_KW, "");
/// addKeyVal(&input.condInput, REPL_NUM_KW, "2");
///
/// RcComm* comm = // Our iRODS connection.
/// char* computed_checksum = NULL;
/// const int ec = rcDataObjChksum(comm, &input, &computed_checksum);
///
/// if (ec < 0) {
///     // Handle error.
/// }
///
/// //
/// // Do something with "computed_checksum".
/// //
///
/// free(computed_checksum);
/// \endcode
int rcDataObjChksum(struct RcComm* comm,
                    struct DataObjInp* dataObjChksumInp,
                    char** outChksum);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // DATA_OBJ_CHKSUM_H__

