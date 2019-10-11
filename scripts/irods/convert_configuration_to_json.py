from __future__ import print_function

import json
import os
import subprocess
import sys
import shutil
import pwd
import grp
import socket

DEBUG = True
DEBUG = False

legacy_key_map = {}
should_be_integers = []


def print_debug(*args, **kwargs):
    if DEBUG:
        print(*args, **kwargs)

def print_error(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

def run_in_place():
    return False

def already_converted(legacy_file, new_file):
    if os.path.isfile(new_file):
        print_debug('skipping [%s], [%s] already exists.' % (legacy_file, new_file))
        return True
    return False

def get_install_dir():
    return os.path.dirname(os.path.dirname(os.path.realpath(__file__)))

def get_owner(filename):
    return pwd.getpwuid(os.stat(filename).st_uid).pw_name

def get_group(filename):
    return grp.getgrgid(os.stat(filename).st_gid).gr_name

def get_config_file_path(config_file):
    fullpath = get_install_dir() + '/iRODS/server/config/' + config_file
    # run-in-place
    if os.path.isfile(fullpath):
        return fullpath
    # binary
    else:
        fullpath = '/etc/irods/' + config_file
        return fullpath

def get_env_file_path(env_file):
    fullpath = get_install_dir() + '/.irods/' + env_file
    # run-in-place
    if os.path.isfile(fullpath):
        return fullpath
    # binary
    else:
        fullpath = '/var/lib/irods/.irods/' + env_file
        return fullpath

def convert_irodshost():
    legacy_file = get_config_file_path('irodsHost')
    if run_in_place():
        new_file = get_install_dir() + '/iRODS/server/config/hosts_config.json'
    else:
        new_file = '/etc/irods/hosts_config.json'

    with open(get_install_dir() + '/packaging/hosts_config.json.template') as fh:
        container_name = json.load(fh)

    # convert if necessary
    if (not already_converted(legacy_file, new_file)):
        print_debug('reading [' + legacy_file + '] begin')
        # read legacy file
        if os.path.isfile(legacy_file):
            with open(legacy_file, 'r') as f:
                for i, row in enumerate(f):
                    columns = row.strip().split()  # splits on consecutive spaces and tabs
                    # skip comments
                    if len(columns) < 2 or columns[0].startswith('#') or columns[0] == '':
                        continue
                    # build new json
                    print_debug(columns)
                    addresses = []
                    if columns[0] == "localhost":
                        for j in columns[1:]:
                            addresses.append({'address': j})
                        container_name['host_entries'].append({'address_type': 'local', 'addresses': addresses})
                    else:
                        for j in columns:
                            addresses.append({'address': j})
                        container_name['host_entries'].append({'address_type': 'remote', 'addresses': addresses})
            # print_debug(json.dumps(container_name, indent=4))
            # write out new file
            print_debug('writing [' + new_file + '] begin')
            with open(new_file, 'w') as fh:
                json.dump(container_name, fh, sort_keys=True, indent=4)
            print_debug('writing [' + new_file + '] end')
        else:
            print_debug(' -- not found')
        print_debug('reading [' + legacy_file + '] end')

def convert_hostaccesscontrol():
    legacy_file = get_config_file_path('HostAccessControl')
    if run_in_place():
        new_file = get_install_dir() + '/iRODS/server/config/host_access_control_config.json'
    else:
        new_file = '/etc/irods/host_access_control_config.json'
    #
    # read the template
    #
    with open(get_install_dir() + '/packaging/host_access_control_config.json.template') as fh:
        container_name = json.load(fh)

    # convert if necessary
    if (not already_converted(legacy_file, new_file)):
        print_debug('reading [' + legacy_file + '] begin')
        # read legacy file
        if os.path.isfile(legacy_file):
            with open(legacy_file, 'r') as f:
                for i, row in enumerate(f):
                    columns = row.strip().split()  # splits on consecutive spaces and tabs
                    # skip comments
                    if (len(columns) != 4
                            or columns[0].startswith('#')
                            or columns[0] == ''
                            or 'acChkHostAccessControl' in columns[0]):
                        continue
                    # build new json
                    print_debug(columns)
                    container_name['access_entries'].append({'user': columns[0], 'group': columns[1], 'address': columns[2], 'mask': columns[3]})
#                print_debug(json.dumps(container_name, indent=4))
            # write out new file
            print_debug('writing [' + new_file + '] begin')
            with open(new_file, 'w') as fh:
                json.dump(container_name, fh, sort_keys=True, indent=4)
            print_debug('writing [' + new_file + '] end')
        else:
            print_debug(' -- not found')
        print_debug('reading [' + legacy_file + '] end')


# .irodsEnv -> irods_environment.json
legacy_key_map["irodsUserName"] = "irods_user_name"
legacy_key_map["irodsHost"] = "irods_host"
legacy_key_map["irodsPort"] = "irods_port"
legacy_key_map["irodsHome"] = "irods_home"
legacy_key_map["irodsCwd"] = "irods_cwd"
legacy_key_map["irodsAuthScheme"] = "irods_authentication_scheme"
legacy_key_map["irodsDefResource"] = "irods_default_resource"
legacy_key_map["irodsZone"] = "irods_zone_name"
legacy_key_map["irodsServerDn"] = "irods_gsi_server_dn"
legacy_key_map["irodsLogLevel"] = "irods_log_level"
legacy_key_map["irodsAuthFileName"] = "irods_authentication_file"
legacy_key_map["irodsDebug"] = "irods_debug"
legacy_key_map["irodsClientServerPolicy"] = "irods_client_server_policy"
legacy_key_map["irodsClientServerNegotiation"] = "irods_client_server_negotiation"
legacy_key_map["irodsEncryptionKeySize"] = "irods_encryption_key_size"
legacy_key_map["irodsEncryptionSaltSize"] = "irods_encryption_salt_size"
legacy_key_map["irodsEncryptionNumHashRounds"] = "irods_encryption_num_hash_rounds"
legacy_key_map["irodsEncryptionAlgorithm"] = "irods_encryption_algorithm"
legacy_key_map["irodsDefaultHashScheme"] = "irods_default_hash_scheme"
legacy_key_map["irodsMatchHashPolicy"] = "irods_match_hash_policy"

# ssl environment variables
legacy_key_map["irodsSSLVerifyServer"] = "irods_ssl_verify_server"
legacy_key_map["irodsSSLCACertificateFile"] = "irods_ssl_ca_certificate_file"
legacy_key_map["irodsSSLCACertificatePath"] = "irods_ssl_ca_certificate_path"

# other client environment variables
legacy_key_map["irodsProt"] = "irods_use_xml_protocol"
legacy_key_map["clientUserName"] = "irods_client_user_name"
legacy_key_map["clientRodsZone"] = "irods_client_zone_name"

# integer list
should_be_integers.append('irods_port')
should_be_integers.append('irods_encryption_key_size')
should_be_integers.append('irods_encryption_salt_size')
should_be_integers.append('irods_encryption_num_hash_rounds')


def convert_irodsenv():
    # use irodsEnvFile if defined
    if 'irodsEnvFile' in os.environ:
        legacy_file = os.environ['irodsEnvFile']
        print_debug('irodsEnvFile detected - reading [' + legacy_file + ']')
    else:
        legacy_file = get_env_file_path('.irodsEnv')
    new_file = get_install_dir() + '/.irods/irods_environment.json'
    container_name = {}
    if (not already_converted(legacy_file, new_file)):

        # default values
        container_name['irods_default_hash_scheme'] = 'SHA256'
        container_name['irods_match_hash_policy'] = 'strict'
        container_name['irods_server_control_plane_encryption_algorithm'] = 'AES-256-CBC'
        container_name['irods_server_control_plane_encryption_num_hash_rounds'] = 16
        container_name['irods_server_control_plane_key'] = 'TEMPORARY__32byte_ctrl_plane_key'
        container_name['irods_server_control_plane_port'] = 1248
        container_name['irods_server_control_plane_timeout_milliseconds'] = 10000
        container_name['irods_maximum_size_for_single_buffer_in_megabytes'] = 32
        container_name['irods_default_number_of_transfer_threads'] = 4
        container_name['irods_transfer_buffer_size_for_parallel_transfer_in_megabytes'] = 4

        #
        # load legacy file
        #
        print_debug('reading [' + legacy_file + '] begin')
        if os.path.isfile(legacy_file):
            # read legacy file
            with open(legacy_file, 'r') as f:
                for i, row in enumerate(f):
                    columns = row.strip().split()  # splits on consecutive spaces and tabs
                    # skip comments
                    if (len(columns) < 1
                            or columns[0].startswith('#')
                            or columns[0] == ''):
                        continue
                    # build new json
                    print_debug(columns)
                    try:
                        new_key = legacy_key_map[columns[0]]
                    except KeyError:
                        print_debug("skipping invalid key [" + columns[0] + "]")
                        continue
                    new_value = columns[1].strip('\'')
                    if new_key in should_be_integers:
                        new_value = int(new_value)
                    container_name[new_key] = new_value
            # sniff the environment variables, and use them if defined (they take priority)
            for x in ['irodsUserName', 'irodsHost', 'irodsPort', 'irodsHome',
                      'irodsCwd', 'irodsAuthScheme', 'irodsDefResource', 'irodsZone', 'irodsServerDn', 'irodsLogLevel',
                      'irodsAuthFileName', 'irodsDebug', 'irodsClientServerPolicy', 'irodsClientServerNegotiation', 'irodsEncryptionKeySize',
                      'irodsEncryptionSaltSize', 'irodsEncryptionNumHashRounds', 'irodsEncryptionAlgorithm', 'irodsDefaultHashScheme',
                      'irodsMatchHashPolicy', 'irodsProt', 'clientUserName', 'clientRodsZone']:
                if x in os.environ:
                    container_name[legacy_key_map[x]] = os.environ[x]
            # sniff the ssl-specific environment variables, with defaults if not defined
            container_name[legacy_key_map['irodsSSLVerifyServer']] = os.getenv('irodsSSLVerifyServer', 'hostname')
            container_name[legacy_key_map['irodsSSLCACertificateFile']] = os.getenv('irodsSSLCACertificateFile', '')
            container_name[legacy_key_map['irodsSSLCACertificatePath']] = os.getenv('irodsSSLCACertificatePath', '')

#            print_debug(json.dumps(container_name, indent=4))
            # write out new file
            print_debug('writing [' + new_file + '] begin')
            with open(new_file, 'w') as fh:
                json.dump(container_name, fh, sort_keys=True, indent=4)
            print_debug('writing [' + new_file + '] end')
        else:
            print_debug(' -- not found')
        print_debug('reading [' + legacy_file + '] end')


# server.config -> server_config.json
legacy_key_map["icatHost"] = "icat_host"
legacy_key_map["reRuleSet"] = "re_rulebase_set"
legacy_key_map["reFuncMapSet"] = "re_function_name_mapping_set"
legacy_key_map["reVariableMapSet"] = "re_data_variable_mapping_set"
legacy_key_map["KerberosName"] = "kerberos_name"
legacy_key_map["pam_password_length"] = "pam_password_length"
legacy_key_map["pam_no_extend"] = "pam_no_extend"
legacy_key_map["pam_password_min_time"] = "pam_password_min_time"
legacy_key_map["pam_password_max_time"] = "pam_password_max_time"
legacy_key_map["default_dir_mode"] = "default_dir_mode"
legacy_key_map["default_file_mode"] = "default_file_mode"
legacy_key_map["default_hash_scheme"] = "default_hash_scheme"
legacy_key_map["LocalZoneSID"] = "zone_key"
legacy_key_map["agent_key"] = "negotiation_key"
legacy_key_map["match_hash_policy"] = "match_hash_policy"

# server.config -> database_config.json
legacy_key_map["catalog_database_type"] = "catalog_database_type"
legacy_key_map["DBUsername"] = "db_username"
legacy_key_map["DBPassword"] = "db_password"

# irods.config -> server_config.json
legacy_key_map["$IRODS_PORT"] = "zone_port"
legacy_key_map["$SVR_PORT_RANGE_START"] = "server_port_range_start"
legacy_key_map["$SVR_PORT_RANGE_END"] = "server_port_range_end"

# irods.config -> database_config.json
legacy_key_map["$DATABASE_ODBC_TYPE"] = "db_odbc_type"
legacy_key_map["$DATABASE_HOST"] = "db_host"
legacy_key_map["$DATABASE_PORT"] = "db_port"
legacy_key_map["$DB_NAME"] = "db_name"

# server environment variables
legacy_key_map["spLogLevel"] = "sp_log_level"
legacy_key_map["spLogSql"] = "sp_log_sql"
legacy_key_map["svrPortRangeStart"] = "server_port_range_start"
legacy_key_map["svrPortRangeEnd"] = "server_port_range_end"

# integer list
should_be_integers.append('pam_password_length')
should_be_integers.append('pam_password_min_time')
should_be_integers.append('pam_password_max_time')
should_be_integers.append('zone_port')
should_be_integers.append('server_port_range_start')
should_be_integers.append('server_port_range_end')
should_be_integers.append('db_port')


def convert_serverconfig_and_irodsconfig(argv):

    #
    # read arguments
    #
    if (argv[0] == None):
        raise ValueError('first parameter server_type must be \'icat\' or \'resource\'')
    server_type = argv[0]

    #
    # legacy and new file names
    #
    legacy_irods_config_fullpath = get_config_file_path('irods.config')
    legacy_server_config_fullpath = get_config_file_path('server.config')
    if run_in_place():
        new_server_config_file = get_install_dir() + '/iRODS/server/config/server_config.json'
        new_database_config_file = get_install_dir() + '/iRODS/server/config/database_config.json'
    else:
        new_server_config_file = '/etc/irods/server_config.json'
        new_database_config_file = '/etc/irods/database_config.json'

    if (not already_converted(legacy_server_config_fullpath, new_server_config_file)
            or
            (server_type == "icat" and not already_converted(legacy_server_config_fullpath, new_database_config_file))):

        #
        # read the templates
        #
        with open(get_install_dir() + '/packaging/server_config.json.template') as fh:
            server_config = json.load(fh)
        with open(get_install_dir() + '/packaging/database_config.json.template') as fh:
            database_config = json.load(fh)

        #
        # load server.config
        #
        legacy_server_config_fullpath = get_config_file_path('server.config')
        print_debug('reading [' + legacy_server_config_fullpath + '] begin =======================================================')
        if os.path.isfile(legacy_server_config_fullpath):
            # read the file
            with open(legacy_server_config_fullpath, 'r') as f:
                for i, row in enumerate(f):
                    columns = row.strip().split()  # splits on consecutive spaces and tabs
                    # skip comments and ignore some deprecated settings
                    if (len(columns) < 1
                            or columns[0].startswith('#')
                            or columns[0] == ''
                            or columns[0] == 'run_server_as_root'
                            or columns[0] == 'SIDKey'
                            or columns[0] == 'DBKey'
                            or columns[0] == 'DBPassword'):
                        continue
                    # overwrite template json with legacy information
                    print_debug(columns)
                    try:
                        new_key = legacy_key_map[columns[0]]
                    except KeyError:
                        print_debug("skipping invalid key [" + columns[0] + "]")
                        continue
                    new_value = columns[1]
                    if columns[0] in ['catalog_database_type', 'DBUsername']:
                        print_debug('========= setting database from legacy')
                        print_debug('===== legacy server.config value == [' + new_value + ']')
                        if new_key in should_be_integers:
                            new_value = int(new_value)
                        database_config[new_key] = new_value
#                        print_debug(json.dumps(database_config, indent=4))
                        print_debug('========= done')
                    elif columns[0] in ['reRuleSet', 'reFuncMapSet', 'reVariableMapSet']:
                        server_config[new_key] = []
                        for j in new_value.split(','):
                            server_config[new_key].append({'filename': j})
                    elif columns[0] in ['RemoteZoneSID']:
                        (j, k) = new_value.split('-')
                        # use placeholder, fill it in with common negotiation_key after for loop completes
                        server_config['federation'].append({'icat_host': '', 'zone_name': j, 'zone_key': k, 'negotiation_key': 'placeholder'})
                    else:
                        if new_key in should_be_integers:
                            new_value = int(new_value)
                        server_config[new_key] = new_value
#            print_debug(json.dumps(server_config, indent=4))
#            print_debug(json.dumps(database_config, indent=4))
            # update any federation negotiation_keys
            for z in server_config['federation']:
                z['negotiation_key'] = server_config['negotiation_key']
#            print_debug(json.dumps(server_config, indent=4))
#            print_debug(json.dumps(database_config, indent=4))
        else:
            print_debug(' -- ' + legacy_server_config_fullpath + 'not found')
        print_debug('reading [' + legacy_server_config_fullpath + '] end =======================================================')

        #
        # load irods.config
        #
        legacy_irods_config_fullpath = get_config_file_path('irods.config')
        print_debug('reading [' + legacy_irods_config_fullpath + '] begin =======================================================')
        if os.path.isfile(legacy_irods_config_fullpath):
            with open(legacy_irods_config_fullpath, 'r') as f:
                for i, row in enumerate(f):
                    columns = row.strip().split()  # splits on consecutive spaces and tabs
                    # skip comments
                    if (len(columns) < 1
                            or columns[0].startswith('#')
                            or columns[0].startswith('return')
                            or columns[0] == ''):
                        continue
                    # overwrite template json with legacy information
                    print_debug(columns)
                    try:
                        new_key = legacy_key_map[columns[0]]
                    except KeyError:
                        print_debug("skipping invalid key [" + columns[0] + "]")
                        continue
                    new_value = columns[2].strip('";\'')  # clean up semicolon and quotes
                    if (new_value == ''):
                        continue
                    if columns[0] in ['$IRODS_PORT', '$SVR_PORT_RANGE_START', '$SVR_PORT_RANGE_END']:
                        print_debug('========= setting server from legacy')
                        print_debug('===== legacy irods.config value == [' + new_value + ']')
                        if new_key in should_be_integers:
                            new_value = int(new_value)
                        server_config[new_key] = new_value
#                        print_debug(json.dumps(server_config, indent=4))
                        print_debug('========= done')
                    if columns[0] in ['$DATABASE_ODBC_TYPE', '$DATABASE_HOST', '$DATABASE_PORT', '$DB_NAME']:
                        print_debug('========= setting database from legacy')
                        print_debug('===== legacy server.config value == [' + new_value + ']')
                        if new_key in should_be_integers:
                            new_value = int(new_value)
                        database_config[new_key] = new_value
#                        print_debug(json.dumps(database_config, indent=4))
                        print_debug('========= done')
#            print_debug(json.dumps(server_config, indent=4))
#            print_debug(json.dumps(database_config, indent=4))
        else:
            print_debug(' -- ' + legacy_irods_config_fullpath + 'not found')
        print_debug('reading [' + legacy_irods_config_fullpath + '] end =========================================================')

        # sniff the environment variables, and use them if defined (they take priority)
        for x in ['spLogLevel', 'spLogSql', 'svrPortRangeStart', 'svrPortRangeEnd']:
            if x in os.environ:
                server_config[legacy_key_map[x]] = os.environ[x]

        # inherit zone_name from irods_environment
        with open(get_install_dir() + '/.irods/irods_environment.json') as fh:
            irods_config = json.load(fh)
            server_config['zone_name'] = irods_config['irods_zone_name']

        # update database password from preinstall script result file
        if (server_type == 'icat'):
            with open(get_install_dir() + '/plaintext_database_password.txt', 'r') as f:
                db_password = f.read().replace('\n', '').strip()
            database_config[legacy_key_map['DBPassword']] = db_password

        #
        # write out new files
        #

        # new server_config file
        print_debug('writing [' + new_server_config_file + '] begin')
        with open(new_server_config_file, 'w') as fh:
            json.dump(server_config, fh, sort_keys=True, indent=4)
        os.chmod(new_server_config_file, 0o600)
        print_debug('writing [' + new_server_config_file + '] end')

        # new database_config file
        if (server_type == 'icat'):
            print_debug('writing [' + new_database_config_file + '] begin')
            with open(new_database_config_file, 'w') as fh:
                json.dump(database_config, fh, sort_keys=True, indent=4)
            os.chmod(new_database_config_file, 0o600)
            print_debug('writing [' + new_database_config_file + '] end')
        else:
            print_debug('resource server - skipping database_config.json')

def convert_connectcontrol():
    connectcontrol_path = get_config_file_path('connectControl.config')
    if not os.path.exists(connectcontrol_path):
        return
    with open(connectcontrol_path) as f:
        lines = [l.strip() for l in f.readlines() if l.strip()[0] != '#']
    if len(lines) == 0:
        return

    if run_in_place():
        server_config_file = get_install_dir() + '/iRODS/server/config/server_config.json'
    else:
        server_config_file = '/etc/irods/server_config.json'

    if lines[0].startswith('maxConnections'):
        maximum_connections = int(lines[0].split()[1])
        with open(server_config_file) as f:
            server_config = json.load(f)
        server_config['maximum_connections'] = maximum_connections
        with open(server_config_file, 'w') as f:
            json.dump(server_config, f)
        if len(lines) == 1:
            return
        else:
            lines = lines[1:]

    if lines[0] == 'allowUserList':
        control_type = 'whitelist'
    elif lines[0] == 'disallowUserList':
        control_type = 'blacklist'
    else:
        return

    with open(server_config_file) as f:
        server_config = json.load(f)
    server_config['controlled_user_connection_list'] = {
            'control_type': control_type,
            'users': lines[1:]
        }
    with open(server_config_file, 'w') as f:
        json.dump(server_config, f)

def convert_legacy_configuration_to_json(argv):
    print_debug('Converting Legacy iRODS Configuration Files...')
    # convert irodsHost to hosts_config.json
    convert_irodshost()
    # convert HostAccessControl to host_access_control_config.json
    convert_hostaccesscontrol()
    # convert .irodsEnv to irods_environment.json
    convert_irodsenv()
    # convert server.config to server_config.json
    convert_serverconfig_and_irodsconfig(argv)
    # convert connectControl.config to fields in server_config.json
    convert_connectcontrol()


def main(argv):
    print_debug('-------------------- DEBUG IS ON --------------------')
    convert_legacy_configuration_to_json(argv)
    print_debug('-------------------- DEBUG IS ON --------------------')

if __name__ == '__main__':
    main(sys.argv[1:])
