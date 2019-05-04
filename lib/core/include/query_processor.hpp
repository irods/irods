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
        // clang-format on

        query_processor(const std::string& _query, job _job)
            : query_{_query}
            , job_{_job}
        {
        }

        query_processor(const query_processor&) = delete;
        query_processor& operator=(const query_processor&) = delete;

        std::future<errors> execute(thread_pool& _thread_pool, ConnectionType& _conn)
        {
            std::promise<errors> p;

            auto future = p.get_future();

            thread_pool::defer(_thread_pool, [this, p = std::move(p), conn = std::move(_conn)]() mutable {
                errors errs;

                try {
                    for (auto&& row : query<ConnectionType>{&conn, query_}) {
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

                p.set_value(std::move(errs));
            });

            return future;
        }

    private:
        std::string query_;
        job job_;
    };
} // namespace irods

#endif // IRODS_QUERY_PROCESSOR_HPP

