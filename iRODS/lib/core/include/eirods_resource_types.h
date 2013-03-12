/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef __EIRODS_RESOURCE_TYPES_H__

// =-=-=-=-=-=-=-
// Boost Includes
#include <boost/shared_ptr.hpp>
#include <boost/any.hpp>

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_plugin_base.h"
#include "eirods_lookup_table.h"

// =-=-=-=-=-=-=-
// irods includes
#include "rcConnect.h"

namespace eirods {
    class first_class_object;

    // =-=-=-=-=-=-=-
    // resource operation function signature, helpful typedefs
    class resource;
    typedef boost::shared_ptr< resource >                          resource_ptr;
    typedef lookup_table<boost::any>                               resource_property_map;
    typedef lookup_table< std::pair< std::string, resource_ptr > > resource_child_map;

    // =-=-=-=-=-=-=-
    // @brief class which holds default values passed to resource plugin
    //        operations.  this allows for easy extension in the future
    //        without the need to rewrite plugin interfaces as well as
    //        pass along references rather than pointers
    class resource_operation_context {
        public:
        // =-=-=-=-=-=-=-
        // ctor
        resource_operation_context( rsComm_t*               _comm,
                                    resource_property_map&  _prop_map,  
                                    resource_child_map&     _child_map, 
                                    first_class_object&     _fco,       
                                    const std::string&      _results)  :
                                    comm_( _comm ),
                                    prop_map_( _prop_map ),
                                    child_map_( _child_map ),
                                    fco_( _fco ),
                                    results_( _results )  {
        } // ctor

        // =-=-=-=-=-=-=-
        // test to determine if contents are valid
        bool valid() { 
            return ( comm_ != 0 ); 
        }

        // =-=-=-=-=-=-=-
        // accessors
        rsComm_t*               comm()         { return comm_;      } 
        resource_property_map&  prop_map()     { return prop_map_;  }
        resource_child_map&     child_map()    { return child_map_; }
        first_class_object&     fco()          { return fco_;       }
        const std::string&      rule_results() { return results_;   }  

        private:
        // =-=-=-=-=-=-=-
        // attributes
        rsComm_t*               comm_;      // server side comm context
        resource_property_map&  prop_map_;  // resource property map
        resource_child_map&     child_map_; // resource child map
        first_class_object&     fco_;       // first class object in question
        const std::string&      results_;   // results from the pre op rule call

    }; // class resource_operation_context

    typedef error (*resource_operation)( rsComm_t*, 
                                         resource_property_map*, 
                                         resource_child_map*, 
                                         first_class_object*,
                                         std::string*, 
                                         ... );
    typedef error (*resource_maintenance_operation)( resource_property_map&, 
                                                     resource_child_map& );

}; // namespace

#define __EIRODS_RESOURCE_TYPES_H__
#endif // __EIRODS_RESOURCE_TYPES_H__



