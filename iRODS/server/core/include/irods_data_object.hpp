/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef __IRODS_DATA_OBJECT_HPP__
#define __IRODS_DATA_OBJECT_HPP__

// =-=-=-=-=-=-=-
#include "irods_first_class_object.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "rcConnect.hpp"


namespace irods {
// =-=-=-=-=-=-=-
// base class for all object types
    class data_object : public first_class_object {
    public:
        // =-=-=-=-=-=-=-
        // Constructors
        data_object();
        data_object(
            const std::string&, // phy path
            const std::string&, // resc hier
            int,                // mode
            int );              // flags
        data_object( const data_object& );

        // =-=-=-=-=-=-=-
        // Destructor
        virtual ~data_object();

        // =-=-=-=-=-=-=-
        // Operators
        virtual data_object& operator=( const data_object& );

        // =-=-=-=-=-=-=-
        // plugin resolution operators
        virtual error resolve(
            const std::string&, // plugin interface name
            plugin_ptr& ) = 0;  // resolved plugin instance

        // =-=-=-=-=-=-=-
        // accessor for rule engine variables
        virtual error get_re_vars( keyValPair_t& );

        // =-=-=-=-=-=-=-
        // Accessors

        virtual std::string physical_path()   const {
            return physical_path_;
        }
        virtual std::string resc_hier()       const {
            return resc_hier_;
        }
        virtual int         mode()            const {
            return mode_;
        }
        virtual int         flags()           const {
            return flags_;
        }

        // =-=-=-=-=-=-=-
        // Mutators
        virtual void physical_path( const std::string& _path ) {
            physical_path_   = _path;
        }
        virtual void resc_hier( const std::string& _hier )     {
            resc_hier_       = _hier;
        }
        virtual void mode( int _m )                         {
            mode_            = _m;
        }
        virtual void flags( int _f )                         {
            flags_           = _f;
        }

    protected:
        // =-=-=-=-=-=-=-
        // Attributes
        // NOTE :: These are not guaranteed to be properly populated right now
        //      :: that will need be done later when these changes are pushed
        //      :: higher in the original design
        std::string physical_path_;   // full physical path in the vault
        std::string resc_hier_;       // where this lives in the resource hierarchy
        int         mode_;            // mode when opened or modified
        int         flags_;           // flags for object operations

    }; // class data_object

/// =-=-=-=-=-=-=-
/// @brief typedef for managed data object pointer
    typedef boost::shared_ptr< data_object > data_object_ptr;

}; // namespace irods

#endif // __IRODS_DATA_OBJECT_HPP__



