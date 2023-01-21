#ifndef IRODS_API_PLUGIN_NUMBER_H
#define IRODS_API_PLUGIN_NUMBER_H

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define API_PLUGIN_NUMBER(NAME, VALUE) static const int NAME = VALUE;

#include "irods/plugins/api/api_plugin_number_data.h"

#undef API_PLUGIN_NUMBER

#endif // IRODS_API_PLUGIN_NUMBER_H
