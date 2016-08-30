/*
 * irods_server_properties.cpp
 *
 *  Created on: Jan 15, 2014
 *      Author: adt
 */

#include "irods_server_properties.hpp"
#include "irods_get_full_path_for_config_file.hpp"

#include "rods.h"
#include "rodsConnect.h"
#include "irods_log.hpp"
#include "irods_exception.hpp"
#include "irods_lookup_table.hpp"

#include <string>
#include <sstream>
#include <algorithm>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#define BUF_LEN 500

//Singleton

namespace irods {

    server_properties& server_properties::instance() {
        static server_properties singleton;
        return singleton;
    }

    server_properties::server_properties() {
        capture();
    } // ctor

    void server_properties::capture() {
        // if a json version exists, then attempt to capture
        // that
        std::string svr_cfg;
        irods::error ret = irods::get_full_path_for_config_file(
                               "server_config.json",
                               svr_cfg );
            capture_json( svr_cfg );

            std::string db_cfg;
            ret = irods::get_full_path_for_config_file(
                      "database_config.json",
                      db_cfg );
            if ( ret.ok() ) {
                capture_json( db_cfg );
            }

    } // capture

    void server_properties::capture_json(
        const std::string& _fn ) {
        error ret = config_props_.load( _fn );
        if ( !ret.ok() ) {
            THROW( ret.code(), ret.result() );
        }

    } // capture_json

    void server_properties::remove( const std::string& _key ) {
        config_props_.remove( _key );
    }

    void delete_server_property( const std::string& _prop ) {
        irods::server_properties::instance().remove(_prop);
    } // delete_server_property

} // namespace irods


