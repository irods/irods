#ifndef IRODS_GRID_CONFIGURATION_TYPES_H
#define IRODS_GRID_CONFIGURATION_TYPES_H

typedef struct GridConfigurationInput
{
    char name_space[2700];
    char option_name[2700];
    char option_value[2700];
} gridConfigurationInp_t;

#define GridConfigurationInp_PI "str name_space[2700]; str option_name[2700]; str option_value[2700];"

typedef struct GridConfigurationOutput
{
    char option_value[2700];
} gridConfigurationOut_t;

#define GridConfigurationOut_PI "str option_value[2700];"

#endif // IRODS_GRID_CONFIGURATION_TYPES_H

