#ifndef __IRODS_COLLECTION_OBJECT_HPP__
#define __IRODS_COLLECTION_OBJECT_HPP__

// =-=-=-=-=-=-=-
// system includes
#include <sys/types.h>
#include <dirent.h>

// =-=-=-=-=-=-=-
#include "irods_data_object.hpp"

namespace irods {

    class collection_object : public data_object {
        public:
            // =-=-=-=-=-=-=-
            // Constructors
            collection_object();
            collection_object( const collection_object& );
            collection_object(
                const std::string&, // phy path
                rodsLong_t,         // resc_id
                int, 				// mode
                int ); 				// flags
            collection_object(
                const std::string&, // phy path
                rodsLong_t,         // resc_id
                int,                // mode
                int,				// flags
                const keyValPair_t& );	// cond_input
            collection_object(
                const std::string&, // phy path
                const std::string&, // resc hier
                int, 				// mode
                int ); 				// flags
            collection_object(
                const std::string&, // phy path
                const std::string&, // resc hier
                int,                // mode
                int,				// flags
                const keyValPair_t& );	// cond_input

            // =-=-=-=-=-=-=-
            // Destructor
            virtual ~collection_object();

            // =-=-=-=-=-=-=-
            // Operators
            virtual collection_object& operator=( const collection_object& );

            // =-=-=-=-=-=-=-
            // plugin resolution operation
            virtual error resolve(
                const std::string&, // plugin interface name
                plugin_ptr& );      // resolved plugin instance

            // =-=-=-=-=-=-=-
            // accessor for rule engine variables
            virtual error get_re_vars( rule_engine_vars_t& );

            // =-=-=-=-=-=-=-
            // Accessors
            virtual DIR* directory_pointer() const {
                return directory_pointer_;
            }

            // =-=-=-=-=-=-=-
            // Mutators
            virtual void directory_pointer( DIR* _p ) {
                directory_pointer_ = _p;
            }

        protected:
            // =-=-=-=-=-=-=-
            // Attributes
            // NOTE :: These are not guaranteed to be properly populated right now
            //      :: that will need be done later when these changes are pushed
            //      :: higher in the original design
            DIR* directory_pointer_;    // pointer to open filesystem directory

    }; // class collection_object

/// =-=-=-=-=-=-=-
/// @brief typedef for managed collection object pointer
    typedef boost::shared_ptr< collection_object > collection_object_ptr;

}; // namespace irods

#endif // __IRODS_COLLECTION_OBJECT_HPP__



