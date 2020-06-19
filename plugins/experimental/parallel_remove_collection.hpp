#ifndef PARALLEL_REMOVE_COLLECTION
#define PARALLEL_REMOVE_COLLECTION

namespace irods::experimental::api {
    struct parallel_remove_options {
        std::string logical_path;
        bool        unregister;
        bool        no_trash;
        int         thread_count;
    }; // remove_options

    void parallel_remove_collection(locking_json&, const parallel_remove_options&);

} // namespace irods::experimental::api

#endif // PARALLEL_REMOVE_COLLECTION
