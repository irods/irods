#ifndef PROC_API_REQUEST_H
#define PROC_API_REQUEST_H

/// \file

#include "irods/packStruct.h"
#include "irods/rods.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Executes an iRODS API request.
///
/// This function uses the global packing instruction table for serialization and deserialization.
///
/// \param[in]  conn        The RcComm used for communication.
/// \param[in]  apiNumber   The integer which identifies the API to execute. See apiNumberData.h.
/// \param[in]  inputStruct The data to serialize, typically an input data structure. Can be
///                         passed \p NULL if no input is necessary.
/// \param[in]  inputBsBBuf The byte buffer to stream to the server. Can be passed \p NULL if no
///                         input byte stream is necessary.
/// \param[out] outStruct   The output data structure to be filled by the server. The output will
///                         be heap-allocated and assigned to the pointer. Can be passed \p NULL
///                         if no output is expected.
/// \param[out] outBsBBuf   The output byte buffer to be filled by the server. Can be passed
///                         \p NULL if no output stream is expected.
///
/// \return An integer indicating success or failure.
/// \retval >=0 On success.
/// \retval  <0 On failure.
int procApiRequest(rcComm_t *conn,
                   int apiNumber,
                   const void *inputStruct,
                   const bytesBuf_t *inputBsBBuf,
                   void **outStruct,
                   bytesBuf_t *outBsBBuf);

/// Executes an iRODS API request.
///
/// Grants the caller control over which packing instructions are used during serialization and
/// deserialization of the data structures. This form of procApiRequest is useful for maintaining
/// backward compatibility.
///
/// \warning This function is provided for handling changes in packing instructions for an API
/// only. All other properties of the original API must be honored. The behavior of this function
/// is undefined if preconditions are violated.
///
/// \param[in]  conn        The RcComm used for communication.
/// \param[in]  apiNumber   The integer which identifies the API to execute. See apiNumberData.h.
/// \param[in]  packingInstructionTable The table holding various packing instructions. All
///                         packing instructions needed for serialization of the data structure
///                         must be defined in the table. If passed \p NULL, the global packing
///                         instruction table will be used.
/// \param[in]  inputPackingInstruction The packing instruction to use for serialization. Cannot
///                         be \p NULL.
/// \param[in]  inputStruct The data to serialize, typically an input data structure. Can be
///                         passed \p NULL if no input is necessary.
/// \param[in]  inputBsBBuf The byte buffer to stream to the server. Can be passed \p NULL if no
///                         input byte stream is necessary.
/// \param[in]  outputPackingInstruction The packing instruction to use for deserialization.
///                         Can be set to \p NULL if no output is expected.
/// \param[out] outStruct   The output data structure to be filled by the server. The output will
///                         be heap-allocated and assigned to the pointer. Can be passed \p NULL
///                         if no output is expected.
/// \param[out] outBsBBuf   The output byte buffer to be filled by the server. Can be passed
///                         \p NULL if no output stream is expected.
///
/// \return An integer indicating success or failure.
/// \retval >=0 On success.
/// \retval  <0 On failure.
///
/// \since 4.3.5
int procApiRequest_raw(rcComm_t* conn,
                       int apiNumber,
                       const struct PackingInstruction* packingInstructionTable,
                       const char* inputPackingInstruction,
                       const void* inputStruct,
                       const bytesBuf_t* inputBsBBuf,
                       const char* outputPackingInstruction,
                       void** outStruct,
                       bytesBuf_t* outBsBBuf);

int sendApiRequest(rcComm_t *conn,
                   int apiInx,
                   const void *inputStruct,
                   const bytesBuf_t *inputBsBBuf);

int procApiReply(rcComm_t *conn,
                 int apiInx,
                 void **outStruct,
                 bytesBuf_t *outBsBBuf,
                 msgHeader_t *myHeader,
                 bytesBuf_t *outStructBBuf,
                 bytesBuf_t *myOutBsBBuf,
                 bytesBuf_t *errorBBuf);

int readAndProcApiReply(rcComm_t *conn,
                        int apiInx,
                        void **outStruct,
                        bytesBuf_t *outBsBBuf);

int branchReadAndProcApiReply(rcComm_t *conn,
                              int apiNumber,
                              void **outStruct,
                              bytesBuf_t *outBsBBuf);

int cliGetCollOprStat(rcComm_t *conn,
                      collOprStat_t *collOprStat,
                      int vFlag,
                      int retval);

int _cliGetCollOprStat(rcComm_t *conn, collOprStat_t **collOprStat);

int apiTableLookup(int apiNumber);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PROC_API_REQUEST_H
