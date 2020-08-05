
#include "experimental_plugin_framework.hpp"
#include "parallel_copy_collection.hpp"

#include "thread_pool.hpp"
#include "connection_pool.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"

namespace irods::experimental::api {
    namespace fs   = irods::experimental::filesystem;
    namespace fsvr = irods::experimental::filesystem::server;

    class copy : public base {

        bool enable_asynchronous_operation() { return true; }

        auto operation(rsComm_t* comm, const json req) -> json
        {
            try {
                blackboard.set({{constants::status, states::running}});

                fs::path source = get<std::string>("logical_path", req);
                fs::path destination = get<std::string>("destination", req);
                int thread_count = get<int>("thread_count", req, 4);

                if(fsvr::is_collection(*comm, source)) {
                    parallel_copy_collection(blackboard, {thread_count, source, destination});
                }
                else {
                    fsvr::copy(*comm, source, destination);
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

    }; // copy

} // namespace irods::experimental::api

extern "C"
irods::experimental::api::copy* plugin_factory(const std::string&, const std::string&) {
    return new irods::experimental::api::copy{};
}
