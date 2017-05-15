#include "irods_pack_table.hpp"
#include "apiPackTable.h"

namespace irods {
    pack_entry_table::pack_entry_table(
        packInstruct_t _defs[] ) {
        int i = 0;
        std::string end_str( PACK_TABLE_END_PI );
        while ( end_str != _defs[ i ].name ) {
            table_[ _defs[ i ].name ].packInstruct = _defs[i].packInstruct;
            ++i;
        }
    }

    pack_entry_table& get_pack_table() {
        static pack_entry_table api_pack_table(api_pack_table_init);
        return api_pack_table;
    }

    void clearInStruct_noop( void* ) {};
}
