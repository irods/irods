


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
#include "eirods_error.h"
#include "eirods_operation_rule_execution_manager.h"

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
		/// @brief constructors
		operation_wrapper(  );
		operation_wrapper( const std::string&,   // instance name
                           const std::string&,   // operation name
                           resource_operation ); // fcn ptr to loaded operation
		
        // =-=-=-=-=-=-=-
		/// @brief destructor
		virtual ~operation_wrapper();

        // =-=-=-=-=-=-=-
		/// @brief copy constructor - necessary for stl containers
		operation_wrapper( const operation_wrapper& _rhs ); 

		// =-=-=-=-=-=-=-
		/// @brief assignment operator - necessary for stl containers
		operation_wrapper& operator=( const operation_wrapper& _rhs );

		// =-=-=-=-=-=-=-
		// operator call that will git 'er done. this clearly must match the resource_operation type signature
		// NOTE :: we are taking the old, long route to handle multiple template params.  when we can use C++11
		//         this will be MUCH cleaner.
		
		// =-=-=-=-=-=-=-
		// public - single parameter template, there will be more...
		template< typename T1 >
		error call( rsComm_t* _comm, resource_property_map* _prop_map, resource_child_map* _cmap, T1 _t1 ) {
            if( operation_ ) {
               // =-=-=-=-=-=-=-
               // ingest incoming params as string to pass to the rule exec
               std::vector< std::string > arg_vec;
               
               std::stringstream p1;  
               p1 << _t1;
               arg_vec.push_back( p1.str() );

               // =-=-=-=-=-=-=-
               // instantiate a rule executor
               operation_rule_execution_manager rule_exec( instance_name_, operation_name_ );

               // =-=-=-=-=-=-=-
               // call the pre-rule for this op
               std::string pre_results;
               rule_exec.exec_pre_op( _comm, arg_vec, pre_results );
               arg_vec.push_back( pre_results );

               // =-=-=-=-=-=-=-
               // call the actual operation
		       error op_err = (*operation_)( _comm, pre_results, _prop_map, _cmap, _t1 ); 
               
               // =-=-=-=-=-=-=-
               // call the poste-rule for this op
               std::string post_results;
               rule_exec.exec_post_op( _comm, arg_vec, post_results );

              return op_err;

		   } else {
			  return ERROR( -1, "operation_wrapper - null resource operation." );
		   }
		   
		} // operator() - T1

		// =-=-=-=-=-=-=-
		// public - two parameter template, there will be more...
		template< typename T1, typename T2 >
		error call( rsComm_t* _comm, resource_property_map* _prop_map, resource_child_map* _cmap, T1 _t1, T2 _t2 ) {
		   if( operation_ ) {
               // =-=-=-=-=-=-=-
               // ingest incoming params as string to pass to the rule exec
               std::vector< std::string > arg_vec;
               
               std::stringstream p1;  
               p1 << _t1;
               arg_vec.push_back( p1.str() );
               
               std::stringstream p2;  
               p1 << _t2;
               arg_vec.push_back( p2.str() );

               // =-=-=-=-=-=-=-
               // instantiate a rule executor
               operation_rule_execution_manager rule_exec( instance_name_, operation_name_ );

               // =-=-=-=-=-=-=-
               // call the pre-rule for this op
               std::string pre_results;
               rule_exec.exec_pre_op( _comm, arg_vec, pre_results );
               arg_vec.push_back( pre_results );

               // =-=-=-=-=-=-=-
               // call the actual operation
			   error op_err = (*operation_)( _comm, pre_results, _prop_map, _cmap, _t1, _t2 ); 
 
               // =-=-=-=-=-=-=-
               // call the poste-rule for this op
               std::string post_results;
               rule_exec.exec_post_op( _comm, arg_vec, post_results );

               return op_err;

		   } else {
			  return ERROR( -1, "operation_wrapper - null resource operation." );
		   }
		   
		} // operator() - T1, T2

		// =-=-=-=-=-=-=-
		// public - three parameter template, there will be more...
		template< typename T1, typename T2, typename T3 >
		error call( rsComm_t* _comm, resource_property_map* _prop_map, resource_child_map* _cmap, 
		                               T1 _t1, T2 _t2, T3 _t3 ) {
		   if( operation_ ) {
               // =-=-=-=-=-=-=-
               // ingest incoming params as string to pass to the rule exec
               std::vector< std::string > arg_vec;
               
               std::stringstream p1;  
               p1 << _t1;
               arg_vec.push_back( p1.str() );
               
               std::stringstream p2;  
               p2 << _t2;
               arg_vec.push_back( p2.str() );
 
               std::stringstream p3;  
               p3 << _t3;
               arg_vec.push_back( p3.str() );

               // =-=-=-=-=-=-=-
               // instantiate a rule executor
               operation_rule_execution_manager rule_exec( instance_name_, operation_name_ );

               // =-=-=-=-=-=-=-
               // call the pre-rule for this op
               std::string pre_results;
               rule_exec.exec_pre_op( _comm, arg_vec, pre_results );
               arg_vec.push_back( pre_results );

               // =-=-=-=-=-=-=-
               // call the actual operation
			   error op_err =  (*operation_)( _comm, pre_results, _prop_map, _cmap, _t1, _t2, _t3 ); 
 
               // =-=-=-=-=-=-=-
               // call the poste-rule for this op
               std::string post_results;
               rule_exec.exec_post_op( _comm, arg_vec, post_results );

               return op_err;

		   } else {
			  return ERROR( -1, "operation_wrapper - null resource operation." );
		   }

		} // operator() - T1, T2, T3

		// =-=-=-=-=-=-=-
		// public - four parameter template, there will be more...
		template< typename T1, typename T2, typename T3, typename T4 >
		error call( rsComm_t* _comm, resource_property_map* _prop_map, resource_child_map* _cmap, 
		                               T1 _t1, T2 _t2, T3 _t3, T4 _t4 ) {
		   if( operation_ ) {
	           // =-=-=-=-=-=-=-
               // ingest incoming params as string to pass to the rule exec
               std::vector< std::string > arg_vec;
               
               std::stringstream p1;  
               p1 << _t1;
               arg_vec.push_back( p1.str() );
               
               std::stringstream p2;  
               p2 << _t2;
               arg_vec.push_back( p2.str() );
               
               std::stringstream p3;  
               p3 << _t3;
               arg_vec.push_back( p3.str() );
            
               std::stringstream p4;  
               p4 << _t4;
               arg_vec.push_back( p4.str() );


               // =-=-=-=-=-=-=-
               // instantiate a rule executor
               operation_rule_execution_manager rule_exec( instance_name_, operation_name_ );

               // =-=-=-=-=-=-=-
               // call the pre-rule for this op
               std::string pre_results;
               rule_exec.exec_pre_op( _comm, arg_vec, pre_results );
               arg_vec.push_back( pre_results );

               // =-=-=-=-=-=-=-
               // call the actual operation		  
               error op_err =  operation_( _comm, pre_results, _prop_map, _cmap, _t1, _t2, _t3, _t4 ); 
 
               // =-=-=-=-=-=-=-
               // call the poste-rule for this op
               std::string post_results;
               rule_exec.exec_post_op( _comm, arg_vec, post_results );

               return op_err;


		   } else {
			  return ERROR( -1, "operation_wrapper - null resource operation." );
		   }
		   
		} // operator() - T1, T2, T3, T4

		// =-=-=-=-=-=-=-
		// public - five parameter template, there will be more...
		template< typename T1, typename T2, typename T3, typename T4, typename T5 >
		error call( rsComm_t* _comm, resource_property_map* _prop_map, resource_child_map* _cmap, 
		                               T1 _t1, T2 _t2, T3 _t3, T4 _t4, T5 _t5 ) {
		   if( operation_ ) {
	           // =-=-=-=-=-=-=-
               // ingest incoming params as string to pass to the rule exec
               std::vector< std::string > arg_vec;
               
               std::stringstream p1;  
               p1 << _t1;
               arg_vec.push_back( p1.str() );
               
               std::stringstream p2;  
               p2 << _t2;
               arg_vec.push_back( p2.str() );
               
               std::stringstream p3;  
               p3 << _t3;
               arg_vec.push_back( p3.str() );
            
               std::stringstream p4;  
               p4 << _t4;
               arg_vec.push_back( p4.str() );
         
               std::stringstream p5;  
               p5 << _t5;
               arg_vec.push_back( p5.str() );

               // =-=-=-=-=-=-=-
               // instantiate a rule executor
               operation_rule_execution_manager rule_exec( instance_name_, operation_name_ );

               // =-=-=-=-=-=-=-
               // call the pre-rule for this op
               std::string pre_results;
               rule_exec.exec_pre_op( _comm, arg_vec, pre_results );
               arg_vec.push_back( pre_results );

               // =-=-=-=-=-=-=-
               // call the actual operation		  
			   error op_err =  (*operation_)( _comm, pre_results, _prop_map, _cmap, _t1, _t2, _t3, _t4, _t5 ); 
 
               // =-=-=-=-=-=-=-
               // call the poste-rule for this op
               std::string post_results;
               rule_exec.exec_post_op( _comm, arg_vec, post_results );

               return op_err;

		   } else {
			  return ERROR( -1, "operation_wrapper - null resource operation." );
		   }

		} // operator() - T1, T2, T3, T4, T5

		// =-=-=-=-=-=-=-
		// public - six parameter template, there will be more...
		template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6 >
		error call( rsComm_t* _comm, resource_property_map* _prop_map, resource_child_map* _cmap, 
		                               T1 _t1, T2 _t2, T3 _t3, T4 _t4, T5 _t5, T6 _t6 ) {
		   if( operation_ ) {
	           // =-=-=-=-=-=-=-
               // ingest incoming params as string to pass to the rule exec
               std::vector< std::string > arg_vec;
               
               std::stringstream p1;  
               p1 << _t1;
               arg_vec.push_back( p1.str() );
               
               std::stringstream p2;  
               p2 << _t2;
               arg_vec.push_back( p2.str() );
               
               std::stringstream p3;  
               p3 << _t3;
               arg_vec.push_back( p3.str() );
            
               std::stringstream p4;  
               p4 << _t4;
               arg_vec.push_back( p4.str() );
         
               std::stringstream p5;  
               p5 << _t5;
               arg_vec.push_back( p5.str() );
      
               std::stringstream p6;  
               p6 << _t6;
               arg_vec.push_back( p6.str() );

               // =-=-=-=-=-=-=-
               // instantiate a rule executor
               operation_rule_execution_manager rule_exec( instance_name_, operation_name_ );

               // =-=-=-=-=-=-=-
               // call the pre-rule for this op
               std::string pre_results;
               rule_exec.exec_pre_op( _comm, arg_vec, pre_results );
               arg_vec.push_back( pre_results );

               // =-=-=-=-=-=-=-
               // call the actual operation		  
			   error op_err = (*operation_)( _comm, pre_results, _prop_map, _cmap, _t1, _t2, _t3, _t4, _t5, _t6 );  
 
               // =-=-=-=-=-=-=-
               // call the poste-rule for this op
               std::string post_results;
               rule_exec.exec_post_op( _comm, arg_vec, post_results );

               return op_err;

		   } else {
			  return ERROR( -1, "operation_wrapper - null resource operation." );
		   }

		} // operator() - T1, T2, T3, T4, T5, T6

		// =-=-=-=-=-=-=-
		// public - seven parameter template, there will be more...
		template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7 >
		error call( rsComm_t* _comm, resource_property_map* _prop_map, resource_child_map* _cmap, 
		                               T1 _t1, T2 _t2, T3 _t3, T4 _t4, T5 _t5, T6 _t6, T7 _t7 ) {
		   if( operation_ ) {
	           // =-=-=-=-=-=-=-
               // ingest incoming params as string to pass to the rule exec
               std::vector< std::string > arg_vec;
               
               std::stringstream p1;  
               p1 << _t1;
               arg_vec.push_back( p1.str() );
               
               std::stringstream p2;  
               p2 << _t2;
               arg_vec.push_back( p2.str() );
               
               std::stringstream p3;  
               p3 << _t3;
               arg_vec.push_back( p3.str() );
            
               std::stringstream p4;  
               p4 << _t4;
               arg_vec.push_back( p4.str() );
         
               std::stringstream p5;  
               p5 << _t5;
               arg_vec.push_back( p5.str() );
      
               std::stringstream p6;  
               p6 << _t6;
               arg_vec.push_back( p6.str() );
   
               std::stringstream p7;  
               p7 << _t7;
               arg_vec.push_back( p7.str() );

               // =-=-=-=-=-=-=-
               // instantiate a rule executor
               operation_rule_execution_manager rule_exec( instance_name_, operation_name_ );

               // =-=-=-=-=-=-=-
               // call the pre-rule for this op
               std::string pre_results;
               rule_exec.exec_pre_op( _comm, arg_vec, pre_results );
               arg_vec.push_back( pre_results );

               // =-=-=-=-=-=-=-
               // call the actual operation		  
			   error op_err = (*operation_)( _comm, pre_results, _prop_map, _cmap, _t1, _t2, _t3, _t4, _t5, _t6, _t7 );  
 
               // =-=-=-=-=-=-=-
               // call the poste-rule for this op
               std::string post_results;
               rule_exec.exec_post_op( _comm, arg_vec, post_results );

               return op_err;

		   } else {
			  return ERROR( -1, "operation_wrapper - null resource operation." );
		   }

		} // operator() - T1, T2, T3, T4, T5, T6, T7

		// =-=-=-=-=-=-=-
		// public - eight parameter template, there will be more...
		template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8 >
		error call( rsComm_t* _comm, resource_property_map* _prop_map, resource_child_map* _cmap, 
		                               T1 _t1, T2 _t2, T3 _t3, T4 _t4, T5 _t5, T6 _t6, T7 _t7, T8 _t8 ) {
		   if( operation_ ) {
	           // =-=-=-=-=-=-=-
               // ingest incoming params as string to pass to the rule exec
               std::vector< std::string > arg_vec;
               
               std::stringstream p1;  
               p1 << _t1;
               arg_vec.push_back( p1.str() );
               
               std::stringstream p2;  
               p2 << _t2;
               arg_vec.push_back( p2.str() );
               
               std::stringstream p3;  
               p3 << _t3;
               arg_vec.push_back( p3.str() );
            
               std::stringstream p4;  
               p4 << _t4;
               arg_vec.push_back( p4.str() );
         
               std::stringstream p5;  
               p5 << _t5;
               arg_vec.push_back( p5.str() );
      
               std::stringstream p6;  
               p6 << _t6;
               arg_vec.push_back( p6.str() );
   
               std::stringstream p7;  
               p7 << _t7;
               arg_vec.push_back( p7.str() );
 
               std::stringstream p8;  
               p8 << _t8;
               arg_vec.push_back( p8.str() );

               // =-=-=-=-=-=-=-
               // instantiate a rule executor
               operation_rule_execution_manager rule_exec( instance_name_, operation_name_ );

               // =-=-=-=-=-=-=-
               // call the pre-rule for this op
               std::string pre_results;
               rule_exec.exec_pre_op( _comm, arg_vec, pre_results );
               arg_vec.push_back( pre_results );

               // =-=-=-=-=-=-=-
               // call the actual operation	
			   error op_err = (*operation_)( _comm, pre_results, _prop_map, _cmap, _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8 );  
 
               // =-=-=-=-=-=-=-
               // call the poste-rule for this op
               std::string post_results;
               rule_exec.exec_post_op( _comm, arg_vec, post_results );

               return op_err;

		   } else {
			  return ERROR( -1, "operation_wrapper - null resource operation." );
		   }

		} // operator() - T1, T2, T3, T4, T5, T6, T7, T8

    private:
		// =-=-=-=-=-=-=-
		/// @brief function pointer to actual operation
		resource_operation operation_;
		
        // =-=-=-=-=-=-=-
		/// @brief instance name used for calling rules
	    std::string instance_name_;
        
        // =-=-=-=-=-=-=-
		/// @brief name of this operation, use for calling rules
	    std::string operation_name_;
        	
	}; // class operation_wrapper 

}; // namespace eirods

#endif // __EIRODS_OPERATION_WRAPPER_H__



