
#include "experimental_plugin_framework.hpp"
#include "parallel_remove_collection.hpp"

#include "thread_pool.hpp"
#include "connection_pool.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"

namespace irods::experimental::api {
    namespace fs   = irods::experimental::filesystem;
    namespace fsvr = irods::experimental::filesystem::server;

    class recursive_remove : public base {

        bool enable_asynchronous_operation() { return true; }

        auto operation(rsComm_t* comm, const json req) -> json
        {
            try {
                blackboard.set({{constants::status, states::running}});

                fs::path path = get<std::string>("logical_path", req);

                auto unregister = get<bool>("unregister", req, false);

                auto no_trash = get<bool>("no_trash", req, false);

                auto thread_count = get<int> ("thread_count", req, 4);

                if(fsvr::is_collection(*comm, path)) {
                    parallel_remove_collection(blackboard, {path, unregister, no_trash, thread_count});
                }
                else {
                    bool verbose{false}, progress{false}, recursive{false};
                    fsvr::remove(*comm, path, {no_trash, verbose, progress, recursive, unregister});
                }

                blackboard.update({{constants::status, states::complete}});
            }
            catch(const irods::exception& e) {
                rodsLog(LOG_ERROR, "%s", e.what());
                blackboard.set({{constants::status,  states::failed},
                                {constants::errors, {
                                {constants::code,    e.code()},
                                {constants::message, e.what()}}}});
            }

            return blackboard.get();

        } // operation

    }; // recursive_remove

} // namespace irods::experimental::api

extern "C"
irods::experimental::api::recursive_remove* plugin_factory(const std::string&, const std::string&) {
    return new irods::experimental::api::recursive_remove{};
}
