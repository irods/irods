#include "irods/private/parallel_filesystem_operation.hpp"

namespace irods::experimental::api {
    class remove : public parallel_filesystem_operation {
        protected:
        void process_object(rcComm_t& comm, const fs::path& path, const json& req) override
        {
            filesystem::extended_remove_options opts{};
            opts.no_trash   = req.contains("no_trash");
            opts.verbose    = false;
            opts.progress   = false;
            opts.recursive  = false;
            opts.unregister = req.contains("unregister");

            fscl::remove(comm, path, opts);
        }

        void collection_postcondition(rcComm_t& comm, const json& req) override
        {
            fscl::remove_all(comm, req.at("logical_path"));
        }

        public:
        remove() : parallel_filesystem_operation("remove") {}
    }; // remove

} // namespace irods::experimental::api

extern "C"
irods::experimental::api::remove* plugin_factory(const std::string&, const std::string&) {
    return new irods::experimental::api::remove{};
}
