#ifndef __IRODS_MS_PLUGIN_HPP__
#define __IRODS_MS_PLUGIN_HPP__

// =-=-=-=-=-=-=-
// STL Includes
#include <string>

// =-=-=-=-=-=-=-
// dlopen, etc
#include <dlfcn.h>

// =-=-=-=-=-=-=-
// My Includes
#include "irods_lookup_table.hpp"
#include "irods_plugin_base.hpp"

namespace irods {

// =-=-=-=-=-=-=-
    /**
     * \author Jason M. Coposky
     * \brief  This is to be used by a microservice developer to provide a dynamic plugin
               to the microservice table found in server/re/include/reActions.header.
               Reference server/re/src/rules.cpp for loading and
               server/re/src/arithemetic.cpp for invocation.
     *
     **/
    class ms_table_entry : public plugin_base {
        public:

            typedef int ( *ms_func_ptr )( ... );

            // =-=-=-=-=-=-=-
            // Attributes
            int          num_args_;
            ms_func_ptr  call_action_;

            // =-=-=-=-=-=-=-
            // Constructors
            ms_table_entry( );

            // =-=-=-=-=-=-=-
            // NOTE :: this ctor should be called by plugin authors
            ms_table_entry(
                int ); // num ms args

            // =-=-=-=-=-=-=-
            // NOTE :: called internally for static plugins
            ms_table_entry(
                const std::string&,  // ms name
                int,                 // num ms args
                ms_func_ptr );       // function pointer

            // =-=-=-=-=-=-=-
            // copy ctor
            ms_table_entry( const ms_table_entry& _rhs );

            // =-=-=-=-=-=-=-
            // Assignment Operator - necessary for stl containers
            ms_table_entry& operator=( const ms_table_entry& _rhs );

            // =-=-=-=-=-=-=-
            // Destructor
            virtual ~ms_table_entry();

            // =-=-=-=-=-=-=-
            // Lazy Loader for MS Fcn Ptr
            error delay_load( void* _h );

            void add_operation(
                    const std::string& _op,
                    const std::string& _fn ) {
                ops_for_delay_load_.push_back( std::make_pair( _op, _fn ) );
            }

    }; // class ms_table_entry

// =-=-=-=-=-=-=-
// create a lookup table for ms_table_entry value type
    typedef lookup_table<ms_table_entry*> ms_table;

// =-=-=-=-=-=-=-
// given the name of a microservice, try to load the shared object
// and then register that ms with the table
    error load_microservice_plugin( ms_table& _table, const std::string _ms );


}; // namespace irods

#endif // __IRODS_MS_PLUGIN_HPP__

