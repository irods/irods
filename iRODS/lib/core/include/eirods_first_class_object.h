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
        virtual error resolve( resource_manager&, resource_ptr& ) = 0;
        virtual error resolve( network_manager&,  network_ptr&  ) = 0;
        
        // =-=-=-=-=-=-=-
        // accessor for rule engine variables
        virtual error get_re_vars( keyValPair_t& ) = 0;
    
    }; // class first_class_object

}; // namespace eirods

#endif // __EIRODS_FIRST_CLASS_OBJECT_H__



