#include "irods/msParam.h"
#include "irods/generalAdmin.h"
#include "irods/physPath.hpp"
#include "irods/reIn2p3SysRule.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/dataObjRepl.h"
#include "irods/rsDataObjOpen.hpp"
#include "irods/rsDataObjClose.hpp"
#include "irods/rsFileStageToCache.hpp"
#include "irods/rsFileSyncToArch.hpp"
#include "irods/dataObjOpr.hpp"
#include "irods/irods_resource_plugin.hpp"
#include "irods/irods_file_object.hpp"
#include "irods/irods_physical_object.hpp"
#include "irods/irods_collection_object.hpp"
#include "irods/irods_string_tokenize.hpp"
#include "irods/irods_hierarchy_parser.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_resource_redirect.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/irods_kvp_string_parser.hpp"
#include "irods/irods_lexical_cast.hpp"
#include "irods/irods_random.hpp"
#include "irods/irods_at_scope_exit.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/function.hpp>
#include <boost/any.hpp>

#include <fmt/format.h>

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <string_view>

/// =-=-=-=-=-=-=-
/// @brief constant to reference the operation type for
///         file modification
const std::string OPERATION_TYPE( "operation_type" );

/// =-=-=-=-=-=-=-
/// @brief constant to index the cache child resource
const std::string CACHE_CONTEXT_TYPE( "cache" );

/// =-=-=-=-=-=-=-
/// @brief constant to index the archive child resource
const std::string ARCHIVE_CONTEXT_TYPE( "archive" );

/// @brief constant indicating the automatic replication policy
const std::string AUTO_REPL_POLICY( "auto_repl" );

/// @brief constant indicating the replication policy is enabled
const std::string AUTO_REPL_POLICY_ENABLED( "on" );

namespace
{
    auto get_archive_replica_number(
        const irods::file_object_ptr _obj,
        const std::string_view _archive_resource_name) -> int
    {
        const auto& replicas = _obj->replicas();
        const auto itr = std::find_if(
            replicas.cbegin(), replicas.cend(),
            [&_archive_resource_name] (const auto& _replica)
            {
                return _archive_resource_name == resc_mgr.resc_id_to_name(_replica.resc_id());
            }
        );

        if (replicas.cend() == itr) {
            THROW(SYS_REPLICA_DOES_NOT_EXIST, fmt::format(
                "no replica found for [{}] on archive resource [{}]",
                _obj->logical_path(), _archive_resource_name));
        }

        return itr->repl_num();
    } // get_archive_replica_number

    auto get_child_resource(const std::string_view _compound_resc_name,
                            const std::string_view _compound_context_type) -> irods::resource_ptr
    {
        irods::resource_ptr resc;

        // Resolve the resource name to a resource object.
        if (const auto err = resc_mgr.resolve(_compound_resc_name.data(), resc); !err.ok()) {
            return nullptr;
        }

        std::vector<std::string> children;
        resc->children(children);

        for (auto&& name : children) {
            if (const auto err = resc_mgr.resolve(name, resc); !err.ok()) {
                irods::log(err);
                continue;
            }

            // Get the type of the child resource (i.e. cache or archive).
            std::string context;
            resc->get_property(irods::RESOURCE_PARENT_CONTEXT, context);

            if (_compound_context_type == context) {
                return resc;
            }
        }

        return nullptr;
    }
} // anonymous namespace

/// =-=-=-=-=-=-=-
/// @brief Check the general parameters passed in to most plugin functions
template< typename DEST_TYPE >
inline irods::error compound_check_param(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // ask the context if it is valid
    irods::error ret = _ctx.valid< DEST_TYPE >();
    if ( !ret.ok() ) {
        return PASSMSG( "resource context is invalid", ret );

    }

    return SUCCESS();

} // compound_check_param

// =-=-=-=-=-=-=-
/// @brief helper function to get the next child in the hier string
///        for use when forwarding an operation
template< typename DEST_TYPE >
irods::error get_next_child(
    irods::plugin_context& _ctx,
    irods::resource_ptr&   _resc ) {
    // =-=-=-=-=-=-=-
    // check the context for validity
    irods::error ret = compound_check_param< DEST_TYPE >( _ctx );
    if ( !ret.ok() ) {
        return PASSMSG( "invalid resource context", ret );
    }
    // =-=-=-=-=-=-=-
    // get the resource name
    std::string name;
    ret = _ctx.prop_map().get< std::string >( irods::RESOURCE_NAME, name );
    if ( !ret.ok() ) {
        PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get the resource after this resource
    irods::hierarchy_parser parser;
    boost::shared_ptr< DEST_TYPE > dst_obj = boost::dynamic_pointer_cast< DEST_TYPE >( _ctx.fco() );
    parser.set_string( dst_obj->resc_hier() );
    std::string child;
    ret = parser.next( name, child );
    if ( !ret.ok() ) {
        PASS( ret );
    }

    irods::resource_child_map* cmap_ref;
    _ctx.prop_map().get< irods::resource_child_map* >(
            irods::RESC_CHILD_MAP_PROP,
            cmap_ref );

    // =-=-=-=-=-=-=-
    // extract the next resource from the child map
    if ( cmap_ref->has_entry( child ) ) {
        std::pair< std::string, irods::resource_ptr > resc_pair;
        ret = cmap_ref->get( child, resc_pair );
        if ( !ret.ok() ) {
            return PASS( ret );
        }
        else {
            _resc = resc_pair.second;
            return SUCCESS();
        }

    }
    else {
        std::stringstream msg;
        msg << "child not found [" << child << "]";
        return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );

    }

} // get_next_child

/// =-=-=-=-=-=-=-
/// @brief helper function to get the cache resource
irods::error get_cache(irods::plugin_context& _ctx, irods::resource_ptr& _resc)
{
    // =-=-=-=-=-=-=-
    // check the context for validity
    irods::error ret = compound_check_param< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        return PASSMSG( "invalid resource context", ret );
    }

    // =-=-=-=-=-=-=-
    // get the cache name
    std::string resc_name;
    ret = _ctx.prop_map().get<std::string>(CACHE_CONTEXT_TYPE, resc_name);
    if (!ret.ok()) {
        // Before returning the error, try finding the cache resource via the resource
        // manager. There are cases where the compound resource will not have the context type
        // for its children.

        const auto comp_resc_name = get_resource_name(_ctx);

        if (auto child_resc = get_child_resource(comp_resc_name, CACHE_CONTEXT_TYPE); child_resc) {
            std::string name;

            if (const auto err = child_resc->get_property(irods::RESOURCE_NAME, name); err.ok()) {
                if (const auto err = _ctx.prop_map().set<std::string>(CACHE_CONTEXT_TYPE, name); err.ok()) {
                    _resc = child_resc;
                    return SUCCESS();
                }
            }
        }

        return PASS(ret);
    }

    // =-=-=-=-=-=-=-
    // extract the resource from the child map
    irods::resource_child_map* cmap_ref;
    _ctx.prop_map().get< irods::resource_child_map* >(
            irods::RESC_CHILD_MAP_PROP,
            cmap_ref );

    std::pair< std::string, irods::resource_ptr > resc_pair;
    ret = cmap_ref->get( resc_name, resc_pair );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << "failed to get child resource [" << resc_name << "]";
        return PASSMSG( msg.str(), ret );
    }

    // =-=-=-=-=-=-=-
    // assign the resource to the out variable
    _resc = resc_pair.second;

    return SUCCESS();

} // get_cache

/// =-=-=-=-=-=-=-
/// @brief helper function to get the archive resource
irods::error get_archive(
    irods::plugin_context& _ctx,
    irods::resource_ptr&   _resc ) {
    // =-=-=-=-=-=-=-
    // check the context for validity
    irods::error ret = compound_check_param< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        return PASSMSG( "invalid resource context", ret );
    }

    // =-=-=-=-=-=-=-
    // get the archive name
    std::string resc_name;
    ret = _ctx.prop_map().get< std::string >( ARCHIVE_CONTEXT_TYPE, resc_name );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // extract the resource from the child map
    irods::resource_child_map* cmap_ref;
    _ctx.prop_map().get< irods::resource_child_map* >(
            irods::RESC_CHILD_MAP_PROP,
            cmap_ref );
    std::pair< std::string, irods::resource_ptr > resc_pair;
    ret = cmap_ref->get( resc_name, resc_pair );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << "failed to get child resource [" << resc_name << "]";
        return PASSMSG( msg.str(), ret );
    }

    // =-=-=-=-=-=-=-
    // assign the resource to the out variable
    _resc = resc_pair.second;
    return SUCCESS();

} // get_archive

/// =-=-=-=-=-=-=-
/// @brief Returns the cache resource making sure it corresponds to the fco resc hier
template <typename DEST_TYPE>
irods::error get_cache_resc(irods::plugin_context& _ctx, irods::resource_ptr& _resc)
{
    irods::resource_ptr next_resc;

    // Get the cache resource.
    if (const auto err = get_cache(_ctx, _resc); !err.ok()) {
        return PASSMSG("Failed to get cache resource.", err);
    }

    // Get the next child resource in the hierarchy.
    if (const auto err  = get_next_child<DEST_TYPE>(_ctx, next_resc); !err.ok()) {
        return PASSMSG("Failed to get the next child resource.", err);
    }

    // Make sure the file object came from the cache resource.
    if (_resc != next_resc) {
        boost::shared_ptr<DEST_TYPE> obj = boost::dynamic_pointer_cast<DEST_TYPE>(_ctx.fco());
        auto msg = fmt::format("Cannot open data object [{}]. It is stored in an archive "
                               "resource which is not directly accessible.",
                               obj->physical_path());
        return ERROR(DIRECT_ARCHIVE_ACCESS, std::move(msg));
    }

    return SUCCESS();
} // get_cache_resc

// =-=-=-=-=-=-=-
/// @brief helper function to take a rule result, find a keyword and then
///        parse it for the value
irods::error get_stage_policy(
    const std::string& _results,
    std::string& _policy ) {
    // =-=-=-=-=-=-=-
    // get a map of the key value pairs
    irods::kvp_map_t kvp;
    irods::error kvp_err = irods::parse_kvp_string(
                               _results,
                               kvp );
    if ( !kvp_err.ok() ) {
        return PASS( kvp_err );
    }

    std::string value = kvp[ irods::RESOURCE_STAGE_TO_CACHE_POLICY ];
    if ( value.empty() ) {
        return ERROR(
                   SYS_INVALID_INPUT_PARAM,
                   "stage policy value not found" );
    }
    _policy = value;

    return SUCCESS();

} // get_stage_policy

// =-=-=-=-=-=-=-
/// @brief start up operation - determine which child is the cache and which is the
///        archive.  cache those names in local variables for ease of use
///        must use C linkage for delay_load
irods::error compound_start_operation(
    irods::plugin_property_map& _prop_map ) {

    irods::resource_child_map* cmap_ref;
    _prop_map.get< irods::resource_child_map* >(
            irods::RESC_CHILD_MAP_PROP,
            cmap_ref );

    // =-=-=-=-=-=-=-
    // trap invalid number of children
    if ( cmap_ref->size() == 0 || cmap_ref->size() > 2 ) {
        std::stringstream msg;
        msg << "compound resource: invalid number of children [";
        msg << cmap_ref->size() << "]";
        return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
    }

    // =-=-=-=-=-=-=-
    // child map is indexed by name, get first name
    std::string first_child_name;
    irods::resource_child_map::iterator itr = cmap_ref->begin();
    if ( itr == cmap_ref->end() ) {
        return ERROR( -1, "child map is empty" );
    }

    std::string           first_child_ctx  = itr->second.first;
    irods::resource_ptr& first_child_resc = itr->second.second;
    irods::error get_err = first_child_resc->get_property<std::string>( irods::RESOURCE_NAME, first_child_name );
    if ( !get_err.ok() ) {
        return PASS( get_err );
    }

    // =-=-=-=-=-=-=-
    // get second name
    std::string second_child_name;
    ++itr;
    if ( itr == cmap_ref->end() ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "child map has only one entry" );
    }

    std::string          second_child_ctx  = itr->second.first;
    irods::resource_ptr second_child_resc = itr->second.second;
    get_err = second_child_resc->get_property<std::string>( irods::RESOURCE_NAME, second_child_name );
    if ( !get_err.ok() ) {
        return PASS( get_err );
    }

    // =-=-=-=-=-=-=-
    // now if first name is a cache, store that name as such
    // otherwise it is the archive
    // cmap is a hash map whose payload is a pair< string, resource_ptr >
    // the first of which is the parent-child context string denoting
    // that either the child is a cache or archive
    if ( first_child_ctx == CACHE_CONTEXT_TYPE ) {
        _prop_map[ CACHE_CONTEXT_TYPE ] = first_child_name;

    }
    else if ( first_child_ctx == ARCHIVE_CONTEXT_TYPE ) {
        _prop_map[ ARCHIVE_CONTEXT_TYPE ] = first_child_name;

    }
    else {
        return ERROR( INVALID_RESC_CHILD_CONTEXT, first_child_ctx );

    }

    // =-=-=-=-=-=-=-
    // do the same for the second resource
    if ( second_child_ctx == CACHE_CONTEXT_TYPE ) {
        _prop_map[ CACHE_CONTEXT_TYPE ] = second_child_name;

    }
    else if ( second_child_ctx == ARCHIVE_CONTEXT_TYPE ) {
        _prop_map[ ARCHIVE_CONTEXT_TYPE ] = second_child_name;

    }
    else {
        return ERROR( INVALID_RESC_CHILD_CONTEXT, second_child_ctx );

    }

    // =-=-=-=-=-=-=-
    // if both context strings match, this is also an error
    if ( first_child_ctx == second_child_ctx ) {
        std::stringstream msg;
        msg << "matching child context strings :: ";
        msg << "[" << first_child_ctx  << "] vs ";
        msg << "[" << second_child_ctx << "]";
        return ERROR( INVALID_RESC_CHILD_CONTEXT, msg.str() );

    }

    // =-=-=-=-=-=-=-
    // and... were done.
    return SUCCESS();

} // compound_start_operation

namespace
{
    int open_source_replica(
        irods::plugin_context& _ctx,
        irods::file_object_ptr obj,
        const std::string& src_hier)
    {
        // Open source replica
        dataObjInp_t source_data_obj_inp{};
        const auto free_cond_input = irods::at_scope_exit{[&source_data_obj_inp] { clearKeyVal(&source_data_obj_inp.condInput); }};
        rstrcpy( source_data_obj_inp.objPath, obj->logical_path().c_str(), MAX_NAME_LEN );
        copyKeyVal( ( keyValPair_t* )&obj->cond_input(), &source_data_obj_inp.condInput );

        // When staging a replica to cache, the source is the archive -- need some special sauce
        char* stage = getValByKey( ( keyValPair_t* )&obj->cond_input(), STAGE_OBJ_KW );
        if ( stage ) {
            addKeyVal( &source_data_obj_inp.condInput, STAGE_OBJ_KW, "" );
            addKeyVal( &source_data_obj_inp.condInput, NO_OPEN_FLAG_KW, "" );
            if (getValByKey(&source_data_obj_inp.condInput, PURGE_CACHE_KW)) {
                rmKeyVal(&source_data_obj_inp.condInput, PURGE_CACHE_KW);
            }
        }

        source_data_obj_inp.oprType = REPLICATE_SRC;
        source_data_obj_inp.openFlags = O_RDONLY;
        addKeyVal( &source_data_obj_inp.condInput, RESC_HIER_STR_KW, src_hier.c_str() );
        int source_l1descInx = rsDataObjOpen(_ctx.comm(), &source_data_obj_inp);
        if ( source_l1descInx < 0 ) {
            std::stringstream msg;
            msg << "Failed to open source for [" << obj->logical_path() << "] ";
            irods::log(LOG_ERROR, msg.str());
            THROW(source_l1descInx, msg.str());
        }
        return source_l1descInx;
    } // open_source_replica

    int open_destination_replica(
        irods::plugin_context& _ctx,
        irods::file_object_ptr obj,
        const int _source_l1_desc_inx,
        const std::string& dst_hier)
    {
        // =-=-=-=-=-=-=-
        // create a data obj input struct to call rsDataObjRepl which given
        // the _stage_sync_kw will either stage or sync the data object
        dataObjInp_t destination_data_obj_inp{};
        const auto free_cond_input = irods::at_scope_exit{[&destination_data_obj_inp] { clearKeyVal(&destination_data_obj_inp.condInput); }};
        rstrcpy( destination_data_obj_inp.objPath, obj->logical_path().c_str(), MAX_NAME_LEN );
        destination_data_obj_inp.createMode = obj->mode();

        copyKeyVal( ( keyValPair_t* )&obj->cond_input(), &destination_data_obj_inp.condInput );
        rmKeyVal( &destination_data_obj_inp.condInput, PURGE_CACHE_KW ); // do not want to accidentally purge

        char* no_chk = getValByKey( ( keyValPair_t* )&obj->cond_input(), NO_CHK_COPY_LEN_KW );
        if ( no_chk ) {
            addKeyVal( &destination_data_obj_inp.condInput, NO_CHK_COPY_LEN_KW, no_chk );
        }

        // When syncing a replica to archive, the destination is the archive -- need some special sauce
        char* sync = getValByKey( ( keyValPair_t* )&obj->cond_input(), SYNC_OBJ_KW );
        if ( sync ) {
            addKeyVal( &destination_data_obj_inp.condInput, SYNC_OBJ_KW, "" );
            addKeyVal( &destination_data_obj_inp.condInput, NO_OPEN_FLAG_KW, "" );
            if (getValByKey(&destination_data_obj_inp.condInput, PURGE_CACHE_KW)) {
                rmKeyVal(&destination_data_obj_inp.condInput, PURGE_CACHE_KW);
            }
        }

        addKeyVal( &destination_data_obj_inp.condInput, RESC_HIER_STR_KW, dst_hier.c_str() );

        addKeyVal(&destination_data_obj_inp.condInput, REG_REPL_KW, "");
        addKeyVal(&destination_data_obj_inp.condInput, FORCE_FLAG_KW, "");
        addKeyVal(&destination_data_obj_inp.condInput, SOURCE_L1_DESC_KW, std::to_string(_source_l1_desc_inx).c_str());
        destination_data_obj_inp.oprType = REPLICATE_DEST;
        destination_data_obj_inp.openFlags = O_CREAT | O_RDWR;

        int destination_l1descInx = rsDataObjOpen(_ctx.comm(), &destination_data_obj_inp);
        if ( destination_l1descInx < 0 ) {
            THROW(destination_l1descInx,
                  (boost::format("Failed to open destination for [%s]") %
                   destination_data_obj_inp.objPath).str().c_str());
        }
        return destination_l1descInx;
    } // open_destination_replica

    int close_replica(
        irods::plugin_context& _ctx,
        const int l1descInx) {
        openedDataObjInp_t data_obj_close_inp{};
        data_obj_close_inp.l1descInx = l1descInx;
        L1desc[data_obj_close_inp.l1descInx].oprStatus = l1descInx;
        addKeyVal(&data_obj_close_inp.condInput, IN_PDMO_KW, L1desc[l1descInx].dataObjInfo->rescHier);
        int close_status = rsDataObjClose( _ctx.comm(), &data_obj_close_inp);
        if (close_status < 0) {
            rodsLog(LOG_ERROR, "[%s] - rsDataObjClose failed with [%d]", __FUNCTION__, close_status);
        }
        clearKeyVal( &data_obj_close_inp.condInput );
        return close_status;
    }
} // anonymous namespace

/// =-=-=-=-=-=-=-
/// @brief replicate a given object for either a sync or a stage
irods::error repl_object(
    irods::plugin_context& _ctx,
    const irods::hierarchy_parser& _hier_from_root_to_compound,
    const std::string_view _stage_sync_kw)
{
    // =-=-=-=-=-=-=-
    // error check incoming params
    if (_stage_sync_kw.empty()) {
        return ERROR(SYS_INVALID_INPUT_PARAM, "empty _stage_sync_kw.");
    }

    // =-=-=-=-=-=-=-
    // get the parent name
    std::string parent_id_str;
    irods::error ret = _ctx.prop_map().get< std::string >( irods::RESOURCE_PARENT, parent_id_str );
    if (!ret.ok()) {
        return PASSMSG("Failed to get the parent name.", ret);
    }

    std::string parent_name;
    ret = resc_mgr.resc_id_to_name(
            parent_id_str,
            parent_name );
    if(!ret.ok()) {
        return PASS(ret);
    }

    // =-=-=-=-=-=-=-
    // get the cache name
    std::string cache_name;
    ret = _ctx.prop_map().get< std::string >( CACHE_CONTEXT_TYPE, cache_name );
    if (!ret.ok()) {
        return PASSMSG("Failed to get the cache name.", ret);
    }

    // =-=-=-=-=-=-=-
    // get the archive name
    std::string arch_name;
    ret = _ctx.prop_map().get< std::string >( ARCHIVE_CONTEXT_TYPE, arch_name );
    if (!ret.ok()) {
        return PASSMSG("Failed to get the archive name.", ret);
    } // if arch_name

    // =-=-=-=-=-=-=-
    // manufacture a resc hier to either the archive or the cache resc
    std::string keyword  = _stage_sync_kw.data();
    std::string inp_hier = _hier_from_root_to_compound.str();

    std::string tgt_name, src_name;
    if ( keyword == STAGE_OBJ_KW ) {
        tgt_name = cache_name;
        src_name = arch_name;
    }
    else if ( keyword == SYNC_OBJ_KW ) {
        tgt_name = arch_name;
        src_name = cache_name;
    }
    else {
        std::stringstream msg;
        msg << "stage_sync_kw value is unexpected [" << _stage_sync_kw.data() << "]";
        return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
    }

    std::string current_name;
    ret = _ctx.prop_map().get<std::string>( irods::RESOURCE_NAME, current_name );
    if (!ret.ok()) {
        return PASSMSG("Failed to get the resource name.", ret);
    } // if current_name

    size_t pos = inp_hier.find( parent_name );
    if ( std::string::npos == pos ) {
        std::stringstream msg;
        msg << "parent resc ["
            << parent_name
            << "] not in fco resc hier ["
            << inp_hier
            << "]";
        return ERROR(
                SYS_INVALID_INPUT_PARAM,
                msg.str() );
    }

    // =-=-=-=-=-=-=-
    // Generate src and tgt hiers
    std::string dst_hier = inp_hier.substr( 0, pos + parent_name.size() );
    if ( !dst_hier.empty() ) {
        dst_hier += irods::hierarchy_parser::delimiter();
    }
    dst_hier += current_name +
        irods::hierarchy_parser::delimiter() +
        tgt_name;

    std::string src_hier = inp_hier.substr( 0, pos + parent_name.size() );
    if ( !src_hier.empty() ) {
        src_hier += irods::hierarchy_parser::delimiter();
    }
    src_hier += current_name + irods::hierarchy_parser::delimiter() + src_name;

    irods::file_object_ptr obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
    addKeyVal((keyValPair_t*)&obj->cond_input(), _stage_sync_kw.data(), "");

    int source_l1descInx{};
    int destination_l1descInx{};
    const irods::at_scope_exit close_l1_descriptors{
        [&]()
        {
            if (destination_l1descInx > 0) {
                close_replica(_ctx, destination_l1descInx);
            }
            if (source_l1descInx > 0) {
                close_replica(_ctx, source_l1descInx);
            }
        }
    };
    if (STAGE_OBJ_KW == keyword) {
        try {
            source_l1descInx = open_source_replica(_ctx, obj, src_hier);
            destination_l1descInx = open_destination_replica(_ctx, obj, source_l1descInx, dst_hier);
            L1desc[destination_l1descInx].srcL1descInx = source_l1descInx;
        }
        catch (const irods::exception& _e) {
            irods::log(_e);
            return irods::error(_e);
        }

        irods::resource_ptr resc;
        ret = get_archive( _ctx, resc );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << "failed to get child resource [" << current_name << "]";
            return PASSMSG( msg.str(), ret );
        }

        irods::file_object_ptr file_obj(
                new irods::file_object(
                    _ctx.comm(),
                    obj->logical_path().c_str(),
                    L1desc[source_l1descInx].dataObjInfo->filePath, "", 0,
                    getDefFileMode(),
                    L1desc[source_l1descInx].dataObjInfo->flags ) );
        file_obj->resc_hier( src_hier );

        // =-=-=-=-=-=-=-
        // pass condInput
        file_obj->cond_input( L1desc[destination_l1descInx].dataObjInp->condInput );

        // set object id if provided
        char *id_str = getValByKey(&file_obj->cond_input(), DATA_ID_KW);
        if (id_str) {
            file_obj->id(strtol(id_str, NULL, 10));
        }

        dataObjInfo_t* destDataObjInfo = L1desc[destination_l1descInx].dataObjInfo;
        dataObjInfo_t* srcDataObjInfo = L1desc[source_l1descInx].dataObjInfo;

        fileStageSyncInp_t file_stage{};
        rstrcpy( file_stage.cacheFilename, destDataObjInfo->filePath, MAX_NAME_LEN );
        rstrcpy( file_stage.rescHier,      destDataObjInfo->rescHier, MAX_NAME_LEN );
        rstrcpy( file_stage.filename,      srcDataObjInfo->filePath,  MAX_NAME_LEN );
        rstrcpy( file_stage.objPath,       srcDataObjInfo->objPath,   MAX_NAME_LEN );
        file_stage.dataSize = srcDataObjInfo->dataSize;
        file_stage.mode = getFileMode(L1desc[destination_l1descInx].dataObjInp);

        int status = rsFileStageToCache(_ctx.comm(), &file_stage);
        if (status < 0) {
            ret = ERROR(status, "rsFileStageToCache failed");
        }
    }
    else if (SYNC_OBJ_KW == keyword) {
        try {
            source_l1descInx = open_source_replica(_ctx, obj, src_hier);
            destination_l1descInx = open_destination_replica(_ctx, obj, source_l1descInx, dst_hier);
            L1desc[destination_l1descInx].srcL1descInx = source_l1descInx;
            // A note in rsDataObjOpen indicates that the dataSize is set to -1 on purpose
            // and in the case of a replication or phymv, the caller is responsible for
            // setting the dataSize to the appropriate value since the destination might
            // be an archive which can't stat the file for the size. This is one of those
            // cases and so the dataSize is set to the source dataSize here.
            L1desc[destination_l1descInx].dataSize = L1desc[source_l1descInx].dataObjInfo->dataSize;
        }
        catch (const irods::exception& _e) {
            irods::log(_e);
            return irods::error(_e);
        }

        irods::resource_ptr resc;
        ret = get_archive( _ctx, resc );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << "failed to get child resource [" << current_name << "]";
            return PASSMSG( msg.str(), ret );
        }

        irods::file_object_ptr file_obj(
                new irods::file_object(
                    _ctx.comm(),
                    obj->logical_path().c_str(),
                    L1desc[destination_l1descInx].dataObjInfo->filePath, "", 0,
                    getDefFileMode(),
                    L1desc[destination_l1descInx].dataObjInfo->flags ) );
        file_obj->resc_hier( L1desc[destination_l1descInx].dataObjInfo->rescHier );

        // =-=-=-=-=-=-=-
        // pass condInput
        file_obj->cond_input( L1desc[destination_l1descInx].dataObjInp->condInput );

        // set object id if provided
        char *id_str = getValByKey(&file_obj->cond_input(), DATA_ID_KW);
        if (id_str) {
            file_obj->id(strtol(id_str, NULL, 10));
        }

        addKeyVal(&L1desc[destination_l1descInx].dataObjInfo->condInput, NO_COMPUTE_KW, "");

        // Sync to archive!
        int dst_create_path = 0;
        irods::error err = resc->get_property<int>(irods::RESOURCE_CREATE_PATH, dst_create_path);
        if (!err.ok()) {
            irods::log(PASS(err));
        }

        dataObjInfo_t* destDataObjInfo = L1desc[destination_l1descInx].dataObjInfo;
        dataObjInfo_t* srcDataObjInfo = L1desc[source_l1descInx].dataObjInfo;
        if (CREATE_PATH == dst_create_path) {
            dataObjInfo_t tmpDataObjInfo{};
            int status = chkOrphanFile(
                _ctx.comm(),
                destDataObjInfo->filePath,
                destDataObjInfo->rescName,
                &tmpDataObjInfo);
            if ( status == 0 && tmpDataObjInfo.dataId != destDataObjInfo->dataId ) {
                /* someone is using it */
                char tmp_str[MAX_NAME_LEN]{};
                snprintf(tmp_str, MAX_NAME_LEN, "%s.%-u", destDataObjInfo->filePath, irods::getRandom<unsigned int>());
                rstrcpy(destDataObjInfo->filePath, tmp_str, MAX_NAME_LEN);
            }
        }

        fileStageSyncInp_t inp{};
        const auto free_cond_input = irods::at_scope_exit{[&inp] { clearKeyVal(&inp.condInput); }};
        inp.dataSize = srcDataObjInfo->dataSize;

        rstrcpy( inp.filename,      destDataObjInfo->filePath,  MAX_NAME_LEN );
        rstrcpy( inp.rescHier,      destDataObjInfo->rescHier,  MAX_NAME_LEN );
        rstrcpy( inp.objPath,       srcDataObjInfo->objPath,    MAX_NAME_LEN );
        rstrcpy( inp.cacheFilename, srcDataObjInfo->filePath,   MAX_NAME_LEN );

        // add object id keyword to pass down to resource plugins
        const std::string object_id = boost::lexical_cast<std::string>(srcDataObjInfo->dataId);
        addKeyVal(&inp.condInput, DATA_ID_KW, object_id.c_str());

        inp.mode = getFileMode(L1desc[destination_l1descInx].dataObjInp);
        fileSyncOut_t* sync_out = nullptr;
        const auto free_out = irods::at_scope_exit{[&sync_out] { if (sync_out) std::free(sync_out); }};
        int status = rsFileSyncToArch(_ctx.comm(), &inp, &sync_out );

        if (status < 0) {
            ret = ERROR(status, "rsFileSyncToArch failed");
        }
        else if (CREATE_PATH == dst_create_path) {
            // Need to update the physical path with whatever the archive resource came up with
            rstrcpy( destDataObjInfo->filePath, sync_out->file_name, MAX_NAME_LEN );
        }
    }

    if ( !ret.ok() ) {
        return PASS( ret );
    }

    if ( destination_l1descInx < 0 ) {
        std::stringstream msg;
        msg << "Failed to replicate the data object [" << obj->logical_path() << "] ";
        msg << "for operation [" << _stage_sync_kw.data() << "]";
        irods::log(LOG_ERROR, msg.str());
        return ERROR( destination_l1descInx, msg.str() );
    }

    return SUCCESS();
} // repl_object

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX create
irods::error compound_file_create(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // check the context for validity
    irods::error ret = compound_check_param< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        return PASSMSG( "invalid resource context", ret );
    }

    // =-=-=-=-=-=-=-
    // get the next child resource
    irods::resource_ptr resc;
    ret = get_next_child< irods::file_object >( _ctx, resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // forward the call
    return resc->call( _ctx.comm(), irods::RESOURCE_OP_CREATE, _ctx.fco() );

} // compound_file_create

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX Open
irods::error compound_file_open(irods::plugin_context& _ctx)
{
    // =-=-=-=-=-=-=-
    // check the context for validity
    irods::error ret = compound_check_param< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        return PASSMSG( "invalid resource context", ret );
    }

    // =-=-=-=-=-=-=-
    // get the cache resource
    irods::resource_ptr cache_resc;
    if ( !( ret = get_cache_resc< irods::file_object >( _ctx, cache_resc ) ).ok() ) {
        return PASSMSG( "Failed to get cache resource.", ret );
    }

    // =-=-=-=-=-=-=-
    // forward the call
    return cache_resc->call( _ctx.comm(), irods::RESOURCE_OP_OPEN, _ctx.fco() );
} // compound_file_open

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX Read
irods::error compound_file_read(irods::plugin_context& _ctx, void* _buf, int _len)
{
    // =-=-=-=-=-=-=-
    // check the context for validity
    irods::error ret = compound_check_param< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        return PASSMSG( "invalid resource context", ret );
    }

    // =-=-=-=-=-=-=-
    // get the cache resource
    irods::resource_ptr resc;
    ret = get_cache( _ctx, resc );
    if ( !ret.ok() ) {
        return PASSMSG( "Unable to get cache resource.", ret );
    }

    // =-=-=-=-=-=-=-
    // forward the call
    return resc->call< void*, int >( _ctx.comm(), irods::RESOURCE_OP_READ, _ctx.fco(), _buf, _len );
} // compound_file_read

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX Write
irods::error compound_file_write(
    irods::plugin_context& _ctx,
    const void*            _buf,
    const int              _len ) {
    // =-=-=-=-=-=-=-
    // check the context for validity
    irods::error ret = compound_check_param< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        return PASSMSG( "invalid resource context", ret );
    }

    // =-=-=-=-=-=-=-
    // get the cache resource
    irods::resource_ptr resc;
    ret = get_cache( _ctx, resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // forward the call
    return resc->call< const void*, const int >( _ctx.comm(), irods::RESOURCE_OP_WRITE, _ctx.fco(), _buf, _len );

} // compound_file_write

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX Close
irods::error compound_file_close(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // check the context for validity
    irods::error ret = compound_check_param< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        return PASSMSG( "invalid resource context", ret );
    }

    // =-=-=-=-=-=-=-
    // get the cache resource
    irods::resource_ptr resc;
    ret = get_cache( _ctx, resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // forward the call
    ret = resc->call( _ctx.comm(), irods::RESOURCE_OP_CLOSE, _ctx.fco() );
    if ( !ret.ok() ) {
        return PASS( ret );

    }

    return SUCCESS();

} // compound_file_close

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX Unlink
irods::error compound_file_unlink(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // check the context for validity
    irods::error ret = compound_check_param< irods::data_object >( _ctx );
    if ( !ret.ok() ) {
        return PASSMSG( "invalid resource context", ret );
    }

    // =-=-=-=-=-=-=-
    // get the next child resource
    irods::resource_ptr resc;
    ret = get_next_child< irods::data_object >( _ctx, resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // forward the call
    return resc->call( _ctx.comm(), irods::RESOURCE_OP_UNLINK, _ctx.fco() );

} // compound_file_unlink

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX Stat
irods::error compound_file_stat(
    irods::plugin_context& _ctx,
    struct stat*                     _statbuf ) {
    // =-=-=-=-=-=-=-
    // check the context for validity
    irods::error ret = compound_check_param< irods::data_object >( _ctx );
    if ( !ret.ok() ) {
        return PASSMSG( "invalid resource context", ret );
    }

    // =-=-=-=-=-=-=-
    // get the next child resource
    irods::resource_ptr resc;
    ret = get_next_child< irods::data_object >( _ctx, resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // forward the call
    return resc->call< struct stat* >( _ctx.comm(), irods::RESOURCE_OP_STAT, _ctx.fco(), _statbuf );

} // compound_file_stat

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX lseek
irods::error compound_file_lseek(
    irods::plugin_context& _ctx,
    const long long        _offset,
    const int              _whence ) {
    // =-=-=-=-=-=-=-
    // check the context for validity
    irods::error ret = compound_check_param< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        return PASSMSG( "invalid resource context", ret );
    }

    // =-=-=-=-=-=-=-
    // get the next child resource
    irods::resource_ptr resc;
    ret = get_next_child< irods::file_object >( _ctx, resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // forward the call
    return resc->call< const long long, const int >( _ctx.comm(), irods::RESOURCE_OP_LSEEK, _ctx.fco(), _offset, _whence );

} // compound_file_lseek

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX mkdir
irods::error compound_file_mkdir(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // check the context for validity
    irods::error ret = compound_check_param< irods::collection_object >( _ctx );
    if ( !ret.ok() ) {
        return PASSMSG( "invalid resource context", ret );
    }

    // =-=-=-=-=-=-=-
    // get the next child resource
    irods::resource_ptr resc;
    ret = get_next_child< irods::collection_object >( _ctx, resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // forward the call
    return resc->call( _ctx.comm(), irods::RESOURCE_OP_MKDIR, _ctx.fco() );

} // compound_file_mkdir

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX rmdir
irods::error compound_file_rmdir(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // check the context for validity
    irods::error ret = compound_check_param< irods::collection_object >( _ctx );
    if ( !ret.ok() ) {
        return PASSMSG( "invalid resource context", ret );
    }

    // =-=-=-=-=-=-=-
    // get the next child resource
    irods::resource_ptr resc;
    ret = get_next_child< irods::collection_object >( _ctx, resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // forward the call
    return resc->call( _ctx.comm(), irods::RESOURCE_OP_RMDIR, _ctx.fco() );

} // compound_file_rmdir

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX opendir
irods::error compound_file_opendir(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // check the context for validity
    irods::error ret = compound_check_param< irods::collection_object >( _ctx );
    if ( !ret.ok() ) {
        return PASSMSG( "invalid resource context", ret );
    }

    // =-=-=-=-=-=-=-
    // get the next child resource
    irods::resource_ptr resc;
    ret = get_next_child< irods::collection_object >( _ctx, resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // forward the call
    return resc->call( _ctx.comm(), irods::RESOURCE_OP_OPENDIR, _ctx.fco() );

} // compound_file_opendir

// =-=-=-=-=-=-=-
/// @brief interface for POSIX closedir
irods::error compound_file_closedir(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // check the context for validity
    irods::error ret = compound_check_param< irods::collection_object >( _ctx );
    if ( !ret.ok() ) {
        return PASSMSG( "invalid resource context", ret );
    }

    // =-=-=-=-=-=-=-
    // get the next child resource
    irods::resource_ptr resc;
    ret = get_next_child< irods::collection_object >( _ctx, resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // forward the call
    return resc->call( _ctx.comm(), irods::RESOURCE_OP_CLOSEDIR, _ctx.fco() );

} // compound_file_closedir

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX readdir
irods::error compound_file_readdir(
    irods::plugin_context& _ctx,
    struct rodsDirent**                 _dirent_ptr ) {
    // =-=-=-=-=-=-=-
    // check the context for validity
    irods::error ret = compound_check_param< irods::collection_object >( _ctx );
    if ( !ret.ok() ) {
        return PASSMSG( "invalid resource context", ret );
    }

    // =-=-=-=-=-=-=-
    // get the next child resource
    irods::resource_ptr resc;
    ret = get_next_child< irods::collection_object >( _ctx, resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // forward the call
    return resc->call< struct rodsDirent** >( _ctx.comm(), irods::RESOURCE_OP_READDIR, _ctx.fco(), _dirent_ptr );

} // compound_file_readdir

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX rename
irods::error compound_file_rename(
    irods::plugin_context& _ctx,
    const char*                      _new_file_name ) {
    // =-=-=-=-=-=-=-
    // check the context for validity
    irods::error ret = compound_check_param< irods::data_object >( _ctx );
    if ( !ret.ok() ) {
        return PASSMSG( "invalid resource context", ret );
    }

    // =-=-=-=-=-=-=-
    // get the next child resource
    irods::resource_ptr resc;
    ret = get_next_child< irods::data_object >( _ctx, resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // forward the call
    return resc->call<const char*>( _ctx.comm(), irods::RESOURCE_OP_RENAME, _ctx.fco(), _new_file_name );

} // compound_file_rename

/// =-=-=-=-=-=-=-
/// @brief interface for POSIX truncate
irods::error compound_file_truncate(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // check the context for validity
    irods::error ret = compound_check_param< irods::data_object >( _ctx );
    if ( !ret.ok() ) {
        return PASSMSG( "invalid resource context", ret );
    }

    // =-=-=-=-=-=-=-
    // get the next child resource
    irods::resource_ptr resc;
    ret = get_next_child< irods::data_object >( _ctx, resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // forward the call
    return resc->call( _ctx.comm(), irods::RESOURCE_OP_TRUNCATE, _ctx.fco() );

} // compound_file_truncate

/// =-=-=-=-=-=-=-
/// @brief interface to determine free space on a device given a path
irods::error compound_file_getfs_freespace(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // check the context for validity
    irods::error ret = compound_check_param< irods::data_object >( _ctx );
    if ( !ret.ok() ) {
        return PASSMSG( "invalid resource context", ret );
    }

    // =-=-=-=-=-=-=-
    // get the next child resource
    irods::resource_ptr resc;
    ret = get_next_child< irods::data_object >( _ctx, resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // forward the call
    return resc->call(
               _ctx.comm(),
               irods::RESOURCE_OP_FREESPACE,
               _ctx.fco() );

} // compound_file_getfs_freespace


/// =-=-=-=-=-=-=-
/// @brief Move data from the archive leaf node to a local cache leaf node
///        This routine is not supported from a coordinating node's perspective
///        the coordinating node would be calling this on a leaf when necessary
irods::error compound_file_stage_to_cache(
    irods::plugin_context& _ctx,
    const char*                      _cache_file_name ) {
    // =-=-=-=-=-=-=-
    // Check the operation parameters and update the physical path
    irods::error ret = compound_check_param< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << "Invalid resource context";
        return PASSMSG( msg.str(), ret );
    }

    // =-=-=-=-=-=-=-
    // get the archive resource
    irods::resource_ptr resc;
    ret = get_archive( _ctx, resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // forward the call to the archive
    return resc->call< const char* >( _ctx.comm(), irods::RESOURCE_OP_STAGETOCACHE, _ctx.fco(), _cache_file_name );

} // compound_file_stage_to_cache

/// =-=-=-=-=-=-=-
/// @brief Move data from a local cache leaf node to the archive leaf node
///        This routine is not supported from a coordinating node's perspective
///        the coordinating node would be calling this on a leaf when necessary
irods::error compound_file_sync_to_arch(
    irods::plugin_context& _ctx,
    const char*                      _cache_file_name ) {
    // =-=-=-=-=-=-=-
    // Check the operation parameters and update the physical path
    irods::error ret = compound_check_param< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << "Invalid resource context";
        return PASSMSG( msg.str(), ret );
    }

    // =-=-=-=-=-=-=-
    // get the archive resource
    irods::resource_ptr resc;
    get_archive( _ctx, resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // forward the call to the archive
    return resc->call< const char* >( _ctx.comm(), irods::RESOURCE_OP_SYNCTOARCH, _ctx.fco(), _cache_file_name );

} // compound_file_sync_to_arch

/// =-=-=-=-=-=-=-
/// @brief interface to notify of a file registration
irods::error compound_file_registered(
    irods::plugin_context& _ctx ) {
    // =-=-=-=-=-=-=-
    // Check the operation parameters and update the physical path
    irods::error ret = compound_check_param< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << "Invalid resource context";
        return PASSMSG( msg.str(), ret );
    }

    return SUCCESS();

} // compound_file_registered

/// =-=-=-=-=-=-=-
/// @brief interface to notify of a file unregistration
irods::error compound_file_unregistered(
    irods::plugin_context& _ctx ) {
    // Check the operation parameters and update the physical path
    irods::error ret = compound_check_param< irods::file_object >( _ctx );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << "Invalid resource context";
        return PASSMSG( msg.str(), ret );
    }
    // NOOP
    return SUCCESS();

} // compound_file_unregistered

static bool auto_replication_is_enabled(
    irods::plugin_context& _ctx ) {
    std::string auto_repl;
    irods::error ret = _ctx.prop_map().get<std::string>(
                           AUTO_REPL_POLICY,
                           auto_repl );
    if( ret.ok() ) {
        if( AUTO_REPL_POLICY_ENABLED != auto_repl ) {
            return false;
        }
    }
    return true;
} // auto_replication_is_enabled

/// =-=-=-=-=-=-=-
/// @brief interface to notify of a file modification - this happens
///        after the close operation and the icat should be up to date
///        at this point
irods::error compound_file_modified(
    irods::plugin_context& _ctx ) {
    irods::error result = SUCCESS();

    // =-=-=-=-=-=-=-
    // Check the operation parameters and update the physical path
    if(!auto_replication_is_enabled(_ctx)) {
        return SUCCESS();
    }

    irods::error ret = compound_check_param< irods::file_object >( _ctx );
    if (!ret.ok()) {
        return PASSMSG("Invalid resource context.", ret);
    }

    std::string name{};
    ret = _ctx.prop_map().get<std::string>( irods::RESOURCE_NAME, name );
    if (!ret.ok()) {
        return PASSMSG("Failed to get the resource name.", ret);
    }

    irods::file_object_ptr file_obj = boost::dynamic_pointer_cast<irods::file_object>(_ctx.fco());
    irods::hierarchy_parser sub_parser;
    sub_parser.set_string( file_obj->in_pdmo() );
    if ( !sub_parser.resc_in_hier( name ) ) {
        irods::hierarchy_parser parser{file_obj->resc_hier()};
        result = repl_object( _ctx, parser, SYNC_OBJ_KW );
    }
    return result;

} // compound_file_modified

/// @brief interface to notify of a file operation
irods::error compound_file_notify(irods::plugin_context& _ctx, const std::string* _opr)
{
    // Check the operation parameters and update the physical path
    irods::error ret = compound_check_param< irods::file_object >( _ctx );
    if (!ret.ok()) {
        return PASSMSG("Invalid resource context.", ret);
    }

    std::string operation;
    ret = _ctx.prop_map().get< std::string >( OPERATION_TYPE, operation );
    if ( ret.ok() ) {
        rodsLog(LOG_DEBUG,
                "compound_file_notify - oper [%s] changed to [%s]",
                _opr->c_str(),
                operation.c_str() );
    } // if ret ok

    if ( irods::WRITE_OPERATION == ( *_opr ) ||
            irods::CREATE_OPERATION == ( *_opr ) ) {
        _ctx.prop_map().set< std::string >( OPERATION_TYPE, ( *_opr ) );
    }
    else {
        rodsLog(LOG_DEBUG,
                "compound_file_notify - skipping [%s]",
                _opr->c_str() );
    }

    return SUCCESS();
} // compound_file_notify

// =-=-=-=-=-=-=-
/// @brief - code to determine redirection for create operation
irods::error compound_file_redirect_create(
    irods::plugin_context&   _ctx,
    const std::string&       _operation,
    const std::string*       _curr_host,
    irods::hierarchy_parser* _out_parser,
    float*                   _out_vote)
{
    // =-=-=-=-=-=-=-
    // determine if the resource is down
    int resc_status = 0;
    irods::error ret = _ctx.prop_map().get< int >( irods::RESOURCE_STATUS, resc_status );
    if ( !ret.ok() ) {
        return PASSMSG( "failed to get 'status' property", ret );
    }

    // =-=-=-=-=-=-=-
    // if the status is down, vote no.
    if ( INT_RESC_STATUS_DOWN == resc_status ) {
        *_out_vote = 0.0f;
        return SUCCESS();
    }

    // =-=-=-=-=-=-=-
    // get the cache resource
    irods::resource_ptr resc;
    ret = get_cache( _ctx, resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // ask the cache if it is willing to accept a new file, politely
    ret = resc->call<const std::string*, const std::string*, irods::hierarchy_parser*, float*>(
        _ctx.comm(), irods::RESOURCE_OP_RESOLVE_RESC_HIER, _ctx.fco(),
        &_operation, _curr_host,
        _out_parser, _out_vote);

    // =-=-=-=-=-=-=-
    // set the operation type to signal that we need to do some work
    // in file modified
    _ctx.prop_map().set< std::string >( OPERATION_TYPE, _operation );

    return ret;

} // compound_file_redirect_create

// =-=-=-=-=-=-=-
/// @brief - code to determine redirection for unlink operation
irods::error compound_file_redirect_unlink(
    irods::plugin_context& _ctx,
    const std::string&               _operation,
    const std::string*               _curr_host,
    irods::hierarchy_parser*         _out_parser,
    float*                           _out_vote ) {
    // =-=-=-=-=-=-=-
    // determine if the resource is down
    int resc_status = 0;
    irods::error ret = _ctx.prop_map().get< int >( irods::RESOURCE_STATUS, resc_status );
    if ( !ret.ok() ) {
        return PASSMSG( "failed to get 'status' property", ret );
    }

    // =-=-=-=-=-=-=-
    // if the status is down, vote no.
    if ( INT_RESC_STATUS_DOWN == resc_status ) {
        ( *_out_vote ) = 0.0;
        return SUCCESS();
    }

    // =-=-=-=-=-=-=-
    // get the cache resource
    irods::resource_ptr resc;
    ret = get_cache( _ctx, resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    std::string cache_name;
    _ctx.prop_map().get< std::string >( CACHE_CONTEXT_TYPE, cache_name );

    // =-=-=-=-=-=-=-
    // ask the cache if it is willing to accept a new file, politely
    ret = resc->call < const std::string*, const std::string*,
    irods::hierarchy_parser*, float* > (
        _ctx.comm(), irods::RESOURCE_OP_RESOLVE_RESC_HIER, _ctx.fco(),
        &_operation, _curr_host,
        _out_parser, _out_vote );

    // =-=-=-=-=-=-=-
    // if cache votes non zero we're done
    if (*_out_vote > 0.0) {
        return SUCCESS();
    }

    // unixfilesystem adds itself to the hierarchy - remove here as it voted 0
    // TODO: hierarchies as a cache/archive would break here
    _out_parser->remove_resource(cache_name);

    std::string archive_name;
    _ctx.prop_map().get< std::string >( ARCHIVE_CONTEXT_TYPE, archive_name );

    // =-=-=-=-=-=-=-
    // otherwise try the archive
    ret = get_archive( _ctx, resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // ask the archive if it is willing to accept a new file, politely
    ret = resc->call<const std::string*, const std::string*, irods::hierarchy_parser*, float*> (
        _ctx.comm(), irods::RESOURCE_OP_RESOLVE_RESC_HIER, _ctx.fco(),
        &_operation, _curr_host, _out_parser, _out_vote );

    return ret;

} // compound_file_redirect_unlink

// =-=-=-=-=-=-=-
/// @brief - handler for prefer archive policy
irods::error open_for_prefer_archive_policy(
    irods::plugin_context&   _ctx,
    const std::string&       _curr_host,
    irods::hierarchy_parser& _out_parser,
    float&                   _out_vote)
{
    // =-=-=-=-=-=-=-
    // get the archive resource
    irods::resource_ptr arch_resc;
    irods::error ret = get_archive( _ctx, arch_resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get the cache resource
    irods::resource_ptr cache_resc;
    ret = get_cache( _ctx, cache_resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    std::string operation{irods::OPEN_OPERATION};
    ret = _ctx.prop_map().get<std::string>(OPERATION_TYPE, operation);
    if (!ret.ok()) {
        rodsLog(LOG_NOTICE, "[%s] - operation not found in property map; using open.", __FUNCTION__);
    }

    // =-=-=-=-=-=-=-
    // repave the repl requested temporarily
    irods::file_object_ptr f_ptr = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
    int repl_requested = f_ptr->repl_requested();
    const irods::at_scope_exit restore_repl_requested{[&] { f_ptr->repl_requested(repl_requested); }};

    float arch_check_vote = 0.0f;
    irods::hierarchy_parser arch_check_parser = _out_parser;

    try {
        std::string archive_name;
        _ctx.prop_map().get<std::string>(ARCHIVE_CONTEXT_TYPE, archive_name);
        f_ptr->repl_requested(get_archive_replica_number(f_ptr, archive_name));

        // =-=-=-=-=-=-=-
        // ask the archive if it has the data object in question, politely
        ret = arch_resc->call<const std::string*, const std::string*, irods::hierarchy_parser*, float*>(
            _ctx.comm(), irods::RESOURCE_OP_RESOLVE_RESC_HIER, _ctx.fco(),
            &operation, &_curr_host,
            &arch_check_parser, &arch_check_vote);
    }
    catch (const irods::exception& e) {
        // continue to the cache vote because failed vote isn't necessarily an error
        arch_check_vote = 0.0;
        irods::log(LOG_DEBUG, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.what()));
    }

    if ( !ret.ok() || 0.0 == arch_check_vote ) {
        rodsLog(
            LOG_DEBUG,
            "replica not found in archive for [%s]",
            f_ptr->logical_path().c_str() );
        // =-=-=-=-=-=-=-
        // the archive query redirect failed, something terrible happened
        // or mounted collection hijinks are afoot.  ask the cache if it
        // has the data object in question, politely as a fallback
        float cache_check_vote = 0.0;
        irods::hierarchy_parser cache_check_parser = _out_parser;
        ret = cache_resc->call<const std::string*, const std::string*, irods::hierarchy_parser*, float*>(
            _ctx.comm(), irods::RESOURCE_OP_RESOLVE_RESC_HIER, _ctx.fco(),
            &operation, &_curr_host,
            &cache_check_parser, &cache_check_vote);
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        if (0.0 == cache_check_vote ) {
            _out_vote = 0.0;
            return SUCCESS();
        }

        // =-=-=-=-=-=-=-
        // set the vote and hier parser
        _out_parser = cache_check_parser;
        _out_vote = cache_check_vote;

        return SUCCESS();
    }

    irods::data_object_ptr d_ptr = boost::dynamic_pointer_cast<irods::data_object>(f_ptr);
    add_key_val(d_ptr, NO_CHK_COPY_LEN_KW, "prefer_archive_policy");

    // =-=-=-=-=-=-=-
    // if the vote is 0 then we do a wholesale stage, not an update
    // otherwise it is an update operation for the stage to cache.
    ret = repl_object(_ctx, _out_parser, STAGE_OBJ_KW);
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    remove_key_val(d_ptr, NO_CHK_COPY_LEN_KW);

    // =-=-=-=-=-=-=-
    // get the parent name
    std::string parent_id_str;
    ret = _ctx.prop_map().get< std::string >( irods::RESOURCE_PARENT, parent_id_str );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    std::string parent_name;
    ret = resc_mgr.resc_id_to_name(parent_id_str, parent_name);
    if (!ret.ok()) {
        return PASS(ret);
    }

    // =-=-=-=-=-=-=-
    // get this resc name
    std::string current_name;
    ret = _ctx.prop_map().get<std::string>( irods::RESOURCE_NAME, current_name );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get the cache name
    std::string cache_name;
    ret = _ctx.prop_map().get< std::string >( CACHE_CONTEXT_TYPE, cache_name );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // get the current hier
    std::string inp_hier = _out_parser.str();

    // =-=-=-=-=-=-=-
    // find the parent in the hier
    // TODO: the input hierarchy is some other hierarchy
    size_t pos = inp_hier.find( parent_name );
    if (std::string::npos == pos) {
        return ERROR(SYS_INVALID_INPUT_PARAM, "parent resc not in fco resc hier");
    }

    // =-=-=-=-=-=-=-
    // create the new resc hier
    std::string dst_hier = inp_hier.substr( 0, pos + parent_name.size() );
    if ( !dst_hier.empty() ) {
        dst_hier += irods::hierarchy_parser::delimiter();
    }
    dst_hier += current_name +
                irods::hierarchy_parser::delimiter() +
                cache_name;
    rodsLog(LOG_DEBUG, "[%s:%d] - inp_hier:[%s],dst_hier:[%s]",
        __FUNCTION__, __LINE__,
        inp_hier.c_str(), dst_hier.c_str());

    // =-=-=-=-=-=-=-
    // set the vote and hier parser
    _out_parser.set_string( dst_hier );
    _out_vote = arch_check_vote;

    return SUCCESS();
} // open_for_prefer_archive_policy

// =-=-=-=-=-=-=-
/// @brief - handler for prefer cache policy
irods::error open_for_prefer_cache_policy(
    irods::plugin_context& _ctx,
    const std::string*               _opr,
    const std::string*               _curr_host,
    irods::hierarchy_parser*        _out_parser,
    float*                           _out_vote )
{
    // =-=-=-=-=-=-=-
    // check incoming parameters
    if ( !_curr_host ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "null operation" );
    }
    if ( !_out_parser ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "null outgoing hier parser" );
    }
    if ( !_out_vote ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "null outgoing vote" );
    }

    // =-=-=-=-=-=-=-
    // get the archive resource
    irods::resource_ptr arch_resc;
    irods::error ret = get_archive( _ctx, arch_resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // If resource hierarchy is provided and it is the archive, use it for open
    std::string archive_resc_name{};
    ret = arch_resc->get_property<std::string>(
            irods::RESOURCE_NAME,
            archive_resc_name );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    irods::file_object_ptr obj = boost::dynamic_pointer_cast<irods::file_object>(_ctx.fco());
    const char* hier = getValByKey((keyValPair_t*)&obj->cond_input(), RESC_HIER_STR_KW);
    if (hier) {
        irods::hierarchy_parser parser;
        parser.set_string(hier);
        if (parser.resc_in_hier(archive_resc_name)) {
            *_out_vote = 1.0;
            *_out_parser = parser;
            return SUCCESS();
        }
    }

    // =-=-=-=-=-=-=-
    // get the cache resource
    irods::resource_ptr cache_resc;
    ret = get_cache( _ctx, cache_resc );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    std::string cache_resc_name;
    ret = cache_resc->get_property<std::string>(
            irods::RESOURCE_NAME,
            cache_resc_name );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    std::string operation{irods::OPEN_OPERATION};
    ret = _ctx.prop_map().get<std::string>(OPERATION_TYPE, operation);
    if (!ret.ok()) {
        rodsLog(LOG_NOTICE, "[%s] - operation not found in property map; using open.", __FUNCTION__);
    }

    // =-=-=-=-=-=-=-
    // ask the cache if it has the data object in question, politely
    float                    cache_check_vote   = 0.0;
    irods::hierarchy_parser cache_check_parser = ( *_out_parser );
    ret = cache_resc->call < const std::string*, const std::string*, irods::hierarchy_parser*, float* > (
        _ctx.comm(), irods::RESOURCE_OP_RESOLVE_RESC_HIER, _ctx.fco(),
        &operation, _curr_host,
        &cache_check_parser, &cache_check_vote );
    if ( !ret.ok() ) {
        irods::log(ret);
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // if the vote is 0 then the cache doesn't have it so it will need be staged
    if ( 0.0 == cache_check_vote ) {
        // =-=-=-=-=-=-=-
        // repave the repl requested temporarily
        irods::file_object_ptr f_ptr = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );
        int repl_requested = f_ptr->repl_requested();
        f_ptr->repl_requested( -1 );

        // =-=-=-=-=-=-=-
        // ask the archive if it has the data object in question, politely
        float arch_check_vote  = 0.0;
        irods::hierarchy_parser arch_check_parser = *_out_parser;

        ret = arch_resc->call<const std::string*, const std::string*, irods::hierarchy_parser*, float*>(
            _ctx.comm(),
            irods::RESOURCE_OP_RESOLVE_RESC_HIER,
            _ctx.fco(),
            &operation,
            _curr_host,
            &arch_check_parser,
            &arch_check_vote);

        if ( !ret.ok() ) {
            irods::log(ret);
            return PASS( ret );
        }

        if( 0.0 == arch_check_vote ) {
            *_out_vote = 0.0;
            return SUCCESS();
        }

        if( irods::UNLINK_OPERATION == ( *_opr ) ) {
            ( *_out_parser ) = arch_check_parser;
            ( *_out_vote )   = arch_check_vote;
            return SUCCESS();
        }

        const auto old_resc_hier = f_ptr->resc_hier();

        // =-=-=-=-=-=-=-
        // repave the resc hier with the archive hier which guarantees that
        // we are in the hier for the repl to do its magic. this is a hack,
        // and will need refactored later with an improved object model
        std::string arch_hier;
        arch_check_parser.str( arch_hier );
        f_ptr->resc_hier( arch_hier );

        // Make sure the file object's resource hierarchy is restored on exit.
        irods::at_scope_exit restore_resc_hier{[&old_resc_hier, f_ptr] {
            f_ptr->resc_hier(old_resc_hier);
        }};

        // =-=-=-=-=-=-=-
        // if the archive has it, then replicate
        ret = repl_object( _ctx, arch_check_parser, STAGE_OBJ_KW );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // restore repl requested
        f_ptr->repl_requested( repl_requested );

        ( *_out_parser ) = cache_check_parser;
        ( *_out_vote ) = arch_check_vote;
    }
    else {
        // =-=-=-=-=-=-=-
        // else it is in the cache so assign the parser
        ( *_out_vote )   = cache_check_vote;
        ( *_out_parser ) = cache_check_parser;
    }

    return SUCCESS();
} // open_for_prefer_cache_policy

// =-=-=-=-=-=-=-
/// @brief - code to determine redirection for the open operation
///          determine the user set policy for staging to cache
///          otherwise the default is to compare checksum
irods::error compound_file_redirect_open(
    irods::plugin_context&   _ctx,
    const std::string*       _opr,
    const std::string*       _curr_host,
    irods::hierarchy_parser* _out_parser,
    float*                   _out_vote)
{
    // =-=-=-=-=-=-=-
    // check incoming parameters
    if ( !_curr_host ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "null operation" );
    }
    if ( !_out_parser ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "null outgoing hier parser" );
    }
    if ( !_out_vote ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "null outgoing vote" );
    }

    // =-=-=-=-=-=-=-
    // determine if the resource is down
    int resc_status = 0;
    irods::error ret = _ctx.prop_map().get< int >( irods::RESOURCE_STATUS, resc_status );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

    // =-=-=-=-=-=-=-
    // if the status is down, vote no.
    if ( INT_RESC_STATUS_DOWN == resc_status ) {
        ( *_out_vote ) = 0.0;
        return SUCCESS();
    }

    // =-=-=-=-=-=-=-
    // acquire the value of the stage policy from the results string
    std::string policy;
    ret = get_stage_policy( _ctx.rule_results(), policy );

    // =-=-=-=-=-=-=-
    // if the policy is prefer cache then if the cache has the object
    // return an upvote
    if ( policy.empty() || irods::RESOURCE_STAGE_PREFER_CACHE == policy ) {
        return open_for_prefer_cache_policy( _ctx, _opr, _curr_host, _out_parser, _out_vote );
    }

    // =-=-=-=-=-=-=-
    // if the policy is "always", then if the archive has it
    // stage from archive to cache and return an upvote
    if ( irods::RESOURCE_STAGE_PREFER_ARCHIVE == policy ) {
        return open_for_prefer_archive_policy( _ctx, *_curr_host, *_out_parser, *_out_vote );
    }

    auto msg = fmt::format("invalid stage policy [{}]", policy);
    irods::log(LOG_ERROR, msg);
    return ERROR(SYS_INVALID_INPUT_PARAM, std::move(msg));
} // compound_file_redirect_open

void replace_archive_for_replica(
    irods::plugin_context& ctx,
    const irods::hierarchy_parser& voted_hier)
{
    irods::resource_ptr arch_resc;
    get_archive(ctx, arch_resc);
    std::string archive_resc_name{};
    arch_resc->get_property<std::string>(irods::RESOURCE_NAME, archive_resc_name);

    irods::file_object_ptr file_obj = boost::dynamic_pointer_cast<irods::file_object>(ctx.fco());
    for (auto& r : file_obj->replicas()) {
        rodsLog(LOG_DEBUG,
            "[%s:%d] - vote:[%f],voted_hier:[%s],hier:[%s],arch:[%s]",
            __FUNCTION__, __LINE__,
            r.vote(),
            voted_hier.str().c_str(),
            r.resc_hier().c_str(),
            archive_resc_name.c_str());
        if (irods::hierarchy_parser{r.resc_hier()}.last_resc() == archive_resc_name) {
            r.resc_hier(voted_hier.str());
            break;
        }
    }
} // replace_archive_for_replica

/// =-=-=-=-=-=-=-
/// @brief - used to allow the resource to determine which host
///          should provide the requested operation
irods::error compound_file_resolve_hierarchy(
    irods::plugin_context&   _ctx,
    const std::string*       _opr,
    const std::string*       _curr_host,
    irods::hierarchy_parser* _out_parser,
    float*                   _out_vote)
{
    // =-=-=-=-=-=-=-
    // check the context validity
    irods::error ret = _ctx.valid< irods::file_object >();
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << "resource context is invalid";
        return PASSMSG( msg.str(), ret );
    }

    // =-=-=-=-=-=-=-
    // check incoming parameters
    if ( !_opr ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "null operation" );
    }
    if ( !_curr_host ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "null operation" );
    }
    if ( !_out_parser ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "null outgoing hier parser" );
    }
    if ( !_out_vote ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "null outgoing vote" );
    }

    ( *_out_vote ) = 0.0f;

    // =-=-=-=-=-=-=-
    // get the name of this resource
    std::string resc_name;
    ret = _ctx.prop_map().get< std::string >( irods::RESOURCE_NAME, resc_name );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << "failed in get property for name";
        return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
    }

    // =-=-=-=-=-=-=-
    // add ourselves to the hierarchy parser by default
    _out_parser->add_child( resc_name );

    // =-=-=-=-=-=-=-
    // test the operation to determine which choices to make
    if (irods::OPEN_OPERATION == *_opr || irods::WRITE_OPERATION == *_opr) {
        _ctx.prop_map().set< std::string >( OPERATION_TYPE, ( *_opr ) );
        // call redirect determination for 'open' / 'write' operation
        const auto err = compound_file_redirect_open( _ctx, _opr, _curr_host, _out_parser, _out_vote );

        if (err.ok()) {
            replace_archive_for_replica(_ctx, *_out_parser);
        }

        return err;
    }

    if (irods::CREATE_OPERATION == ( *_opr )) {
        // call redirect determination for 'create' operation
        return compound_file_redirect_create( _ctx, ( *_opr ), _curr_host, _out_parser, _out_vote );
    }

    if (irods::UNLINK_OPERATION == ( *_opr )) {
        // call redirect determination for 'unlink' operation
        return compound_file_redirect_unlink( _ctx, ( *_opr ), _curr_host, _out_parser, _out_vote );
    }

    // =-=-=-=-=-=-=-
    // must have been passed a bad operation
    return ERROR(-1, fmt::format("operation not supported [{}]", *_opr));
} // compound_file_resolve_hierarchy

/// =-=-=-=-=-=-=-
/// @brief - code which would rebalance the subtree
irods::error compound_file_rebalance(
    irods::plugin_context& _ctx ) {
    irods::resource_child_map* cmap_ref;
    _ctx.prop_map().get< irods::resource_child_map* >(
            irods::RESC_CHILD_MAP_PROP,
            cmap_ref );
    // =-=-=-=-=-=-=-
    // forward request for rebalance to children
    irods::error result = SUCCESS();
    irods::resource_child_map::iterator itr = cmap_ref->begin();
    for ( ; itr != cmap_ref->end(); ++itr ) {
        irods::error ret = itr->second.second->call(
                               _ctx.comm(),
                               irods::RESOURCE_OP_REBALANCE,
                               _ctx.fco() );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            result = ret;
        }
    }

    if ( !result.ok() ) {
        return PASS( result );
    }

    return SUCCESS();

} // compound_file_rebalance


// =-=-=-=-=-=-=-
// 3. create derived class to handle universal mss resources
//    context string will hold the script to be called.
class compound_resource : public irods::resource {
    public:
        compound_resource(const std::string& _inst_name, const std::string& _context)
            : irods::resource(_inst_name, _context)
        {
            // =-=-=-=-=-=-=-
            // set the start operation to identify the cache and archive children
            set_start_operation(compound_start_operation);

            // =-=-=-=-=-=-=-
            // parse context string into property pairs assuming a ; as a separator
            std::vector<std::string> props;
            irods::kvp_map_t kvp;
            irods::error ret = irods::parse_kvp_string(_context, kvp);
            if (!ret.ok()) {
                rodsLog(LOG_ERROR, "libcompound: invalid context [%s]", _context.c_str());
            }

            // =-=-=-=-=-=-=-
            // copy the properties from the context to the prop map
            for (auto itr = kvp.begin(); itr != kvp.end(); ++itr) {
                properties_.set<std::string>(itr->first, itr->second);
            }
        }

        // =-=-=-=-=-=-
        // override from plugin_base
        irods::error need_post_disconnect_maintenance_operation( bool& _flg ) {
            _flg = false;
            return SUCCESS();
        }

        // =-=-=-=-=-=-
        // override from plugin_base
        irods::error post_disconnect_maintenance_operation( irods::pdmo_type& ) {
            return ERROR( -1, "nop" );
        }
}; // class compound_resource

// =-=-=-=-=-=-=-
// 4. create the plugin factory function which will return a dynamically
//    instantiated object of the previously defined derived resource.  use
//    the add_operation member to associate a 'call name' to the interfaces
//    defined above.  for resource plugins these call names are standardized
//    as used by the irods facing interface defined in
//    server/drivers/src/fileDriver.cpp
extern "C"
irods::resource* plugin_factory(const std::string& _inst_name,
                                const std::string& _context)
{
    // =-=-=-=-=-=-=-
    // 4a. create compound_resource object
    compound_resource* resc = new compound_resource( _inst_name, _context );

    // =-=-=-=-=-=-=-
    // 4b. map function names to operations.  this map will be used to load
    //     the symbols from the shared object in the delay_load stage of
    //     plugin loading.

    using namespace irods;
    using namespace std;
    resc->add_operation(
        RESOURCE_OP_CREATE,
        function<error(plugin_context&)>(
            compound_file_create ) );

    resc->add_operation(
        irods::RESOURCE_OP_OPEN,
        function<error(plugin_context&)>(
            compound_file_open ) );

    resc->add_operation(
        irods::RESOURCE_OP_READ,
        std::function<
            error(irods::plugin_context&,void*,const int)>(
                compound_file_read ) );

    resc->add_operation(
        irods::RESOURCE_OP_WRITE,
        function<error(plugin_context&,const void*,const int)>(
            compound_file_write ) );

    resc->add_operation(
        RESOURCE_OP_CLOSE,
        function<error(plugin_context&)>(
            compound_file_close ) );

    resc->add_operation(
        irods::RESOURCE_OP_UNLINK,
        function<error(plugin_context&)>(
            compound_file_unlink ) );

    resc->add_operation(
        irods::RESOURCE_OP_STAT,
        function<error(plugin_context&, struct stat*)>(
            compound_file_stat ) );

    resc->add_operation(
        irods::RESOURCE_OP_MKDIR,
        function<error(plugin_context&)>(
            compound_file_mkdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_OPENDIR,
        function<error(plugin_context&)>(
            compound_file_opendir ) );

    resc->add_operation(
        irods::RESOURCE_OP_READDIR,
        function<error(plugin_context&,struct rodsDirent**)>(
            compound_file_readdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_RENAME,
        function<error(plugin_context&, const char*)>(
            compound_file_rename ) );

    resc->add_operation(
        irods::RESOURCE_OP_FREESPACE,
        function<error(plugin_context&)>(
            compound_file_getfs_freespace ) );

    resc->add_operation(
        irods::RESOURCE_OP_LSEEK,
        function<error(plugin_context&, const long long, const int)>(
            compound_file_lseek ) );

    resc->add_operation(
        irods::RESOURCE_OP_RMDIR,
        function<error(plugin_context&)>(
            compound_file_rmdir ) );

    resc->add_operation(
        irods::RESOURCE_OP_CLOSEDIR,
        function<error(plugin_context&)>(
            compound_file_closedir ) );

    resc->add_operation(
        irods::RESOURCE_OP_STAGETOCACHE,
        function<error(plugin_context&, const char*)>(
            compound_file_stage_to_cache ) );

    resc->add_operation(
        irods::RESOURCE_OP_SYNCTOARCH,
        function<error(plugin_context&, const char*)>(
            compound_file_sync_to_arch ) );

    resc->add_operation(
        irods::RESOURCE_OP_REGISTERED,
        function<error(plugin_context&)>(
            compound_file_registered ) );

    resc->add_operation(
        irods::RESOURCE_OP_UNREGISTERED,
        function<error(plugin_context&)>(
            compound_file_unregistered ) );

    resc->add_operation(
        irods::RESOURCE_OP_MODIFIED,
        function<error(plugin_context&)>(
            compound_file_modified ) );

    resc->add_operation(
        irods::RESOURCE_OP_NOTIFY,
        function<error(plugin_context&, const std::string*)>(
            compound_file_notify ) );

    resc->add_operation(
        irods::RESOURCE_OP_TRUNCATE,
        function<error(plugin_context&)>(
            compound_file_truncate ) );

    resc->add_operation(
        irods::RESOURCE_OP_RESOLVE_RESC_HIER,
        function<error(plugin_context&,const std::string*, const std::string*, irods::hierarchy_parser*, float*)>(
            compound_file_resolve_hierarchy ) );

    resc->add_operation(
        irods::RESOURCE_OP_REBALANCE,
        function<error(plugin_context&)>(
            compound_file_rebalance ) );

    // =-=-=-=-=-=-=-
    // set some properties necessary for backporting to iRODS legacy code
    resc->set_property< int >( irods::RESOURCE_CHECK_PATH_PERM, 2 );//DO_CHK_PATH_PERM );
    resc->set_property< int >( irods::RESOURCE_CREATE_PATH,     1 );//CREATE_PATH );

    // =-=-=-=-=-=-=-
    // 4c. return the pointer through the generic interface of an
    //     irods::resource pointer
    return dynamic_cast<irods::resource*>( resc );

} // plugin_factory
