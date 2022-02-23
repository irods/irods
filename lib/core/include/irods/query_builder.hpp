#ifndef IRODS_QUERY_BUILDER_HPP
#define IRODS_QUERY_BUILDER_HPP

#include "irods/irods_query.hpp"
#include "irods/rodsErrorTable.h"

#include <vector>

namespace irods::experimental
{
    enum class query_type
    {
        general,
        specific
    };

    class query_builder
    {
    public:
        auto type(query_type _v) noexcept -> query_builder&
        {
            type_ = _v;
            return *this;
        }

        auto zone_hint(const std::string& _v) -> query_builder&
        {
            zone_hint_ = _v;
            return *this;
        }

        auto row_limit(std::uintmax_t _v) noexcept -> query_builder&
        {
            limit_ = _v;
            return *this;
        }

        auto row_offset(std::uintmax_t _v) noexcept -> query_builder&
        {
            offset_ = _v;
            return *this;
        }

        auto options(int _v) noexcept -> query_builder&
        {
            options_ = _v;
            return *this;
        }

        auto bind_arguments(const std::vector<std::string>& _args) -> query_builder&
        {
            args_ = &_args;
            return *this;
        }

        auto clear_bound_arguments() noexcept -> query_builder&
        {
            args_ = nullptr;
            return *this;
        }

        auto clear() -> query_builder&
        {
            args_ = nullptr;
            zone_hint_.clear();
            limit_ = 0;
            offset_ = 0;
            type_ = query_type::general;
            options_ = 0;

            return *this;
        }

        template <typename ConnectionType>
        auto build(ConnectionType& _conn, const std::string& _query) -> irods::query<ConnectionType>
        {
            if (_query.empty()) {
                THROW(USER_INPUT_STRING_ERR, "query string is empty");
            }

            using T = typename query<ConnectionType>::query_type;

            return {&_conn,
                    _query,
                    args_,
                    zone_hint_,
                    limit_,
                    offset_,
                    type_ == query_type::general ? T::GENERAL : T::SPECIFIC,
                    options_};
        }

    private:
        const std::vector<std::string>* args_{};
        std::string zone_hint_;
        std::uintmax_t limit_ = 0;
        std::uintmax_t offset_ = 0;
        query_type type_ = query_type::general;
        int options_ = 0;
    }; // class query_builder
} // namespace irods::experimental

#endif // IRODS_QUERY_BUILDER_HPP

