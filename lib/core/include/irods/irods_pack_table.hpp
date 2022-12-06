#ifndef IRODS_PACK_TABLE_HPP
#define IRODS_PACK_TABLE_HPP

#include "irods/irods_lookup_table.hpp"
#include "irods/packStruct.h"

namespace irods
{
    class pack_entry
    {
    public:
        std::string packInstruct;
    }; // class pack_entry

    class pack_entry_table : public lookup_table<pack_entry>
    {
    public:
        // NOLINTNEXTLINE(modernize-avoid-c-arrays, cppcoreguidelines-avoid-c-arrays)
        explicit pack_entry_table(packInstruct_t _defs[]);
    }; // class pack_entry_table

    // NOLINTNEXTLINE(modernize-use-trailing-return-type)
    irods::pack_entry_table& get_pack_table();

    void clearInStruct_noop(void*);

    void clearOutStruct_noop(void*);
} // namespace irods

#endif // IRODS_PACK_TABLE_HPP
