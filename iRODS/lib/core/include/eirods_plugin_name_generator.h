/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _plugin_name_generator_H_
#define _plugin_name_generator_H_

#include "eirods_error.h"

namespace eirods {

/**
 * @brief Functor for generating plugin filenames from the resource name
 */
    class plugin_name_generator {
    public:
        /// @brief ctor
        plugin_name_generator(void);
        virtual ~plugin_name_generator(void);

        /// @brief functor for generating the name from a base name and a directory
        virtual error operator() (const std::string& _base_name, const std::string& _dir_name, std::string& _rtn_soname);

        /// @brief Returns constructs a library name from the plugin name and returns true if the library exists and is readable.
        bool exists(const std::string& _base_name, const std::string& _dir_name);

    private:
    };
}; // namespace eirods

#endif // _plugin_name_generator_H_
