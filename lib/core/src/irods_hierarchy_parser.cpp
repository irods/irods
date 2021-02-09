#include "irods_hierarchy_parser.hpp"
#include "irods_string_tokenize.hpp"
#include "rodsErrorTable.h"
#include "irods_log.hpp"

#include <iostream>
#include <sstream>

#include "fmt/format.h"

namespace irods {

    static const std::string DELIM = ";";

    hierarchy_parser::hierarchy_parser( void ) {
    }

    hierarchy_parser::hierarchy_parser(const std::string& _hier) {
        resc_list_.clear();
        string_tokenize(_hier, DELIM, resc_list_);
        if (resc_list_.empty()) {
            THROW(SYS_INVALID_INPUT_PARAM, "invalid hierarchy string");
        }
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
        resc_list_.clear();
        string_tokenize( _resc_hier, DELIM, resc_list_ );
        if (resc_list_.empty()) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "empty hierarchy string" );
        }
        return SUCCESS();
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

    std::string hierarchy_parser::str(const std::string& _term_resc) const {
        std::string ret_string{};
        bool first = true;
        for (const auto& resc : resc_list_) {
            if ( first ) {
                first = false;
            }
            else {
                ret_string += DELIM;
            }
            ret_string += resc;
            if (resc == _term_resc) {
                break;
            }
        }
        return ret_string;
    }

    error hierarchy_parser::add_child(
        const std::string& _resc ) {
        if (_resc == hierarchy_parser::delimiter()) {
            return ERROR(SYS_INVALID_INPUT_PARAM, "invalid resource name");
        }

        resc_list_.push_back( _resc );
        return SUCCESS();
    }

    auto hierarchy_parser::remove_resource(const std::string_view _resource_name) -> void
    {
        if (_resource_name.empty() || _resource_name == hierarchy_parser::delimiter()) {
            THROW(SYS_INVALID_INPUT_PARAM, "invalid resource name");
        }

        if (num_levels() < 2) {
            THROW(SYS_NOT_ALLOWED, "cannot remove the last resource in the hierarchy");
        }

        auto it = std::find(resc_list_.begin(), resc_list_.end(), _resource_name);
        if (resc_list_.end() == it) {
            THROW(CHILD_NOT_FOUND, fmt::format("resource [{}] not in hierarchy.", _resource_name));
        }

        resc_list_.erase(it);
    } // remove_resource

    void hierarchy_parser::add_parent(
        const std::string& _parent,
        const std::string& _child) {
        if (_parent == hierarchy_parser::delimiter()) {
            THROW(SYS_INVALID_INPUT_PARAM, "invalid resource name");
        }
        auto it = resc_list_.begin();
        if (!_child.empty()) {
            it = std::find(resc_list_.begin(), resc_list_.end(), _child);
            if (it == resc_list_.end()) {
                THROW(CHILD_NOT_FOUND, fmt::format("resource [{}] not in hierarchy.", _child));
            }
        }
        resc_list_.insert(it, _parent);
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

    std::string hierarchy_parser::first_resc() const {
        if (resc_list_.size()) {
            return resc_list_.front();
        }
        return {};
    }

    std::string hierarchy_parser::last_resc() const {
        if (resc_list_.size()) {
            return resc_list_.back();
        }
        return {};
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

    std::string hierarchy_parser::next(
        const std::string& _current) const {
        for (resc_list_t::const_iterator itr = resc_list_.begin();
             itr != resc_list_.end(); ++itr) {
            if (*itr == _current) {
                resc_list_t::const_iterator next_itr = itr + 1;
                if (next_itr != resc_list_.end()) {
                    return *next_itr;
                }
                THROW(NO_NEXT_RESC_FOUND, (boost::format(
                      "there is no next resource. [%s] is a leaf resource.") % _current).str());
            }
        }
        THROW(CHILD_NOT_FOUND, (boost::format(
              "resource [%s] not in hierarchy.") % _current).str());
    }


    error hierarchy_parser::num_levels(
        int& levels ) const {
        error result = SUCCESS();
        levels = resc_list_.size();
        return result;
    }

    int hierarchy_parser::num_levels() const {
        return resc_list_.size();
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

    bool hierarchy_parser::contains(const std::string_view _resource_name) const {
        return std::any_of(resc_list_.begin(), resc_list_.end(),
            [&_resource_name](const std::string_view _r) {
                return _r == _resource_name;
            });
    } // contains

    bool hierarchy_parser::resc_in_hier(const std::string& _resc) const {
        return contains(_resc);
    } // resc_in_hier

}; // namespace irods
