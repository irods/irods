
#include "irods/private/parallel_filesystem_operation.hpp"

#include "irods/dataObjRepl.h"

namespace irods::experimental::api {
    class replicate : parallel_filesystem_operation {
        private:
            void process_object(rcComm_t& comm, const fs::path& path, const json& req)
            {
                auto lp  = path.string();
                auto src = req.at("source_resource").get<std::string>();
                auto all = req.contains("update_all_replicas");
                auto upd = req.contains("update_one_replica");

                auto adm = req.contains("admin_mode");
                auto dst = req.contains("destination_resource")              ?
                           req.at("destination_resource").get<std::string>() :
                           std::string{};

                if(upd && dst.empty()) {
                    THROW(SYS_INVALID_INPUT_PARAM,
                          "update requested without a destination resource");
                }
                else if(!all && dst.empty()) {
                    THROW(SYS_INVALID_INPUT_PARAM,
                          "destination resource is not specified");
                }

                dataObjInp_t doi{};
                rstrcpy(doi.objPath,      lp.c_str(),   MAX_NAME_LEN);
                addKeyVal(&doi.condInput, RESC_NAME_KW, src.c_str());

                if(!dst.empty()) {
                    log::api::trace(fmt::format("replicating {} from {} to {}", lp, src, dst));
                    addKeyVal(&doi.condInput, DEST_RESC_NAME_KW, dst.c_str());
                }

                if(all) {
                    log::api::trace(fmt::format("updating all replicas of {} from {}", lp, src));
                    addKeyVal(&doi.condInput, ALL_KW, "");
                }

                if(upd && !all) {
                    log::api::trace(fmt::format("updating replica of {} from {} to {}", lp, src, dst));
                }

                if(adm) { addKeyVal(&doi.condInput, ADMIN_KW, ""); }

                if(upd||all) { addKeyVal(&doi.condInput, UPDATE_REPL_KW, ""); }

                if(auto err = rcDataObjRepl(&comm, &doi); err < 0) {
                    THROW(err, fmt::format("failed to replicate {} from {} to {}", lp, src, dst));
                }

            } // process_object

        public:

            replicate() : parallel_filesystem_operation("replicate") {}

    }; // replicate

} // namespace irods::experimental::api

extern "C"
irods::experimental::api::replicate* plugin_factory(const std::string&, const std::string&) {
    return new irods::experimental::api::replicate{};
}
