

#ifndef IRODS_LEXICAL_CAST_HPP
#define IRODS_LEXICAL_CAST_HPP

#include "rodsErrorTable.h"
#include "irods_error.hpp"
#include <boost/lexical_cast.hpp>
#include <string>

namespace irods {
    template<typename T, typename S>
    error lexical_cast( S _s, T& _t ) {
        try {
            _t = boost::lexical_cast<T>(_s);
        } catch( boost::bad_lexical_cast ) {
            std::stringstream msg;
            msg << "failed to cast " << _s;
            return ERROR(
                       INVALID_LEXICAL_CAST,
                       msg.str());
        }
        return SUCCESS();
    } // lexical_cast

    template<typename T>
    error lexical_cast( T& _t, uint64_t _s ) {
        try {
            _t = boost::lexical_cast<T>(_s);
        } catch( boost::bad_lexical_cast ) {
            std::stringstream msg;
            msg << "failed to cast " << _s;
            return ERROR(
                       INVALID_LEXICAL_CAST,
                       msg.str());
        }
        return SUCCESS();
    } // lexical_cast

    template<typename T>
    error lexical_cast( T& _t, const std::string& _s ) {
        try {
            _t = boost::lexical_cast<T>(_s);
        } catch( boost::bad_lexical_cast ) {
            std::stringstream msg;
            msg << "failed to cast " << _s;
            return ERROR(
                       INVALID_LEXICAL_CAST,
                       msg.str());
        }
        return SUCCESS();
    } // lexical_cast

}; // namespace irods

#endif // IRODS_LEXICAL_CAST_HPP

