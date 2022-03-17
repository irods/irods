#ifndef IRODS_RS_GET_GRID_CONFIGURATION_VALUE_HPP
#define IRODS_RS_GET_GRID_CONFIGURATION_VALUE_HPP

/// \file

#include "irods/plugins/api/grid_configuration_types.h"

struct RsComm;

#ifdef __cplusplus
extern "C" {
#endif

/// Given a (namespace, option_name) tuple, returns the option_value from the R_GRID_CONFIGURATION table.
///
/// \param[in]  _comm   A pointer to a RsComm.
/// \param[in]  _input  An object holding criteria used to locate a table entry.
/// \param[out] _output An object holding the option value.
///
/// \return An integer.
/// \retval 0        On success.
/// \retval Non-zero On failure.
///
/// \since 4.3.0
int rs_get_grid_configuration_value(RsComm* _comm,
                                    const GridConfigurationInput* _input,
                                    GridConfigurationOutput** _output);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_RS_GET_GRID_CONFIGURATION_VALUE_HPP

