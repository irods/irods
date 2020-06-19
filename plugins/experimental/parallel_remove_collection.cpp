#include "filesystem.hpp"
#include "experimental_plugin_framework.hpp"
#include "parallel_remove_collection.hpp"

#include "json.hpp"
using json = nlohmann::json;

#include <map>

namespace irods::experimental::api {
    namespace fs   = irods::experimental::filesystem;
    namespace fscl = irods::experimental::filesystem::client;

    using entry_type    = fscl::recursive_collection_iterator::value_type;
    using dispatch_type = dispatch_processor<fscl::recursive_collection_iterator>;

    bool ends_with(const std::string &str, const std::string &suffix)
    {
        return str.size() >= suffix.size() &&
               str.compare(str.size() - suffix.size(),
                           suffix.size(), suffix) == 0;
    } // ends_with

    auto get_query(const fs::path& p)
    {
        auto tmp = p;

        if(!ends_with(tmp.string(), std::string{fs::path::preferred_separator})) {
            tmp += fs::path::preferred_separator;
        }

        tmp += "%";

        return std::string{"SELECT COUNT(DATA_ID) WHERE COLL_NAME like '"} + tmp.string() + "'";
    } // get_query

    void parallel_remove_collection(locking_json& blackboard, const parallel_remove_options& opts)
    {
        auto exit_flag = std::atomic_bool{false};

        auto cp = make_connection_pool(opts.thread_count+1);
        auto tp = thread_pool{opts.thread_count+2};

        auto comm = cp->get_connection();

        progress_handler p_hdlr{comm, get_query(opts.logical_path), blackboard, tp, exit_flag};
        cancellation_handler c_hdlr{p_hdlr, blackboard, tp, exit_flag};

        auto job = [cp, &p_hdlr, &opts] (const entry_type entry) {
                   auto comm = cp->get_connection();
                   if(entry.is_data_object()) {
                      fscl::remove(comm, entry, {opts.no_trash, false, false, false, opts.unregister});
                      p_hdlr++;
                   }
               };

        auto itr = fscl::recursive_collection_iterator(comm, opts.logical_path);
        auto dp = dispatch_type(exit_flag, itr, job);
        auto f = dp.execute(tp);

        blackboard.update(to_blackboard(f));

        // something broke?

        if(!exit_flag) {
            tp.join();
            fscl::remove_all(comm, opts.logical_path, {true, false, false, true});
        }

    } // parallel_remove_collection

} // irods::experimental::api
