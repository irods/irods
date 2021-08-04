////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Plug-in defining a replicating resource. This resource makes sure that all of its data is replicated to all of its children
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// =-=-=-=-=-=-=-
// irods includes
#include "icatHighLevelRoutines.hpp"
#include "msParam.h"
#include "rodsLog.h"

// =-=-=-=-=-=-=-
#include "irods_at_scope_exit.hpp"
#include "irods_resource_plugin.hpp"
#include "irods_file_object.hpp"
#include "irods_collection_object.hpp"
#include "irods_string_tokenize.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_resource_backport.hpp"
#include "irods_plugin_base.hpp"
#include "irods_stacktrace.hpp"
#include "irods_repl_retry.hpp"
#include "irods_repl_types.hpp"
#include "irods_object_oper.hpp"
#include "irods_replicator.hpp"
#include "irods_create_write_replicator.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_repl_rebalance.hpp"
#include "irods_kvp_string_parser.hpp"
#include "irods_random.hpp"
#include "irods_exception.hpp"
#include "rsGenQuery.hpp"
#include "key_value_proxy.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <list>
#include <boost/lexical_cast.hpp>

#include "fmt/format.h"

// =-=-=-=-=-=-=-
// system includes
#ifndef _WIN32
#include <sys/file.h>
#include <sys/param.h>
#endif
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/types.h>
#if defined(osx_platform)
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif
#include <fcntl.h>
#ifndef _WIN32
#include <sys/file.h>
#include <unistd.h>
#endif
#include <dirent.h>

#if defined(solaris_platform)
#include <sys/statvfs.h>
#endif
#if defined(linux_platform)
#include <sys/vfs.h>
#endif
#include <sys/stat.h>

#include <string.h>


const std::string NUM_REPL_KW( "num_repl" );
const std::string READ_KW( "read" );
const std::string READ_RANDOM_POLICY( "random" );

/// @brief Check the general parameters passed in to most plugin functions
template< typename DEST_TYPE >
irods::error replCheckParams(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    // =-=-=-=-=-=-=-
    // verify that the resc context is valid
    irods::error ret = _ctx.valid< DEST_TYPE >();
    if ( !ret.ok() ) {
        result = PASSMSG( "resource context is invalid", ret );
    }

    return result;
}

const int DEFAULT_REBALANCE_BATCH_SIZE = 500;

// =-=-=-=-=-=-=-
// 2. Define operations which will be called by the file*
//    calls declared in server/driver/include/fileDriver.h
// =-=-=-=-=-=-=-

// =-=-=-=-=-=-=-
// NOTE :: to access properties in the _prop_map do the
//      :: following :
//      :: double my_var = 0.0;
//      :: irods::error ret = _prop_map.get< double >( "my_key", my_var );
// =-=-=-=-=-=-=-

//////////////////////////////////////////////////////////////////////
// Utility functions

/**
 * @brief Gets the name of the child of this resource from the hierarchy
 */
irods::error replGetNextRescInHier(
    const irods::hierarchy_parser& _parser,
    irods::plugin_context& _ctx,
    irods::resource_ptr& _ret_resc ) {
    std::string this_name{};
    auto ret{_ctx.prop_map().get<std::string>(irods::RESOURCE_NAME, this_name)};
    if (!ret.ok()) {
        return PASS(ret);
    }
    std::string child{};
    ret = _parser.next(this_name, child);
    if (!ret.ok()) {
        return PASS(ret);
    }
    irods::resource_child_map* cmap_ref;
    _ctx.prop_map().get<irods::resource_child_map*>(irods::RESC_CHILD_MAP_PROP, cmap_ref);
    _ret_resc = (*cmap_ref)[child].second;
    return SUCCESS();
}

/// @brief Returns true if the specified object is in the specified object list
bool replObjectInList(
    const object_list_t& _object_list,
    const irods::file_object_ptr _object,
    irods::object_oper& _rtn_oper ) {
    bool result{};
    for (auto& oper : _object_list) {
        if (oper.object() == (*_object.get())) {
            _rtn_oper = oper;
            result = true;
        }
    }
    return result;
}

/// @brief Updates the fields in the resources properties for the object
irods::error replUpdateObjectAndOperProperties(
    irods::plugin_context& _ctx,
    const std::string& _oper ) {
    irods::error result = SUCCESS();
    irods::error ret;
    object_list_t object_list;
    // The object list is now a queue of operations and their associated objects. Their corresponding replicating operations
    // will be performed one at a time in the order in which they were put into the queue.
    irods::file_object_ptr file_obj = boost::dynamic_pointer_cast<irods::file_object >( ( _ctx.fco() ) );
    ret = _ctx.prop_map().get<object_list_t>( OBJECT_LIST_PROP, object_list );
    irods::object_oper oper;
    if ( !ret.ok() && ret.code() != KEY_NOT_FOUND ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Failed to get the object list from the resource.";
        result = PASSMSG( msg.str(), ret );
    }
    else if ( replObjectInList( object_list, file_obj, oper ) ) {
        // confirm the operations are compatible
        bool mismatched = false;
        if ( _oper == irods::CREATE_OPERATION ) {
            if ( oper.operation() != irods::CREATE_OPERATION ) {
                mismatched = true;
            }
        }
        else if ( _oper == irods::WRITE_OPERATION ) {
            // write is allowed after create
            if ( oper.operation() != irods::CREATE_OPERATION && oper.operation() != irods::WRITE_OPERATION ) {
                mismatched = true;
            }
        }

        result = ASSERT_ERROR( !mismatched, INVALID_OPERATION,
                               "Existing object operation: \"%s\" does not match current operation: \"%s\".",
                               oper.operation().c_str(), _oper.c_str() );
    }
    else {
        oper.object() = *( file_obj.get() );
        oper.operation() = _oper;
        object_list.push_back( oper );
        ret = _ctx.prop_map().set<object_list_t>( OBJECT_LIST_PROP, object_list );
        result = ASSERT_PASS( ret, "Failed to set the object list property on the resource." );
    }

    if ( !result.ok() ) {
        irods::log( result );
    }

    return result;
}

irods::error get_selected_hierarchy(
    irods::plugin_context& _ctx,
    irods::hierarchy_parser& _out_parser) {
    // Need to make sure selected hierarchy contains this resource
    std::string name{};
    const auto ret = _ctx.prop_map().get<std::string>(irods::RESOURCE_NAME, name);
    if (!ret.ok()) {
        return PASS(ret);
    }

    // Get selected hier from RESC_HIER_STR_KW, set at time of resolution
    irods::hierarchy_parser selected_parser{};
    bool resc_hier_in_keyword{};
    irods::file_object_ptr file_obj = boost::dynamic_pointer_cast<irods::file_object>(_ctx.fco());

    if (!resc_hier_in_keyword) {
        auto cond_input = irods::experimental::make_key_value_proxy((KeyValPair&)file_obj->cond_input());
        if (cond_input.contains(SELECTED_HIERARCHY_KW)) {
            const auto hier_str = cond_input.at(SELECTED_HIERARCHY_KW).value();
            if (hier_str.empty()) {
                return ERROR(SYS_INTERNAL_NULL_INPUT_ERR,
                             (boost::format(
                              "[%s] - No hierarchy string found in keywords for file object.") %
                              __FUNCTION__).str().c_str());
            }
            selected_parser.set_string(hier_str.data());
            resc_hier_in_keyword = selected_parser.resc_in_hier(name);
        }
    }

    if (!resc_hier_in_keyword) {
        if (file_obj->l1_desc_idx() > 0) {
            const auto hier_str{getValByKey(&L1desc[file_obj->l1_desc_idx()].dataObjInp->condInput, RESC_HIER_STR_KW)};
            if (!hier_str) {
                return ERROR(SYS_INTERNAL_NULL_INPUT_ERR,
                             (boost::format(
                              "[%s] - No hierarchy string found in keywords for file object.") %
                              __FUNCTION__).str().c_str());
            }
            selected_parser.set_string(hier_str);
            resc_hier_in_keyword = selected_parser.resc_in_hier(name);
        }
    }

    // Get resc hier from the file object directly if not in RESC_HIER_STR_KW
    if (!resc_hier_in_keyword) {
        const auto& selected_hier = file_obj->resc_hier();
        if (selected_hier.empty()) {
            return ERROR(SYS_INTERNAL_NULL_INPUT_ERR,
                         (boost::format(
                          "[%s] - file object does not have a resource hierarchy.") %
                          __FUNCTION__).str().c_str());
        }
        selected_parser.set_string(selected_hier);
        if (!selected_parser.resc_in_hier(name)) {
            return ERROR(HIERARCHY_ERROR,
                         (boost::format(
                          "[%s] - Replicating a file object which does not exist in this hierarchy.") %
                          __FUNCTION__).str().c_str());
        }
    }

    _out_parser = selected_parser;
    return SUCCESS();
}

irods::error replicate_create_write_operation(irods::plugin_context& _ctx) {
    object_list_t object_list_to_repl{};
    irods::error ret{_ctx.prop_map().get<object_list_t>(OBJECT_LIST_PROP, object_list_to_repl)};
    if (!ret.ok() && KEY_NOT_FOUND != ret.code()) {
        return PASS(ret);
    }
    if (object_list_to_repl.empty()) {
        return SUCCESS();
    }

    child_list_t child_list{};
    ret = _ctx.prop_map().get<child_list_t>(CHILD_LIST_PROP, child_list);
    if (!ret.ok()) {
        return PASS(ret);
    }
    if (child_list.empty()) {
        return SUCCESS();
    }

    // get the root resource name as well as the child hierarchy string
    irods::hierarchy_parser selected_parser{};
    ret = get_selected_hierarchy(_ctx, selected_parser);
    if (!ret.ok()) {
        return PASS(ret);
    }
    std::string child{};
    ret = selected_parser.str(child);
    if (!ret.ok()) {
        return PASSMSG(
                   (boost::format(
                    "[%s] - Failed to get the hierarchy string from the parser.") %
                    __FUNCTION__).str(), ret );
    }
    std::string root_resc{};
    ret = selected_parser.first_resc(root_resc);
    if (!ret.ok()) {
        return PASSMSG(
                   (boost::format(
                    "[%s] - Failed to get the root resource from the parser.") %
                    __FUNCTION__).str(), ret );
    }

    std::string name{};
    ret = _ctx.prop_map().get<std::string>(irods::RESOURCE_NAME, name);
    if (!ret.ok()) {
        return PASS(ret);
    }

    irods::create_write_replicator oper_repl{root_resc, name, child};
    irods::replicator replicator{&oper_repl};
    ret = replicator.replicate(_ctx, child_list, object_list_to_repl);
    if (!ret.ok()) {
        return PASSMSG(
                   (boost::format(
                    "[%s] - Failed to replicate the create/write operation to the siblings.") %
                    __FUNCTION__).str(), ret );
    }

    ret = _ctx.prop_map().set<object_list_t>(OBJECT_LIST_PROP, object_list_to_repl);
    if (!ret.ok()) {
        return PASS(ret);
    }
    return SUCCESS();
} // replicate_create_write_operation

//////////////////////////////////////////////////////////////////////
// Actual operations

// Called after a new file is registered with the ICAT
irods::error repl_file_registered(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;
    ret = replCheckParams< irods::file_object >( _ctx );
    if ( ( result = ASSERT_PASS( ret, "Error checking passed parameters." ) ).ok() ) {

        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast<irods::file_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( ( result = ASSERT_PASS( ret, "Failed to get the next resource in hierarchy." ) ).ok() ) {
            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_REGISTERED, _ctx.fco() );
            result = ASSERT_PASS( ret, "Failed while calling child operation." );
        }
    }
    return result;
}

// Called when a file is unregistered from the ICAT
irods::error repl_file_unregistered(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;
    ret = replCheckParams< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Error found checking passed parameters.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call( file_obj->comm(), irods::RESOURCE_OP_UNREGISTERED, file_obj ) ;
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
        }
    }
    return result;
}

irods::error proc_child_list_for_create_policy(irods::plugin_context& _ctx) {
    size_t num_repl{};
    irods::error ret = _ctx.prop_map().get<size_t>(NUM_REPL_KW, num_repl);
    if(!ret.ok()) {
        return SUCCESS();
    }

    child_list_t new_list{};
    if(num_repl <= 1) {
        ret = _ctx.prop_map().set<child_list_t>(CHILD_LIST_PROP, new_list);
        if(!ret.ok()) {
            return PASS(ret);
        }
        return SUCCESS();
    }

    child_list_t child_list{};
    ret = _ctx.prop_map().get<child_list_t>(CHILD_LIST_PROP, child_list);
    if(!ret.ok()) {
        return PASS(ret);
    }

    // decrement num_repl by 1 to count first replica
    num_repl--;

    if(num_repl < child_list.size()) {
        bool pick_done{};
        std::vector<size_t> picked_indices{};
        while(!pick_done) {
             size_t rand_index{irods::getRandom<size_t>() % child_list.size()};
             if(std::find(
                 picked_indices.begin(),
                 picked_indices.end(),
                 rand_index) == picked_indices.end()) {
                 new_list.push_back(child_list[rand_index]);
                 picked_indices.push_back(rand_index);
                 if(picked_indices.size() >= num_repl) {
                     pick_done = true;
                 }
             }
        }

        // if we have an empty new_list, simply keep the old one
        if(0 == new_list.size()) {
            return SUCCESS();
        }

        ret = _ctx.prop_map().set<child_list_t>(CHILD_LIST_PROP, new_list);
        if(!ret.ok()) {
            return PASS(ret);
        }
    }

    return SUCCESS();

} // proc_child_list_for_create_policy

irods::error create_replication_list(
    irods::plugin_context& _ctx,
    const std::string& operation) {
    // Get selected hierarchy parser
    irods::hierarchy_parser selected_parser{};
    irods::error ret = get_selected_hierarchy(_ctx, selected_parser);
    if (!ret.ok()) {
        return PASS(ret);
    }

    // Get hierarchy for current resource for use when resolving children
    std::string name{};
    ret = _ctx.prop_map().get<std::string>(irods::RESOURCE_NAME, name);
    if (!ret.ok()) {
        return PASS(ret);
    }
    std::string current_resc_hier_str{};
    ret = selected_parser.str(current_resc_hier_str, name);
    if (!ret.ok()) {
        return PASS(ret);
    }

    // Get list of children for this resource 
    irods::resource_child_map* cmap_ref;
    _ctx.prop_map().get<irods::resource_child_map*>(irods::RESC_CHILD_MAP_PROP, cmap_ref);
    if (cmap_ref->empty()) {
        return SUCCESS();
    }

    // Get current hostname for vote
    char host_name_str[MAX_NAME_LEN];
    if (gethostname(host_name_str, MAX_NAME_LEN) < 0) {
        return ERROR(SYS_GET_HOSTNAME_ERR, "failed in gethostname");
    }
    const std::string host_name(host_name_str);

    // Loop over children and create a list of hierarchies to which objects will be replicated
    float out_vote{};
    child_list_t repl_vector{};
    for (auto& entry : *cmap_ref) {
        auto child{entry.second.second};
        std::string child_name{};
        ret = child->get_property<std::string>(irods::RESOURCE_NAME, child_name);
        if (!ret.ok()) {
            return PASS(ret);
        }

        // Don't repl to the selected hier
        if (selected_parser.resc_in_hier(child_name)) {
            continue;
        }

        // Resolve child and add to repl list based on vote
        irods::hierarchy_parser parser{};
        parser.set_string(current_resc_hier_str);
        ret = child->call<const std::string*, const std::string*, irods::hierarchy_parser*, float*>(
                  _ctx.comm(), irods::RESOURCE_OP_RESOLVE_RESC_HIER, _ctx.fco(), &operation, &host_name, &parser, &out_vote );
        if (!ret.ok() && CHILD_NOT_FOUND != ret.code()) {
            rodsLog(LOG_WARNING,
                "[%s:%d] - failed resolving hierarchy for [%s]",
                __FUNCTION__, __LINE__, entry.first.c_str());
        }
        else if (out_vote > 0.0) {
            repl_vector.push_back(parser);
        }
    }

    // add the resulting vector as a property of the resource
    ret = _ctx.prop_map().set<child_list_t>(CHILD_LIST_PROP, repl_vector);
    if (!ret.ok()) {
        return PASS(ret);
    }

    // Determine which children of the child list will receive replicas
    if (irods::CREATE_OPERATION == operation) {
        ret = proc_child_list_for_create_policy(_ctx);
        if (!ret.ok()) {
            return PASS(ret);
        }
    }

    return SUCCESS();
} // create_replication_list

// Called when a files entry is modified in the ICAT
irods::error repl_file_modified(irods::plugin_context& _ctx) {
    // Make sure context is valid
    irods::error ret{replCheckParams<irods::file_object>(_ctx)};
    if (!ret.ok()) {
        return PASS(ret);
    }

    // Get next resource on which to call file_modified
    irods::file_object_ptr file_obj{boost::dynamic_pointer_cast<irods::file_object>(_ctx.fco())};
    irods::hierarchy_parser parser{};
    parser.set_string(file_obj->resc_hier());
    irods::resource_ptr child{};
    ret = replGetNextRescInHier(parser, _ctx, child);
    if (!ret.ok()) {
        return PASS(ret);
    }

    // Call file_modified on next resource in hierarchy
    ret = child->call( _ctx.comm(), irods::RESOURCE_OP_MODIFIED, _ctx.fco() );
    if (!ret.ok()) {
        return PASS(ret);
    }

    // If this resource is in PDMO subparser, the operation is for someone else
    std::string name;
    ret = _ctx.prop_map().get<std::string>(irods::RESOURCE_NAME, name);
    if (!ret.ok()) {
        return PASS(ret);
    }
    irods::hierarchy_parser sub_parser{};
    sub_parser.set_string(file_obj->in_pdmo());
    if (sub_parser.resc_in_hier(name)) {
        return SUCCESS();
    }

    // The selected child resource is not added to the child list property
    // Only replicate if the selected child resource is not in the list
    // TODO: Replace with a query
    child_list_t child_list{};
    ret = _ctx.prop_map().get<child_list_t>(CHILD_LIST_PROP, child_list);
    if (ret.ok()) {
        return SUCCESS();
    }

    const irods::at_scope_exit clear_child_list{[&_ctx]() {
        _ctx.prop_map().erase(CHILD_LIST_PROP);
    }};

    // Get operation type based on reason for opening the file
    const auto open_type_str{getValByKey(&file_obj->cond_input(), OPEN_TYPE_KW)};
    if (nullptr == open_type_str) {
        irods::log(LOG_DEBUG,
                   (boost::format("[%s] - no OPEN_TYPE_KW value provided") %
                    __FUNCTION__).str());
        return SUCCESS();
    }

    std::string operation{};
    try {
        switch(std::stoi(open_type_str)) {
            case CREATE_TYPE:
                operation = irods::CREATE_OPERATION;
                break;
            case OPEN_FOR_WRITE_TYPE:
                operation = irods::WRITE_OPERATION;
                break;
            default:
                throw std::invalid_argument(open_type_str);
        }
    }
    catch (const std::invalid_argument&) {
        return ERROR(SYS_INVALID_INPUT_PARAM,
                     (boost::format("[%s] - invalid OPEN_TYPE_KW value provided:[%s]") %
                      __FUNCTION__ % open_type_str).str());
    }

    // Create child list and list of operations to replicate - then, replicate
    ret = create_replication_list(_ctx, operation);
    if (!ret.ok()) {
        return PASS(ret);
    }
    ret = replUpdateObjectAndOperProperties(_ctx, operation);
    if (!ret.ok()) {
        return PASS(ret);
    }
    ret = replicate_create_write_operation(_ctx);
    if (!ret.ok()) {
        return PASSMSG((boost::format(
                    "[%s] - Failed to replicate create/write operation for object: \"%s\".") %
                    __FUNCTION__ % file_obj->logical_path()).str(), ret);
    }

    // set object list property back to empty so leftover items are not considered in future plugin operations
    ret = _ctx.prop_map().set<object_list_t>(OBJECT_LIST_PROP, {});
    if (!ret.ok()) {
        return PASS(ret);
    }

    return SUCCESS();
} // repl_file_modified

// =-=-=-=-=-=-=-
// interface for POSIX create
irods::error repl_file_create(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    // get the list of objects that need to be replicated
    object_list_t object_list;
    ret = _ctx.prop_map().get<object_list_t>( OBJECT_LIST_PROP, object_list );

    ret = replCheckParams< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        result = PASSMSG( "repl_file_create - bad params.", ret );
    }
    else {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_CREATE, _ctx.fco() );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
        }
    }
    return result;
} // repl_file_create

// =-=-=-=-=-=-=-
// interface for POSIX Open
irods::error repl_file_open(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast<irods::file_object >( ( _ctx.fco() ) );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_OPEN, _ctx.fco() );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
        }
    }
    return result;
} // repl_file_open

// =-=-=-=-=-=-=-
// interface for POSIX Read
irods::error repl_file_read(
    irods::plugin_context& _ctx,
    void*                          _buf,
    int                            _len ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast<irods::file_object >( ( _ctx.fco() ) );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call<void*, int>( _ctx.comm(), irods::RESOURCE_OP_READ, _ctx.fco(), _buf, _len );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                // have to return the actual code because it contains the number of bytes read
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_read

// =-=-=-=-=-=-=-
// interface for POSIX Write
irods::error repl_file_write(
    irods::plugin_context& _ctx,
    void*                          _buf,
    int                            _len ) {
    irods::error result = SUCCESS();
    irods::error ret;
    // get the list of objects that need to be replicated
    object_list_t object_list;
    ret = _ctx.prop_map().get<object_list_t>( OBJECT_LIST_PROP, object_list );

    ret = replCheckParams< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast<irods::file_object >( ( _ctx.fco() ) );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call<void*, int>( _ctx.comm(), irods::RESOURCE_OP_WRITE, _ctx.fco(), _buf, _len );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );

            }
        }
    }
    return result;
} // repl_file_write

// =-=-=-=-=-=-=-
// interface for POSIX Close
irods::error repl_file_close(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;
    // get the list of objects that need to be replicated
    object_list_t object_list;
    ret = _ctx.prop_map().get<object_list_t>( OBJECT_LIST_PROP, object_list );

    ret = replCheckParams< irods::file_object >( _ctx );
    if ( ( result = ASSERT_PASS( ret, "Bad params." ) ).ok() ) {

        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast<irods::file_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( ( result = ASSERT_PASS( ret, "Failed to get the next resource in hierarchy." ) ).ok() ) {

            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_CLOSE, _ctx.fco() );
            result = ASSERT_PASS( ret, "Failed while calling child operation." );
        }
    }
    return result;

} // repl_file_close

// =-=-=-=-=-=-=-
// interface for POSIX Unlink
irods::error repl_file_unlink(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::data_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::data_object_ptr data_obj = boost::dynamic_pointer_cast<irods::data_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( data_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_UNLINK, _ctx.fco() );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );

            }
        }
    }
    return result;
} // repl_file_unlink

// =-=-=-=-=-=-=-
// interface for POSIX Stat
irods::error repl_file_stat(
    irods::plugin_context& _ctx,
    struct stat*                   _statbuf ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::data_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::data_object_ptr data_obj = boost::dynamic_pointer_cast< irods::data_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( data_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call<struct stat*>( _ctx.comm(), irods::RESOURCE_OP_STAT, _ctx.fco(), _statbuf );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_stat

// =-=-=-=-=-=-=-
// interface for POSIX lseek
irods::error repl_file_lseek(
    irods::plugin_context& _ctx,
    long long                       _offset,
    int                             _whence ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call<long long, int>( _ctx.comm(), irods::RESOURCE_OP_LSEEK, _ctx.fco(), _offset, _whence );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_lseek

// =-=-=-=-=-=-=-
// interface for POSIX mkdir
irods::error repl_file_mkdir(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::collection_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::collection_object_ptr collection_obj = boost::dynamic_pointer_cast< irods::collection_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( collection_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_MKDIR, _ctx.fco() );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_mkdir

// =-=-=-=-=-=-=-
// interface for POSIX mkdir
irods::error repl_file_rmdir(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::collection_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::collection_object_ptr file_obj = boost::dynamic_pointer_cast< irods::collection_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_RMDIR, _ctx.fco() );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_rmdir

// =-=-=-=-=-=-=-
// interface for POSIX opendir
irods::error repl_file_opendir(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::collection_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::collection_object_ptr collection_obj = boost::dynamic_pointer_cast< irods::collection_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( collection_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_OPENDIR, _ctx.fco() );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_opendir

// =-=-=-=-=-=-=-
// interface for POSIX closedir
irods::error repl_file_closedir(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::collection_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::collection_object_ptr collection_obj = boost::dynamic_pointer_cast< irods::collection_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( collection_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_CLOSEDIR, _ctx.fco() );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_closedir

// =-=-=-=-=-=-=-
// interface for POSIX readdir
irods::error repl_file_readdir(
    irods::plugin_context& _ctx,
    struct rodsDirent**            _dirent_ptr ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::collection_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::collection_object_ptr collection_obj = boost::dynamic_pointer_cast< irods::collection_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( collection_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call<rodsDirent**>( _ctx.comm(), irods::RESOURCE_OP_READDIR, _ctx.fco(), _dirent_ptr );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_readdir

// =-=-=-=-=-=-=-
// interface for POSIX readdir
irods::error repl_file_rename(
    irods::plugin_context& _ctx,
    const char*                    _new_file_name ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call<const char*>( _ctx.comm(), irods::RESOURCE_OP_RENAME, _ctx.fco(), _new_file_name );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_rename

// =-=-=-=-=-=-=-
// interface for POSIX truncate
irods::error repl_file_truncate(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams<irods::file_object>( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::file_object_ptr ptr = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( ptr->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_TRUNCATE, _ctx.fco() );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_truncate

// =-=-=-=-=-=-=-
// interface to determine free space on a device given a path
irods::error repl_file_getfs_freespace(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_FREESPACE, _ctx.fco() );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_getfs_freespace

// =-=-=-=-=-=-=-
// repl_file_stage_to_cache - This routine is for testing the TEST_STAGE_FILE_TYPE.
// Just copy the file from filename to cacheFilename. optionalInfo info
// is not used.
irods::error repl_file_stage_to_cache(
    irods::plugin_context& _ctx,
    const char*                      _cache_file_name ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call< const char* >(
                      _ctx.comm(),
                      irods::RESOURCE_OP_STAGETOCACHE,
                      _ctx.fco(),
                      _cache_file_name );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
        }
    }
    return result;
} // repl_file_stage_to_cache

// =-=-=-=-=-=-=-
// repl_file_sync_to_arch - This routine is for testing the TEST_STAGE_FILE_TYPE.
// Just copy the file from cacheFilename to filename. optionalInfo info
// is not used.
irods::error repl_file_sync_to_arch(
    irods::plugin_context& _ctx,
    const char*                    _cache_file_name ) {
    irods::error result = SUCCESS();
    irods::error ret;

    ret = replCheckParams< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call<const char*>( _ctx.comm(), irods::RESOURCE_OP_SYNCTOARCH, _ctx.fco(), _cache_file_name );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                result = CODE( ret.code() );
            }
        }
    }
    return result;
} // repl_file_sync_to_arch

/// @brief Adds the current resource to the specified resource hierarchy
irods::error add_self_to_hierarchy(
    irods::plugin_context& _ctx,
    irods::hierarchy_parser& _parser ) {
    std::string name;
    auto ret{_ctx.prop_map().get<std::string>(irods::RESOURCE_NAME, name)};
    if (!ret.ok()) {
        return PASS(ret);
    }
    ret = _parser.add_child(name);
    if (!ret.ok()) {
        return PASS(ret);
    }
    return SUCCESS();
} // add_self_to_hierarchy

/// @brief Loop through the children and call resolve hierarchy on each one to populate the hierarchy vector
std::pair<redirect_map_t, irods::error> resolve_children(
    irods::plugin_context& ctx,
    const std::string& operation,
    const std::string& local_hostname,
    irods::hierarchy_parser& out_parser)
{
    redirect_map_t map;
    irods::resource_child_map* cmap_ref;
    ctx.prop_map().get<irods::resource_child_map*>(irods::RESC_CHILD_MAP_PROP, cmap_ref);
    if (cmap_ref->empty()) {
        return {map, ERROR(CHILD_NOT_FOUND, "No children found for resource")};
    }

    irods::error last_err = SUCCESS();
    float out_vote{};
    for (auto& entry : *cmap_ref) {
        auto parser{out_parser};
        const auto ret{entry.second.second->call<const std::string*, const std::string*, irods::hierarchy_parser*, float*>(
                        ctx.comm(), irods::RESOURCE_OP_RESOLVE_RESC_HIER, ctx.fco(), &operation, &local_hostname, &parser, &out_vote)};
        if (!ret.ok() && CHILD_NOT_FOUND != ret.code()) {
            irods::log(LOG_WARNING, fmt::format(
                "[{}:{}] - failed resolving hierarchy for [{}]; msg:[{}],ec:[{}]",
                __FUNCTION__, __LINE__, entry.first, ret.result(), ret.code()));
            last_err = ret;
        }
        else {
            map.insert({out_vote, parser});
        }
    }
    return {map, last_err};
} // resolve_children

/// @brief honor the read context string keyword if present
irods::error process_redirect_map_for_random_open(
    const redirect_map_t&    _redirect_map,
    irods::hierarchy_parser* _out_parser,
    float*                   _out_vote ) {

    std::vector<std::pair<float, irods::hierarchy_parser>> items;
    redirect_map_t::const_iterator itr = _redirect_map.cbegin();
    for( ; itr != _redirect_map.cend(); ++itr ) {
        if(itr->first > 0) {
            items.push_back(std::make_pair(itr->first, itr->second));
        }
    }

    size_t rand_index = irods::getRandom<size_t>() % items.size();
    std::vector<std::pair<float, irods::hierarchy_parser>>::iterator a_itr = items.begin();
    std::advance(a_itr, rand_index);
    *_out_vote   = a_itr->first;
    *_out_parser = a_itr->second;

    return SUCCESS();
} // process_redirect_map_for_random_open

/// @brief Selects a child from the vector of parsers based on host access
irods::error select_child(
    irods::plugin_context&   _ctx,
    const std::string&       _operation,
    const redirect_map_t&    _redirect_map,
    irods::hierarchy_parser* _out_parser,
    float*                   _out_vote ) {
    (*_out_vote) = 0.0;
    if (_redirect_map.empty()) {
        // there are no votes to consider
        return SUCCESS();
    }

    // Enact read policy if specified
    if (irods::OPEN_OPERATION == _operation) {
        std::string read_policy{};
        auto ret{_ctx.prop_map().get<std::string>(READ_KW, read_policy)};
        if (ret.ok() && READ_RANDOM_POLICY == read_policy) {
            ret = process_redirect_map_for_random_open(_redirect_map, _out_parser, _out_vote);
            if (!ret.ok()) {
                return PASS(ret);
            }
            return SUCCESS();
        }
    }

    // Select hierarchy and store vote value
    const auto& it{_redirect_map.begin()};
    const auto& vote{it->first};
    const auto& parser{it->second};
    *_out_parser = parser;
    *_out_vote = vote;
    return SUCCESS();
} // select_child

/// @brief Determines which child should be used for the specified operation
irods::error repl_file_resolve_hierarchy(
    irods::plugin_context&   _ctx,
    const std::string*       _operation,
    const std::string*       _curr_host,
    irods::hierarchy_parser* _inout_parser,
    float*                   _out_vote ) {
    if (nullptr == _operation || nullptr == _curr_host || nullptr == _inout_parser || nullptr == _out_vote) {
        return ERROR(SYS_INVALID_INPUT_PARAM,
                     (boost::format(
                      "[%s]: null parameters passed to redirect") %
                      __FUNCTION__).str().c_str() );
    }

    // If child list property exists, use previously selected parser for the vote
    irods::file_object_ptr file_obj = boost::dynamic_pointer_cast<irods::file_object >( ( _ctx.fco() ) );
    const auto hier_str{getValByKey(&file_obj->cond_input(), RESC_HIER_STR_KW)};
    if (hier_str) {
        irods::hierarchy_parser selected_parser{};
        selected_parser.set_string(hier_str);
        *_out_vote = 1.0;
        *_inout_parser = selected_parser;
        return SUCCESS();
    }

    // add ourselves to the hierarchy parser
    auto ret = add_self_to_hierarchy(_ctx, *_inout_parser);
    if (!ret.ok()) {
        return PASS(ret);
    }

    // Resolve each one of our children and put into redirect_map
    auto [redirect_map, last_err] = resolve_children(_ctx, *_operation, *_curr_host, *_inout_parser);
    if (redirect_map.empty()) {
        return last_err;
    }

    // Select a resolved hierarchy from redirect_map for the operation
    ret = select_child(_ctx, *_operation, redirect_map, _inout_parser, _out_vote);
    if (!ret.ok()) {
        return PASS(ret);
    }

    return SUCCESS();
} // repl_file_resolve_hierarchy

irods::error call_rebalance_on_children(irods::plugin_context& _ctx) {
    irods::resource_child_map *cmap_ptr;
    _ctx.prop_map().get<irods::resource_child_map*>(irods::RESC_CHILD_MAP_PROP, cmap_ptr);
    for (auto& entry : *cmap_ptr) {
        irods::error ret = entry.second.second->call(_ctx.comm(), irods::RESOURCE_OP_REBALANCE, _ctx.fco() );
        if (!ret.ok()) {
            return PASS(ret);
        }
    }
    return SUCCESS();
}

// throws irods::exception
int get_rebalance_batch_size(irods::plugin_context& _ctx) {
    if (!_ctx.rule_results().empty()) {
        irods::kvp_map_t kvp;
        irods::error kvp_err = irods::parse_kvp_string(_ctx.rule_results(), kvp);
        if (!kvp_err.ok()) {
            THROW(kvp_err.code(), kvp_err.result());
        }

        const auto it = kvp.find(irods::REPL_LIMIT_KEY);
        if (it != kvp.end()) {
            try {
                return boost::lexical_cast<int>(it->second);
            } catch (const boost::bad_lexical_cast&) {
                THROW(SYS_INVALID_INPUT_PARAM, boost::format("failed to cast string [%s] to integer") % it->second);
            }
        }
    }
    return DEFAULT_REBALANCE_BATCH_SIZE;
}

// repl_file_rebalance - code which would rebalance the subtree
irods::error repl_file_rebalance(
    irods::plugin_context& _ctx ) {
    irods::error result = call_rebalance_on_children(_ctx);
    if (!result.ok()) {
        return PASS(result);
    }

    // get invocation timestamp
    // only replicas at rest prior to this invocation timestamp will be rebalanced
    // prevents race condition of new/inflight data being 'over'-rebalanced
    char invocation_timestamp[50];
    getNowStr(invocation_timestamp);

    std::string resource_name;
    result = _ctx.prop_map().get<std::string>(irods::RESOURCE_NAME, resource_name);
    if (!result.ok()) {
        return PASS(result);
    }

    try {
        const int batch_size = get_rebalance_batch_size(_ctx);
        const std::vector<leaf_bundle_t> leaf_bundles = resc_mgr.gather_leaf_bundles_for_resc(resource_name);
        irods::update_out_of_date_replicas(_ctx, leaf_bundles, batch_size, invocation_timestamp, resource_name);
        irods::create_missing_replicas(_ctx, leaf_bundles, batch_size, invocation_timestamp, resource_name);
    } catch (const irods::exception& e) {
        return irods::error(e);
    }
    return SUCCESS();
}

// Called when a files entry is modified in the ICAT
irods::error repl_file_notify(
    irods::plugin_context& _ctx,
    const std::string*               _opr ) {
    irods::error result = SUCCESS();
    irods::error ret = replCheckParams< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - bad params.";
        result = PASSMSG( msg.str(), ret );
    }
    else {
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast<irods::file_object >( ( _ctx.fco() ) );
        irods::hierarchy_parser parser;
        parser.set_string( file_obj->resc_hier() );
        irods::resource_ptr child;
        ret = replGetNextRescInHier( parser, _ctx, child );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the next resource in hierarchy.";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            ret = child->call( _ctx.comm(), irods::RESOURCE_OP_NOTIFY, _ctx.fco(), _opr );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed while calling child operation.";
                result = PASSMSG( msg.str(), ret );
            }
        }
    }
    return result;
}

// =-=-=-=-=-=-=-
// 3. create derived class to handle unix file system resources
//    necessary to do custom parsing of the context string to place
//    any useful values into the property map for reference in later
//    operations.  semicolon is the preferred delimiter
class repl_resource : public irods::resource {

    public:
        repl_resource(
            const std::string& _inst_name,
            const std::string& _context ) :
            irods::resource(
                    _inst_name,
                    _context ) {

                if ( _context.empty() ) {
                    // Retry capability requires default values to be used if not set
                    properties_.set< decltype( irods::DEFAULT_RETRY_ATTEMPTS ) >( irods::RETRY_ATTEMPTS_KW, irods::DEFAULT_RETRY_ATTEMPTS );
                    properties_.set< decltype( irods::DEFAULT_RETRY_FIRST_DELAY_IN_SECONDS ) >( irods::RETRY_FIRST_DELAY_IN_SECONDS_KW, irods::DEFAULT_RETRY_FIRST_DELAY_IN_SECONDS );
                    properties_.set< decltype( irods::DEFAULT_RETRY_BACKOFF_MULTIPLIER ) >( irods::RETRY_BACKOFF_MULTIPLIER_KW, irods::DEFAULT_RETRY_BACKOFF_MULTIPLIER );
                    return;
                }

                irods::kvp_map_t kvp_map;
                irods::error ret = irods::parse_kvp_string( _context, kvp_map );
                if ( !ret.ok() ) {
                    irods::log( PASS( ret ) );
                }

                if ( kvp_map.find( NUM_REPL_KW ) != kvp_map.end() ) {
                    irods::log(LOG_WARNING,
                               (boost::format(
                               "[%s] - resource [%s] is using deprecated context string [%s]") %
                               __FUNCTION__ %
                               _inst_name %
                               NUM_REPL_KW).str());
                    try {
                        int num_repl = boost::lexical_cast< int >( kvp_map[ NUM_REPL_KW ] );
                        if( num_repl <= 0 ) {
                            irods::log( ERROR( SYS_INVALID_INPUT_PARAM,
                                            boost::format(
                                            "[%s] - [%s] for resource [%s] is 0" ) %
                                            __FUNCTION__ %
                                            NUM_REPL_KW %
                                            _inst_name) );
                        }
                        else {
                            properties_.set< size_t >( NUM_REPL_KW, static_cast< size_t >( num_repl ) );
                        }
                    }
                    catch ( const boost::bad_lexical_cast& ) {
                        irods::log( ERROR( SYS_INVALID_INPUT_PARAM,
                                        boost::format(
                                        "[%s] - failed to cast [%s] for resource [%s] to value [%s]") %
                                        __FUNCTION__ %
                                        NUM_REPL_KW %
                                        _inst_name %
                                        kvp_map[ NUM_REPL_KW ] ) );
                    }
                }

                auto retry_attempts = irods::DEFAULT_RETRY_ATTEMPTS;
                if ( kvp_map.find( irods::RETRY_ATTEMPTS_KW ) != kvp_map.end() ) {
                    try {
                        // boost::lexical_cast does not raise errors on negatives when casting to unsigned
                        const int int_retry_attempts = boost::lexical_cast< int >( kvp_map[ irods::RETRY_ATTEMPTS_KW ] );
                        if ( int_retry_attempts < 0 ) {
                            irods::log( ERROR( SYS_INVALID_INPUT_PARAM,
                                           boost::format(
                                           "[%s] - [%s] for resource [%s] is < 0; using default value [%d]" ) %
                                           __FUNCTION__ %
                                           _inst_name %
                                           irods::RETRY_ATTEMPTS_KW.c_str() %
                                           irods::DEFAULT_RETRY_ATTEMPTS ) );
                        }
                        else {
                            retry_attempts = static_cast< decltype( retry_attempts ) >( int_retry_attempts );
                        }
                    }
                    catch ( const boost::bad_lexical_cast& ) {
                        irods::log( ERROR( SYS_INVALID_INPUT_PARAM,
                                        boost::format(
                                        "[%s] - failed to cast [%s] for resource [%s] to value [%s]; using default value [%d]") %
                                        __FUNCTION__ %
                                        irods::RETRY_ATTEMPTS_KW %
                                        _inst_name %
                                        kvp_map[ irods::RETRY_ATTEMPTS_KW ] %
                                        irods::DEFAULT_RETRY_ATTEMPTS ) );
                    }
                }
                properties_.set< decltype( retry_attempts ) >( irods::RETRY_ATTEMPTS_KW, retry_attempts );

                auto retry_delay_in_seconds = irods::DEFAULT_RETRY_FIRST_DELAY_IN_SECONDS;
                if ( kvp_map.find( irods::RETRY_FIRST_DELAY_IN_SECONDS_KW ) != kvp_map.end() ) {
                    try {
                        // boost::lexical_cast does not raise errors on negatives when casting to unsigned
                        const int int_retry_delay = boost::lexical_cast< int >( kvp_map[ irods::RETRY_FIRST_DELAY_IN_SECONDS_KW ] );
                        if ( int_retry_delay <= 0 ) {
                            irods::log( ERROR( SYS_INVALID_INPUT_PARAM,
                                           boost::format(
                                           "[%s] - [%s] for resource [%s] is <= 0; using default value [%d]" ) %
                                           __FUNCTION__ %
                                           _inst_name %
                                           irods::RETRY_FIRST_DELAY_IN_SECONDS_KW.c_str() %
                                           irods::DEFAULT_RETRY_FIRST_DELAY_IN_SECONDS ) );
                        }
                        else {
                            retry_delay_in_seconds = static_cast< decltype( retry_delay_in_seconds ) >( int_retry_delay );
                        }
                    }
                    catch ( const boost::bad_lexical_cast& ) {
                        irods::log( ERROR( SYS_INVALID_INPUT_PARAM,
                                        boost::format(
                                        "[%s] - failed to cast [%s] for resource [%s] to value [%s]; using default value [%d]") %
                                        __FUNCTION__ %
                                        irods::RETRY_FIRST_DELAY_IN_SECONDS_KW.c_str() %
                                        _inst_name %
                                        kvp_map[ irods::RETRY_FIRST_DELAY_IN_SECONDS_KW ] %
                                        irods::DEFAULT_RETRY_FIRST_DELAY_IN_SECONDS ) );
                    }
                }
                properties_.set< decltype( retry_delay_in_seconds ) >( irods::RETRY_FIRST_DELAY_IN_SECONDS_KW, retry_delay_in_seconds );

                auto backoff_multiplier = irods::DEFAULT_RETRY_BACKOFF_MULTIPLIER;
                if ( kvp_map.find( irods::RETRY_BACKOFF_MULTIPLIER_KW ) != kvp_map.end() ) {
                    try {
                        backoff_multiplier = boost::lexical_cast< decltype( backoff_multiplier ) >( kvp_map[ irods::RETRY_BACKOFF_MULTIPLIER_KW ] );
                        if ( backoff_multiplier < 1 ) {
                            irods::log( ERROR( SYS_INVALID_INPUT_PARAM,
                                           boost::format(
                                           "[%s] - [%s] for resource [%s] is < 1; using default value [%d]" ) %
                                           __FUNCTION__ %
                                           _inst_name %
                                           irods::RETRY_BACKOFF_MULTIPLIER_KW.c_str() %
                                           irods::DEFAULT_RETRY_BACKOFF_MULTIPLIER ) );
                            backoff_multiplier = irods::DEFAULT_RETRY_BACKOFF_MULTIPLIER;
                        }
                    }
                    catch ( const boost::bad_lexical_cast& ) {
                        irods::log( ERROR( SYS_INVALID_INPUT_PARAM,
                                        boost::format(
                                        "[%s] - failed to cast [%s] for resource [%s] to value [%s]; using default value [%d]") %
                                        __FUNCTION__ %
                                        irods::RETRY_BACKOFF_MULTIPLIER_KW.c_str() %
                                        _inst_name %
                                        kvp_map[ irods::RETRY_BACKOFF_MULTIPLIER_KW ] %
                                        irods::DEFAULT_RETRY_BACKOFF_MULTIPLIER ) );
                    }
                }
                properties_.set< decltype( backoff_multiplier ) >( irods::RETRY_BACKOFF_MULTIPLIER_KW, backoff_multiplier );

                if ( kvp_map.find( READ_KW ) != kvp_map.end() ) {
                    properties_.set< std::string >( READ_KW, kvp_map[ READ_KW ] );
                }
            } // ctor

        irods::error post_disconnect_maintenance_operation(
            irods::pdmo_type& ) {
            irods::error result = SUCCESS();
            // nothing to do
            return result;
        }

        irods::error need_post_disconnect_maintenance_operation(
            bool& _flag ) {
            irods::error result = SUCCESS();
            _flag = false;
            return result;
        }

}; // class repl_resource

// =-=-=-=-=-=-=-
// 4. create the plugin factory function which will return a dynamically
//    instantiated object of the previously defined derived resource.  use
//    the add_operation member to associate a 'call name' to the interfaces
//    defined above.  for resource plugins these call names are standardized
//    as used by the irods facing interface defined in
//    server/drivers/src/fileDriver.c
extern "C"
irods::resource* plugin_factory( const std::string& _inst_name, const std::string& _context ) {

    // =-=-=-=-=-=-=-
    // 4a. create repl_resource
    repl_resource* resc = new repl_resource( _inst_name, _context );

    // =-=-=-=-=-=-=-
    // 4b. map function names to operations.  this map will be used to load
    //     the symbols from the shared object in the delay_load stage of
    //     plugin loading.
    using namespace irods;
    using namespace std;
    resc->add_operation(
        RESOURCE_OP_CREATE,
        function<error(plugin_context&)>(
            repl_file_create ) );

    resc->add_operation(
        irods::RESOURCE_OP_OPEN,
        function<error(plugin_context&)>(
            repl_file_open ) );

    resc->add_operation<void*,int>(
        irods::RESOURCE_OP_READ,
        std::function<
            error(irods::plugin_context&,void*,int)>(
                repl_file_read ) );

    resc->add_operation<void*,int>(
        irods::RESOURCE_OP_WRITE,
        function<error(plugin_context&,void*,int)>(
            repl_file_write ) );

    resc->add_operation(
        RESOURCE_OP_CLOSE,
        function<error(plugin_context&)>(
            repl_file_close ) );

    resc->add_operation(
        irods::RESOURCE_OP_UNLINK,
        function<error(plugin_context&)>(
            repl_file_unlink ) );

    resc->add_operation<struct stat*>(
        irods::RESOURCE_OP_STAT,
        function<error(plugin_context&, struct stat*)>(
            repl_file_stat ) );

    resc->add_operation(
        irods::RESOURCE_OP_MKDIR,
        function<error(plugin_context&)>(
            repl_file_mkdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_OPENDIR,
        function<error(plugin_context&)>(
            repl_file_opendir ) );

    resc->add_operation<struct rodsDirent**>(
        irods::RESOURCE_OP_READDIR,
        function<error(plugin_context&,struct rodsDirent**)>(
            repl_file_readdir ) );

    resc->add_operation<const char*>(
        irods::RESOURCE_OP_RENAME,
        function<error(plugin_context&, const char*)>(
            repl_file_rename ) );

    resc->add_operation(
        irods::RESOURCE_OP_FREESPACE,
        function<error(plugin_context&)>(
            repl_file_getfs_freespace ) );

    resc->add_operation<long long, int>(
        irods::RESOURCE_OP_LSEEK,
        function<error(plugin_context&, long long, int)>(
            repl_file_lseek ) );

    resc->add_operation(
        irods::RESOURCE_OP_RMDIR,
        function<error(plugin_context&)>(
            repl_file_rmdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_CLOSEDIR,
        function<error(plugin_context&)>(
            repl_file_closedir ) );

    resc->add_operation<const char*>(
        irods::RESOURCE_OP_STAGETOCACHE,
        function<error(plugin_context&, const char*)>(
            repl_file_stage_to_cache ) );

    resc->add_operation<const char*>(
        irods::RESOURCE_OP_SYNCTOARCH,
        function<error(plugin_context&, const char*)>(
            repl_file_sync_to_arch ) );

    resc->add_operation(
        irods::RESOURCE_OP_REGISTERED,
        function<error(plugin_context&)>(
            repl_file_registered ) );

    resc->add_operation(
        irods::RESOURCE_OP_UNREGISTERED,
        function<error(plugin_context&)>(
            repl_file_unregistered ) );

    resc->add_operation(
        irods::RESOURCE_OP_MODIFIED,
        function<error(plugin_context&)>(
            repl_file_modified ) );

    resc->add_operation<const std::string*>(
        irods::RESOURCE_OP_NOTIFY,
        function<error(plugin_context&, const std::string*)>(
            repl_file_notify ) );

    resc->add_operation(
        irods::RESOURCE_OP_TRUNCATE,
        function<error(plugin_context&)>(
            repl_file_truncate ) );

    resc->add_operation<const std::string*, const std::string*, irods::hierarchy_parser*, float*>(
        irods::RESOURCE_OP_RESOLVE_RESC_HIER,
        function<error(plugin_context&,const std::string*, const std::string*, irods::hierarchy_parser*, float*)>(
            repl_file_resolve_hierarchy ) );

    resc->add_operation(
        irods::RESOURCE_OP_REBALANCE,
        function<error(plugin_context&)>(
            repl_file_rebalance ) );

    // =-=-=-=-=-=-=-
    // set some properties necessary for backporting to iRODS legacy code
    resc->set_property< int >( irods::RESOURCE_CHECK_PATH_PERM, 2 );//DO_CHK_PATH_PERM );
    resc->set_property< int >( irods::RESOURCE_CREATE_PATH,     1 );//CREATE_PATH );

    // =-=-=-=-=-=-=-
    // 4c. return the pointer through the generic interface of an
    //     irods::resource pointer
    return dynamic_cast<irods::resource*>( resc );

} // plugin_factory
