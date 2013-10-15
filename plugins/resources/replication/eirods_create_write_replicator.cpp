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
        resource_plugin_context& _ctx,
        const child_list_t& _siblings,
        const object_oper& _object_oper)
    {
        error result = SUCCESS();
        if((result = ASSERT_ERROR(_object_oper.operation() == create_oper || _object_oper.operation() == write_oper,
                                   EIRODS_INVALID_OPERATION, "Performing create/write replication but objects operation is: \"%s\".",
                                  _object_oper.operation().c_str())).ok()) {

            // Generate a resource hierarchy string up to and including this resource
            hierarchy_parser child_parser;
            child_parser.set_string(child_);
            std::string sub_hier;
            child_parser.str(sub_hier, resource_);
            

            if(true) {
                std::stringstream msg;
                msg << "qqq - Resource: \"";
                msg << resource_;
                msg << "\"\tChild: \"";
                msg << child_;
                msg << "\"\tSub: \"";
                msg << sub_hier;
                msg << "\"";
                DEBUGMSG(msg.str());
            }

            file_object object = _object_oper.object();
            child_list_t::const_iterator it;
            int sibling_count = 0;
            for(it = _siblings.begin(); result.ok() && it != _siblings.end(); ++it) {
                ++sibling_count;
                hierarchy_parser sibling = *it;
                std::string hierarchy_string;
                error ret = sibling.str(hierarchy_string);
                if((result = ASSERT_PASS(ret, "Failed to get the hierarchy string from the sibling hierarchy parser.")).ok()) {

                    dataObjInp_t dataObjInp;
                    bzero(&dataObjInp, sizeof(dataObjInp));
                    rstrcpy(dataObjInp.objPath, object.logical_path().c_str(), MAX_NAME_LEN);
                    dataObjInp.createMode = object.mode();
                    addKeyVal(&dataObjInp.condInput, RESC_HIER_STR_KW, child_.c_str());
                    addKeyVal(&dataObjInp.condInput, DEST_RESC_HIER_STR_KW, hierarchy_string.c_str());
                    addKeyVal(&dataObjInp.condInput, RESC_NAME_KW, resource_.c_str());
                    addKeyVal(&dataObjInp.condInput, DEST_RESC_NAME_KW, resource_.c_str());
                    addKeyVal(&dataObjInp.condInput, IN_PDMO_KW, sub_hier.c_str());
                    if(_object_oper.operation() == write_oper) {
                        addKeyVal(&dataObjInp.condInput, UPDATE_REPL_KW, "");
                    }
                    transferStat_t* trans_stat = NULL;
                    int status = rsDataObjRepl(_ctx.comm(), &dataObjInp, &trans_stat);
                    char* sys_error;
                    char* rods_error = rodsErrorName(status, &sys_error);
                    result = ASSERT_ERROR(status >= 0, status, "Failed to replicate the data object: \"%s\" from resource: \"%s\" "
                                          "to sibling: \"%s\" - %s %s.", object.logical_path().c_str(), child_.c_str(),
                                          hierarchy_string.c_str(), rods_error, sys_error);

                    if(trans_stat != NULL) {
                        free(trans_stat);
                    }
                }                
            }
        }
        return result;
    }
    
}; // namespace eirods
