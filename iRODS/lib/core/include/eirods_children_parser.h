/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _children_parser_H_
#define _children_parser_H_

#include "eirods_error.h"

#include <map>
#include <utility>
#include <string>

namespace eirods {

/**
 * @brief Class for managing the children string of a resource
 */
    class children_parser {
    public:
        typedef std::map<std::string, std::string> children_list_t;
        /**
         * @brief Constructor
         */
        children_parser(void);
        virtual ~children_parser(void);

        
        /// @brief Returns the list of children and context strings
        error list(children_list_t& list);

        /// @brief Returns the encoded children string
        error str(std::string& ret_string) const;

        /// @brief Adds the specified child and context string to the children list
        error add_child(const std::string& child, const std::string& context);

        /// @brief Removes the specified child from the list
        error remove_child(const std::string& child);

        /// @brief Sets the children string to parse
        error set_string(const std::string& str);

        /// @brief undocumented
        error first_child(std::string& _child);

    private:
        std::string children_string_;
        children_list_t children_list_;
    };
}; // namespace eirods

#endif // _children_parser_H_
