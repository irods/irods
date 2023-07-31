#include "irods/irods_pack_table.hpp"

#include "irods/api_pack_table.hpp"

#include <cstring>

namespace irods
{
    // NOLINTNEXTLINE(modernize-avoid-c-arrays, cppcoreguidelines-avoid-c-arrays)
    pack_entry_table::pack_entry_table(const packInstruct_t _defs[])
    {
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        for (int i = 0; std::strcmp(_defs[i].name, PACK_TABLE_END_PI) != 0; ++i) {
            table_[_defs[i].name].packInstruct = _defs[i].packInstruct;
        }
        // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    } // constructor

    pack_entry_table& get_pack_table() // NOLINT(modernize-use-trailing-return-type)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        static pack_entry_table api_pack_table(api_pack_table_init);
        return api_pack_table;
    } // get_pack_table

    void clearInStruct_noop([[maybe_unused]] void* _p)
    {
    } // clearInStruct_noop

    void clearOutStruct_noop([[maybe_unused]] void* _p)
    {
    } // clearOutStruct_noop
} // namespace irods
