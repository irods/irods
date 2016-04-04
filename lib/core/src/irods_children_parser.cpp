#include "irods_children_parser.hpp"
#include "irods_log.hpp"
#include "rodsErrorTable.h"

#include <sstream>
#include <iostream>

namespace irods {

    children_parser::children_parser( void ) {
    }

    children_parser::~children_parser( void ) {
        // TODO - stub
    }

    error children_parser::list(
        children_map_t& list ) {
        list = children_list_;
        return SUCCESS();
    }

    error children_parser::str(
        std::string& ret_string ) const {
        std::stringstream children_stream;
        bool first = true;
        children_map_t::const_iterator itr;
        for ( itr = children_list_.begin(); itr != children_list_.end(); ++itr ) {
            if ( first ) {
                first = false;
            }
            else {
                children_stream << ";";
            }
            children_stream << itr->first << "{" << itr->second << "}";
        }
        ret_string = children_stream.str();
        return SUCCESS();
    }

    error children_parser::add_child(
        const std::string& child,
        const std::string& context ) {
        error ret = SUCCESS();
        children_map_t::const_iterator itr = children_list_.find( child );
        if ( itr != children_list_.end() ) {
            std::stringstream msg;
            msg << "child [" << child << "] already exists";
            ret = ERROR( CHILD_EXISTS, msg.str() );
        }
        else {
            children_list_[child] = context;
        }
        return ret;
    }

    error children_parser::remove_child(
        const std::string& child ) {
        error ret = SUCCESS();
        children_map_t::iterator itr = children_list_.find( child );
        if ( itr == children_list_.end() ) {
            std::stringstream msg;
            msg << "child [" << child << "] not found";
            ret = ERROR( CHILD_NOT_FOUND, msg.str() );
        }
        else {
            children_list_.erase( itr );
        }

        return ret;
    }

    error children_parser::first_child(
        std::string& _child ) {
        error result = SUCCESS();
        if ( children_list_.begin() != children_list_.end() ) {
            _child = children_list_.begin()->first;
        }
        else {
            _child.clear();
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Trying to retrieve first child from children string but string appears to be empty.";
            result = ERROR( CHILD_NOT_FOUND, msg.str() );
        }
        return result;
    }


    error children_parser::last_child(
        std::string& _child ) {
        error result = SUCCESS();
        if ( children_list_.begin() != children_list_.end() ) {
            irods::children_parser::const_iterator itr = children_list_.end();
            itr--;
            _child = itr->first;
        }
        else {
            _child.clear();
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Trying to retrieve last child from children string but string appears to be empty.";
            result = ERROR( CHILD_NOT_FOUND, msg.str() );
        }
        return result;
    }



    error children_parser::set_string(
        const std::string& str ) {
        error result = SUCCESS();

        // clear existing map
        children_list_.clear();

        // check if the string is empty
        if ( !str.empty() ) {

            // loop until we have parsed all of the children
            bool done = false;
            std::size_t current_pos = 0;
            while ( result.ok() && !done ) {

                // get the complete child string
                std::size_t end_pos = str.find( ";", current_pos );
                std::string complete_child = str.substr( current_pos, end_pos - current_pos );

                // get the child context string if there is one
                std::size_t context_start = complete_child.find( "{" );
                std::string child = complete_child.substr( 0, context_start );
                std::string context;
                if ( context_start != std::string::npos ) {
                    ++context_start;
                    std::size_t context_end = complete_child.find( "}", context_start );
                    if ( context_end == std::string::npos ) {
                        std::stringstream msg;
                        msg << "missing matching \"}\" in child context string \"" << str << "\"";
                        result = ERROR( CHILD_NOT_FOUND, msg.str() );
                    }
                    else {
                        context = complete_child.substr( context_start, context_end - context_start );
                    }
                }

                // add the child to the map
                if ( result.ok() ) {
                    children_list_[child] = context;
                    if ( end_pos == std::string::npos ) {
                        done = true;
                    }
                    else {
                        current_pos = end_pos + 1;
                        // check for trailing semicolon
                        if ( current_pos >= str.size() ) {
                            done = true;
                        }
                    }
                }
            }
        }

        return result;
    }


    children_parser::const_iterator children_parser::begin( void ) const {
        return children_list_.begin();
    }

    children_parser::const_iterator children_parser::end( void ) const {
        return children_list_.end();
    }

}; // namespace irods
