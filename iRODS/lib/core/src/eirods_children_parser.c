/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include "eirods_children_parser.h"

#include <sstream>
#include <iostream>

namespace eirods {

    children_parser::children_parser(void) {
    }

    children_parser::~children_parser(void) {
        // TODO - stub
    }

    error children_parser::list(
        children_list_t& list) {
        list = children_list_;
        return SUCCESS();
    }

    error children_parser::str(
        std::string& ret_string) const {
        std::stringstream children_stream;
        bool first = true;
        children_list_t::const_iterator itr;
        for(itr = children_list_.begin(); itr != children_list_.end(); ++itr) {
            if(first) {
                first = false;
            } else {
                children_stream << ";";
            }
            children_stream << itr->first << "{" << itr->second << "}";
        }
        ret_string = children_stream.str();
        return SUCCESS();
    }

    error children_parser::add_child(
        const std::string& child,
        const std::string& context) {
        error ret = SUCCESS();
        children_list_t::const_iterator itr = children_list_.find(child);
        if(itr != children_list_.end()) {
            std::stringstream msg;
            msg << "eirods::eirods_children_parser::add_child child \"" << child << "\" already exists";
            ret = ERROR(-1, msg.str());
        } else {
            children_list_[child] = context;
        }
        return ret;
    }

    error children_parser::remove_child(
        const std::string& child) {
        error ret = SUCCESS();
        children_list_t::iterator itr = children_list_.find(child);
        if(itr == children_list_.end()) {
            std::stringstream msg;
            msg << "children_parser::remove_child: child \"" << child << "\" not found";
            ret = ERROR(-1, msg.str());
        } else {
            children_list_.erase(itr);
        }
        
        return ret;
    }

    error children_parser::set_string(
        const std::string& str) {

        error ret = SUCCESS();
        bool done = false;
        std::size_t pos = 0;
        std::size_t prev_pos = 0;
        while(!done) {
            pos = str.find("{", prev_pos);
            if(pos != std::string::npos) {
                std::string child = str.substr(prev_pos, pos - prev_pos);
                prev_pos = pos + 1;
                pos = str.find("}", prev_pos);
                if(pos != std::string::npos) {
                    std::string context = str.substr(prev_pos, pos - prev_pos);
                    children_list_[child] = context;
                    prev_pos = pos + 1;
                    if(prev_pos < str.size() && str.at(prev_pos) == ';') {
                        ++prev_pos;
                    } else {
                        done = true;
                    }
                } else {
                    std::stringstream msg;
                    msg << "eirods::children_parser::set_string invalid context string in input: \"" << str << "\"";
                    ret = ERROR(-1, msg.str());
                }
            } else {
                done = true;
            }
        }
        return ret;
    }

    error children_parser::first_child(
        std::string& _child) {
        _child = children_list_.begin()->first;
        return SUCCESS();
    }

}; // namespace eirods
