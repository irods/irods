/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include "eirods_hierarchy_parser.h"
#include "eirods_string_tokenize.h"

#include <iostream>

namespace eirods {

    hierarchy_parser::hierarchy_parser(void) {
    }

    hierarchy_parser::~hierarchy_parser() {
        // TODO - stub
    }

    error hierarchy_parser::set_string(
        const std::string& _resc_hier) {
        
        error result = SUCCESS();
        resc_list_.clear();
        string_tokenize(_resc_hier, resc_list_, ";");
        std::cerr << "qqq - " << __FUNCTION__ << " - Found " << resc_list_.size() << " resources in hierarchy \"" << _resc_hier << "\"" << std::endl;
        return result;
    }
    
    error hierarchy_parser::str(
        std::string& _ret_string) {

        error result = SUCCESS();
        _ret_string.clear();
        bool first = true;
        for(resc_list_t::const_iterator itr = resc_list_.begin();
            itr != resc_list_.end(); ++itr) {
            if(first) {
                first = false;
            } else {
                _ret_string += ";";
            }
            _ret_string += *itr;
        }
        return result;
    }

    error hierarchy_parser::add_child(
        const std::string& _resc) {

        error result = SUCCESS();
        resc_list_.push_back(_resc);
        return result;
    }

    error hierarchy_parser::first_resc(
        std::string& _ret_resc) const {

        error result = SUCCESS();
        if(resc_list_.size()) {
            _ret_resc = resc_list_.front();
        } else {
            _ret_resc.clear();  // return empty string
        }
        return result;
    }

    error hierarchy_parser::last_resc(
        std::string& _ret_resc) const {

        error result = SUCCESS();
        if(resc_list_.size()) {
            _ret_resc = resc_list_.back();
        } else {
            _ret_resc.clear();  // return empty string
        }
        return result;
    }

    error hierarchy_parser::next(
        const std::string& _current,
        std::string& _ret_resc) const {

        error result = SUCCESS();
        _ret_resc.clear();
        bool found = false;
        for(resc_list_t::const_iterator itr = resc_list_.begin();
            !found && itr != resc_list_.end(); ++itr) {
            if(*itr == _current) {
                found = true;
                resc_list_t::const_iterator next_itr = itr + 1;
                if(next_itr != resc_list_.end()) {
                    _ret_resc = *next_itr;
                } else {
                    std::stringstream msg;
                    msg << __FUNCTION__ << " - There is no next resource. \"" << _current;
                    msg << "\" is a leaf resource.";
                    result = ERROR(-1, msg.str());
                }
            }
        }
        if(!found) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Resource \"" << _current << "\" not in hierarchy.";
            result = ERROR(-1, msg.str());
        }
        return result;
    }

}; // namespace eirods
