#ifndef __IRODS_PACK_TABLE_HPP__
#define __IRODS_PACK_TABLE_HPP__

#include "irods_lookup_table.hpp"
#include "packStruct.h"

namespace irods {
    class pack_entry {
    public:
        std::string packInstruct;
    };

    class pack_entry_table : public lookup_table< pack_entry > {
    public:
        pack_entry_table( packInstruct_t[] );
    };

    irods::pack_entry_table& get_pack_table();
    void clearInStruct_noop( void* );
}

#endif
