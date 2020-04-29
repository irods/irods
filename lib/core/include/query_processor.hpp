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
        using error        = std::tuple<int, std::string>;
        using errors       = std::vector<error>;
        using result_row   = typename query<ConnectionType>::value_type;
        using job          = std::function<void (const result_row&)>;
        using query_type   = typename query<ConnectionType>::query_type;
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
        uint32_t size() { return size_; }

        class future
        {
            friend query_processor;

            std::vector<std::shared_ptr<std::promise<error>>> promises;

            void push_back(std::shared_ptr<std::promise<error>> p) {
                promises.push_back(p);
            }

            public:
            auto get() {
                errors errs{};
                errs.reserve(promises.size());
                for(auto&& p : promises) {
                    auto&& e = p->get_future().get();
                    if(std::get<0>(e) < 0) {
                        errs.push_back(std::move(e));
                    }
                }

                return errs;
            }
        };

        auto execute(thread_pool& _thread_pool, ConnectionType& _conn)
        {
            future f;
            query<ConnectionType> q{&_conn, query_, limit_, 0, type_};

            size_ = q.size();

            for (auto&& r : q) {
               auto p = std::make_shared<std::promise<error>>();
               f.push_back(p);

               thread_pool::post(_thread_pool, [this, p, r]() mutable {
                    try {
                        job_(r);
                        p->set_value({0, ""});
                    }
                    catch (const irods::exception& e) {
                        p->set_value({e.code(), e.what()});
                    }
                    catch (const std::exception& e) {
                        p->set_value({SYS_UNKNOWN_ERROR, e.what()});
                    }
                    catch (...) {
                        p->set_value({SYS_UNKNOWN_ERROR, "Unknown error occurred while processing job."});
                    }
                });

            } // for row

            return f;
        }

    private:
        uint32_t size_;
        std::string query_;
        job job_;
        uint32_t limit_;
        query_type type_;
    };
} // namespace irods

#endif // IRODS_QUERY_PROCESSOR_HPP

