/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include "eirods_unlink_replicator.h"

#include "dataObjUnlink.h"

namespace eirods {

    unlink_replicator::unlink_replicator(void) {
        // TODO - stub
    }

    unlink_replicator::~unlink_replicator(void) {
        // TODO - stub
    }

    error unlink_replicator::replicate(
        resource_plugin_context& _ctx,
        const child_list_t& _siblings,
        const object_oper& _object_oper)
    {
        error result = SUCCESS();
        if(_object_oper.operation() != unlink_oper) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Performing replication of unlink operation but specified operation is [";
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
                    addKeyVal(&dataObjInp.condInput, RESC_HIER_STR_KW, hierarchy_string.c_str());
                    addKeyVal(&dataObjInp.condInput, FORCE_FLAG_KW, "");
                    addKeyVal(&dataObjInp.condInput, IN_PDMO_KW, "");
                    int status = rsDataObjUnlink(_ctx.comm(), &dataObjInp);
                    // CAT_NO_ROWS_FOUND is okay if the file being unlinked is a registered file in which case there are no replicas
                    if(status < 0 && status != CAT_NO_ROWS_FOUND) {
                        char* sys_error;
                        char* rods_error = rodsErrorName(status, &sys_error);
                        std::stringstream msg;
                        msg << __FUNCTION__;
                        msg << " - Failed to unlink the object \"";
                        msg << object.logical_path();
                        msg << "\" from the resource \"" << hierarchy_string << "\" ";
                        msg << rods_error << " " << sys_error;
                        eirods::log(LOG_ERROR, msg.str());
                        result = ERROR(status, msg.str());
                    }
                }
            }
        }
        return result;
    }
    
}; // namespace eirods
