#include "irods_hierarchy_parser.hpp"
#include "irods_string_tokenize.hpp"
#include "rodsErrorTable.h"
#include "irods_log.hpp"

#include <iostream>
#include <sstream>

namespace irods {

    static const std::string DELIM = ";";

    hierarchy_parser::hierarchy_parser( void ) {
    }

    hierarchy_parser::hierarchy_parser(
        const hierarchy_parser& parser ) {
        hierarchy_parser::const_iterator it;
        for ( it = parser.begin(); it != parser.end(); ++it ) {
            add_child( *it );
        }
    }

    hierarchy_parser::~hierarchy_parser() {
        // TODO - stub
    }

    error hierarchy_parser::set_string(
        const std::string& _resc_hier ) {
        if ( _resc_hier.empty() ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "empty hierarchy string" );
        }
        error result = SUCCESS();
        resc_list_.clear();
        string_tokenize( _resc_hier, DELIM, resc_list_ );
        return result;
    }

    error hierarchy_parser::str(
        std::string& _ret_string,
        const std::string& _term_resc ) const {

        error result = SUCCESS();
        _ret_string.clear();
        bool first = true;
        bool done = false;
        for ( resc_list_t::const_iterator itr = resc_list_.begin();
                !done && itr != resc_list_.end(); ++itr ) {
            if ( first ) {
                first = false;
            }
            else {
                _ret_string += DELIM;
            }
            _ret_string += *itr;
            if ( *itr == _term_resc ) {
                done = true;
            }
        }
        return result;
    }

    error hierarchy_parser::add_child(
        const std::string& _resc ) {

        error result = SUCCESS();
        resc_list_.push_back( _resc );
        return result;
    }

    error hierarchy_parser::first_resc(
        std::string& _ret_resc ) const {

        error result = SUCCESS();
        if ( resc_list_.size() ) {
            _ret_resc = resc_list_.front();
        }
        else {
            _ret_resc.clear();  // return empty string
        }
        return result;
    }

    error hierarchy_parser::last_resc(
        std::string& _ret_resc ) const {

        error result = SUCCESS();
        if ( resc_list_.size() ) {
            _ret_resc = resc_list_.back();
        }
        else {
            _ret_resc.clear();  // return empty string
        }
        return result;
    }

    error hierarchy_parser::next(
        const std::string& _current,
        std::string& _ret_resc ) const {

        error result = SUCCESS();
        _ret_resc.clear();
        bool found = false;
        for ( resc_list_t::const_iterator itr = resc_list_.begin();
                !found && itr != resc_list_.end(); ++itr ) {
            if ( *itr == _current ) {
                found = true;
                resc_list_t::const_iterator next_itr = itr + 1;
                if ( next_itr != resc_list_.end() ) {
                    _ret_resc = *next_itr;
                }
                else {
                    std::stringstream msg;
                    msg << "there is no next resource. [" << _current;
                    msg << "] is a leaf resource.";
                    result = ERROR( NO_NEXT_RESC_FOUND, msg.str() );
                }
            }
        }
        if ( !found ) {
            std::stringstream msg;
            msg << "resource [" << _current << "] not in hierarchy.";
            result = ERROR( CHILD_NOT_FOUND, msg.str() );
        }
        return result;
    }

    error hierarchy_parser::num_levels(
        int& levels ) const {
        error result = SUCCESS();
        levels = resc_list_.size();
        return result;
    }

    hierarchy_parser::const_iterator hierarchy_parser::begin( void ) const {
        return resc_list_.begin();
    }

    hierarchy_parser::const_iterator hierarchy_parser::end( void ) const {
        return resc_list_.end();
    }

    hierarchy_parser& hierarchy_parser::operator=(
        const hierarchy_parser& rhs ) {
        resc_list_ = rhs.resc_list_;
        return *this;
    }

    const std::string& hierarchy_parser::delimiter( void ) {
        return DELIM;
    }

    bool hierarchy_parser::resc_in_hier(
        const std::string& _resc ) const {
        bool result = false;
        for ( resc_list_t::const_iterator itr = resc_list_.begin(); !result && itr != resc_list_.end(); ++itr ) {
            if ( *itr == _resc ) {
                result = true;
            }
        }
        return result;
    }

}; // namespace irods
