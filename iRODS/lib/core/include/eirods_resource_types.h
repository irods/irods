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
        error valid() { 
            error ret = SUCCESS();

            // =-=-=-=-=-=-=
            // trap case of bad comm pointer
            bool comm_valid = ( comm_ != 0 ); 
            if( !comm_valid ) {
                ret = ERROR( -1, "resource_operation_context::valid - bad comm pointer" );
            }
 
            return ret;

        } // valid

        // =-=-=-=-=-=-=-
        // test to determine if contents are valid
        template < typename OBJ_TYPE >
        error valid() { 
            // =-=-=-=-=-=-=
            // trap case of non type related checks
            error ret = valid();

            // =-=-=-=-=-=-=
            // trap case of incorrect type for first class object
            bool cast_valid = false;
            try {
                OBJ_TYPE& ref = dynamic_cast< OBJ_TYPE& >( fco_ );
            } catch( std::bad_cast exp ) {
                ret = PASSMSG( "resource_operation_context::valid - invalid type for fco cast", ret );
            }

            return ret;

        } // valid

        // =-=-=-=-=-=-=-
        // accessors
        rsComm_t*               comm()         { return comm_;      } 
        resource_property_map&  prop_map()     { return prop_map_;  }
        resource_child_map&     child_map()    { return child_map_; }
        first_class_object&     fco()          { return fco_;       }
        const std::string       rule_results() { return results_;   }  
        
        // =-=-=-=-=-=-=-
        // mutators
        void rule_results( const std::string& _s ) { results_ = _s; }  

        private:
        // =-=-=-=-=-=-=-
        // attributes
        rsComm_t*               comm_;      // server side comm context
        resource_property_map&  prop_map_;  // resource property map
        resource_child_map&     child_map_; // resource child map
        first_class_object&     fco_;       // first class object in question
        std::string             results_;   // results from the pre op rule call

    }; // class resource_operation_context

    typedef error (*resource_operation)( resource_operation_context*, 
                                         ... );
    typedef error (*resource_maintenance_operation)( resource_property_map&, 
                                                     resource_child_map& );

}; // namespace

#define __EIRODS_RESOURCE_TYPES_H__
#endif // __EIRODS_RESOURCE_TYPES_H__



