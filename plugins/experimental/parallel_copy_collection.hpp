#ifndef PARALLEL_COPY_COLLECTION_HPP
#define PARALLEL_COPY_COLLECTION_HPP


namespace irods::experimental::api {

    struct parallel_copy_options {
        int              thread_count;
        std::string      source_logical_path;
        std::string      destination_logical_path;
    }; // parallel_copy_options

    void parallel_copy_collection(locking_json&, const parallel_copy_options&);

} // namespace irods::experimental::api

#endif // PARALLEL_COPY_COLLECTION_HPP
