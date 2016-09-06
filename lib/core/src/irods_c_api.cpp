#include "irods_client_api_table.hpp"

extern "C" {
    void init_client_api_table(void) {
        irods::api_entry_table&  api_tbl = irods::get_client_api_table();
        irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
        init_api_table(api_tbl, pk_tbl);
    }
}
