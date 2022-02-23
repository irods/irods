#ifndef IRODS_RS_MOD_A_V_U_METADATA_HPP
#define IRODS_RS_MOD_A_V_U_METADATA_HPP

/// \file

#include "irods/modAVUMetadata.h"

struct RsComm;
struct ModifyAVUMetadataInput;

int rsModAVUMetadata(RsComm* rsComm, ModifyAVUMetadataInput* modAVUMetadataInp);
int _rsModAVUMetadata(RsComm* rsComm, ModifyAVUMetadataInput* modAVUMetadataInp);

#endif // IRODS_RS_MOD_A_V_U_METADATA_HPP
