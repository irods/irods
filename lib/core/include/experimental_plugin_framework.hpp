#ifndef IRODS_EXPERIMENTAL_PLUGIN_FRAMEWORK
#define IRODS_EXPERIMENTAL_PLUGIN_FRAMEWORK

// =-=-=-=-=-=-=-
// irods includes
#include "rodsDef.h"
#include "msParam.h"
#include "rcConnect.h"
#include "irods_plugin_base.hpp"

#include "thread_pool.hpp"
#include "connection_pool.hpp"
#include "dispatch_processor.hpp"
#include "query_builder.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <sstream>
#include <string>
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <thread>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace irods::experimental::api {
    using flag_type = std::atomic_bool;

    namespace states {
        const std::string paused{"paused"};
        const std::string failed{"failed"};
        const std::string running{"running"};
        const std::string unknown{"unknown"};
        const std::string complete{"complete"};
    }; // states

    namespace commands {
        const std::string pause{"pause"};
        const std::string resume{"resume"};
        const std::string cancel{"cancel"};
        const std::string request{"request"};
        const std::string progress{"progress"};
    }; // commands

    namespace endpoints {
        const std::string command{"command"};
        const std::string operation{"operation"};
    }; // endpoints

    namespace constants {
        const std::string code{"code"};
        const std::string status{"status"};
        const std::string plugin{"plugin"};
        const std::string errors{"errors"};
        const std::string command{"command"};
        const std::string message{"message"};
        const std::string progress{"progress"};
    }; // constants

    class base;

    namespace {
        void sleep()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        auto invoke(rcComm_t& comm, const json& msg)
        {
            const auto ADAPTER_APN{120000};

            auto str = msg.dump();

            bytesBuf_t inp{};
            inp.buf = (void*)str.c_str();
            inp.len = str.size();

            bytesBuf_t* resp{};

            auto err = procApiRequest(&comm, ADAPTER_APN, (void*)&inp, NULL, (void**)&resp, NULL );

            if (err < 0) {
                THROW(err, "failed to perform the invocation");
            }

            const auto* buf = static_cast<char*>(resp->buf);

            return json::parse(buf, buf + resp->len);
        } // invoke

        template<typename T>
        auto get(const std::string& n, const json& p)
        {
            if(!p.contains(n)) {
                THROW(
                    SYS_INVALID_INPUT_PARAM,
                    boost::format("missing [%s] parameter")
                    % n);
            }

            return p.at(n).get<T>();
        } // get

        template<typename T>
        auto get(const std::string& n, const json& p, T d)
        {
            if(!p.contains(n)) {
                return d;
            }

            return p.at(n).get<T>();
        } // get

        auto to_blackboard(const irods::future& f) -> json
        {
            auto bb = json{{constants::status, states::complete}};

            try {
                auto xx = f.get();

                if(xx.size() > 0) {
                    auto arr = json::array();
                    for(auto&& e : xx) {
                        arr.push_back({{constants::code,    std::get<0>(e)},
                                       {constants::message, std::get<1>(e)}});
                    }

                    bb[constants::errors] = arr;
                    bb.update({{constants::status, states::failed}});
                }
            }
            catch(...) {
                return json{{constants::errors, {
                            {constants::code, SYS_INTERNAL_ERR},
                            {constants::message, "unknown error in to_blackboard"}}}};
            }

            return bb;

        } // to_blackboard

#ifdef RODS_SERVER
        static auto resolve_api_plugin(const std::string& operation, const std::string& type)
        {
            std::string lower{operation};
            std::transform(lower.begin(),
                           lower.end(),
                           lower.begin(),
                           ::tolower );

            base* plugin{};
            auto err = irods::load_plugin<base>(
                            plugin,
                            operation,
                            "experimental",
                            "irods::experimental::api::base",
                            "empty_context" );
            if(!err.ok()) {
                THROW(err.code(), err.result());
            }

            return plugin;

        } // resolve_api_plugin
#endif

    } // namespace

    class locking_json
    {
    public:
        locking_json(const json& j) : c_{j} {}

        void set(const json& j)
        {
            std::scoped_lock l(m_);
            c_ = j;
        }

        json get()
        {
            std::scoped_lock l(m_);
            return c_;
        }

        json update(const json& j)
        {
            std::scoped_lock l(m_);
            c_.update(j);
            return c_;
        }

    private:
        json       c_;
        std::mutex m_;

    }; // locking_json

    using progress_handler_type = std::function<void(const std::string&)>;

    class client
    {
    public:
        json operator()(
            rcComm_t&             conn
          , flag_type&            exit_flag
          , progress_handler_type progress_handler
          , const json&           options
          , const std::string&    endpoint)
        {
            json req{}, rep{};

            try {
                req = {{commands::request, endpoints::operation},
                       {constants::plugin, endpoint}};

                req.update(options);

                rep = invoke(conn, req);

                std::string status{}, progress{}, command{};

                while(states::complete != status &&
                      states::failed   != status) {

                    sleep();

                    command = exit_flag ? commands::cancel
                                        : commands::progress;

                    req = {{commands::request,  endpoints::command},
                           {constants::command, command},
                           {constants::plugin,  endpoint}};

                    rep = invoke(conn, req);

                    status   = get<std::string>(constants::status,   rep, "");
                    progress = get<std::string>(constants::progress, rep, "");

                    progress_handler(progress);

                } // while
            }
            catch (const irods::exception& e) {
                return json{{constants::errors, {
                            {constants::code, e.code()},
                            {constants::message, e.what()}}}};
            }

            return rep;

        } // operator()

    }; // class client

    class progress_handler
    {
    public:
        progress_handler(
            rcComm_t&          comm
          , uint64_t           total
          , locking_json&      b_board
          , thread_pool&       t_pool
          , flag_type&         e_flag) :
            exit_flag_{e_flag}
          , count_{}
          , total_{total}
          , blackboard_{b_board}
        {
            try {
                // launch the tracking thread
                thread_pool::post(t_pool, [&]() {
                    while(!exit_flag_ && !complete()) {
                        sleep();
                        auto t = static_cast<uint64_t>(100*(double)count_/(double)total_);
                        blackboard_.update({{constants::progress, std::to_string(t)}});
                    }
                });
            }
            catch(const irods::exception& e) {
                rodsLog(LOG_ERROR,
                        "exception caught in progress_handler %d:%s",
                        e.code(), e.what());
            }
        } // ctor

        void operator++(int) { count_++; }

        bool complete() const { return count_ >= total_; }

    private:
        flag_type&           exit_flag_;
        std::atomic_uint64_t count_;
        std::atomic_uint64_t total_;
        locking_json&        blackboard_;
    }; // class progress_handler

    class cancellation_handler
    {
    public:
        cancellation_handler(
            const progress_handler& p_hdlr
          , locking_json&           b_board
          , thread_pool&            t_pool
          , flag_type&              e_flag
          ) :
            exit_flag_{e_flag}
          , blackboard_{b_board}
          , p_handler_{p_hdlr}
        {
            thread_pool::post(t_pool, [&t_pool, this]() {
                while(!exit_flag_ && !p_handler_.complete()) {
                    sleep();

                    json bb = blackboard_.get();

                    if(bb.contains(constants::command) &&
                        bb.at(constants::command) == commands::cancel) {
                        exit_flag_ = true;
                        t_pool.stop();
                        blackboard_.update({{constants::status, states::complete}});
                        break;
                    }
                } // while
            });

        } // ctor

    private:
        flag_type&              exit_flag_;
        locking_json&           blackboard_;
        const progress_handler& p_handler_;
    }; // class cancellation_handler

    class base : public irods::plugin_base
    {
    protected:
        thread_pool async_pool{1};

        #define WRAPPER(C, F) \
        std::function<json(C*, const json&)>([&](C* c, const json& j) -> json { \
                return F(c, j);})

        template<typename COMM_T>
        json sync(COMM_T* comm, const std::string& n, const json& req)
        {
            if(operations_.find(n) == operations_.end()) {
                THROW(SYS_INVALID_INPUT_PARAM,
                      boost::format("call operation :: missing operation[%s]") % n);
            }

            using fcn_t = std::function<json(COMM_T*, const json&)>;

            auto op = boost::any_cast<fcn_t&>(operations_[n]);

            return op(comm, req);

        } // sync

        template<typename COMM_T>
        json async(COMM_T* comm, const std::string& n, const json& req)
        {
            if(operations_.find(n) == operations_.end()) {
                THROW(SYS_INVALID_INPUT_PARAM,
                      boost::format("call operation :: missing operation[%s]") % n);
            }

            using fcn_t = std::function<json(COMM_T*, const json&)>;

            auto op = boost::any_cast<fcn_t&>(operations_[n]);

            std::function<void(void)> f = [=]() -> void {
                op(comm, req);
            };

            thread_pool::post(async_pool, f);

            return {{constants::status, states::running}};

        } // call

        locking_json blackboard{{constants::status, states::unknown}};

    public:

        base(const std::string& n) : plugin_base(n, "empty_context_string")
        {
            operations_[endpoints::operation] = WRAPPER(rsComm_t, operation);
            operations_[endpoints::command]   = WRAPPER(rsComm_t, command);
        } // ctor

        virtual ~base() {
            async_pool.join();
        }

        template<typename COMM_T>
        auto call(COMM_T* comm, const std::string& n, const json& req)
        {
            auto op = get<std::string>(commands::request, req);
            auto is_op = endpoints::operation == op;
            auto is_as = enable_asynchronous_operation();

            if(is_op && is_as) {
                return async(comm, n, req);
            }
            else {
                return sync(comm, n, req);
            }

        } // call

        virtual bool enable_asynchronous_operation() { return false; }

        virtual json operation(rsComm_t* comm, const json  req) = 0;

        virtual json   command(rsComm_t* comm, const json& req)
        {
            // using update() for bidirectional communication
            return blackboard.update(req);
        };

    }; // class base

} // namespace irods::experimental::api

#endif // IRODS_EXPERIMENTAL_PLUGIN_FRAMEWORK
