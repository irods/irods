#include "filesystem.hpp"
#include "experimental_plugin_framework.hpp"
#include "parallel_copy_collection.hpp"
#include "irods_logger.hpp"

#include "json.hpp"
using json = nlohmann::json;

#include "fmt/format.h"

#include <map>

namespace irods::experimental::api {
    namespace fs   = irods::experimental::filesystem;
    namespace fscl = irods::experimental::filesystem::client;

    using entry_type    = fscl::recursive_collection_iterator::value_type;
    using dispatch_type = dispatch_processor<fscl::recursive_collection_iterator>;

    namespace {
        bool ends_with(const std::string &str, const std::string &suffix)
        {
            return str.size() >= suffix.size() &&
                   str.compare(str.size() - suffix.size(),
                               suffix.size(), suffix) == 0;
        } // ends_with

        bool replace(std::string& str, const std::string& from, const std::string& to) {
            size_t start_pos = str.find(from);
            if(start_pos == std::string::npos)
                return false;
            str.replace(start_pos, from.length(), to);
            return true;
        }
    }

    auto get_query(const fs::path& p)
    {
        auto tmp = p;

        if(!ends_with(tmp.string(), std::string{fs::path::preferred_separator})) {
            tmp += fs::path::preferred_separator;
        }

        tmp += "%";

        return std::string{"SELECT COUNT(DATA_ID) WHERE COLL_NAME like '"} + tmp.string() + "'";
    } // get_query

    void parallel_copy_collection(locking_json& blackboard, const parallel_copy_options& opts)
    {
        auto exit_flag = std::atomic_bool{false};

        auto cp = make_connection_pool(opts.thread_count+1);
        auto tp = thread_pool{opts.thread_count+2};

        auto comm = cp->get_connection();

        progress_handler p_hdlr{comm, get_query(opts.source_logical_path), blackboard, tp, exit_flag};
        cancellation_handler c_hdlr{p_hdlr, blackboard, tp, exit_flag};

        auto job = [cp, &p_hdlr, &opts] (const entry_type entry) {
                   std::string path{entry.path()};

                   auto& slp = opts.source_logical_path;
                   auto& dlp = opts.destination_logical_path;

                   if(entry.is_data_object() && replace(path, slp, dlp)) {
                       auto comm   = cp->get_connection();
                       auto parent = fs::path{path}.parent_path();

                       try {
                           fscl::create_collections(comm, parent);
                       }
                       catch(...) {
                           log::server::error(fmt::format("copy - failed to create parent collection [{}]", parent.string()));
                           return;
                       }

                       fscl::copy(comm, entry.path(), path);
                       p_hdlr++;
                   }
               };

        auto itr = fscl::recursive_collection_iterator(comm, opts.source_logical_path);
        auto dp = dispatch_type(exit_flag, itr, job);
        auto f = dp.execute(tp);

        blackboard.update(to_blackboard(f));

    } // parallel_copy_collection

} // irods::experimental::api
