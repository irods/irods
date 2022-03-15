#include "irods/irods_resource_manager.hpp"

#include "irods/genQuery.h"
#include "irods/generalAdmin.h"
#include "irods/getRescQuota.h"
#include "irods/irods_children_parser.hpp"
#include "irods/irods_hierarchy_parser.hpp"
#include "irods/irods_lexical_cast.hpp"
#include "irods/irods_load_plugin.hpp"
#include "irods/irods_log.hpp"
#include "irods/irods_resource_plugin_impostor.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/irods_string_tokenize.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/rcMisc.h"
#include "irods/rsGenQuery.hpp"
#include "irods/rsGlobalExtern.hpp"

#define IRODS_QUERY_ENABLE_SERVER_SIDE_API
#include "irods/query_builder.hpp"

#include <fmt/format.h>

#include <iostream>
#include <vector>
#include <iterator>

// =-=-=-=-=-=-=-
// global singleton
irods::resource_manager resc_mgr;

namespace irods
{
    const std::string EMPTY_RESC_HOST( "EMPTY_RESC_HOST" );
    const std::string EMPTY_RESC_PATH( "EMPTY_RESC_PATH" );

    // =-=-=-=-=-=-=-
    /// @brief definition of the resource interface
    const std::string RESOURCE_INTERFACE( "irods_resource_interface" );

    // =-=-=-=-=-=-=-
    /// @brief special resource for local file system operations only
    const std::string LOCAL_USE_ONLY_RESOURCE( "LOCAL_USE_ONLY_RESOURCE" );
    const std::string LOCAL_USE_ONLY_RESOURCE_VAULT( "/var/lib/irods/LOCAL_USE_ONLY_RESOURCE_VAULT" );
    const std::string LOCAL_USE_ONLY_RESOURCE_TYPE( "unixfilesystem" );

    // =-=-=-=-=-=-=-
    // public - Constructor
    resource_manager::resource_manager() {
    } // ctor

    // =-=-=-=-=-=-=-
    // public - Copy Constructor
    resource_manager::resource_manager( const resource_manager& ) {
    } // cctor

    // =-=-=-=-=-=-=-
    // public - Destructor
    resource_manager::~resource_manager( ) {
    } // cctor

    // =-=-=-=-=-=-=-
    // public - retrieve a resource given its key
    error resource_manager::resolve(
        std::string   _key,
        resource_ptr& _value ) {

        if ( _key.empty() ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "empty key" );
        }

        if ( resource_name_map_.has_entry( _key ) ) {
            _value = resource_name_map_[ _key ];
            return SUCCESS();

        }
        else {
            std::stringstream msg;
            msg << "no resource found for name ["
                << _key << "]";
            return ERROR( SYS_RESC_DOES_NOT_EXIST, msg.str() );

        }

    } // resolve

// =-=-=-=-=-=-=-
// public - retrieve a resource given its key
    error resource_manager::resolve(
        rodsLong_t    _resc_id,
        resource_ptr& _value ) {
        if ( resource_id_map_.has_entry( _resc_id ) ) {
            _value = resource_id_map_[ _resc_id ];
            return SUCCESS();

        }
        else {
            std::stringstream msg;
            msg << "no resource found for name ["
                << _resc_id << "]";
            return ERROR( SYS_RESC_DOES_NOT_EXIST, msg.str() );

        }

    } // resolve

    error resource_manager::load_resource_plugin(
        resource_ptr&     _plugin,
        const std::string _plugin_name,
        const std::string _inst_name,
        const std::string _context ) {
        resource* resc = 0;
        error ret = load_plugin< resource >(
                        resc,
                        _plugin_name,
                        PLUGIN_TYPE_RESOURCE,
                        _inst_name,
                        _context );
        if ( ret.ok() && resc ) {
            _plugin.reset( resc );
        }
        else {
            if ( ret.code() == PLUGIN_ERROR_MISSING_SHARED_OBJECT ) {
                rodsLog(
                    LOG_DEBUG,
                    "loading impostor resource for [%s] of type [%s] with context [%s] and load_plugin message [%s]",
                    _inst_name.c_str(),
                    _plugin_name.c_str(),
                    _context.c_str(),
                    ret.result().c_str());
                _plugin.reset(
                    new impostor_resource(
                        "impostor_resource", "" ) );
            } else {
                return PASS( ret );
            }
        }

        return SUCCESS();

    } // load_resource_plugin

// =-=-=-=-=-=-=-
// public - retrieve a resource given a vault path
    error resource_manager::validate_vault_path(
        std::string       _physical_path,
        rodsServerHost_t* _svr_host,
        std::string&      _out_path ) {
        // =-=-=-=-=-=-=-
        // simple flag to state a resource matching the prop and value is found
        bool found = false;

        if (!_svr_host) {
            return ERROR(USER__NULL_INPUT_ERR, "empty server host");
        }

        // =-=-=-=-=-=-=-
        // quick check on the resource table
        if ( resource_name_map_.empty() ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "empty resource table" );
        }

        // =-=-=-=-=-=-=-
        // quick check on the path that it has something in it
        if ( _physical_path.empty() ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "empty property" );
        }

        // =-=-=-=-=-=-=-
        // iterate through the map and search for our path
        lookup_table< resource_ptr >::iterator itr = resource_name_map_.begin();
        for ( ; !found && itr != resource_name_map_.end(); ++itr ) {
            // =-=-=-=-=-=-=-
            // get the host pointer from the resource
            rodsServerHost_t* svr_host = 0;
            error ret = itr->second->get_property< rodsServerHost_t* >( RESOURCE_HOST, svr_host );
            if ( !ret.ok() ) {
                PASS( ret );
            }

            // =-=-=-=-=-=-=-
            // if this host matches the incoming host pointer then were good
            // otherwise continue searching
            if ( svr_host != _svr_host ) {
                continue;
            }

            // =-=-=-=-=-=-=-
            // query resource for the property value
            std::string path;
            ret = itr->second->get_property<std::string>( RESOURCE_PATH, path );

            // =-=-=-=-=-=-=-
            // if we get a good parameter and do not match non-storage nodes with an empty physical path
            if ( ret.ok() ) {
                // =-=-=-=-=-=-=-
                // compare incoming value and stored value
                // one may be a subset of the other so compare both ways
                if ( !path.empty() && ( _physical_path.find( path ) != std::string::npos ) ) {
                    found     = true;
                    _out_path = path;
                }

            }
            else {
                std::stringstream msg;
                msg << "resource_manager::resolve_from_physical_path - ";
                msg << "failed to get vault parameter from resource";
                msg << ret.code();
                irods::log( PASSMSG( msg.str(), ret ) );

            }

        } // for itr

        // =-=-=-=-=-=-=-
        // did we find a resource and is the ptr valid?
        if ( true == found ) {
            return SUCCESS();
        }
        else {
            std::stringstream msg;
            msg << "failed to find resource for path [";
            msg << _physical_path;
            msg << "]";
            return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
        }

    } // validate_vault_path

// =-=-=-=-=-=-=-
// public - connect to the catalog and query for all the
//          attached resources and instantiate them
    error resource_manager::init_from_catalog(rsComm_t& _comm)
    {
        resource_name_map_.clear();

        constexpr char* query_str = "select "
                                    "RESC_ID, "             // 0
                                    "RESC_NAME, "           // 1
                                    "RESC_ZONE_NAME, "      // 2
                                    "RESC_TYPE_NAME, "      // 3
                                    "RESC_CLASS_NAME, "     // 4
                                    "RESC_LOC, "            // 5
                                    "RESC_VAULT_PATH, "     // 6
                                    "RESC_FREE_SPACE, "     // 7
                                    "RESC_INFO, "           // 8
                                    "RESC_COMMENT, "        // 9
                                    "RESC_CREATE_TIME, "    // 10
                                    "RESC_MODIFY_TIME, "    // 11
                                    "RESC_STATUS, "         // 12
                                    "RESC_CHILDREN, "       // 13
                                    "RESC_CONTEXT, "        // 14
                                    "RESC_PARENT, "         // 15
                                    "RESC_PARENT_CONTEXT";  // 16

        for (auto&& row : irods::experimental::query_builder{}.build(_comm, query_str)) {
            const auto& name = row.at(1);
            const auto& type = row.at(3);
            const auto& context = row.at(14);

            resource_ptr resc;
            if (const auto ret = load_resource_plugin(resc, type, name, context); !ret.ok()) {
                irods::log(PASS(ret));
                continue;
            }

            const auto& location = row.at(5);
            if (irods::EMPTY_RESC_HOST != location) {
                const auto& zone_name = row.at(2);
                rodsHostAddr_t addr{};
                std::strncpy(addr.hostAddr, location.c_str(), LONG_NAME_LEN);
                std::strncpy(addr.zoneName, zone_name.c_str(), NAME_LEN);

                rodsServerHost_t* host = nullptr;
                if (resolveHost(&addr, &host) < 0) {
                    irods::log(LOG_NOTICE, fmt::format(
                        fmt::runtime("[{}:{}] - resolveHost error for [{}]"),
                        __func__, __LINE__, addr.hostAddr));
                }

                resc->set_property<rodsServerHost_t*>(RESOURCE_HOST, host);
            }
            else {
                resc->set_property<rodsServerHost_t*>(RESOURCE_HOST, nullptr);
            }

            const rodsLong_t id = std::strtoll(row.at(0).c_str(), 0, 0);
            resc->set_property<rodsLong_t>(RESOURCE_ID, id);
            resc->set_property<long>(RESOURCE_QUOTA, RESC_QUOTA_UNINIT);

            const auto& status = row.at(12);
            resc->set_property(RESOURCE_STATUS, status == RESC_DOWN
                                                ? INT_RESC_STATUS_DOWN
                                                : INT_RESC_STATUS_UP);

            resc->set_property<std::string>(RESOURCE_FREESPACE,      row.at(7));
            resc->set_property<std::string>(RESOURCE_ZONE,           row.at(2));
            resc->set_property<std::string>(RESOURCE_NAME,           name);
            resc->set_property<std::string>(RESOURCE_LOCATION,       location);
            resc->set_property<std::string>(RESOURCE_TYPE,           type);
            resc->set_property<std::string>(RESOURCE_CLASS,          row.at(4));
            resc->set_property<std::string>(RESOURCE_PATH,           row.at(6));
            resc->set_property<std::string>(RESOURCE_INFO,           row.at(8));
            resc->set_property<std::string>(RESOURCE_COMMENTS,       row.at(9));
            resc->set_property<std::string>(RESOURCE_CREATE_TS,      row.at(10));
            resc->set_property<std::string>(RESOURCE_MODIFY_TS,      row.at(11));
            resc->set_property<std::string>(RESOURCE_CHILDREN,       row.at(13));
            resc->set_property<std::string>(RESOURCE_CONTEXT,        context);
            resc->set_property<std::string>(RESOURCE_PARENT,         row.at(15));
            resc->set_property<std::string>(RESOURCE_PARENT_CONTEXT, row.at(16));

            resource_name_map_[name] = resc;
            resource_id_map_[id] = resc;
        }

        // =-=-=-=-=-=-=-
        // Update child resource maps
        if (const auto proc_ret = init_child_map(); !proc_ret.ok()) {
            return PASSMSG( "init_child_map failed.", proc_ret );
        }

        // =-=-=-=-=-=-=-
        // gather the post disconnect maintenance operations
        error op_ret = gather_operations();
        if ( !op_ret.ok() ) {
            return PASSMSG( "gather_operations failed.", op_ret );
        }

        // =-=-=-=-=-=-=-
        // call start for plugins
        error start_err = start_resource_plugins();
        if ( !start_err.ok() ) {
            return PASSMSG( "start_resource_plugins failed.", start_err );
        }

        // =-=-=-=-=-=-=-
        // win!
        return SUCCESS();
    } // init_from_catalog

// =-=-=-=-=-=-=-
/// @brief call shutdown on resources before destruction
    error resource_manager::shut_down_resources( ) {
        // =-=-=-=-=-=-=-
        // iterate over all resources in the table
        lookup_table< boost::shared_ptr< resource > >::iterator itr;
        for ( itr =  resource_name_map_.begin();
                itr != resource_name_map_.end();
                ++itr ) {
            itr->second->stop_operation();

        } // for itr

        return SUCCESS();

    } // shut_down_resources

// =-=-=-=-=-=-=-
/// @brief create a list of resources who do not have parents ( roots )
    error resource_manager::get_root_resources(
        std::vector< std::string >& _list ) {
        // =-=-=-=-=-=-=-
        // iterate over all resources in the table
        lookup_table< boost::shared_ptr< resource > >::iterator itr;
        for ( itr =  resource_name_map_.begin();
                itr != resource_name_map_.end();
                ++itr ) {
            resource_ptr resc = itr->second;
            resource_ptr parent_ptr;
            error ret = itr->second->get_parent( parent_ptr );
            if ( !ret.ok() ) {
                std::string resc_name;
                ret = resc->get_property< std::string >( RESOURCE_NAME, resc_name );
                if ( !ret.ok() ) {
                    return PASS( ret );
                }

                _list.push_back( resc_name );

            }

        } // for itr

        return SUCCESS();

    } // get_root_resources

    error resource_manager::get_parent_name(
        resource_ptr _resc,
        std::string& _name ) {

        std::string my_name;
        error ret = _resc->get_property<std::string>(
                        RESOURCE_NAME,
                        my_name);
        if(!ret.ok()) {
            return PASS(ret);
        }

        std::string parent_id_str;
        ret = _resc->get_property<std::string>(
                        RESOURCE_PARENT,
                        parent_id_str);
        if(!ret.ok()) {
            return PASS(ret);
        }

        if(parent_id_str.empty()) {
            std::stringstream msg;
            return ERROR(
                    HIERARCHY_ERROR,
                    "empty parent string");
        }

        rodsLong_t parent_id;
        ret = lexical_cast<rodsLong_t>(
                  parent_id_str,
                  parent_id);
        if(!ret.ok()) {
            return PASS(ret);
        }

        resource_ptr parent_resc;
        ret = resolve(
                parent_id,
                parent_resc);
        if(!ret.ok()) {
            return PASS(ret);
        }

        return parent_resc->get_property<std::string>(
                   RESOURCE_NAME,
                   _name );
    } // get_parent_name

    std::string resource_manager::get_hier_to_root_for_resc(std::string_view _resource_name)
    {
        if (_resource_name.empty()) {
            return {};
        }

        irods::hierarchy_parser hierarchy{_resource_name.data()};

        for (std::string parent_name = _resource_name.data(); !parent_name.empty();) {
            resource_ptr resc;

            if (const auto ret = resolve(parent_name, resc); !ret.ok()) {
                THROW(ret.code(), ret.result());
            }

            if (const auto ret = get_parent_name(resc, parent_name); !ret.ok()) {
                if(HIERARCHY_ERROR == ret.code()) {
                    break;
                }

                THROW(ret.code(), ret.result());
            }

            if(!parent_name.empty()) {
                hierarchy.add_parent(parent_name);
            }
        }

        return hierarchy.str();
    } // get_hier_to_root_for_resc

    error resource_manager::get_hier_to_root_for_resc(
        const std::string& _resc_name,
        std::string&       _hierarchy ) {

        _hierarchy = _resc_name;
        std::string parent_name = _resc_name;

        resource_ptr resc;
        while( !parent_name.empty() ) {
            error ret = resolve(
                            parent_name,
                            resc);
            if(!ret.ok()) {
                return PASS(ret);
            }

            ret = get_parent_name(
                      resc,
                      parent_name );
            if(!ret.ok()) {
                if(HIERARCHY_ERROR == ret.code()) {
                    break;
                }

                return PASS(ret);
            }

            if(!parent_name.empty()) {
                _hierarchy = parent_name +
                             irods::hierarchy_parser::delimiter() +
                             _hierarchy;
            }
        } // while

        return SUCCESS();

    } // get_hier_to_root_for_resc

    error resource_manager::gather_leaf_bundle_for_child(
        const std::string& _resc_name,
        leaf_bundle_t&     _bundle ) {
        resource_ptr resc;
        error ret = resolve(
                        _resc_name,
                        resc);
        if(!ret.ok()) {
            return PASS(ret);
        }

        if(resc->num_children() > 0 ) {
            // still more children to traverse
            std::vector<std::string> children;
            resc->children(children);

            for( size_t idx = 0;
                 idx < children.size();
                 ++idx ) {

                ret = gather_leaf_bundle_for_child(
                          children[idx],
                          _bundle );
                if(!ret.ok() ) {
                    return PASS(ret);
                }

            } // for idx
        }
        else {
            // we have found a leaf
            rodsLong_t resc_id;
            ret = resc->get_property<rodsLong_t>(
                    RESOURCE_ID,
                    resc_id);
            if(!ret.ok()) {
                return PASS(ret);
            }

            _bundle.push_back( resc_id );
        }

        return SUCCESS();

    } // gather_leaf_bundle_for_child

    // throws irods::exception
    std::vector<resource_manager::leaf_bundle_t> resource_manager::gather_leaf_bundles_for_resc(const std::string& _resource_name) {
        resource_ptr resc;
        error err = resolve(_resource_name, resc);
        if (!err.ok()) {
            THROW(err.code(), err.result());
        }

        std::vector<std::string> children;
        resc->children(children);
        std::vector<leaf_bundle_t> ret;
        ret.resize(children.size());
        for (size_t i=0; i<children.size(); ++i) {
            err = gather_leaf_bundle_for_child(children[i], ret[i]);
            if (!err.ok()) {
                THROW(err.code(), err.result());
            }
            std::sort(std::begin(ret[i]), std::end(ret[i]));
        }
        return ret;
    }

// =-=-=-=-=-=-=-
// public - given a type, load up a resource plugin
    error resource_manager::init_from_type( std::string   _type,
                                            std::string   _key,
                                            std::string   _inst,
                                            std::string   _ctx,
                                            resource_ptr& _resc ) {
        // =-=-=-=-=-=-=-
        // create the resource and add properties for column values
        error ret = load_resource_plugin( _resc, _type, _inst, _ctx );
        if ( !ret.ok() ) {
            return PASSMSG( "Failed to load Resource Plugin", ret );
        }

        resource_name_map_[ _key ] = _resc;

        return SUCCESS();

    } // init_from_type

// =-=-=-=-=-=-=-
// private - walk the resource map and wire children up to parents
    error resource_manager::init_child_map( void ) {
        for( auto itr : resource_name_map_ ) {
            const std::string& child_name = itr.first;
            resource_ptr       child_resc = itr.second;

            std::string parent_id_str;
            error ret = child_resc->get_property<std::string>(
                            RESOURCE_PARENT,
                            parent_id_str );
            if(!ret.ok() || parent_id_str.empty()) {
                continue;
            }

            rodsLong_t parent_id = 0;
            ret = lexical_cast<rodsLong_t>(
                      parent_id_str,
                      parent_id );
            if(!ret.ok()) {
                irods::log(PASS(ret));
                continue;
            }

            if(!resource_id_map_.has_entry(parent_id)) {
                rodsLog(
                    LOG_ERROR,
                    "invalid parent resource id %ld",
                    parent_id );
                continue;
            }

            resource_ptr parent_resc = resource_id_map_[parent_id];

            std::string parent_child_context;
            ret = child_resc->get_property<std::string>(
                      RESOURCE_PARENT_CONTEXT,
                      parent_child_context );
            if(!ret.ok()) {
            }

            parent_resc->add_child(
                child_name,
                parent_child_context,
                child_resc);

            child_resc->set_parent(parent_resc);

            rodsLog(
                LOG_DEBUG,
                "%s - add [%s][%s] to [%ld]",
                __FUNCTION__,
                child_name.c_str(),
                parent_child_context.c_str(),
                parent_id);

        } // for itr

        return SUCCESS();

    } // init_child_map

// =-=-=-=-=-=-=-
// public - print the list of local resources out to stderr
    void resource_manager::print_local_resources() {
        lookup_table< boost::shared_ptr< resource > >::iterator itr;
        for ( itr = resource_name_map_.begin(); itr != resource_name_map_.end(); ++itr ) {
            std::string loc, path, name;
            error path_err = itr->second->get_property< std::string >( RESOURCE_PATH, path );
            error loc_err  = itr->second->get_property< std::string >( RESOURCE_LOCATION, loc );
            if ( path_err.ok() && loc_err.ok() && "localhost" == loc ) {
                rodsLog( LOG_NOTICE, "   RescName: %s, VaultPath: %s\n",
                         itr->first.c_str(), path.c_str() );
            }

        } // for itr

    } // print_local_resources

// =-=-=-=-=-=-=-
// private - gather the post disconnect maintenance operations
//           from the resource plugins
    error resource_manager::gather_operations() {
        // =-=-=-=-=-=-=-
        // vector of already processed resources
        std::vector< std::string > proc_vec;

        // =-=-=-=-=-=-=-
        // iterate over all of the resources
        lookup_table< boost::shared_ptr< resource > >::iterator resc_itr;
        for ( resc_itr = resource_name_map_.begin(); resc_itr != resource_name_map_.end(); ++resc_itr ) {
            resource_ptr& resc = resc_itr->second;

            // =-=-=-=-=-=-=-
            // skip if already processed
            std::string name;
            error get_err = resc->get_property< std::string >( RESOURCE_NAME, name );

            if ( get_err.ok() ) {
                std::vector< std::string >::iterator itr;
                itr = std::find< std::vector< std::string >::iterator, std::string >( proc_vec.begin(), proc_vec.end(), name );
                if ( proc_vec.end() != itr ) {
                    continue;
                }
            }
            else {
                std::stringstream msg;
                msg << "resource_manager::gather_operations - failed to get property ";
                msg << "[name] for resource";
                return PASSMSG( msg.str(), get_err );
            }

            // =-=-=-=-=-=-=-
            // vector which will hold this 'top level resource' ops
            std::vector< pdmo_type > resc_ops;

            // =-=-=-=-=-=-=-
            // cache the parent operator
            pdmo_type pdmo_op;
            error pdmo_err = resc->post_disconnect_maintenance_operation( pdmo_op );
            if ( pdmo_err.ok() ) {
                resc_ops.push_back( pdmo_op );
            }

            // =-=-=-=-=-=-=-
            // mark this resource done
            proc_vec.push_back( name );

            // =-=-=-=-=-=-=-
            // dive if children are present
            std::string child_str;
            error child_err = resc->get_property< std::string >( RESOURCE_CHILDREN, child_str );
            if ( child_err.ok() && !child_str.empty() ) {
                gather_operations_recursive( child_str, proc_vec, resc_ops );
            }

            // =-=-=-=-=-=-=-
            // if we got ops, add vector of ops to mgr's vector
            if ( !resc_ops.empty() ) {
                maintenance_operations_.push_back( resc_ops );
            }

        } // for itr

        return SUCCESS();

    } // gather_operations

// =-=-=-=-=-=-=-
/// private - lower level recursive call to gather the post disconnect
//            maintenance operations from the resources, in breadth first order
    error resource_manager::gather_operations_recursive( const std::string&          _children,
            std::vector< std::string >& _proc_vec,
            std::vector< pdmo_type  >&  _resc_ops ) {
        // =-=-=-=-=-=-=-
        // create a child parser to traverse the list
        children_parser parser;
        parser.set_string( _children );
        children_parser::children_map_t children_list;
        error ret = parser.list( children_list );
        if ( !ret.ok() ) {
            return PASSMSG( "gather_operations_recursive failed.", ret );
        }

        // =-=-=-=-=-=-=-
        // iterate over all of the children, cache the operators
        children_parser::children_map_t::const_iterator itr;
        for ( itr = children_list.begin(); itr != children_list.end(); ++itr ) {
            std::string child = itr->first;

            // =-=-=-=-=-=-=-
            // lookup the child resource pointer
            resource_ptr resc;
            error get_err = resource_name_map_.get( child, resc );
            if ( get_err.ok() ) {
                // =-=-=-=-=-=-=-
                // cache operation if there is one
                pdmo_type pdmo_op;
                error pdmo_ret = resc->post_disconnect_maintenance_operation( pdmo_op );
                if ( pdmo_ret.ok() ) {
                    _resc_ops.push_back( pdmo_op );
                }

                // =-=-=-=-=-=-=-
                // mark this child as done
                _proc_vec.push_back( child );

            }
            else {
                std::stringstream msg;
                msg << "failed to get resource for key [";
                msg << child;
                msg << "]";
                return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
            }

        } // for itr

        // =-=-=-=-=-=-=-
        // iterate over all of the children again, recurse if they have more children
        for ( itr = children_list.begin(); itr != children_list.end(); ++itr ) {
            std::string child = itr->first;

            // =-=-=-=-=-=-=-
            // lookup the child resource pointer
            resource_ptr resc;
            error get_err = resource_name_map_.get( child, resc );
            if ( get_err.ok() ) {
                std::string child_str;
                error child_err = resc->get_property< std::string >( RESOURCE_CHILDREN, child_str );
                if ( child_err.ok() && !child_str.empty() ) {
                    error gather_err = gather_operations_recursive( child_str, _proc_vec, _resc_ops );
                }

            }
            else {
                std::stringstream msg;
                msg << "failed to get resource for key [";
                msg << child;
                msg << "]";
                return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
            }

        } // for itr

        return SUCCESS();

    } // gather_operations_recursive

// =-=-=-=-=-=-=-
// public - call the start op on the resource plugins
    error resource_manager::start_resource_plugins( ) {
        // =-=-=-=-=-=-=-
        // iterate through resource plugins
        lookup_table< resource_ptr >::iterator itr;
        for ( itr  = resource_name_map_.begin();
                itr != resource_name_map_.end();
                ++itr ) {
            // =-=-=-=-=-=-=-
            // if any resources need a pdmo, return true;
            error ret = itr->second->start_operation( );
            if ( !ret.ok() ) {
                irods::log( ret );
            }

        } // for itr

        return SUCCESS();

    } // start_resource_operations


// =-=-=-=-=-=-=-
// public - exec the pdmos ( post disconnect maintenance operations ) in order
    bool resource_manager::need_maintenance_operations( ) {
        bool need_pdmo = false;

        // =-=-=-=-=-=-=-
        // iterate through resource plugins
        lookup_table< resource_ptr >::iterator itr;
        for ( itr  = resource_name_map_.begin();
                itr != resource_name_map_.end();
                ++itr ) {
            // =-=-=-=-=-=-=-
            // if any resources need a pdmo, return true;
            bool flg = false;
            itr->second->need_post_disconnect_maintenance_operation( flg );
            if ( flg ) {
                need_pdmo = true;
                break;
            }

        } // for itr

        return need_pdmo;

    } // need_maintenance_operations

// =-=-=-=-=-=-=-
// public - exec the pdmos ( post disconnect maintenance operations ) in order
    int resource_manager::call_maintenance_operations( rcComm_t* _comm ) {
        int result = 0;

        // =-=-=-=-=-=-=-
        // iterate through op vectors
        std::vector< std::vector< pdmo_type > >::iterator vec_itr;
        for ( vec_itr  = maintenance_operations_.begin();
                vec_itr != maintenance_operations_.end();
                ++vec_itr ) {
            // =-=-=-=-=-=-=-
            // iterate through ops
            std::vector< pdmo_type >::iterator op_itr;
            for ( op_itr  = vec_itr->begin();
                    op_itr != vec_itr->end();
                    ++op_itr ) {
                // =-=-=-=-=-=-=-
                // call the op
                try {
                    error ret = ( ( *op_itr ) )( _comm );
                    if ( !ret.ok() ) {
                        log( PASSMSG( "resource_manager::call_maintenance_operations - op failed", ret ) );
                        result = ret.code();
                    }
                }
                catch ( const boost::bad_function_call& ) {
                    rodsLog( LOG_ERROR, "maintenance operation threw boost::bad_function_call" );
                    result = SYS_INTERNAL_ERR;
                }

            } // for op_itr

        } // for vec_itr

        return result;
    } // call_maintenance_operations

    /*
     * construct a vector of all resource hierarchies in the system
     * throws irods::exception
     */
    std::vector<std::string> resource_manager::get_all_resc_hierarchies( void ) {
        std::vector<std::string> hier_list;
        for ( const auto& entry : resource_name_map_ ) {
            const resource_ptr resc = entry.second;
            if ( resc->num_children() > 0 ) {
                continue;
            }

            rodsLong_t leaf_id = 0;
            error err = resc->get_property<rodsLong_t>(
                            RESOURCE_ID,
                            leaf_id );
            if ( !err.ok() ) {
                THROW( err.code(), err.result() );
            }

            std::string curr_hier;
            err = leaf_id_to_hier( leaf_id, curr_hier );
            if ( !err.ok() ) {
                THROW( err.code(), err.result() );
            }
            hier_list.push_back( curr_hier );
        }
        return hier_list;
    } // get_all_resc_hierarchies

    rodsLong_t resource_manager::hier_to_leaf_id(std::string_view _hierarchy)
    {
        if (_hierarchy.empty()) {
            THROW(HIERARCHY_ERROR, "empty hierarchy string");
        }

        const std::string leaf = irods::hierarchy_parser{_hierarchy.data()}.last_resc();
        if (!resource_name_map_.has_entry(leaf)) {
            THROW(SYS_RESC_DOES_NOT_EXIST, leaf);
        }

        rodsLong_t id = 0;
        const resource_ptr resc = resource_name_map_[leaf];
        if (const error ret = resc->get_property<rodsLong_t>(RESOURCE_ID, id); !ret.ok()) {
            THROW(ret.code(), ret.result());
        }
        return id;
    } // hier_to_leaf_id

    error resource_manager::hier_to_leaf_id(
        const std::string& _hier,
        rodsLong_t&        _id ) {
        if(_hier.empty()) {
            return ERROR(
                       HIERARCHY_ERROR,
                       "empty hierarchy string" );
        }

        hierarchy_parser p;
        p.set_string( _hier );

        std::string leaf;
        p.last_resc( leaf );

        if( !resource_name_map_.has_entry(leaf) ) {
            return ERROR(
                       SYS_RESC_DOES_NOT_EXIST,
                       leaf );
        }

        resource_ptr resc = resource_name_map_[leaf];

        rodsLong_t id = 0;
        error ret = resc->get_property<rodsLong_t>(
                        RESOURCE_ID,
                        id );
        if( !ret.ok() ) {
            return PASS(ret);
        }

        _id = id;

        return SUCCESS();

    } // hier_to_leaf_id

    std::string resource_manager::leaf_id_to_hier(const rodsLong_t _leaf_resource_id)
    {
        if(!resource_id_map_.has_entry(_leaf_resource_id)) {
            THROW(SYS_RESC_DOES_NOT_EXIST, fmt::format("invalid resource id: {}", _leaf_resource_id));
        }

        resource_ptr resc = resource_id_map_[_leaf_resource_id];

        std::string leaf_name;
        if (const error ret = resc->get_property<std::string>(RESOURCE_NAME, leaf_name); !ret.ok()) {
            THROW(ret.code(), ret.result());
        }

        irods::hierarchy_parser parser{leaf_name};

        resc->get_parent(resc);
        while(resc.get()) {
            std::string name;

            if (const error ret = resc->get_property<std::string>(RESOURCE_NAME, name); !ret.ok()) {
                THROW(ret.code(), ret.result());
            }

            parser.add_parent(name);

            resc->get_parent(resc);
        }

        return parser.str();
    } // leaf_id_to_hier

    error resource_manager::leaf_id_to_hier(
        const rodsLong_t& _id,
        std::string&      _hier ) {
        if( !resource_id_map_.has_entry(_id) ) {
            std::stringstream msg;
            msg << "invalid resource id: " << _id;
            return ERROR(
                       SYS_RESC_DOES_NOT_EXIST,
                       msg.str() );
        }

        resource_ptr resc = resource_id_map_[ _id ];

        std::string hier;
        error ret = resc->get_property<std::string>(
                        RESOURCE_NAME,
                        hier );
        if( !ret.ok() ) {
            return PASS(ret);
        }

        resc->get_parent(resc);
        while( resc.get() ) {
            std::string name;
            error ret = resc->get_property<std::string>(
                            RESOURCE_NAME,
                            name );
            if( !ret.ok() ) {
                return PASS(ret);
            }

            hier.insert( 0, ";" );
            hier.insert( 0, name);

            resc->get_parent(resc);
        }

        _hier = hier;

        return SUCCESS();

    } // leaf_id_to_hier

    error resource_manager::resc_id_to_name(
        const rodsLong_t& _id,
        std::string&      _name ) {
        // parent name might be 'empty'
        if(!_id) {
            return SUCCESS();
        }

        if( !resource_id_map_.has_entry(_id) ) {
            std::stringstream msg;
            msg << "invalid resource id: " << _id;
            return ERROR(
                       SYS_RESC_DOES_NOT_EXIST,
                       msg.str() );
        }

        resource_ptr resc = resource_id_map_[ _id ];

        std::string hier;
        error ret = resc->get_property<std::string>(
                        RESOURCE_NAME,
                        _name );
        if( !ret.ok() ) {
            return PASS(ret);
        }

        return SUCCESS();

    } // resc_id_to_name

    error resource_manager::resc_id_to_name(
        const std::string& _id_str,
        std::string&       _name ) {
        // parent name might be 'empty'
        if("0" == _id_str || _id_str.empty()) {
            return SUCCESS();
        }

        rodsLong_t resc_id = 0;
        error ret = lexical_cast<rodsLong_t>(
                       _id_str,
                       resc_id );
        if(!ret.ok()) {
            return PASS(ret);
        }

        if( !resource_id_map_.has_entry(resc_id) ) {
            std::stringstream msg;
            msg << "invalid resource id: " << _id_str;
            return ERROR(
                       SYS_RESC_DOES_NOT_EXIST,
                       msg.str() );
        }
        resource_ptr resc = resource_id_map_[ resc_id ];

        std::string hier;
        ret = resc->get_property<std::string>(
                        RESOURCE_NAME,
                        _name );
        if( !ret.ok() ) {
            return PASS(ret);
        }

        return SUCCESS();

    } // resc_id_to_name

    std::string resource_manager::resc_id_to_name(const rodsLong_t& _id)
    {
        if(!_id) {
            return {};
        }

        if(!resource_id_map_.has_entry(_id)) {
            THROW(SYS_RESC_DOES_NOT_EXIST, fmt::format("invalid resource id: {}", _id));
        }

        std::string resource_name;
        const resource_ptr resc = resource_id_map_[_id];
        if (const error ret = resc->get_property<std::string>(RESOURCE_NAME, resource_name); !ret.ok()) {
            THROW(ret.code(), ret.result());
        }
        return resource_name;
    } // resc_id_to_name

    std::string resource_manager::resc_id_to_name(std::string_view _id)
    {
        if(_id.empty()) {
            return {};
        }

        rodsLong_t id = 0;
        if (const error ret = lexical_cast<rodsLong_t>(_id, id); !ret.ok()) {
            THROW(ret.code(), ret.result());
        }
        return resc_id_to_name(id);
    } // resc_id_to_name

    error resource_manager::is_coordinating_resource(
        const std::string& _resc_name,
        bool&              _ret ) {
        if( !resource_name_map_.has_entry(_resc_name) ) {
            return ERROR(
                       SYS_RESC_DOES_NOT_EXIST,
                       _resc_name );
        }

        resource_ptr resc = resource_name_map_[_resc_name];

        _ret = resc->num_children() > 0;

        return SUCCESS();
    } // is_coordinating_resource

    bool resource_manager::is_coordinating_resource(
        const std::string& _resc_name) {
        if(!resource_name_map_.has_entry(_resc_name)) {
            THROW(SYS_RESC_DOES_NOT_EXIST, _resc_name);
        }
        resource_ptr resc = resource_name_map_[_resc_name];
        return resc->num_children() > 0;
    } // is_coordinating_resource
} // namespace irods
