/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _hierarchy_parser_H_
#define _hierarchy_parser_H_

#include "eirods_error.h"

#include <vector>
#include <string>

namespace eirods {

/**
 * @brief Class to manage resource hierarchy strings
 */
    class hierarchy_parser {
    public:
        typedef std::vector<std::string> resc_list_t;

        /// @brief ctor doesn't do much, until it has a string
        hierarchy_parser(void);
        virtual ~hierarchy_parser();

        /// @brief Parses the specified hierarchy string
        error set_string(const std::string& _resc_hier);

        /// @brief Returns the resource hierarchy string
        error str(std::string& _ret_string);

        /// @brief Adds another level of hierarchy by adding the specified child resource
        error add_child(const std::string& _resc);

        /// @brief Returns the first resource
        error first_resc(std::string& _ret_resc) const;

        /// @brief Returns the last resource in the hierarchy
        error last_resc(std::string& _ret_resc) const;

        /// @brief Returns the next resource in the hierarchy after the specified resource
        error next(const std::string& _current, std::string& _ret_resc) const;

    private:
        resc_list_t resc_list_;
    };
}; // namespace eirods

#endif // _hierarchy_parser_H_
