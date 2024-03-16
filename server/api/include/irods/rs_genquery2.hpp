#ifndef IRODS_RS_GENQUERY2_HPP
#define IRODS_RS_GENQUERY2_HPP

/// \file

#include "irods/genquery2.h"

struct RsComm;

/// TODO
///
/// \since 4.3.2
int rs_genquery2(RsComm* _comm, GenQuery2Input* _input, char** _output);

#endif // IRODS_RS_GENQUERY2_HPP
