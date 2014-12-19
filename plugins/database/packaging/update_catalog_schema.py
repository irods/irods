from __future__ import print_function

import json
import os
import subprocess
import sys
import time

from server_config import ServerConfig

schema_directory = 'schema_updates'

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

    if (
        dbtype == 'postgres' and 'relation "r_grid_configuration" does not exist' in result[2].decode('utf-8')
        or
        dbtype == 'mysql' and "Table '{database}.R_GRID_CONFIGURATION' doesn't exist".format(
            database=cfg.get('Database')) in result[2].decode('utf-8')
        or
        # sqlplus puts the output into stdout
        dbtype == 'oracle' and 'table or view does not exist' in result[1].decode('utf-8')
    ):

        # create and populate configuration table
        indexstring = ''
        if dbtype == 'mysql':
            indexstring = '(767)'
        result = cfg.exec_sql_cmd('create table R_GRID_CONFIGURATION ( \
                                   namespace varchar(2700), \
                                   option_name varchar(2700), \
                                   option_value varchar(2700) );')
        print_debug(result)

        result = cfg.exec_sql_cmd('create unique index idx_grid_configuration \
                                   on R_GRID_CONFIGURATION (namespace %s, option_name %s);' % (indexstring, indexstring))
        print_debug(result)

        result = cfg.exec_sql_cmd("insert into R_GRID_CONFIGURATION VALUES ( \
                                   'database', 'schema_version', '1' );")
        print_debug(result)

        # special error case for oracle as it prints out '1 row created' on success
        if dbtype == 'oracle':
            if result[err_idx].decode('utf-8').find('row created') == -1:
                print_error('ERROR: Creating Grid Configuration Table did not complete...')
                print_error(result[err_idx])
                return
        else:
            if result[err_idx].decode('utf-8') != '':
                print_error('ERROR: Creating Grid Configuration Table did not complete...')
                print_error(result[err_idx])
                return

        # use default value
        current_schema_version = 1
    else:
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


def get_target_schema_version():
    # default
    target_schema_version = 2
    # read version value from VERSION.json file
    if os.path.isfile('/var/lib/irods/VERSION.json'):
        version_file = os.path.abspath('/var/lib/irods/VERSION.json')
    else:
        version_file = os.path.dirname(os.path.dirname(os.path.realpath(__file__))) + '/VERSION.json'
    with open(version_file) as fh:
        data = json.load(fh)
    print_debug('catalog_schema_version found in %s...' % version_file)
    target_schema_version = data['catalog_schema_version']
    print_debug('catalog_schema_version: %d' % target_schema_version)
    # return it
    return target_schema_version


def update_schema_version(cfg, version):
    print_debug('Updating schema_version...')
    # update the database
    thesql = "update R_GRID_CONFIGURATION \
              set option_value = '%d' \
              where namespace = 'database' \
              and option_name = 'schema_version';" % version
    result = cfg.exec_sql_cmd(thesql)
    if result[2].decode('utf-8') != '':
        print_error('ERROR: Updating schema_version did not complete...')
        print_error(result[2])
        return
    # success
    print_debug('SUCCESS, updated to schema_version %d' % version)


def get_update_files(version, dbtype):
    # list all files
    sd = os.path.dirname(os.path.realpath(__file__)) + '/' + schema_directory
    if not os.path.exists(sd):
        sd = os.path.dirname(
            os.path.dirname(os.path.realpath(__file__))) + '/plugins/database/packaging/' + schema_directory
    mycmd = 'find %s -name %s' % (sd, '*.' + dbtype + '.*sql')
    print_debug('finding files: %s' % mycmd)
    p = subprocess.Popen(mycmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    myout, myerr = p.communicate()
    if p.returncode == 0:
        return myout.decode('utf-8').strip().split('\n')
    else:
        print_error('ERROR: %s' % myerr.decode('utf-8'))
        return


def update_database_to_latest_version():
    # get config
    cfg = ServerConfig()
    # get current version
    current_schema_version = get_current_schema_version(cfg)
    # get target version
    target_schema_version = get_target_schema_version()
    # check if any work to be done
    if current_schema_version > target_schema_version:
        print_error('Catalog Schema Version is from the future (current=%d > target=%d).' % (
            current_schema_version, target_schema_version))
        return
    if current_schema_version == target_schema_version:
        print('Catalog Schema Version is already up to date (version=%d).' %
              target_schema_version)
        return
    # read possible update sql files
    foundfiles = get_update_files(current_schema_version, cfg.get('catalog_database_type'))
    print_debug('files: %s' % foundfiles)
    updatefiles = {}
    for f in foundfiles:
        version = int(f.split('/')[-1].split('.')[0])
        updatefiles[version] = f
    print_debug('updatefiles: %s' % updatefiles)
    # check all necessary update files exist
    for version in range(current_schema_version + 1, target_schema_version + 1):
        print_debug('checking... %d' % version)
        if version not in updatefiles.keys():
            print_error('ERROR: SQL Update File Not Found for Schema Version %d' % version)
            return
    # run each update sql file
    for version in range(current_schema_version + 1, target_schema_version + 1):
        print('Updating to Catalog Schema... %d' % version)
        print_debug('running: %s' % updatefiles[version])
        result = cfg.exec_sql_file(updatefiles[version])
        if result[2].decode('utf-8') != '':
            print_error('ERROR: SQL did not complete...')
            print_error(result[2])
            return
        print_debug('sql result...')
        print_debug(result)
        # update schema_version in database
        update_schema_version(cfg, version)
    # done
    print('Done.')


def main():
    print_debug('-------------------- DEBUG IS ON --------------------')
    update_database_to_latest_version()
    print_debug('-------------------- DEBUG IS ON --------------------')

if __name__ == '__main__':
    main()
