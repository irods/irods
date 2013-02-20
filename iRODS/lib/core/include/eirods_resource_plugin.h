/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef ___EIRODS_RESC_PLUGIN_H__
#define ___EIRODS_RESC_PLUGIN_H__

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_plugin_base.h"
#include "eirods_operation_wrapper.h"

#include <iostream>

namespace eirods {


    // =-=-=-=-=-=-=-
    /// @brief delimiter used for parsing resource context strings
    const std::string RESOURCE_DELIMITER(";");


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
        /// @brief Constructors
        resource( const std::string&,   // instance name
                  const std::string& ); // context

        // =-=-=-=-=-=-=-
        /// @brief Destructor
        virtual ~resource();

        // =-=-=-=-=-=-=-
        /// @brief copy ctor
        resource( const resource& _rhs ); 

        // =-=-=-=-=-=-=-
        /// @brief Assignment Operator - necessary for stl containers
        resource& operator=( const resource& _rhs );

        // =-=-=-=-=-=-=-
        /// @brief override from parent plugin_base
        virtual error delay_load( void* _h );
        
        // =-=-=-=-=-=-=-
        /// @brief get a property from the map if it exists.  catch the exception in the case where
        // the template types may not match and return sucess/fail
        template< typename T >
        error get_property( const std::string& _key, T& _val ) {
            error ret = properties_.get< T >( _key, _val );
            return PASS( ret.status(), ret.code(), "resource::get_property", ret );
        } // get_property

        // =-=-=-=-=-=-=-
        /// @brief set a property in the map
        template< typename T >
        error set_property( const std::string& _key, const T& _val ) {
            error ret = properties_.set< T >( _key, _val );
            return PASS( ret.status(), ret.code(), "resource::set_property", ret );
        } // set_property

        // =-=-=-=-=-=-=-
        /// @brief interface to add and remove children using the zone_name::resource_name
        virtual error add_child( const std::string&, const std::string&, resource_ptr );
        virtual error remove_child( const std::string& );
       
        // =-=-=-=-=-=-=-
        /// @brief interface to get and set a resource's parent pointer
        virtual error set_parent( const resource_ptr& );
        virtual error get_parent( resource_ptr& );
                
        // =-=-=-=-=-=-=-
        /// @brief interface to set start / stop functions
        void set_start_operation( const std::string& );
        void set_stop_operation ( const std::string& );

        // =-=-=-=-=-=-=-
        /// @brief interface to call start / stop functions
        error start_operation( void ) { return (*start_operation_)( properties_, children_ ); }
        error stop_operation ( void ) { return (*stop_operation_ )( properties_, children_ ); }

        // =-=-=-=-=-=-=-
        /// @brief default start operation
        static error default_start_operation( resource_property_map&,
                                              resource_child_map& ) {
             return SUCCESS(); 
        };
        
        // =-=-=-=-=-=-=-
        /// @brief default stop operation
        static error default_stop_operation( resource_property_map&,
                                             resource_child_map& ) { 
            return SUCCESS(); 
        };

        // =-=-=-=-=-=-=-
        /// @brief delegate the call to the operation in question to the operation wrapper, with 1 param
        error call( rsComm_t* _comm, const std::string& _op, eirods::first_class_object* _obj ) {
            error ret = check_operation_params( _op, _comm, _obj );
            if( ret.ok() ) {        
                return operations_[ _op ].call( _comm, &properties_, &children_, _obj );
            } else {
                return PASS( false, -1, "check_operation_params failed.", ret );
            }
        } // call - 

        // =-=-=-=-=-=-=-
        /// @brief delegate the call to the operation in question to the operation wrapper, with 1 param
        template< typename T1 >
        error call( rsComm_t* _comm, const std::string& _op, eirods::first_class_object* _obj, T1 _t1 ) {
            error ret = check_operation_params( _op, _comm, _obj );
            if( ret.ok() ) {        
                return operations_[ _op ].call< T1 >( _comm, &properties_, &children_, _obj, _t1 );
            } else {
                return PASS( false, -1, "check_operation_params failed.", ret );
            } } // call - T1 
        // =-=-=-=-=-=-=-
        /// @brief delegate the call to the operation in question to the operation wrapper, with 2 params
        template< typename T1, typename T2 >
        error call( rsComm_t* _comm, const std::string& _op, eirods::first_class_object* _obj, T1 _t1, T2 _t2 ) {
            error ret = check_operation_params( _op, _comm, _obj );
            if( ret.ok() ) {
                return operations_[ _op ].call< T1, T2 >( _comm, &properties_, &children_, _obj, _t1, _t2 );
            } else {
                return PASS( false, -1, "check_operation_params failed.", ret );
            }
        } // call - T1, T2

        // =-=-=-=-=-=-=-
        /// @brief delegate the call to the operation in question to the operation wrapper, with 3 params
        template< typename T1, typename T2, typename T3 >
        error call( rsComm_t* _comm, const std::string& _op, eirods::first_class_object* _obj, T1 _t1, T2 _t2, T3 _t3 ) {
            error ret = check_operation_params( _op, _comm, _obj );
            if( ret.ok() ) {
                return operations_[ _op ].call< T1, T2, T3 >( _comm, &properties_, &children_, _obj, _t1, _t2, _t3 );
            } else {
                return PASS( false, -1, "check_operation_params failed.", ret );
            }
        } // call - T1, T2, T3

        // =-=-=-=-=-=-=-
        /// @brief delegate the call to the operation in question to the operation wrapper, with 4 params
        template< typename T1, typename T2, typename T3, typename T4 >
        error call( rsComm_t* _comm, const std::string& _op, eirods::first_class_object* _obj, T1 _t1, T2 _t2, T3 _t3, T4 _t4 ) {
            error ret = check_operation_params( _op, _comm, _obj );
            if( ret.ok() ) {
                return  operations_[ _op ].call< T1, T2, T3, T4 >( _comm, &properties_, &children_, _obj, _t1, _t2, _t3, _t4 );
            } else {
                return PASS( false, -1, "check_operation_params failed.", ret );
            }
        } // call - T1, T2, T3, T4

        // =-=-=-=-=-=-=-
        /// @brief delegate the call to the operation in question to the operation wrapper, with 5 params
        template< typename T1, typename T2, typename T3, typename T4, typename T5 >
        error call( rsComm_t* _comm, const std::string& _op, eirods::first_class_object* _obj, T1 _t1, T2 _t2, T3 _t3, T4 _t4, T5 _t5 ) {
            error ret = check_operation_params( _op, _comm, _obj );
            if( ret.ok() ) {
                return operations_[ _op ].call< T1, T2, T3, T4, T5 >( _comm, &properties_, &children_, _obj, _t1, _t2, _t3, _t4, _t5 );
            } else {
                return PASS( false, -1, "check_operation_params failed.", ret );
            }
        } // call - T1, T2, T3, T4, T5

        // =-=-=-=-=-=-=-
        /// @brief delegate the call to the operation in question to the operation wrapper, with 6 params
        template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6 >
        error call( rsComm_t* _comm, const std::string& _op, eirods::first_class_object* _obj, T1 _t1, T2 _t2, T3 _t3, T4 _t4, T5 _t5, T6 _t6 ) {
            error ret = check_operation_params( _op, _comm, _obj );
            if( ret.ok() ) {
                return operations_[ _op ].call< T1, T2, T3, T4, T5, T6 >( _comm, &properties_, &children_, _obj, _t1, _t2, _t3, _t4, 
                                                                          _t5, _t6 );
            } else {
                return PASS( false, -1, "check_operation_params failed.", ret );
            }
        } // call - T1, T2, T3, T4, T5, T6
 
        // =-=-=-=-=-=-=-
        /// @brief delegate the call to the operation in question to the operation wrapper, with 7 params
        template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7 >
        error call( rsComm_t* _comm, const std::string& _op, eirods::first_class_object* _obj, T1 _t1, T2 _t2, T3 _t3, T4 _t4, T5 _t5, T6 _t6, T7 _t7 ) {
            error ret = check_operation_params( _op, _comm, _obj );
            if( ret.ok() ) {
                return operations_[ _op ].call< T1, T2, T3, T4, T5, T6, T7 >( _comm, &properties_, &children_, _obj, _t1, _t2, _t3, 
                                                                              _t4, _t5, _t6, _t7 );
            } else {
                return PASS( false, -1, "check_operation_params failed.", ret );
            }
        } // call - T1, T2, T3, T4, T5, T6, T7

        // =-=-=-=-=-=-=-
        /// @brief delegate the call to the operation in question to the operation wrapper, with 8 params
        template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8 >
        error call( rsComm_t* _comm, const std::string& _op, eirods::first_class_object* _obj, T1 _t1, T2 _t2, T3 _t3, T4 _t4, T5 _t5, T6 _t6, T7 _t7, T8 _t8 ) {
            error ret = check_operation_params( _op, _comm, _obj );
            if( ret.ok() ) {
                return operations_[ _op ].call< T1, T2, T3, T4, T5, T6, T7, T8 >( _comm, &properties_, &children_, _obj, _t1, _t2, 
                                                                                  _t3, _t4, _t5, _t6, _t7, _t8 );
            } else {
                return PASS( false, -1, "check_operation_params failed.", ret );
            }
        } // call - T1, T2, T3, T4, T5, T6, T7, T8

    protected:

        // =-=-=-=-=-=-=-
        /// @brief Start / Stop functions - can be overwritten by plugin devs
        //         called when a plugin is loaded or unloaded for init / cleanup
        std::string start_opr_name_;
        resource_maintenance_operation start_operation_;

        std::string stop_opr_name_;
        resource_maintenance_operation stop_operation_;

        // =-=-=-=-=-=-=-
        /// @brief Pointers to Child and Parent Resources
        resource_child_map  children_;
        resource_ptr        parent_;
                               
        // =-=-=-=-=-=-=-
        /// @brief Map holding resource properties
        resource_property_map properties_;
        
        // =-=-=-=-=-=-=- 
        /// @brief operations to be loaded from the plugin
        lookup_table< operation_wrapper > operations_;    

    private:

        // =-=-=-=-=-=-=-
        // function which checks incoming parameters for the operator() to reduce
        // code duplication in the morass of template overloads
        error check_operation_params( const std::string&, rsComm_t*, first_class_object* );

       
    }; // class resource

    // =-=-=-=-=-=-=-
    // given the name of a resource, try to load the shared object
    error load_resource_plugin( resource_ptr&,       // plugin
                                const std::string,   // plugin name
                                const std::string,   // instance name
                                const std::string ); // context string


}; // namespace eirods


#endif // ___EIRODS_RESC_PLUGIN_H__



