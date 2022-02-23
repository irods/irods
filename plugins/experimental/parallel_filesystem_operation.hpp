#ifndef PARALLEL_FILESYSTEM_OPERATION_HPP
#define PARALLEL_FILESYSTEM_OPERATION_HPP

#include "irods/experimental_plugin_framework.hpp"
#include "irods/irods_get_full_path_for_config_file.hpp"
#include "irods/irods_logger.hpp"
#include "irods/filesystem.hpp"
#include "irods/thread_pool.hpp"
#include "irods/connection_pool.hpp"

#include "fmt/format.h"

namespace irods::experimental::api {

    namespace fs   = irods::experimental::filesystem;
    namespace fscl = irods::experimental::filesystem::client;

    class parallel_filesystem_operation : public base {
        public:
            parallel_filesystem_operation(const std::string& n) : base(n) {}
            virtual ~parallel_filesystem_operation() {}

        protected:
            using entry_type    = fscl::recursive_collection_iterator::value_type;
            using dispatch_type = dispatch_processor<fscl::recursive_collection_iterator>;

            const int DEFAULT_NUMBER_OF_THREADS = 4;

            virtual void collection_precondition(rcComm_t&, const json&) {};
            virtual void process_object(rcComm_t&, const fs::path&, const json&) = 0;
            virtual void collection_postcondition(rcComm_t&, const json&) {}

            auto get_thread_count()
            {
                std::string path{};
                if (auto e = irods::get_full_path_for_config_file("server_config.json", path); e.ok()) {
                    json cfg{};

                    { std::ifstream f{path}; f >> cfg; }

                    auto psc = cfg.at(irods::CFG_PLUGIN_CONFIGURATION_KW);

                    if(psc.contains("api") && psc.at("api").contains(instance_name_)) {
                        auto in = psc.at("api").at(instance_name_);
                        if(in.contains("thread_count")) {
                            return in.at("thread_count").get<int>();
                        }
                    }
                }

                return DEFAULT_NUMBER_OF_THREADS;
            }

            auto ends_with(const std::string &str, const std::string &suffix)
            {
                return str.size() >= suffix.size() &&
                       str.compare(str.size() - suffix.size(),
                                   suffix.size(), suffix) == 0;
            } // ends_with

            auto get_object_count(rcComm_t& comm, const fs::path& path)
            {
                auto p_str = path.string();

                if(ends_with(path, std::string{fs::path::preferred_separator})) {
                    p_str.erase(p_str.size()-1);
                }

                auto q_str = fmt::format("SELECT COUNT(DATA_ID) WHERE COLL_NAME like '{}/%' ||= '{}'", p_str, p_str);

                auto query = query_builder{}.build<rcComm_t>(comm, q_str);

                auto count = std::strtoull(query.front()[0].c_str(), nullptr, 10);

                return count;

            } // get_object_count

            void process_collection(locking_json& bb, const json& req)
            {
                try {
                    json tmp = req;

                    tmp["recursive"] = true; // indicates this is a collection based operation

                    auto ef = std::atomic_bool{false};

                    auto tc = tmp.at("thread_count").get<int>();
                    auto cp = make_connection_pool(tc+1);
                    auto tp = thread_pool{tc + 2};

                    auto lp = tmp.at("logical_path").get<std::string>();

                    auto conn = cp->get_connection();

                    progress_handler p_hdlr{conn, get_object_count(conn, lp), bb, tp, ef};

                    cancellation_handler c_hdlr{p_hdlr, bb, tp, ef};

                    collection_precondition(conn, tmp);

                    auto job = [this, cp, &p_hdlr, &tmp] (const entry_type entry) {
                                   if(entry.is_data_object()) {
                                       auto conn = cp->get_connection();
                                       process_object(conn, entry.path(), tmp);
                                       p_hdlr++;
                                   }
                               };

                    auto itr = fscl::recursive_collection_iterator(conn, lp);
                    auto dp = dispatch_type(ef, itr, job);
                    auto f = dp.execute(tp);

                    tp.join();

                    bb.update(to_blackboard(f));

                    if(!ef) {
                        collection_postcondition(conn, tmp);
                    }
                }
                catch(const irods::exception& e) {
                    blackboard.set({{constants::status,  states::failed},
                                    {constants::errors, {
                                    {constants::code,    e.code()},
                                    {constants::message, e.client_display_what()}}}});
                }
                catch(const json::exception& e) {
                    blackboard.set({{constants::status,  states::failed},
                                    {constants::errors, {
                                    {constants::code,    SYS_INTERNAL_ERR},
                                    {constants::message, e.what()}}}});
                }
                catch(const std::exception& e) {
                    blackboard.set({{constants::status,  states::failed},
                                    {constants::errors, {
                                    {constants::code,    SYS_INTERNAL_ERR},
                                    {constants::message, e.what()}}}});
                }

            } // process_collection

            bool enable_asynchronous_operation() { return true; }

            auto operation(rsComm_t* comm, const json req) -> json
            {
                try {
                    blackboard.set({{constants::status, states::running}});

                    json tmp = req;

                    if(!tmp.contains("thread_count")) {
                        tmp["thread_count"] = get_thread_count();
                    }

                    auto lp = get<std::string>("logical_path", req, {});

                    if(lp.empty()) {
                        THROW(SYS_INVALID_INPUT_PARAM,
                              "filesystem operations require a logical_path");
                    }

                    if(ends_with(lp, std::string{fs::path::preferred_separator})) {
                        lp.erase(lp.size()-1);
                    }

                    auto cp   = make_connection_pool(1);
                    auto comm = cp->get_connection();

                    if(!fscl::exists(comm, lp)) {
                        THROW(SYS_INVALID_INPUT_PARAM,
                              fmt::format("logical_path [{}] does not exist", lp));
                    }

                    if(fscl::is_collection(comm, lp)) {
                        process_collection(blackboard, tmp);
                    }
                    else {
                        process_object(comm, lp, tmp);
                    }

                    blackboard.update({{constants::status, states::complete}});
                }
                catch(const irods::exception& e) {
                    log::api::error(e.what());
                    blackboard.set({{constants::status,  states::failed},
                                    {constants::errors, {
                                    {constants::code,    e.code()},
                                    {constants::message, e.what()}}}});
                }
                catch(const json::exception& e) {
                    log::api::error(e.what());
                    blackboard.set({{constants::status,  states::failed},
                                    {constants::errors, {
                                    {constants::code,    SYS_INTERNAL_ERR},
                                    {constants::message, e.what()}}}});
                }
                catch(const std::exception& e) {
                    log::api::error(e.what());
                    blackboard.set({{constants::status,  states::failed},
                                    {constants::errors, {
                                    {constants::code,    SYS_INTERNAL_ERR},
                                    {constants::message, e.what()}}}});
                }
                catch(...) {
                    blackboard.set({{constants::status,  states::failed},
                                    {constants::errors, {
                                    {constants::code,    SYS_INTERNAL_ERR},
                                    {constants::message, "caught unknown exception"}}}});

                }

                return blackboard.get();

            } // operation

    }; // parallel_filesystem_operation

} // namespace irods::experimental::api

#endif // PARALLEL_FILESYSTEM_OPERATION_HPP
