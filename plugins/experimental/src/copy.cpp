#include "irods/private/parallel_filesystem_operation.hpp"

namespace irods::experimental::api {
    class copy : public parallel_filesystem_operation {
        protected:
        bool replace(std::string& str, const std::string& from, const std::string& to)
        {
            size_t start_pos = str.find(from);
            if(start_pos == std::string::npos)
                return false;
            str.replace(start_pos, from.length(), to);
            return true;
        }

        void process_object(rcComm_t& comm, const fs::path& path, const json& req) override
        {
            fs::path slp = get<std::string>("logical_path", req);
            fs::path dlp = get<std::string>("destination_logical_path", req);

            if(req.contains("recursive")) {
                auto replaced = path.string();
                if(replace(replaced, slp, dlp)) {
                    fscl::copy(comm, path, {replaced});
                }
            }
            else {
                fscl::copy(comm, slp, dlp);
            }
        } // process_object

        void collection_precondition(rcComm_t& comm, const json& req) override
        {
            fs::path slp = get<std::string>("logical_path", req);
            fs::path dlp = get<std::string>("destination_logical_path", req);

            if(!req.contains("recursive")) {
                return;
            }

            for(auto e : fscl::recursive_collection_iterator(comm, slp)) {
                if(e.is_collection()) {
                    auto replaced = e.path().string();

                    if(replace(replaced, slp, dlp)) {
                        auto dst = fs::path{replaced};
                        try {
                            fscl::create_collections(comm, dst);
                        }
                        catch(...) {
                            log::server::error("copy - failed to create collection(s) [{}]", dst.parent_path().c_str());
                            return;
                        }
                    } // if replaced
                } // is_collection
            } // for e
        } // collection_precondition

        public:

            copy() : parallel_filesystem_operation("copy") {}

    }; // copy

} // namespace irods::experimental::api

extern "C"
irods::experimental::api::copy* plugin_factory(const std::string&, const std::string&) {
    return new irods::experimental::api::copy{};
}
