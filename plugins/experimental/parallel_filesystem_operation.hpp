#ifndef PARALLEL_FILESYSTEM_OPERATION_HPP
#define PARALLEL_FILESYSTEM_OPERATION_HPP

#include "experimental_plugin_framework.hpp"
#include "irods_get_full_path_for_config_file.hpp"
#include "irods_logger.hpp"
#include "filesystem.hpp"
#include "thread_pool.hpp"
#include "connection_pool.hpp"

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

        auto thread_count(const json& req)
        {
            if(req.contains("thread_count")) {
                return req.at("thread_count").get<int>();
            }

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

        auto query(const fs::path& p)
        {
            std::string prefix{"SELECT COUNT(DATA_ID) WHERE COLL_NAME = '"};

            auto tmp = p.string();

            if(ends_with(tmp, std::string{fs::path::preferred_separator})) {
                tmp.erase(tmp.size()-1);
            }

            return prefix + tmp + "'";

        } // query

        void process_collection(locking_json& bb, const json& req)
        {
            auto ef = std::atomic_bool{false};

            auto tc = req.at("thread_count").get<int>();
            auto cp = make_connection_pool(tc+1);
            auto tp = thread_pool{tc+2};

            auto lp = req.at("logical_path").get<std::string>();

            auto conn = cp->get_connection();

            progress_handler p_hdlr{conn, query(lp), bb, tp, ef};

            cancellation_handler c_hdlr{p_hdlr, bb, tp, ef};

            auto job = [this, cp, &p_hdlr, &req] (const entry_type entry) {
                           if(entry.is_data_object()) {
                               auto conn = cp->get_connection();
                               process_object(conn, entry.path(), req);
                               p_hdlr++;
                           }
                       };

            auto itr = fscl::recursive_collection_iterator(conn, lp);
            auto dp = dispatch_type(ef, itr, job);
            auto f = dp.execute(tp);

            bb.update(to_blackboard(f));

            if(!ef) {
                tp.join();
                collection_predicate(conn, req);
            }

        } // process_collection

        virtual void process_object(rcComm_t&, const fs::path&, const json&) = 0;
        virtual void collection_predicate(rcComm_t&, const json&) {};

        bool enable_asynchronous_operation() { return true; }

        auto operation(rsComm_t* comm, const json req) -> json
        {
            try {
                blackboard.set({{constants::status, states::running}});

                auto tmp = req;

                if(!tmp.contains("thread_count")) {
                    tmp["thread_count"] = thread_count(tmp);
                }

                if(!tmp.contains("logical_path")) {
                    THROW(SYS_INVALID_INPUT_PARAM,
                          "filesystem operations require a logical_path");
                }

                auto lp = tmp.at("logical_path").get<std::string>();

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

            return blackboard.get();

        } // operation

    }; // parallel_filesystem_operation

} // namespace irods::experimental::api

#endif // PARALLEL_FILESYSTEM_OPERATION_HPP
