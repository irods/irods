#ifndef __IRODS_DATA_OBJECT_HPP__
#define __IRODS_DATA_OBJECT_HPP__

// =-=-=-=-=-=-=-
#include "irods_first_class_object.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "rcConnect.h"
#include "rcMisc.h"


namespace irods {

/// =-=-=-=-=-=-=-
/// @brief typedef for managed data object pointer
    class data_object;
    typedef boost::shared_ptr< data_object > data_object_ptr;

// =-=-=-=-=-=-=-
// base class for all object types
    class data_object : public first_class_object {
        public:
            // =-=-=-=-=-=-=-
            // Constructors
            data_object();
            data_object(
                const std::string&,		// phy path
                rodsLong_t,             // resc id
                int,                	// mode
                int );					// flags
            data_object(
                const std::string&,		// phy path
                rodsLong_t,             // resc id
                int,                	// mode
                int,					// flags
                const keyValPair_t& );	// cond_input
            data_object(
                const std::string&,		// phy path
                const std::string&,		// resc hier
                int,                	// mode
                int );					// flags
            data_object(
                const std::string&,		// phy path
                const std::string&,		// resc hier
                int,                	// mode
                int,					// flags
                const keyValPair_t& );	// cond_input


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
            virtual error get_re_vars( rule_engine_vars_t& );

            // =-=-=-=-=-=-=-
            // Accessors

            virtual std::string physical_path()   const {
                return physical_path_;
            }
            virtual std::string resc_hier()       const {
                return resc_hier_;
            }
            virtual long        id()              const {
                return id_;
            }
            virtual int         mode()            const {
                return mode_;
            }
            virtual int         flags()           const {
                return flags_;
            }
            virtual const keyValPair_t& cond_input()	const {
                return cond_input_;
            }
            virtual rodsLong_t resc_id() const {
                return resc_id_;
            }

            // =-=-=-=-=-=-=-
            // Mutators
            virtual void physical_path( const std::string& _path ) {
                physical_path_   = _path;
            }
            virtual void resc_hier( const std::string& _hier )     {
                resc_hier_       = _hier;
            }
            virtual void id( long _id ) {
                id_ = _id;
            }
            virtual void mode( int _m )                         {
                mode_            = _m;
            }
            virtual void flags( int _f )                         {
                flags_           = _f;
            }
            virtual void cond_input( const keyValPair_t& _cond_input ) {
                replKeyVal( &_cond_input, &cond_input_ );
            }
            virtual void resc_id( rodsLong_t _id ) {
                resc_id_ = _id;
            }

            friend void add_key_val(
                data_object_ptr&   _do,
                const std::string& _k,
                const std::string& _v );

            friend void remove_key_val(
                data_object_ptr&   _do,
                const std::string& _k );

        protected:
            // =-=-=-=-=-=-=-
            // Attributes
            // NOTE :: These are not guaranteed to be properly populated right now
            //      :: that will need be done later when these changes are pushed
            //      :: higher in the original design
            std::string  physical_path_; // full physical path in the vault
            std::string  resc_hier_;     // where this lives in the resource hierarchy
            long        id_;             // object id
            int          mode_;	         // mode when opened or modified
            int          flags_;         // flags for object operations
            keyValPair_t cond_input_;    // input key-value pairs
            rodsLong_t   resc_id_;       // leaf resource id used to generate hierarchy

    }; // class data_object

}; // namespace irods

#endif // __IRODS_DATA_OBJECT_HPP__
