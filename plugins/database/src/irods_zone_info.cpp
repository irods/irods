#include "irods_zone_info.hpp"

#include "irods/rodsLog.h"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/private/mid_level.hpp"
#include "irods/rodsDef.h"

namespace irods {

    zone_info* zone_info::the_instance_ = NULL;

    zone_info* zone_info::get_instance( void ) {
        if ( the_instance_ == NULL ) {
            the_instance_ = new zone_info();
        }

        return the_instance_;
    }

    zone_info::zone_info( void ) {
        // TODO - stub
    }

    zone_info::~zone_info() {
        // TODO - stub
    }

    error zone_info::get_local_zone(
        icatSessionStruct _icss,
        int               _logSQL,
        std::string&      _rtn_local_zone ) {
        error result = SUCCESS();
        if ( local_zone_.empty() ) {
            if ( _logSQL != 0 ) {
                rodsLog( LOG_SQL, "getLocalZone SQL 1 " );
            }
            char localZone[MAX_NAME_LEN];
            int status = cmlGetStringValueFromSql( "select zone_name from R_ZONE_MAIN where zone_type_name=?",
                                                   localZone, MAX_NAME_LEN, "local", 0, 0, &_icss );
            if ( status != 0 ) {
                chlRollback( NULL );
                result = ERROR( status, "getLocalZone failure" );
            }

            local_zone_ = localZone;
        }

        _rtn_local_zone = local_zone_;
        return result;

    }

}; // namespace irods
