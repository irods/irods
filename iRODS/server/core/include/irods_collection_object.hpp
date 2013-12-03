/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef __EIRODS_COLLECTION_OBJECT_H__
#define __EIRODS_COLLECTION_OBJECT_H__

// =-=-=-=-=-=-=-
// system includes
#include <sys/types.h>
#include <dirent.h>

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_data_object.hpp"

namespace eirods {

    class collection_object : public data_object {
    public:
        // =-=-=-=-=-=-=-
        // Constructors
        collection_object();
        collection_object( const collection_object& );
        collection_object( 
            const std::string&, // phy path
            const std::string&, // resc hier
            int,                // mode
            int );              // flags

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
        virtual error get_re_vars( keyValPair_t& ); 

        // =-=-=-=-=-=-=-
        // Accessors
        virtual DIR* directory_pointer() const { return directory_pointer_; }
                
        // =-=-=-=-=-=-=-
        // Mutators     
        virtual void directory_pointer( DIR* _p ) { directory_pointer_ = _p; }

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

}; // namespace eirods

#endif // __EIRODS_COLLECTION_OBJECT_H__



