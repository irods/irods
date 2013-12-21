


#ifndef __EIRODS_DATABASE_CONSTANTS_H__
#define __EIRODS_DATABASE_CONSTANTS_H__

// =-=-=-=-=-=-=-
// stl includes
#include <string>

namespace irods {
    /// =-=-=-=-=-=-=-
    /// @brief constants for network object indexing
    const std::string DATABASE_OP_START( "database_start" );
    const std::string DATABASE_OP_STOP( "database_stop" );

    const std::string DATABASE_OP_DEBUG( "database_debug" );
    const std::string DATABASE_OP_OPEN( "database_open" );
    const std::string DATABASE_OP_CLOSE( "database_close" );
    const std::string DATABASE_OP_GET_LOCAL_ZONE( "database_get_local_zone" );
    const std::string DATABASE_OP_UPDATE_RESC_OBJ_COUNT( "database_update_resc_obj_count" );
    const std::string DATABASE_OP_MOD_DATA_OBJ_META( "database_mod_data_obj_meta" );
    const std::string DATABASE_OP_REG_DATA_OBJ( "database_reg_data_obj" );
    const std::string DATABASE_OP_REG_REPLICA( "database_reg_replica" );
    const std::string DATABASE_OP_UNREG_REPLICA( "database_unreg_replica" );
    const std::string DATABASE_OP_REG_RULE_EXEC( "database_reg_rule_exec" );
    const std::string DATABASE_OP_MOD_RULE_EXEC( "database_mod_rule_exec" );
    const std::string DATABASE_OP_DEL_RULE_EXEC( "database_del_rule_exec" );
    const std::string DATABASE_OP_RESC_OBJ_COUNT( "pg_resc_obj_count_op" );

}; // namespace irods

#endif // __EIRODS_DATABASE_CONSTANTS_H__



