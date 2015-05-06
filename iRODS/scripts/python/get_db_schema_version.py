from __future__ import print_function

import sys
import imp
import os

top_level_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
packaging_path = top_level_dir + "/packaging"
module_tuple = imp.find_module('server_config', [packaging_path])
imp.load_module('server_config', *module_tuple)
import server_config


DEBUG = True
DEBUG = False


def print_debug(*args, **kwargs):
    if DEBUG:
        print(*args, **kwargs)


def print_error(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


def get_current_schema_version(cfg):
    dbtype = cfg.get('catalog_database_type')
    result = cfg.exec_sql_cmd( "select option_value \
                                from R_GRID_CONFIGURATION \
                                where namespace='database' \
                                and option_name='schema_version';")

    # use default value
    current_schema_version = 1

    err_idx = 2
    if dbtype == 'oracle':
        err_idx = 1

    print_debug(cfg.combined_config_dict)
    print_debug(result)

    sql_output_lines = result[1].decode('utf-8').split('\n')
    for i, line in enumerate(sql_output_lines):
        if 'option_value' in line.lower():
            result_line = i + 1
            # oracle and postgres have line of '------' separating column title from entry
            if '-' in sql_output_lines[result_line]:
                result_line += 1
            break
    else:
        raise RuntimeError(
           'get_current_schema_version: failed to parse schema_version\n\n' + '\n'.join(sql_output_lines))

    try:
        current_schema_version = int(sql_output_lines[result_line])
    except ValueError:
        print('Failed to convert [' + sql_output_lines[result_line] + '] to an int')
        raise RuntimeError(
            'get_current_schema_version: failed to parse schema_version\n\n' + '\n'.join(sql_output_lines))
    print_debug('current_schema_version: {0}'.format(current_schema_version))

    return current_schema_version


# get config
cfg = server_config.ServerConfig()
# get current version
current_schema_version = get_current_schema_version(cfg)
# print
print(current_schema_version)
