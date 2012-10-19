


#ifndef ___EIRODS_OPERATION_WRAPPER_H__
#define ___EIRODS_OPERATION_WRAPPER_H__

// =-=-=-=-=-=-=-
// STL Includes
#include <string>
#include <utility>

// =-=-=-=-=-=-=-
// Boost Includes
#include <boost/shared_ptr.hpp>
#include <boost/any.hpp>

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_plugin_base.h"
#include "eirods_lookup_table.h"
#include "eirods_resource_types.h"

// =-=-=-=-=-=-=-
// irods includes
#include "reGlobalsExtern.h"
#include "dataObjInpOut.h"

namespace eirods {
	
	// =-=-=-=-=-=-=-
	/**
	  * \class 
	  * \author Jason M. Coposky 
	  * \date   July 2012
	  * \brief  
	  * 
	  **/
    class operation_wrapper {
    public:
		// =-=-=-=-=-=-=-
		// Constructors
		operation_wrapper(  );
		operation_wrapper( resource_operation );

		// =-=-=-=-=-=-=-
		// Destructor
		virtual ~operation_wrapper();

        // =-=-=-=-=-=-=-
		// Copy Constructor - necessary for stl containers
		operation_wrapper( const operation_wrapper& _rhs ); 

		// =-=-=-=-=-=-=-
		// Assignment Operator - necessary for stl containers
		operation_wrapper& operator=( const operation_wrapper& _rhs );

		// =-=-=-=-=-=-=-
		// operator call that will git 'er done. this clearly must match the resource_operation type signature
		// NOTE :: we are taking the old, long route to handle multiple template params.  when we can use C++11
		//         this will be MUCH cleaner.
		
		// =-=-=-=-=-=-=-
		// public - single parameter template, there will be more...
		template< typename T1 >
		error call( resource_property_map* _prop_map, resource_child_map* _cmap, T1 _t1 ) {
		   if( operation_ ) {
			  return (*operation_)( _prop_map, _cmap, _t1 ); 
		   } else {
			  return ERROR( false, -1, "operation_wrapper - null resource operation." );
		   }
		   
		} // operator() - T1

		// =-=-=-=-=-=-=-
		// public - two parameter template, there will be more...
		template< typename T1, typename T2 >
		error call( resource_property_map* _prop_map, resource_child_map* _cmap, T1 _t1, T2 _t2 ) {
		   if( operation_ ) {
			  return (*operation_)( _prop_map, _cmap, _t1, _t2 ); 
		   } else {
			  return ERROR( false, -1, "operation_wrapper - null resource operation." );
		   }
		   
		} // operator() - T1, T2

		// =-=-=-=-=-=-=-
		// public - three parameter template, there will be more...
		template< typename T1, typename T2, typename T3 >
		error call( resource_property_map* _prop_map, resource_child_map* _cmap, 
		                               T1 _t1, T2 _t2, T3 _t3 ) {
		   if( operation_ ) {
			  return (*operation_)( _prop_map, _cmap, _t1, _t2, _t3 ); 
		   } else {
			  return ERROR( false, -1, "operation_wrapper - null resource operation." );
		   }

		} // operator() - T1, T2, T3

		// =-=-=-=-=-=-=-
		// public - four parameter template, there will be more...
		template< typename T1, typename T2, typename T3, typename T4 >
		error call( resource_property_map* _prop_map, resource_child_map* _cmap, 
		                               T1 _t1, T2 _t2, T3 _t3, T4 _t4 ) {
		   if( operation_ ) {
			  return operation_( _prop_map, _cmap, _t1, _t2, _t3, _t4 ); 
		   } else {
			  return ERROR( false, -1, "operation_wrapper - null resource operation." );
		   }
		   
		} // operator() - T1, T2, T3, T4

		// =-=-=-=-=-=-=-
		// public - five parameter template, there will be more...
		template< typename T1, typename T2, typename T3, typename T4, typename T5 >
		error call( resource_property_map* _prop_map, resource_child_map* _cmap, 
		                               T1 _t1, T2 _t2, T3 _t3, T4 _t4, T5 _t5 ) {
		   if( operation_ ) {
			  return (*operation_)( _prop_map, _cmap, _t1, _t2, _t3, _t4, _t5 ); 
		   } else {
			  return ERROR( false, -1, "operation_wrapper - null resource operation." );
		   }

		} // operator() - T1, T2, T3, T4, T5

		// =-=-=-=-=-=-=-
		// public - six parameter template, there will be more...
		template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6 >
		error call( resource_property_map* _prop_map, resource_child_map* _cmap, 
		                               T1 _t1, T2 _t2, T3 _t3, T4 _t4, T5 _t5, T6 _t6 ) {
		   if( operation_ ) {
			  return (*operation_)( _prop_map, _cmap, _t1, _t2, _t3, _t4, _t5, _t6 ); 
		   } else {
			  return ERROR( false, -1, "operation_wrapper - null resource operation." );
		   }

		} // operator() - T1, T2, T3, T4, T5, T6

		// =-=-=-=-=-=-=-
		// public - seven parameter template, there will be more...
		template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7 >
		error call( resource_property_map* _prop_map, resource_child_map* _cmap, 
		                               T1 _t1, T2 _t2, T3 _t3, T4 _t4, T5 _t5, T6 _t6, T7 _t7 ) {
		   if( operation_ ) {
			  return (*operation_)( _prop_map, _cmap, _t1, _t2, _t3, _t4, _t5, _t6, _t7 ); 
		   } else {
			  return ERROR( false, -1, "operation_wrapper - null resource operation." );
		   }

		} // operator() - T1, T2, T3, T4, T5, T6, T7

		// =-=-=-=-=-=-=-
		// public - eight parameter template, there will be more...
		template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8 >
		error call( resource_property_map* _prop_map, resource_child_map* _cmap, 
		                               T1 _t1, T2 _t2, T3 _t3, T4 _t4, T5 _t5, T6 _t6, T7 _t7, T8 _t8 ) {
		   if( operation_ ) {
			  return (*operation_)( _prop_map, _cmap, _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8 ); 
		   } else {
			  return ERROR( false, -1, "operation_wrapper - null resource operation." );
		   }

		} // operator() - T1, T2, T3, T4, T5, T6, T7, T8

    private:
		// =-=-=-=-=-=-=-
		// function pointer to actual operation
		resource_operation operation_;
		
	}; // class operation_wrapper 

}; // namespace eirods

#endif // __EIRODS_OPERATION_WRAPPER_H__



