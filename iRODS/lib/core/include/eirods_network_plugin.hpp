/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef ___EIRODS_NETWORK_PLUGIN_H__
#define ___EIRODS_NETWORK_PLUGIN_H__

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_plugin_context.hpp"
#include "eirods_network_types.hpp"
#include "eirods_operation_wrapper.hpp"

#include <iostream>

namespace eirods {
    typedef error (*network_maintenance_operation)( plugin_property_map& );
    // =-=-=-=-=-=-=-
    /**
     * \class 
     * \author Jason M. Coposky 
     * \date   July 2013
     * \brief  
     * 
     **/
    class network : public plugin_base {
    public:
        // =-=-=-=-=-=-=-
        /// @brief Constructors
        network( const std::string&,   // instance name
                 const std::string& ); // context

        // =-=-=-=-=-=-=-
        /// @brief Destructor
        virtual ~network();

        // =-=-=-=-=-=-=-
        /// @brief copy ctor
        network( const network& _rhs ); 

        // =-=-=-=-=-=-=-
        /// @brief Assignment Operator - necessary for stl containers
        network& operator=( const network& _rhs );

        // =-=-=-=-=-=-=-
        /// @brief override from parent plugin_base
        virtual error delay_load( void* _h );
        
        // =-=-=-=-=-=-=-
        /// @brief get a property from the map if it exists.  catch the exception in the case where
        // the template types may not match and return sucess/fail
        template< typename T >
        error get_property( const std::string& _key, T& _val ) {
            error ret = properties_.get< T >( _key, _val );
            return PASS( ret );
        } // get_property

        // =-=-=-=-=-=-=-
        /// @brief set a property in the map
        template< typename T >
        error set_property( const std::string& _key, const T& _val ) {
            error ret = properties_.set< T >( _key, _val );
            return PASS( ret );
        } // set_property

        // =-=-=-=-=-=-=-
        /// @brief interface to set start / stop functions
        void set_start_operation( const std::string& );
        void set_stop_operation ( const std::string& );

        // =-=-=-=-=-=-=-
        /// @brief interface to call start / stop functions
        error start_operation( void ) { return (*start_operation_)( properties_ ); }
        error stop_operation ( void ) { return (*stop_operation_ )( properties_ ); }

        // =-=-=-=-=-=-=-
        /// @brief default start operation
        static error default_start_operation( 
            plugin_property_map& ) {
            return SUCCESS(); 
        };
        
        // =-=-=-=-=-=-=-
        /// @brief default stop operation
        static error default_stop_operation( 
            plugin_property_map& ) { 
            return SUCCESS(); 
        };

        // =-=-=-=-=-=-=-
        /// @brief delegate the call to the operation in question to the operation wrapper, with 1 param
        error call( 
            const std::string& _op, 
            eirods::first_class_object_ptr _obj ) {
            plugin_context ctx( properties_, _obj, "" );
            return operations_[ _op ].call( ctx );
        
        } // call - 

        // =-=-=-=-=-=-=-
        /// @brief delegate the call to the operation in question to the operation wrapper, with 1 param
        template< typename T1 >
        error call( 
            const std::string& _op, 
            eirods::first_class_object_ptr _obj, 
            T1 _t1 ) {
            plugin_context ctx( properties_, _obj, "" );
            return operations_[ _op ].call< T1 >( ctx, _t1 );
        
        } // call - T1
        
        // =-=-=-=-=-=-=-
        /// @brief delegate the call to the operation in question to the operation wrapper, with 2 params
        template< typename T1, typename T2 >
        error call( 
            const std::string& _op, 
            eirods::first_class_object_ptr _obj, 
            T1 _t1, 
            T2 _t2 ) {
            plugin_context ctx( properties_, _obj, "" );
            return operations_[ _op ].call< T1, T2 >( 
                       ctx, _t1, _t2 );
        
        } // call - T1, T2

        // =-=-=-=-=-=-=-
        /// @brief delegate the call to the operation in question to the operation wrapper, with 3 params
        template< typename T1, typename T2, typename T3 >
        error call( 
            const std::string& _op, 
            eirods::first_class_object_ptr _obj, 
            T1 _t1, 
            T2 _t2, 
            T3 _t3 ) {
            plugin_context ctx( properties_, _obj, "" );
            return operations_[ _op ].call< T1, T2, T3 >( 
                       ctx, _t1, _t2, _t3 );
            
        } // call - T1, T2, T3

        // =-=-=-=-=-=-=-
        /// @brief delegate the call to the operation in question to the operation wrapper, with 4 params
        template< typename T1, typename T2, typename T3, typename T4 >
        error call( 
            const std::string& _op, 
            eirods::first_class_object_ptr _obj, 
            T1 _t1, 
            T2 _t2, 
            T3 _t3, 
            T4 _t4 ) {
            plugin_context ctx( properties_, _obj, "" );
            return  operations_[ _op ].call< T1, T2, T3, T4 >( 
                        ctx, _t1, _t2, _t3, _t4 );
            
        } // call - T1, T2, T3, T4

        // =-=-=-=-=-=-=-
        /// @brief delegate the call to the operation in question to the operation wrapper, with 5 params
        template< typename T1, typename T2, typename T3, typename T4, typename T5 >
        error call( 
            const std::string& _op, 
            eirods::first_class_object_ptr _obj, 
            T1 _t1, 
            T2 _t2, 
            T3 _t3, 
            T4 _t4, 
            T5 _t5 ) {
            plugin_context ctx( properties_, _obj, "" );
            return operations_[ _op ].call< T1, T2, T3, T4, T5 >( 
                       ctx, _t1, _t2, _t3, _t4, _t5 );
            
        } // call - T1, T2, T3, T4, T5

        // =-=-=-=-=-=-=-
        /// @brief delegate the call to the operation in question to the operation wrapper, with 6 params
        template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6 >
        error call( 
            const std::string& _op, 
            eirods::first_class_object_ptr _obj, 
            T1 _t1, 
            T2 _t2, 
            T3 _t3, 
            T4 _t4, 
            T5 _t5, 
            T6 _t6 ) {
            plugin_context ctx( properties_, _obj, "" );
            return operations_[ _op ].call< T1, T2, T3, T4, T5, T6 >( 
                       ctx, _t1, _t2, _t3, _t4, _t5, _t6 );
                                                                          
        } // call - T1, T2, T3, T4, T5, T6
 
        // =-=-=-=-=-=-=-
        /// @brief delegate the call to the operation in question to the operation wrapper, with 7 params
        template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7 >
        error call( 
            const std::string& _op, 
            eirods::first_class_object_ptr _obj, 
            T1 _t1, 
            T2 _t2, 
            T3 _t3, 
            T4 _t4, 
            T5 _t5, 
            T6 _t6, 
            T7 _t7 ) {
            plugin_context ctx( properties_, _obj, "" );
            return operations_[ _op ].call< T1, T2, T3, T4, T5, T6, T7 >( 
                       ctx, _t1, _t2, _t3, _t4, _t5, _t6, _t7 );
                                                                              
        } // call - T1, T2, T3, T4, T5, T6, T7

        // =-=-=-=-=-=-=-
        /// @brief delegate the call to the operation in question to the operation wrapper, with 8 params
        template< typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8 >
        error call( 
            const std::string& _op, 
            eirods::first_class_object_ptr _obj, 
            T1 _t1, 
            T2 _t2, 
            T3 _t3, 
            T4 _t4, 
            T5 _t5, 
            T6 _t6, 
            T7 _t7, 
            T8 _t8 ) {
            plugin_context ctx( properties_, _obj, "" );
            return operations_[ _op ].call< T1, T2, T3, T4, T5, T6, T7, T8 >( 
                       ctx, _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8 );

        } // call - T1, T2, T3, T4, T5, T6, T7, T8

    protected:

        // =-=-=-=-=-=-=-
        /// @brief Start / Stop functions - can be overwritten by plugin devs
        //         called when a plugin is loaded or unloaded for init / cleanup
        std::string start_opr_name_;
        network_maintenance_operation start_operation_;

        std::string stop_opr_name_;
        network_maintenance_operation stop_operation_;

        // =-=-=-=-=-=-=- 
        /// @brief operations to be loaded from the plugin
        lookup_table< operation_wrapper > operations_;    
       
    }; // class network

    // =-=-=-=-=-=-=-
    // given the name of a network, try to load the shared object
    error load_network_plugin( 
        network_ptr&,         // plugin
        const std::string&,   // plugin name
        const std::string&,   // instance name
        const std::string& ); // context string

}; // namespace eirods


#endif // ___EIRODS_NETWORK_PLUGIN_H__



