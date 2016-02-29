// =-=-=-=-=-=-=-
#include "irods_collection_object.hpp"
#include "irods_resource_manager.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_stacktrace.hpp"

extern irods::resource_manager resc_mgr;

namespace irods {

// =-=-=-=-=-=-=-
// public - ctor
    collection_object::collection_object() :
        data_object(),
        directory_pointer_( 0 ) {
    } // collection_object

// =-=-=-=-=-=-=-
// public - cctor
    collection_object::collection_object(
        const collection_object& _rhs ) :
        data_object( _rhs ) {
        directory_pointer_ = _rhs.directory_pointer_;

    } // cctor

// =-=-=-=-=-=-=-
// public - ctor
    collection_object::collection_object(
        const std::string& _fn,
        rodsLong_t _resc_id,
        int _m,
        int _f ) :
        data_object(
            _fn,
            _resc_id,
            _m,
            _f ),
        directory_pointer_( 0 ) {

    } // collection_object


// =-=-=-=-=-=-=-
// public - ctor
    collection_object::collection_object(
        const std::string& _fn,
        rodsLong_t _resc_id,
        int _m,
        int _f,
        const keyValPair_t& _cond_input ) :
        data_object(
            _fn,
            _resc_id,
            _m,
            _f,
            _cond_input ),
        directory_pointer_( 0 ) {

    } // collection_object

// =-=-=-=-=-=-=-
// public - ctor
    collection_object::collection_object(
        const std::string& _fn,
        const std::string& _resc_hier,
        int _m,
        int _f ) :
        data_object(
            _fn,
            _resc_hier,
            _m,
            _f ),
        directory_pointer_( 0 ) {

    } // collection_object


// =-=-=-=-=-=-=-
// public - ctor
    collection_object::collection_object(
        const std::string& _fn,
        const std::string& _resc_hier,
        int _m,
        int _f,
        const keyValPair_t& _cond_input ) :
        data_object(
            _fn,
            _resc_hier,
            _m,
            _f,
            _cond_input ),
        directory_pointer_( 0 ) {

    } // collection_object

// =-=-=-=-=-=-=-
// public - dtor
    collection_object::~collection_object() {

    } // dtor

// =-=-=-=-=-=-=-
// public - assignment operator
    collection_object& collection_object::operator=( const collection_object& _rhs ) {
        // =-=-=-=-=-=-=-
        // call base class assignment first
        data_object::operator=( _rhs );

        directory_pointer_  = _rhs.directory_pointer_;

        return *this;

    }  // operator=

// =-=-=-=-=-=-=-
// plugin - resolve resource plugin for this object
    error collection_object::resolve(
        const std::string& _interface,
        plugin_ptr&        _ptr ) {
        // =-=-=-=-=-=-=-
        // check to see if this is for a resource plugin
        // resolution, otherwise it is an error
        if ( RESOURCE_INTERFACE != _interface ) {
            std::stringstream msg;
            msg << "collection_object does not support a [";
            msg << _interface;
            msg << "] for plugin resolution";
            return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
        }

        error result = SUCCESS();
        error ret;

        hierarchy_parser hparse;
        ret = hparse.set_string( resc_hier() );

        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - ";
            msg << "error parsing resource hierarchy \"" << resc_hier() << "\"";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            std::string resc;

            ret = hparse.first_resc( resc );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__ << " - ERROR getting first resource from hierarchy.";
                result = PASSMSG( msg.str(), ret );
            }
            else {

                if ( resc.empty() && resc_hier().empty() ) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - No resource hierarchy or resource specified.";
                    return ERROR( HIERARCHY_ERROR, msg.str() );
                }
                else if ( resc.empty() ) {
                    return ERROR( HIERARCHY_ERROR, "Hierarchy string is not empty but first resource is!" );
                }

                resource_ptr resc_ptr;
                ret = resc_mgr.resolve( resc, resc_ptr );
                if ( !ret.ok() ) {
                    std::stringstream msg;
                    msg << __FUNCTION__ << " - ERROR resolving resource \"" << resc << "\"";
                    result = PASSMSG( msg.str(), ret );
                }

                _ptr = boost::dynamic_pointer_cast< resource >( resc_ptr );
            }
        }
        return result;

    } // resolve

// =-=-=-=-=-=-=-
// public - get vars from object for rule engine
    error collection_object::get_re_vars(
        rule_engine_vars_t& _kvp ) {
        data_object::get_re_vars( _kvp );
        return SUCCESS();

    } // get_re_vars


}; // namespace irods



