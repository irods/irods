/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include "eirods_create_write_replicator.h"

#include "dataObjRepl.h"

namespace eirods {

    create_write_replicator::create_write_replicator(
        const std::string& _resource,
        const std::string& _child)
    {
        resource_ = _resource;
        child_ = _child;
    }

    create_write_replicator::~create_write_replicator(void) {
        // TODO - stub
    }

    error create_write_replicator::replicate(
        resource_operation_context* _ctx,
        const child_list_t& _siblings,
        const object_oper& _object_oper)
    {
        error result = SUCCESS();
        if(_object_oper.operation() != create_oper && _object_oper.operation() != write_oper) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Performing create/write replication but object's operation is: [";
            msg << _object_oper.operation();
            msg << "]";
            result = ERROR(-1, msg.str());
        } else {
            file_object object = _object_oper.object();
            child_list_t::const_iterator it;
            for(it = _siblings.begin(); result.ok() && it != _siblings.end(); ++it) {
                hierarchy_parser sibling = *it;
                std::string hierarchy_string;
                error ret = sibling.str(hierarchy_string);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed to get the hierarchy string from the sibling hierarchy parser.";
                    result = PASSMSG(msg.str(), ret);
                } else {
                    dataObjInp_t dataObjInp;
                    bzero(&dataObjInp, sizeof(dataObjInp));
                    rstrcpy(dataObjInp.objPath, object.logical_path().c_str(), MAX_NAME_LEN);
                    dataObjInp.createMode = object.mode();
                    addKeyVal(&dataObjInp.condInput, RESC_HIER_STR_KW, child_.c_str());
                    addKeyVal(&dataObjInp.condInput, DEST_RESC_HIER_STR_KW, hierarchy_string.c_str());
                    addKeyVal(&dataObjInp.condInput, RESC_NAME_KW, resource_.c_str());
                    addKeyVal(&dataObjInp.condInput, DEST_RESC_NAME_KW, resource_.c_str());
                    addKeyVal(&dataObjInp.condInput, IN_PDMO_KW, "");
                    if(_object_oper.operation() == write_oper) {
                        addKeyVal(&dataObjInp.condInput, UPDATE_REPL_KW, "");
                    }
                    transferStat_t* trans_stat = NULL;
                    int status = rsDataObjRepl(_ctx->comm(), &dataObjInp, &trans_stat);
                    if(status < 0) {
                        char* sys_error;
                        char* rods_error = rodsErrorName(status, &sys_error);
                        std::stringstream msg;
                        msg << __FUNCTION__;
                        msg << " - Failed to replicate the data object\"" << object.logical_path() << "\"";
                        msg << "from resource \"" << child_ << "\"";
                        msg << " to sibling \"" << hierarchy_string << "\" - ";
                        msg << rods_error << " " << sys_error;
                        eirods::log(LOG_ERROR, msg.str());
                        result = ERROR(status, msg.str());
                    }
                    if(trans_stat != NULL) {
                        free(trans_stat);
                    }
                }                
            }
        }
        return result;
    }
    
}; // namespace eirods
