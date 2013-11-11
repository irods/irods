/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef __EIRODS_FIRST_CLASS_OBJECT_H__
#define __EIRODS_FIRST_CLASS_OBJECT_H__

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_log.h"
#include "eirods_resource_types.h"
#include "eirods_network_types.h"

// =-=-=-=-=-=-=-
// irods includes
#include "rcConnect.h"

// =-=-=-=-=-=-=-
// boost includs
#include <boost/shared_ptr.hpp>

namespace eirods {
    // =-=-=-=-=-=-=-
    // base class for all object types
    class first_class_object {
    public:
        // =-=-=-=-=-=-=-
        // Constructors
        first_class_object() {};
            
        // =-=-=-=-=-=-=-
        // Destructor
        virtual ~first_class_object() {};

        // =-=-=-=-=-=-=-
        // plugin resolution operators
        virtual error resolve( 
                          const std::string&, // plugin interface
                          plugin_ptr& ) = 0;  // resolved plugin
        
        // =-=-=-=-=-=-=-
        // accessor for rule engine variables
        virtual error get_re_vars( keyValPair_t& ) = 0;
    
    }; // class first_class_object

    /// =-=-=-=-=-=-=-
    /// @brief shared pointer to first_class_object
    typedef boost::shared_ptr< first_class_object > first_class_object_ptr;

}; // namespace eirods

#endif // __EIRODS_FIRST_CLASS_OBJECT_H__



