#ifndef IRODS_QUERY_PROCESSOR_HPP
#define IRODS_QUERY_PROCESSOR_HPP

#include "thread_pool.hpp"
#include "irods_query.hpp"
#include "irods_exception.hpp"

#include <string>
#include <functional>
#include <future>
#include <vector>
#include <tuple>
#include <memory>
#include <exception>

namespace irods
{
    template <typename ConnectionType>
    class query_processor
    {
    public:
        // clang-format off
        using error      = std::tuple<int, std::string>;
        using errors     = std::vector<error>;
        using result_row = typename query<ConnectionType>::value_type;
        using job        = std::function<void (const result_row&)>;
        using query_type = typename query<ConnectionType>::query_type;
        // clang-format on

        query_processor(const std::string& _query, job _job, uint32_t _limit = 0, query_type _type = query_type::GENERAL)
            : query_{_query}
            , job_{_job}
            , limit_{_limit}
            , type_{_type}
        {
        }

        query_processor(const query_processor&) = delete;
        query_processor& operator=(const query_processor&) = delete;

        std::future<errors> execute(thread_pool& _thread_pool, ConnectionType& _conn)
        {
            auto p = std::make_shared<std::promise<errors>>();
            auto future = p->get_future();

            thread_pool::post(_thread_pool, [this, p, conn = std::move(_conn)]() mutable {
                errors errs;

                try {
                    for (auto&& row : query<ConnectionType>{&conn, query_, limit_, type_}) {
                        try {
                            job_(row);
                        }
                        catch (const irods::exception& e) {
                            errs.emplace_back(e.code(), e.what());
                        }
                        catch (const std::exception& e) {
                            errs.emplace_back(SYS_UNKNOWN_ERROR, e.what());
                        }
                    }
                }
                catch (...) {
                    errs.emplace_back(SYS_UNKNOWN_ERROR, "Unknown error occurred while processing job.");
                }

                p->set_value(std::move(errs));
            });

            return future;
        }

    private:
        std::string query_;
        job job_;
        uint32_t limit_;
        query_type type_;
    };
} // namespace irods

#endif // IRODS_QUERY_PROCESSOR_HPP

