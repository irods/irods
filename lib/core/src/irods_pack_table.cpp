#include "irods_pack_table.hpp"
#include "apiPackTable.h"

namespace irods {

    pack_entry_table& get_pack_table() {
        static pack_entry_table api_pack_table(
            api_pack_table_init );
        return api_pack_table;

    } // get_pack_table

};


