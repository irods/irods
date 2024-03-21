#ifndef IRODS_GENQUERY2_SQL_HPP
#define IRODS_GENQUERY2_SQL_HPP

#include <string>
#include <string_view>
#include <vector>

namespace irods::experimental::api::genquery2
{
    struct select;

    struct options
    {
        std::string_view user_name;
        std::string_view user_zone;
        std::string_view database;
        std::uint16_t default_number_of_rows = 16;
        bool admin_mode = false;
    }; // struct options

    auto to_sql(const select& _select, const options& _opts) -> std::tuple<std::string, std::vector<std::string>>;
} // namespace irods::experimental::api::genquery2

#endif // IRODS_GENQUERY2_SQL_HPP
