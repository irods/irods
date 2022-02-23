#ifndef IRODS_ENTITY_HPP
#define IRODS_ENTITY_HPP

namespace irods::experimental::entity {

    enum class entity_type {
        collection,
        data_object,
        resource,
        user,
        zone
    };

} // irods::experimental::entity

#endif // IRODS_ENTITY_HPP
