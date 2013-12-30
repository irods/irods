/*
 * catalog_properties.c
 *
 *  Created on: Oct 9, 2013
 *      Author: adt
 */

// =-=-=-=-=-=-=-
#include "irods_error.hpp"
#include "irods_catalog_properties.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "icatHighLevelRoutines.hpp"
#include "icatMidLevelRoutines.hpp"

namespace irods {

// Access singleton through its getInstance() method
    catalog_properties& catalog_properties::getInstance() {
        static catalog_properties instance;
        return instance;
    }


// Query iCAT settings and fill catalog_properties::properties
    error catalog_properties::capture( icatSessionStruct* _icss ) {
        rodsLong_t row_count = 0; 	// total number of rows to get
        int col_nbr = 2;	// 2 columns for now: pg_settings.name, pg_settings.setting
        char *sql_out = NULL;	// sql result string
        char *row_ptr = NULL;	// for parsing result string
        std::string prop_name, prop_setting; // property name and setting

        int i, status = 0;
        irods::error result = SUCCESS();

#if ORA_ICAT
        return ERROR( SYS_NOT_IMPLEMENTED, "Capturing catalog properties is not available for Oracle" );
#elif MY_ICAT
        return ERROR( SYS_NOT_IMPLEMENTED, "Capturing catalog properties is not available for MySQL" );
#endif


        // First query to get number of rows
        status = cmlGetIntegerValueFromSqlV3( "select count(*) from pg_settings", &row_count, _icss );

        if ( status < 0 ) {
            return ERROR( status, "Unable to get row count from pg_settings" );
        }

        // Allocate memory for result string
        sql_out = ( char* )malloc( MAX_NAME_LEN * row_count * col_nbr );
        if ( !sql_out ) {
            return ERROR( SYS_MALLOC_ERR, "(x_x)" );
        }

        // Main query to get settings
        status = cmlGetMultiRowStringValuesFromSql( "select name, setting from pg_settings",
                 sql_out, MAX_NAME_LEN, row_count * col_nbr, NULL, NULL, NULL, _icss );

        if ( status < 0 ) {
            free( sql_out );
            return ERROR( status, "Unable to get values from pg_settings" );
        }

        // Parse results
        for ( i = 0; i < row_count; i++ ) {
            // Set row pointer at beginning of row
            row_ptr = sql_out + i * col_nbr * MAX_NAME_LEN;

            // Make sure name and setting are null-terminated
            row_ptr[MAX_NAME_LEN - 1] = '\0';
            row_ptr[2 * MAX_NAME_LEN - 1] = '\0';

            // Get name
            prop_name.assign( row_ptr );

            // Get setting
            prop_setting.assign( row_ptr + MAX_NAME_LEN );

            // Update properties table
            result = properties.set<std::string>( prop_name, prop_setting );

            if ( !result.ok() ) {
                break;
            }

        }

        // cleanup
        free( sql_out );

        return result;

    } // catalog_properties::capture()


} // namespace irods



