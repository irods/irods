


#ifndef ___EIRODS_RESC_PLUGIN_H__
#define ___EIRODS_RESC_PLUGIN_H__

// =-=-=-=-=-=-=-
// My Includes
#include "eirods_operation_wrapper.h"

namespace eirods {

	// =-=-=-=-=-=-=-
	/**
	  * \class 
	  * \author Jason M. Coposky 
	  * \date   July 2012
	  * \brief  
	  * 
	  **/
	class resource : public plugin_base {
        public:

		// =-=-=-=-=-=-=-
		// Constructors
		resource( );

		// =-=-=-=-=-=-=-
		// Destructor
		virtual ~resource();

        // =-=-=-=-=-=-=-
		// copy ctor
		resource( const resource& _rhs ); 

		// =-=-=-=-=-=-=-
		// Assignment Operator - necessary for stl containers
		resource& operator=( const resource& _rhs );

		// =-=-=-=-=-=-=-
		// override from parent plugin_base
		virtual error delay_load( void* _h );
		
		// =-=-=-=-=-=-=-
		// get a property from the map if it exists.  catch the exception in the case where
		// the template types may not match and return sucess/fail
		template< typename T >
		error get_property( std::string _key, T& _val ) {
			// =-=-=-=-=-=-=-
			// check params
			#ifdef DEBUG
			if( _key.empty() ) {
				std::cout << "[!]\teirods::resource::get_property - empty key" << std::endl;
				return ERROR( false, -1, "empty key" );
			}
			#endif
			
			// =-=-=-=-=-=-=-
			// attempt to any_cast property value to given type.  catch exception and log
			// failure then exit
			try {
				_val = boost::any_cast< T >( properties_[ _key ] );
				return SUCCESS();
			} catch ( const boost::bad_any_cast & ) {
				return ERROR( false, -1, "get_propery - type and property key mistmatch" );
			}
	 
			return ERROR( false, -1, "get_property - shouldn't get here." );

		} // get_property

		// =-=-=-=-=-=-=-
		// set a property in the map
		template< typename T >
		error set_property( std::string _key, const T& _val ) {
			// =-=-=-=-=-=-=-
			// check params	
			#ifdef DEBUG
			if( _key.empty() ) {
				return ERROR( false, -1, "set_property - empty key" );
			}
			
			if( properties_.has_entry( _key ) ) {
				std::cout << "[+]\teirods::resource::set_property - overwriting entry for key [" 
						  << key << "]" << std::endl;
			}
			#endif	
		
			// =-=-=-=-=-=-=-
			// add property to map
			properties_[ _key ] = _val;
				
			return SUCCESS() ;

		} // set_property


		// =-=-=-=-=-=-=-
		// interface to add and remove children using the zone_name::resource_name
		virtual error add_child( std::string, std::string, resource_ptr );
		virtual error remove_child( std::string );
		
		// =-=-=-=-=-=-=-
		// interface to add operations - key, function name
		error add_operation( std::string, std::string );
		
		// =-=-=-=-=-=-=-
		// interface to set start / stop functions
		void set_start_operation( std::string );
		void set_stop_operation ( std::string );

        static error default_start_operation() { return SUCCESS(); };
        static error default_stop_operation () { return SUCCESS(); };

		// =-=-=-=-=-=-=-
		// delegate the call to the operation in question to the operation wrapper, with 1 param
		template< typename T1 >
		error call( std::string _op, T1 _t1 ) {
			error ret = check_operation_params( _op );
			if( ret.ok() ) {	
				return operations_[ _op ].call< T1 >( &properties_, &children_, _t1 );
			} else {
				return PASS( false, -1, "check_operation_params failed.", ret );
			}
		} // call - T1

		// =-=-=-=-=-=-=-
		// delegate the call to the operation in question to the operation wrapper, with 2 params
		template< typename T1, typename T2 >
		error call( std::string _op, T1 _t1, T2 _t2 ) {
			error ret = check_operation_params( _op );
			if( ret.ok() ) {
				return operations_[ _op ].call< T1, T2 >( &properties_, &children_, _t1, _t2 );
			} else {
				return PASS( false, -1, "check_operation_params failed.", ret );
			}
		} // call - T1, T2

		// =-=-=-=-=-=-=-
		// delegate the call to the operation in question to the operation wrapper, with 3 params
		template< typename T1, typename T2, typename T3 >
		error call( std::string _op, T1 _t1, T2 _t2, T3 _t3 ) {
			error ret = check_operation_params( _op );
			if( ret.ok() ) {
				return operations_[ _op ].call< T1, T2, T3 >( &properties_, &children_, _t1, _t2, _t3 );
			} else {
				return PASS( false, -1, "check_operation_params failed.", ret );
			}
		} // call - T1, T2, T3

		// =-=-=-=-=-=-=-
		// delegate the call to the operation in question to the operation wrapper, with 4 params
		template< typename T1, typename T2, typename T3, typename T4 >
		error call( std::string _op, T1 _t1, T2 _t2, T3 _t3, T4 _t4 ) {
			error ret = check_operation_params( _op );
			if( ret.ok() ) {
				return  operations_[ _op ].call< T1, T2, T3, T4 >( &properties_, &children_, _t1, _t2, _t3, _t4 );
			} else {
				return PASS( false, -1, "check_operation_params failed.", ret );
			}
		} // call - T1, T2, T3, T4

		// =-=-=-=-=-=-=-
		// delegate the call to the operation in question to the operation wrapper, with 5 params
		template< typename T1, typename T2, typename T3, typename T4, typename T5 >
		error call( std::string _op, T1 _t1, T2 _t2, T3 _t3, T4 _t4, T5 _t5 ) {
			error ret = check_operation_params( _op );
			if( ret.ok() ) {
				return operations_[ _op ].call< T1, T2, T3, T4, T5 >( &properties_, &children_, _t1, _t2, _t3, _t4, _t5 );
			} else {
				return PASS( false, -1, "check_operation_params failed.", ret );
			}
		} // call - T1, T2, T3, T4, T5

		// =-=-=-=-=-=-=-
		// delegate the call to the operation in question to the operation wrapper, with 6 params
		template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6 >
		error call( std::string _op, T1 _t1, T2 _t2, T3 _t3, T4 _t4, T5 _t5, T6 _t6 ) {
			error ret = check_operation_params( _op );
			if( ret.ok() ) {
				return operations_[ _op ].call< T1, T2, T3, T4, T5, T6 >( &properties_, &children_, _t1, _t2, _t3, _t4, 
				                               _t5, _t6 );
			} else {
				return PASS( false, -1, "check_operation_params failed.", ret );
			}
		} // call - T1, T2, T3, T4, T5, T6
 
		// =-=-=-=-=-=-=-
		// delegate the call to the operation in question to the operation wrapper, with 7 params
		template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7 >
		error call( std::string _op, T1 _t1, T2 _t2, T3 _t3, T4 _t4, T5 _t5, T6 _t6, T7 _t7 ) {
			error ret = check_operation_params( _op );
			if( ret.ok() ) {
				return operations_[ _op ].call< T1, T2, T3, T4, T5, T6, T7 >( &properties_, &children_, _t1, _t2, _t3, 
				                                _t4, _t5, _t6, _t7 );
			} else {
				return PASS( false, -1, "check_operation_params failed.", ret );
			}
		} // call - T1, T2, T3, T4, T5, T6, T7

		// =-=-=-=-=-=-=-
		// delegate the call to the operation in question to the operation wrapper, with 8 params
		template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8 >
		error call( std::string _op, T1 _t1, T2 _t2, T3 _t3, T4 _t4, T5 _t5, T6 _t6, T7 _t7, T8 _t8 ) {
			error ret = check_operation_params( _op );
			if( ret.ok() ) {
				return operations_[ _op ].call< T1, T2, T3, T4, T5, T6, T7, T8 >( &properties_, &children_, _t1, _t2, 
				                               _t3, _t4, _t5, _t6, _t7, _t8 );
			} else {
				return PASS( false, -1, "check_operation_params failed.", ret );
			}
		} // call - T1, T2, T3, T4, T5, T6, T7, T8

		protected:

        // =-=-=-=-=-=-=-
		// Start / Stop functions - can be overwritten by plugin devs
		// called when a plugin is loaded or unloaded for init / cleanup
		std::string start_opr_name_;
        resource_maintenance_operation start_operation_;

		std::string stop_opr_name_;
        resource_maintenance_operation stop_operation_;

        // =-=-=-=-=-=-=-
		// Pointers to Child and Parent Resources
		resource_child_map  children_;
        
		// =-=-=-=-=-=-=-
		// Map holding resource operations
        lookup_table< operation_wrapper >                    operations_;
		std::vector< std::pair< std::string, std::string > > ops_for_delay_load_;
		
		// =-=-=-=-=-=-=-
		// Map holding resource properties
        resource_property_map properties_;

        private:

		// =-=-=-=-=-=-=-
		// function which checks incoming parameters for the operator() to reduce
		// code duplication in the morass of template overloads
		error check_operation_params( std::string );

       
	}; // class resource

	// =-=-=-=-=-=-=-
	// given the name of a microservice, try to load the shared object
	// and then register that ms with the table
	error load_resource_plugin( resource_ptr&, const std::string );


}; // namespace eirods


#endif // ___EIRODS_RESC_PLUGIN_H__



