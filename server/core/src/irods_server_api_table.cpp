#define CREATE_API_TABLE_FOR_SERVER
#include "apiTable.hpp"
#undef CREATE_API_TABLE_FOR_SERVER
#include "irods_server_api_table.hpp"

namespace irods {
    api_entry_table& get_server_api_table() {
        static int server_num_api_defs = sizeof( server_api_table_inp ) / sizeof( irods::apidef_t );
        static api_entry_table server_api_table(
            server_api_table_inp,
            server_num_api_defs );

        return server_api_table;

    } // server_api_table

}; // namespace irods
