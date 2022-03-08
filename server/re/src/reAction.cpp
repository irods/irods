
#include "irods/reAction.hpp"
#include "irods/irods_ms_plugin.hpp"

irods::ms_table& get_microservice_table() {
    static irods::ms_table micros_table;
    return micros_table;
}


