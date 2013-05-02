/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Plug-in defining a replicating resource. This resource makes sure that all of its data is replicated to all of its children
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// =-=-=-=-=-=-=-
// irods includes
#include "msParam.h"
#include "reGlobalsExtern.h"
#include "rodsLog.h"
#include "icatHighLevelRoutines.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_resource_plugin.h"
#include "eirods_file_object.h"
#include "eirods_collection_object.h"
#include "eirods_string_tokenize.h"
#include "eirods_hierarchy_parser.h"
#include "eirods_resource_backport.h"
#include "eirods_plugin_base.h"
#include "eirods_stacktrace.h"
#include "eirods_repl_types.h"
#include "eirods_object_oper.h"
#include "eirods_replicator.h"
#include "eirods_create_write_replicator.h"
#include "eirods_unlink_replicator.h"

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <list>

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

extern "C" {

    // =-=-=-=-=-=-=-
    // 1. Define plugin Version Variable, used in plugin
    //    creation when the factory function is called.
    //    -- currently only 1.0 is supported.
    double EIRODS_PLUGIN_INTERFACE_VERSION=1.0;

    // =-=-=-=-=-=-=-
    // 2. Define operations which will be called by the file*
    //    calls declared in server/driver/include/fileDriver.h
    // =-=-=-=-=-=-=-

    // =-=-=-=-=-=-=-
    // NOTE :: to access properties in the _prop_map do the 
    //      :: following :
    //      :: double my_var = 0.0;
    //      :: eirods::error ret = _prop_map.get< double >( "my_key", my_var ); 
    // =-=-=-=-=-=-=-

    //////////////////////////////////////////////////////////////////////
    // Utility functions

    /// @brief Check the general parameters passed in to most plugin functions
    eirods::error replCheckParams(
        eirods::resource_operation_context* _ctx)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        // =-=-=-=-=-=-=-
        // check incoming parameters
        if( !_ctx ) {
            result = ERROR( SYS_INVALID_INPUT_PARAM, "null resource_property_map" );
        } 
        
        // =-=-=-=-=-=-=-
        // verify that the resc context is valid 
        ret = _ctx->valid(); 
        if( !ret.ok() ) { 
            result = PASSMSG( "resource context is invalid", ret );
        }
        
        return result;
    }

    /**
     * @brief Gets the name of the child of this resource from the hierarchy
     */
    eirods::error replGetNextRescInHier(
        const eirods::hierarchy_parser& _parser,
        eirods::resource_operation_context* _ctx,
        eirods::resource_ptr& _ret_resc)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        std::string this_name;
        ret = _ctx->prop_map().get<std::string>("name", this_name);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get resource name from property map.";
            result = ERROR(-1, msg.str());
        } else {
            std::string child;
            ret = _parser.next(this_name, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in the hierarchy.";
                result = ERROR(-1, msg.str());
            } else {
                _ret_resc = (_ctx->child_map())[child].second;
            }
        }
        return result;
    }

    /// @brief Returns true if the specified object is in the specified object list
    bool replObjectInList(
        const object_list_t& _object_list,
        const eirods::file_object& _object,
        eirods::object_oper& _rtn_oper)
    {
        bool result = false;
        object_list_t::const_iterator it;
        for(it = _object_list.begin(); !result && it != _object_list.end(); ++it) {
            eirods::object_oper oper = *it;
            if(oper.object() == _object) {
                _rtn_oper = oper;
                result = true;
            }
        }
        return result;
    }

    /// @brief Updates the fields in the resources properties for the object
    eirods::error replUpdateObjectAndOperProperties(
        eirods::resource_operation_context* _ctx,
        const std::string& _oper)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        object_list_t object_list;

        // The object list is now a queue of operations and their associated objects. Their corresponding replicating operations
        // will be performed one at a time in the order in which they were put into the queue.
        eirods::file_object* object = dynamic_cast<eirods::file_object*>(&(_ctx->fco()));
        if(object != NULL) {
            ret = _ctx->prop_map().get<object_list_t>(object_list_prop, object_list);
            eirods::object_oper oper;
            if(!ret.ok() && ret.code() != EIRODS_KEY_NOT_FOUND) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the object list from the resource.";
                result = PASSMSG(msg.str(), ret);
            } else if(replObjectInList(object_list, *object, oper)) {
                // confirm the operations are compatible
                bool mismatched = false;
                if(_oper == create_oper) {
                    if(oper.operation() != create_oper) {
                        mismatched = true;
                    }
                } else if(_oper == write_oper) {
                    // write is allowed after create
                    if(oper.operation() != create_oper && oper.operation() != write_oper) {
                        mismatched = true;
                    }
                } else if(_oper == unlink_oper) {
                    if(oper.operation() != unlink_oper) {
                        mismatched = true;
                    }
                }
                if(mismatched) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Existing object's operation \"" << oper.operation() << "\"";
                    msg << " does not match current operation \"" << _oper << "\"";
                    result = ERROR(-1, msg.str());
                }
            } else {
                oper.object() = *object;
                oper.operation() = _oper;
                object_list.push_back(oper);
                ret = _ctx->prop_map().set<object_list_t>(object_list_prop, object_list);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed to set the object list property on the resource.";
                    result = PASSMSG(msg.str(), ret);
                }
            }
        } else {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed casting first class object \"";
            msg << _ctx->fco().logical_path();
            msg << "\" to file object.";
            result = ERROR(-1, msg.str());
        }
        return result;
    }

    eirods::error get_selected_hierarchy(
        eirods::resource_operation_context* _ctx,
        std::string& _hier_string,
        std::string& _root_resc)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        eirods::hierarchy_parser selected_parser;
        ret = _ctx->prop_map().get<eirods::hierarchy_parser>(hierarchy_prop, selected_parser);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the parser for the selected resource hierarchy.";
            result = PASSMSG(msg.str(), ret);
        } else {
            ret = selected_parser.str(_hier_string);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the hierarchy string from the parser.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = selected_parser.first_resc(_root_resc);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed to get the root resource from the parser.";
                    result = PASSMSG(msg.str(), ret);
                }
            }
        }
        return result;
    }
    
    eirods::error replReplicateCreateWrite(
        eirods::resource_operation_context* _ctx)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        // get the list of objects that need to be replicated
        object_list_t object_list;
        ret = _ctx->prop_map().get<object_list_t>(object_list_prop, object_list);
        if(!ret.ok() && ret.code() != EIRODS_KEY_NOT_FOUND) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get object list for replication.";
            result = PASSMSG(msg.str(), ret);
        } else if(object_list.size() > 0) {
            // get the child list
            child_list_t child_list;
            ret = _ctx->prop_map().get<child_list_t>(child_list_prop, child_list);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to retrieve child list from repl resource.";
                result = PASSMSG(msg.str(), ret);
            } else {
                // get the root resource name as well as the child hierarchy string
                std::string root_resc;
                std::string child;
                ret = get_selected_hierarchy(_ctx, child, root_resc);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed to determine the root resource and selected hierarchy.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    // create a create/write replicator
                    eirods::create_write_replicator oper_repl(root_resc, child);
                    
                    // create a replicator
                    eirods::replicator replicator(&oper_repl);
                    
                    // call replicate
                    ret = replicator.replicate(_ctx, child_list, object_list);
                    if(!ret.ok()) {
                        std::stringstream msg;
                        msg << __FUNCTION__;
                        msg << " - Failed to replicate the create/write operation to the siblings.";
                        result = PASSMSG(msg.str(), ret);
                    } else {

                        // update the object list in the properties
                        ret = _ctx->prop_map().set<object_list_t>(object_list_prop, object_list);
                        if(!ret.ok()) {
                            std::stringstream msg;
                            msg << __FUNCTION__;
                            msg << " - Failed to update the object list in the properties.";
                            result = PASSMSG(msg.str(), ret);
                        }
                    }
                }
            }
        }
        return result;
    }

    eirods::error replReplicateUnlink(
        eirods::resource_operation_context* _ctx)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        // get the list of objects that need to be replicated
        object_list_t object_list;
        ret = _ctx->prop_map().get<object_list_t>(object_list_prop, object_list);
        if(!ret.ok() && ret.code() != EIRODS_KEY_NOT_FOUND) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get object list for replication.";
            result = PASSMSG(msg.str(), ret);
        } else if(object_list.size() > 0) {
            // get the child list
            child_list_t child_list;
            ret = _ctx->prop_map().get<child_list_t>(child_list_prop, child_list);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to retrieve child list from repl resource.";
                result = PASSMSG(msg.str(), ret);
            } else {
                // get the root resource name as well as the child hierarchy string
                std::string root_resc;
                std::string child;
                ret = get_selected_hierarchy(_ctx, child, root_resc);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed to determine the root resource and selected hierarchy.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    // create an unlink replicator
                    eirods::unlink_replicator oper_repl;
                    
                    // create a replicator
                    eirods::replicator replicator(&oper_repl);
                    
                    // call replicate
                    ret = replicator.replicate(_ctx, child_list, object_list);
                    if(!ret.ok()) {
                        std::stringstream msg;
                        msg << __FUNCTION__;
                        msg << " - Failed to replicate the unlink operation to the siblings.";
                        result = PASSMSG(msg.str(), ret);
                    } else {

                        // update the object list in the properties
                        ret = _ctx->prop_map().set<object_list_t>(object_list_prop, object_list);
                        if(!ret.ok()) {
                            std::stringstream msg;
                            msg << __FUNCTION__;
                            msg << " - Failed to update the object list in the properties.";
                            result = PASSMSG(msg.str(), ret);
                        }
                    }
                }
            }
        }
        return result;
    }
    
    //////////////////////////////////////////////////////////////////////
    // Actual operations

    // Called after a new file is registered with the ICAT
    eirods::error replFileRegistered(
        eirods::resource_operation_context* _ctx)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        eirods::file_object* file_object;
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Error found checking passed parameters.";
            result = PASSMSG(msg.str(), ret);
        } else {
            file_object = dynamic_cast<eirods::file_object*>(&(_ctx->fco()));
            if(file_object != NULL) {
                eirods::hierarchy_parser parser;
                parser.set_string(file_object->resc_hier());
                eirods::resource_ptr child;
                ret =replGetNextRescInHier(parser, _ctx, child);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed to get the next resource in hierarchy.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_REGISTERED, _ctx->fco());
                    if(!ret.ok()) {
                        std::stringstream msg;
                        msg << __FUNCTION__;
                        msg << " - Failed while calling child operation.";
                        result = PASSMSG(msg.str(), ret);
                    } else {
                        if(!file_object->in_pdmo()) {
                            eirods::error ret1 = replReplicateCreateWrite(_ctx);
                            if(!ret1.ok()) {
                                std::stringstream msg;
                                msg << __FUNCTION__;
                                msg << " - Failed to replicate create/write operation for object \"";
                                msg << file_object->logical_path();
                                msg << "\"";
                                eirods::log(LOG_NOTICE, msg.str());
                                // result = PASSMSG(msg.str(), ret1);
                                result = CODE(ret.code());
                            } else {
                                result = CODE(ret.code());
                            }
                        } else {
                            result = CODE(ret.code());
                        }
                    }
                }
            } else {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to cast first class object \"";
                msg << _ctx->fco().logical_path();
                msg << "\" to file object.";
                result = ERROR(-1, msg.str());
            }
        }
        return result;
    }

    // Called when a file is unregistered from the ICAT
    eirods::error replFileUnregistered(
        eirods::resource_operation_context* _ctx)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Error found checking passed parameters.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret =replGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_UNREGISTERED, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                }
            }
        }
        return result;
    }

    // Called when a files entry is modified in the ICAT
    eirods::error replFileModified(
        eirods::resource_operation_context* _ctx)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        eirods::file_object* file_object;
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Error found checking passed parameters.";
            result = PASSMSG(msg.str(), ret);
        } else {
            file_object = dynamic_cast<eirods::file_object*>(&(_ctx->fco()));
            if(file_object != NULL) {
                eirods::hierarchy_parser parser;
                parser.set_string(file_object->resc_hier());
                eirods::resource_ptr child;
                ret =replGetNextRescInHier(parser, _ctx, child);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed to get the next resource in hierarchy.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_MODIFIED, _ctx->fco());
                    if(!ret.ok()) {
                        std::stringstream msg;
                        msg << __FUNCTION__;
                        msg << " - Failed while calling child operation.";
                        result = PASSMSG(msg.str(), ret);
                    } else {
                        if(!file_object->in_pdmo()) {
                            eirods::error ret1 = replReplicateCreateWrite(_ctx);
                            if(!ret1.ok()) {
                                std::stringstream msg;
                                msg << __FUNCTION__;
                                msg << " - Failed to replicate create/write operation for object \"";
                                msg << file_object->logical_path();
                                msg << "\"";
                                eirods::log(LOG_NOTICE, msg.str());
                                // result = PASSMSG(msg.str(), ret1);
                                result = CODE(ret.code());
                            } else {
                                result = CODE(ret.code());
                            }
                        } else {
                            result = CODE(ret.code());
                        }
                    }
                }
            } else {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to cast first class object \"";
                msg << _ctx->fco().logical_path();
                msg << "\" to file object.";
                result = ERROR(-1, msg.str());
            }
        }
        return result;
    }

    // =-=-=-=-=-=-=-
    // interface for POSIX create
    eirods::error replFileCreate(
        eirods::resource_operation_context* _ctx)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            result = PASSMSG("replFileCreatePlugin - bad params.", ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret =replGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_CREATE, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    ret = replUpdateObjectAndOperProperties(_ctx, create_oper);
                    if(!ret.ok()) {
                        std::stringstream msg;
                        msg << __FUNCTION__;
                        msg << " - Failed to update the properties with the object and operation.";
                        result = PASSMSG(msg.str(), ret);
                    }
                }
            }
        }
        return result;
    } // replFileCreate

    // =-=-=-=-=-=-=-
    // interface for POSIX Open
    eirods::error replFileOpen(
        eirods::resource_operation_context* _ctx)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret =replGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_OPEN, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                }
            }
        }
        return result;
    } // replFileOpen

    // =-=-=-=-=-=-=-
    // interface for POSIX Read
    eirods::error replFileRead(
        eirods::resource_operation_context* _ctx,
        void*                          _buf, 
        int                            _len )
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret =replGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<void*, int>(_ctx->comm(), eirods::RESOURCE_OP_READ, _ctx->fco(), _buf, _len);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    // have to return the actual code because it contains the number of bytes read
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileRead

    // =-=-=-=-=-=-=-
    // interface for POSIX Write
    eirods::error replFileWrite(
        eirods::resource_operation_context* _ctx,
        void*                          _buf, 
        int                            _len )
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret =replGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<void*, int>(_ctx->comm(), eirods::RESOURCE_OP_WRITE, _ctx->fco(), _buf, _len);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    // Have to return the actual code value here because it contains the bytes written
                    result = CODE(ret.code());
                    ret = replUpdateObjectAndOperProperties(_ctx, write_oper);
                    if(!ret.ok()) {
                        std::stringstream msg;
                        msg << __FUNCTION__;
                        msg << " - Failed to update the object and operation properties.";
                        result = PASSMSG(msg.str(), ret);
                    }
                }
            }
        }
        return result;
    } // replFileWrite

    // =-=-=-=-=-=-=-
    // interface for POSIX Close
    eirods::error replFileClose(
        eirods::resource_operation_context* _ctx)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        eirods::file_object* file_object;
        
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            file_object = dynamic_cast<eirods::file_object*>(&(_ctx->fco()));
            if(file_object != NULL) {
                eirods::hierarchy_parser parser;
                parser.set_string(_ctx->fco().resc_hier());
                eirods::resource_ptr child;
                ret =replGetNextRescInHier(parser, _ctx, child);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed to get the next resource in hierarchy.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_CLOSE, _ctx->fco());
                    if(!ret.ok()) {
                        std::stringstream msg;
                        msg << __FUNCTION__;
                        msg << " - Failed while calling child operation.";
                        result = PASSMSG(msg.str(), ret);
                    } else {
                        if(false && !file_object->in_pdmo()) {
                            eirods::error ret1 = replReplicateCreateWrite(_ctx);
                            if(!ret1.ok()) {
                                std::stringstream msg;
                                msg << __FUNCTION__;
                                msg << " - Failed to replicate create/write operation for object \"";
                                msg << _ctx->fco().logical_path();
                                msg << "\"";
                                eirods::log(LOG_NOTICE, msg.str());
                                // result = PASSMSG(msg.str(), ret1);
                                result = CODE(ret.code());
                            } else {
                                result = CODE(ret.code());
                            }
                        } else {
                            result = CODE(ret.code());
                        }
                    }
                }
            } else {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to cast first class object \"";
                msg << _ctx->fco().logical_path();
                msg << "\" to file object.";
                result = ERROR(-1, msg.str());
            }
        }
        return result;
        
    } // replFileClose
    
    // =-=-=-=-=-=-=-
    // interface for POSIX Unlink
    eirods::error replFileUnlink(
        eirods::resource_operation_context* _ctx)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::file_object* file_object = dynamic_cast<eirods::file_object*>(&(_ctx->fco()));
            if(file_object != NULL) {
                eirods::hierarchy_parser parser;
                parser.set_string(_ctx->fco().resc_hier());
                eirods::resource_ptr child;
                ret =replGetNextRescInHier(parser, _ctx, child);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed to get the next resource in hierarchy.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_UNLINK, _ctx->fco());
                    if(!ret.ok()) {
                        std::stringstream msg;
                        msg << __FUNCTION__;
                        msg << " - Failed while calling child operation.";
                        result = PASSMSG(msg.str(), ret);
                    } else {
                        result = CODE(ret.code());
                        if(false && !file_object->in_pdmo()) { // dont replicate unlink as it automagically deletes everything
                            ret = replUpdateObjectAndOperProperties(_ctx, unlink_oper);
                            if(!ret.ok()) {
                                std::stringstream msg;
                                msg << __FUNCTION__;
                                msg << " - Failed to update the object and operation properties.";
                                result = PASSMSG(msg.str(), ret);
                            } else {
                                if(!file_object->in_pdmo()) {
                                    ret = replReplicateUnlink(_ctx);
                                    if(!ret.ok()) {
                                        std::stringstream msg;
                                        msg << __FUNCTION__;
                                        msg << " - Failed to replicate the unlink operation for file \"";
                                        msg << _ctx->fco().logical_path();
                                        msg << "\"";
                                        result = PASSMSG(msg.str(), ret);

                                        if(true) {
                                            eirods::stacktrace st;
                                            st.trace();
                                            st.dump();
                                        }

                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to cast first class object \"";
                msg << _ctx->fco().logical_path();
                msg << "\" to file object.";
                result = ERROR(-1, msg.str());
            }
        }
        return result;
    } // replFileUnlink

    // =-=-=-=-=-=-=-
    // interface for POSIX Stat
    eirods::error replFileStat(
        eirods::resource_operation_context* _ctx,
        struct stat*                   _statbuf )
    { 
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret =replGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<struct stat*>(_ctx->comm(), eirods::RESOURCE_OP_STAT, _ctx->fco(), _statbuf);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileStat

    // =-=-=-=-=-=-=-
    // interface for POSIX Fstat
    eirods::error replFileFstat(
        eirods::resource_operation_context* _ctx,
        struct stat*                   _statbuf )
    { 
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret =replGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<struct stat*>(_ctx->comm(), eirods::RESOURCE_OP_FSTAT, _ctx->fco(), _statbuf);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.ok());
                }
            }
        }
        return result;
    } // replFileFstat

    // =-=-=-=-=-=-=-
    // interface for POSIX lseek
    eirods::error replFileLseek(
        eirods::resource_operation_context* _ctx,
        size_t                         _offset, 
        int                            _whence )
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret =replGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<size_t, int>(_ctx->comm(), eirods::RESOURCE_OP_LSEEK, _ctx->fco(), _offset, _whence);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileLseek

    // =-=-=-=-=-=-=-
    // interface for POSIX fsync
    eirods::error replFileFsync(
        eirods::resource_operation_context* _ctx)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret =replGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_FSYNC, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileFsync

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error replFileMkdir(
        eirods::resource_operation_context* _ctx)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret =replGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_MKDIR, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileMkdir

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error replFileChmod(
        eirods::resource_operation_context* _ctx)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;

        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret =replGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_CHMOD, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileChmod

    // =-=-=-=-=-=-=-
    // interface for POSIX mkdir
    eirods::error replFileRmdir(
        eirods::resource_operation_context* _ctx)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret =replGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_RMDIR, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileRmdir

    // =-=-=-=-=-=-=-
    // interface for POSIX opendir
    eirods::error replFileOpendir(
        eirods::resource_operation_context* _ctx)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret =replGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_OPENDIR, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileOpendir

    // =-=-=-=-=-=-=-
    // interface for POSIX closedir
    eirods::error replFileClosedir(
        eirods::resource_operation_context* _ctx)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret =replGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_CLOSEDIR, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileClosedir

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error replFileReaddir(
        eirods::resource_operation_context* _ctx,
        struct rodsDirent**            _dirent_ptr )
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret =replGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<rodsDirent**>(_ctx->comm(), eirods::RESOURCE_OP_READDIR, _ctx->fco(), _dirent_ptr);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileReaddir

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error replFileStage(
        eirods::resource_operation_context* _ctx)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret =replGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_STAGE, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileStage

    // =-=-=-=-=-=-=-
    // interface for POSIX readdir
    eirods::error replFileRename(
        eirods::resource_operation_context* _ctx,
        const char*                    _new_file_name )
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret =replGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<const char*>(_ctx->comm(), eirods::RESOURCE_OP_RENAME, _ctx->fco(), _new_file_name);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileRename

    // =-=-=-=-=-=-=-
    // interface for POSIX truncate
    eirods::error replFileTruncate(
        eirods::resource_operation_context* _ctx)
    {
        // =-=-=-=-=-=-=-
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret =replGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_TRUNCATE, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileTruncate

        
    // =-=-=-=-=-=-=-
    // interface to determine free space on a device given a path
    eirods::error replFileGetFsFreeSpace(
        eirods::resource_operation_context* _ctx)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret =replGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_FREESPACE, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replFileGetFsFreeSpace

    // =-=-=-=-=-=-=-
    // replStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from filename to cacheFilename. optionalInfo info
    // is not used.
    eirods::error replStageToCache(
        eirods::resource_operation_context* _ctx,
        const char*                    _cache_file_name )
    { 
        eirods::error result = SUCCESS();
        eirods::error ret;

        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret =replGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call(_ctx->comm(), eirods::RESOURCE_OP_FREESPACE, _ctx->fco());
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replStageToCache

    // =-=-=-=-=-=-=-
    // passthruSyncToArch - This routine is for testing the TEST_STAGE_FILE_TYPE.
    // Just copy the file from cacheFilename to filename. optionalInfo info
    // is not used.
    eirods::error replSyncToArch(
        eirods::resource_operation_context* _ctx,
        const char*                    _cache_file_name )
    { 
        eirods::error result = SUCCESS();
        eirods::error ret;
        
        ret = replCheckParams(_ctx);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - bad params.";
            result = PASSMSG(msg.str(), ret);
        } else {
            eirods::hierarchy_parser parser;
            parser.set_string(_ctx->fco().resc_hier());
            eirods::resource_ptr child;
            ret =replGetNextRescInHier(parser, _ctx, child);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the next resource in hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = child->call<const char*>(_ctx->comm(), eirods::RESOURCE_OP_SYNCTOARCH, _ctx->fco(), _cache_file_name);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed while calling child operation.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    result = CODE(ret.code());
                }
            }
        }
        return result;
    } // replSyncToArch

    /// @brief Adds the current resource to the specified resource hierarchy
    eirods::error replAddSelfToHierarchy(
        eirods::resource_operation_context* _ctx,
        eirods::hierarchy_parser& _parser)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        std::string name;
        ret = _ctx->prop_map().get<std::string>("name", name);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to get the resource name.";
            result = PASSMSG(msg.str(), ret);
        } else {
            ret = _parser.add_child(name);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to add resource to hierarchy.";
                result = PASSMSG(msg.str(), ret);
            }
        }
        return result;
    }

    /// @brief Loop through the children and call redirect on each one to populate the hierarchy vector
    eirods::error replRedirectToChildren(
        eirods::resource_operation_context* _ctx,
        const std::string*             _operation,
        const std::string*             _curr_host,
        eirods::hierarchy_parser&      _parser,
        redirect_map_t&                _redirect_map)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        eirods::resource_child_map::iterator it;
        float out_vote;
        for(it = _ctx->child_map().begin(); result.ok() && it != _ctx->child_map().end(); ++it) {
            eirods::hierarchy_parser parser(_parser);
            eirods::resource_ptr child = it->second.second;
            ret = child->call<const std::string*, const std::string*, eirods::hierarchy_parser*, float*>(
                _ctx->comm(), eirods::RESOURCE_OP_RESOLVE_RESC_HIER, _ctx->fco(), _operation, _curr_host, &parser, &out_vote);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed calling redirect on the child \"" << it->first << "\"";
                result = PASSMSG(msg.str(), ret);
            } else {
                _redirect_map.insert(std::pair<float, eirods::hierarchy_parser>(out_vote, parser));
            }
        }
        return result;
    }

    /// @brief Creates a list of hierarchies to which this operation must be replicated, all children except the one on which we are
    /// operating.
    eirods::error replCreateChildReplList(
        eirods::resource_operation_context* _ctx,
        const redirect_map_t& _redirect_map)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;

        // Check for an existing child list property. If it exists assume it is correct and do nothing.
        // This assumes that redirect always resolves to the same child. Is that ok? - hcj
        child_list_t repl_vector;
        ret = _ctx->prop_map().get<child_list_t>(child_list_prop, repl_vector);
        if(!ret.ok()) {
            
            // loop over all of the children in the map except the first (selected) and add them to a vector
            redirect_map_t::const_iterator it = _redirect_map.begin();
            for(++it; it != _redirect_map.end(); ++it) {
                eirods::hierarchy_parser parser = it->second;
                repl_vector.push_back(parser);
            }
        
            // add the resulting vector as a property of the resource
            eirods::error ret = _ctx->prop_map().set<child_list_t>(child_list_prop, repl_vector);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to store the repl child list as a property.";
                result = PASSMSG(msg.str(), ret);
            }
        }
        return result;
    }

    /// @brief Selects a child from the vector of parsers based on host access
    eirods::error replSelectChild(
        eirods::resource_operation_context* _ctx,
        const std::string& _curr_host,
        const redirect_map_t& _redirect_map,
        eirods::hierarchy_parser* _out_parser,
        float* _out_vote)
    {
        eirods::error result = SUCCESS();
        eirods::error ret;

        // pluck the first entry out of the map. if its vote is non-zero then ship it
        redirect_map_t::const_iterator it;
        it = _redirect_map.begin();
        float vote = it->first;
        eirods::hierarchy_parser parser = it->second;
        if(vote == 0.0) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - No valid child resource found for file.";
            result = ERROR(-1, msg.str());
        } else {
            *_out_parser = parser;
            *_out_vote = vote;
            ret = replCreateChildReplList(_ctx, _redirect_map);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to add unselected children to the replication list.";
                result = PASSMSG(msg.str(), ret);
            } else {
                ret = _ctx->prop_map().set<eirods::hierarchy_parser>(hierarchy_prop, parser);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed to add hierarchy property to resource.";
                    result = PASSMSG(msg.str(), ret);
                }
            }
        }
        
        return result;
    }

    /// @brief Make sure the requested operation on the requested file object is valid
    eirods::error replValidOperation(
        eirods::resource_operation_context* _ctx)
    {
        eirods::error result = SUCCESS();
        eirods::first_class_object fco = _ctx->fco();

        // cast the first class object to a file object
        eirods::file_object* object = dynamic_cast<eirods::file_object*>(&(_ctx->fco()));
        if(object == NULL) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Invalid first class object.";
            result = ERROR(EIRODS_INVALID_FILE_OBJECT, msg.str());
        }

        // if the file object has a requested replica then fail since that circumvents the coordinating nodes management.
        else if(object->repl_requested() >= 0) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Requesting replica: " << object->repl_requested();
            msg << "\tCannot request specific replicas from replicating resource.";
            result = ERROR(EIRODS_INVALID_OPERATION, msg.str());
        }
        return result;
    }
    
    /// @brief Determines which child should be used for the specified operation
    eirods::error replRedirect(
        eirods::resource_operation_context* _ctx,
        const std::string*             _operation,
        const std::string*             _curr_host,
        eirods::hierarchy_parser*      _inout_parser,
        float*                         _out_vote )
    {
        eirods::error result = SUCCESS();
        eirods::error ret;
        eirods::hierarchy_parser parser = *_inout_parser;
        redirect_map_t redirect_map;

        // Make sure this is a valid repl operation.
        if(!(ret = replValidOperation(_ctx)).ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Invalid operation on replicating resource.";
            result = PASSMSG(msg.str(), ret);
        }
        
        // add ourselves to the hierarchy parser
        else if(!(ret = replAddSelfToHierarchy(_ctx, parser)).ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to add ourselves to the resource hierarchy.";
            result = PASSMSG(msg.str(), ret);
        }

        // call redirect on each child with the appropriate parser
        else if(!(ret = replRedirectToChildren(_ctx, _operation, _curr_host, parser, redirect_map)).ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to redirect to all children.";
            result = PASSMSG(msg.str(), ret);
        }
        
        // foreach child parser determine the best to access based on host
        else if(!(ret = replSelectChild(_ctx, *_curr_host, redirect_map, _inout_parser, _out_vote)).ok()) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to select an appropriate child.";
            result = PASSMSG(msg.str(), ret);
        }
        
        return result;
    }

    // =-=-=-=-=-=-=-
    // 3. create derived class to handle unix file system resources
    //    necessary to do custom parsing of the context string to place
    //    any useful values into the property map for reference in later
    //    operations.  semicolon is the preferred delimiter
    class repl_resource : public eirods::resource {

    public:
        repl_resource(
            const std::string _inst_name,
            const std::string& _context ) : 
            eirods::resource( _inst_name, _context )
            {
                // =-=-=-=-=-=-=-
                // parse context string into property pairs assuming a ; as a separator
                std::vector< std::string > props;
                eirods::string_tokenize( _context, ";", props );
                
                // =-=-=-=-=-=-=-
                // parse key/property pairs using = as a separator and
                // add them to the property list
                std::vector< std::string >::iterator itr = props.begin();
                for( ; itr != props.end(); ++itr ) {
                    // =-=-=-=-=-=-=-
                    // break up key and value into two strings
                    std::vector< std::string > vals;
                    eirods::string_tokenize( *itr, "=", vals );
                    
                    // =-=-=-=-=-=-=-
                    // break up key and value into two strings
                    properties_[ vals[0] ] = vals[1];
                        
                } // for itr 

            } // ctor

        eirods::error post_disconnect_maintenance_operation(
            eirods::pdmo_type& _out_pdmo)
            {
                eirods::error result = SUCCESS();
                // nothing to do
                return result;
            }

        eirods::error need_post_disconnect_maintenance_operation(
            bool& _flag)
            {
                eirods::error result = SUCCESS();
                _flag = false;
                return result;
            }

    }; // class repl_resource

    // =-=-=-=-=-=-=-
    // 4. create the plugin factory function which will return a dynamically
    //    instantiated object of the previously defined derived resource.  use
    //    the add_operation member to associate a 'call name' to the interfaces
    //    defined above.  for resource plugins these call names are standardized
    //    as used by the eirods facing interface defined in
    //    server/drivers/src/fileDriver.c
    eirods::resource* plugin_factory( const std::string& _inst_name, const std::string& _context  ) {

        // =-=-=-=-=-=-=-
        // 4a. create repl_resource
        repl_resource* resc = new repl_resource( _inst_name, _context );

        // =-=-=-=-=-=-=-
        // 4b. map function names to operations.  this map will be used to load
        //     the symbols from the shared object in the delay_load stage of
        //     plugin loading.
        resc->add_operation( eirods::RESOURCE_OP_CREATE,       "replFileCreate" );
        resc->add_operation( eirods::RESOURCE_OP_OPEN,         "replFileOpen" );
        resc->add_operation( eirods::RESOURCE_OP_READ,         "replFileRead" );
        resc->add_operation( eirods::RESOURCE_OP_WRITE,        "replFileWrite" );
        resc->add_operation( eirods::RESOURCE_OP_CLOSE,        "replFileClose" );
        resc->add_operation( eirods::RESOURCE_OP_UNLINK,       "replFileUnlink" );
        resc->add_operation( eirods::RESOURCE_OP_STAT,         "replFileStat" );
        resc->add_operation( eirods::RESOURCE_OP_FSTAT,        "replFileFstat" );
        resc->add_operation( eirods::RESOURCE_OP_FSYNC,        "replFileFsync" );
        resc->add_operation( eirods::RESOURCE_OP_MKDIR,        "replFileMkdir" );
        resc->add_operation( eirods::RESOURCE_OP_CHMOD,        "replFileChmod" );
        resc->add_operation( eirods::RESOURCE_OP_OPENDIR,      "replFileOpendir" );
        resc->add_operation( eirods::RESOURCE_OP_READDIR,      "replFileReaddir" );
        resc->add_operation( eirods::RESOURCE_OP_STAGE,        "replFileStage" );
        resc->add_operation( eirods::RESOURCE_OP_RENAME,       "replFileRename" );
        resc->add_operation( eirods::RESOURCE_OP_FREESPACE,    "replFileGetFsFreeSpace" );
        resc->add_operation( eirods::RESOURCE_OP_LSEEK,        "replFileLseek" );
        resc->add_operation( eirods::RESOURCE_OP_RMDIR,        "replFileRmdir" );
        resc->add_operation( eirods::RESOURCE_OP_CLOSEDIR,     "replFileClosedir" );
        resc->add_operation( eirods::RESOURCE_OP_TRUNCATE,     "replFileTruncate" );
        resc->add_operation( eirods::RESOURCE_OP_STAGETOCACHE, "replStageToCache" );
        resc->add_operation( eirods::RESOURCE_OP_SYNCTOARCH,   "replSyncToArch" );
        resc->add_operation( eirods::RESOURCE_OP_RESOLVE_RESC_HIER,     "replRedirect" );
        resc->add_operation( eirods::RESOURCE_OP_REGISTERED,   "replFileRegistered" );
        resc->add_operation( eirods::RESOURCE_OP_UNREGISTERED, "replFileUnregistered" );
        resc->add_operation( eirods::RESOURCE_OP_MODIFIED,     "replFileModified" );
        
        // =-=-=-=-=-=-=-
        // set some properties necessary for backporting to iRODS legacy code
        resc->set_property< int >( "check_path_perm", 2 );//DO_CHK_PATH_PERM );
        resc->set_property< int >( "create_path",     1 );//CREATE_PATH );
        resc->set_property< int >( "category",        0 );//FILE_CAT );

        // =-=-=-=-=-=-=-
        // 4c. return the pointer through the generic interface of an
        //     eirods::resource pointer
        return dynamic_cast<eirods::resource*>( resc );
        
    } // plugin_factory

}; // extern "C" 



