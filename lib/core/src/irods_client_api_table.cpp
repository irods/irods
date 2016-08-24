#define CREATE_API_TABLE_FOR_CLIENT
#include "apiTable.hpp"
#undef CREATE_API_TABLE_FOR_CLIENT
#include "irods_client_api_table.hpp"

namespace irods {
    api_entry_table& get_client_api_table() {
        static int client_num_api_defs = sizeof( client_api_table_inp ) / sizeof( irods::apidef_t );
        static api_entry_table client_api_table(
            client_api_table_inp,
            client_num_api_defs );

        return client_api_table;

    } // client_api_table

}; // namespace irods
