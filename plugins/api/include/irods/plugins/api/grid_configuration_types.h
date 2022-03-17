#ifndef IRODS_GRID_CONFIGURATION_TYPES_H
#define IRODS_GRID_CONFIGURATION_TYPES_H

#define IRODS_GRID_CONFIGURATION_COLUMN_SIZE 2700

typedef struct GridConfigurationInput
{
    char name_space[IRODS_GRID_CONFIGURATION_COLUMN_SIZE];
    char option_name[IRODS_GRID_CONFIGURATION_COLUMN_SIZE];
} gridConfigurationInp_t;

#define GridConfigurationInp_PI "str name_space[IRODS_GRID_CONFIGURATION_COLUMN_SIZE]; str option_name[IRODS_GRID_CONFIGURATION_COLUMN_SIZE];"

typedef struct GridConfigurationOutput
{
    char option_value[IRODS_GRID_CONFIGURATION_COLUMN_SIZE];
} gridConfigurationOut_t;

#define GridConfigurationOut_PI "str option_value[IRODS_GRID_CONFIGURATION_COLUMN_SIZE];"

#endif // IRODS_GRID_CONFIGURATION_TYPES_H

