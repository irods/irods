from __future__ import print_function

import itertools
import json
import logging
import os
import pprint
import sys
import tempfile
import time

try:
    from . import pypyodbc
except ImportError:
    pass
from . import six

from . import lib
from . import password_obfuscation
from .exceptions import IrodsError, IrodsWarning

def load_odbc_ini(f):
    odbc_dict = {}
    section = None
    for line in f:
        line = line.strip()
        if not line:
            continue
        if line[0] == '[' and line[-1] == ']':
            section = line[1:-1]
            if section in odbc_dict:
                raise IrodsError('Multiple sections named %s in %s.' %
                        (section, f.name))
            odbc_dict[section] = {}
        elif section is None:
                raise IrodsError('Section headings of the form [section] '
                        'must precede entries in %s.' % (f.name))
        elif '=' in line:
            key, _, value = [e.strip() for e in line.partition('=')]
            if key in odbc_dict[section]:
                raise IrodsError('Multiple entries titled \'%s\' in the '
                        'section titled %s in %s' % (key, section, f.name))
            odbc_dict[section][key] = value
        else:
            raise IrodsError('Invalid line in %s. All lines must be section '
                    'headings of the form [section], entries containing an '
                    '\'=\', or a blank line.' % (f.name))

    return odbc_dict

def dump_odbc_ini(odbc_dict, f):
    for section in odbc_dict:
        print('[%s]' % (section), file=f)
        for key in odbc_dict[section]:
            if odbc_dict[section][key] is None:
                raise IrodsError('Value for key \'%s\' is None.', key)
            print('%s=%s' % (key, odbc_dict[section][key]), file=f)
        print('', file=f)

def get_odbc_entry(db_config, catalog_database_type):
    if catalog_database_type == 'postgres':
        return {
            'Description': 'iRODS Catalog',
            'Driver': db_config['db_odbc_driver'],
            'Trace': 'No',
            'Debug': '0',
            'CommLog': '0',
            'TraceFile': '',
            'Database': db_config['db_name'],
            'Servername': db_config['db_host'],
            'Port': str(db_config['db_port']),
            'ReadOnly': 'No',
            'Ksqo': '0',
            'RowVersioning': 'No',
            'ShowSystemTables': 'No',
            'ShowOidColumn': 'No',
            'FakeOidIndex': 'No',
            'ConnSettings': ''
        }
    elif catalog_database_type == 'mysql':
        return {
            'Description': 'iRODS catalog',
            'Option': '2',
            'Driver': db_config['db_odbc_driver'],
            'Server': db_config['db_host'],
            'Port': str(db_config['db_port']),
            'Database': db_config['db_name']
        }
    elif catalog_database_type == 'oracle':
        return {
            'Description': 'iRODS catalog',
            'Driver': db_config['db_odbc_driver']
        }
    else:
        raise IrodsError('No odbc template exists for %s' % (catalog_database_type))

def get_installed_odbc_drivers():
    out, _, code = lib.execute_command_permissive(['odbcinst', '-q', '-d'])
    return [] if code else [s[1:-1] for s in out.splitlines() if s]

def get_odbc_drivers_for_db_type(db_type):
    return [d for d in get_installed_odbc_drivers() if db_type in d.lower()]

def get_odbc_driver_paths(db_type, oracle_home=None):
    if db_type == 'postgres':
        return sorted(unique_list(itertools.chain(
                lib.find_shared_object('psqlodbcw.so'),
                lib.find_shared_object('libodbcpsql.so'),
                lib.find_shared_object('psqlodbc.*\.so', regex=True))),
            key = lambda p: 0 if is_64_bit_ELF(p) else 1)
    elif db_type == 'mysql':
        return sorted(unique_list(itertools.chain(
                    lib.find_shared_object('libmyodbc.so'),
                    lib.find_shared_object('libmyodbc5.so'),
                    lib.find_shared_object('libmyodbc3.so'),
                    lib.find_shared_object('libmyodbc.*\.so', regex=True))),
            key = lambda p: 0 if is_64_bit_ELF(p) else 1)
    elif db_type == 'oracle':
        return sorted(unique_list(itertools.chain(
                    lib.find_shared_object('libsqora\.so.*', regex=True, additional_directories=[d for d in [oracle_home] if d]))),
            key = lambda p: 0 if is_64_bit_ELF(p) else 1)
    else:
        raise IrodsError('No default ODBC driver paths for %s' % (db_type))

def unique_list(it):
    seen = set()
    seen_add = seen.add
    return [x for x in it if not (x in seen or seen_add(x))]

def is_64_bit_ELF(path):
    try:
        out, err, returncode = lib.execute_command_permissive(['readelf', '-h', path])
    except IrodsError as e:
        l = logging.getLogger(__name__)
        l.debug('Could not call readelf on %s, unable to ensure it is an ELF64 file.\nException:\n%s', path, lib.indent(str(e)))
        return True
    if returncode != 0:
        return False
    else:
        for line in out.splitlines():
            key, _, value = [e.strip() for e in line.partition(':')]
            if key == 'Class':
                if value == 'ELF64':
                    return True
                else:
                    return False
    return False

def get_default_port_for_database_type(catalog_database_type):
    if catalog_database_type == 'postgres':
        return 5432
    elif catalog_database_type == 'mysql':
        return 3306
    elif catalog_database_type == 'oracle':
        return 1521
    raise IrodsError('Unknown database type: %s' % (catalog_database_type))

#oracle completely ignores all settings in the odbc.ini file (though
#the unixODBC driver will pick up Driver and Password), so we have
#to set TWO_TASK to '//<host>:<port>/<service_name>' as well.
def get_two_task_for_oracle(db_config):
    return '//%s:%d/%s' % (db_config['db_host'],
            db_config['db_port'],
            db_config['db_name'])

def get_connection_string(db_config):
    odbc_dict = {}
    odbc_dict['Password'] = db_config['db_password']
    odbc_dict['PWD'] = db_config['db_password']
    odbc_dict['Username'] = db_config['db_username']
    odbc_dict['User'] = db_config['db_username']
    odbc_dict['UID'] = db_config['db_username']
    keys = [k for k in odbc_dict.keys()]

    return ';'.join(itertools.chain(['DSN=iRODS Catalog'], ['%s=%s' % (k, odbc_dict[k]) for k in keys]))

def get_database_connection(irods_config):
    l = logging.getLogger(__name__)
    if irods_config.catalog_database_type == 'oracle':
        os.environ['TWO_TASK'] = get_two_task_for_oracle(irods_config.database_config)
        l.debug('set TWO_TASK For oracle to "%s"', os.environ['TWO_TASK'])

    connection_string = get_connection_string(irods_config.database_config)
    sync_odbc_ini(irods_config)
    os.environ['ODBCINI'] = irods_config.odbc_ini_path
    os.environ['ODBCSYSINI'] = '/etc'

    try:
        return pypyodbc.connect(connection_string.encode('ascii'), ansi=True)
    except pypyodbc.Error as e:
        if 'file not found' in str(e):
            message = (
                    'pypyodbc registered a \'file not found\' error when connecting to the database. '
                    'If your driver path exists, this is most commonly caused by a library required by the '
                    'driver being unable to be found by the linker. Try running ldd on the odbc driver '
                    'binary (or sudo ldd if you are running in sudo) to see which libraries are not '
                    'being found and add any necessary library paths to the LD_LIBRARY_PATH environment '
                    'variable. If you are running setup_irods.py, instead set the LD_LIBRARY_PATH with '
                    'the --ld_library_path command line option.\n'
                    'The specific error pypyodbc reported was:'
                )
        else:
            message = 'pypyodbc encountered an error connecting to the database:'
        six.reraise(IrodsError,
                IrodsError('%s\n%s' % (message, str(e))),
            sys.exc_info()[2])

def execute_sql_statement(cursor, statement, *params, **kwargs):
    l = logging.getLogger(__name__)
    log_params = kwargs.get('log_params', True)
    l.debug('Executing SQL statement:\n%s\nwith the following parameters:\n%s',
            statement,
            pprint.pformat(params) if log_params else '<hidden>')
    try:
        return cursor.execute(statement, params)
    except pypyodbc.Error as e:
        six.reraise(IrodsError,
                IrodsError('pypyodbc encountered an error executing the statement:\n\t%s\n%s' % (statement, str(e))),
            sys.exc_info()[2])

def execute_sql_file(filepath, cursor, by_line=False):
    l = logging.getLogger(__name__)
    l.debug('Executing SQL in %s', filepath)
    with open(filepath, 'r') as f:
        if by_line:
            for line in f.readlines():
                if not line.strip():
                    continue
                l.debug('Executing SQL statement:\n%s', line)
                try:
                    cursor.execute(line)
                except IrodsError as e:
                    six.reraise(IrodsError,
                        IrodsError('Error encountered while executing '
                            'the statement:\n\t%s\n%s' % (line, str(e))),
                        sys.exc_info()[2])
        else:
            try:
                cursor.execute(f.read())
            except IrodsError as e:
                six.reraise(IrodsError,
                    IrodsError('Error encountered while executing '
                        'the sql in %s:\n%s' % (filepath, str(e))),
                    sys.exc_info()[2])

def list_database_tables(cursor):
    l = logging.getLogger(__name__)
    l.info('Listing database tables...')
    table_names = [row[2] for row in cursor.tables()]
    l.debug('List of tables:\n%s', pprint.pformat(table_names))
    return table_names

def irods_tables_in_database(irods_config, cursor):
    with open(os.path.join(irods_config.irods_directory, 'packaging', 'sql', 'icatSysTables.sql')) as f:
        irods_tables = [l.split()[2].lower() for l in f.readlines() if l.lower().startswith('create table')]
    table_names = list_database_tables(cursor)
    return [t for t in table_names if t.lower() in irods_tables]

def get_schema_version_in_database(cursor):
    l = logging.getLogger(__name__)
    query = "select option_value from R_GRID_CONFIGURATION where namespace='database' and option_name='schema_version';"
    try:
        rows = execute_sql_statement(cursor, query).fetchall()
    except IrodsError as e:
        six.reraise(IrodsError,
            IrodsError('Error encountered while executing '
                'the query:\n\t%s\n%s' % (query, str(e))),
            sys.exc_info()[2])
    if len(rows) == 0:
        raise IrodsError('No schema version present, unable to upgrade. '
                'If this is an upgrade from a pre-4.0 installation, '
                'a manual upgrade is required.')
    if len(rows) > 1:
        raise IrodsError('Expected one row when querying '
            'for database schema version, received %d rows' % (len(rows)))

    try:
        schema_version = int(rows[0][0])
    except ValueError:
        raise RuntimeError(
            'Failed to convert [%s] to an int for database schema version' % (rows[0][0]))
    l.debug('Schema_version in database: %s' % (schema_version))

    return schema_version

def sync_odbc_ini(irods_config):
    odbc_dict = get_odbc_entry(irods_config.database_config, irods_config.catalog_database_type)

    #The 'Driver' keyword must be first
    keys = [k for k in odbc_dict.keys()]
    keys[keys.index('Driver')] = keys[0]
    keys[0] = 'Driver'

    template = '\n'.join(itertools.chain(['[iRODS Catalog]'], ['%s=%s' % (k, odbc_dict[k]) for k in keys]))
    lib.execute_command(['odbcinst', '-i', '-s', '-h', '-r'],
            input=template,
            env={'ODBCINI': irods_config.odbc_ini_path, 'ODBCSYSINI': '/etc'})

def create_database_tables(irods_config, cursor, default_resource_directory=None):
    l = logging.getLogger(__name__)
    irods_table_names = irods_tables_in_database(irods_config, cursor)
    if irods_table_names:
        l.info('The following tables already exist in the database, table creation will be skipped:\n%s', pprint.pformat(irods_table_names))
    else:
        if irods_config.catalog_database_type == 'mysql':
            l.info('Defining mysql functions...')
            with tempfile.NamedTemporaryFile() as f:
                f.write('\n'.join([
                        '[client]',
                        '='.join(['user', irods_config.database_config['db_username']]),
                        '='.join(['password', irods_config.database_config['db_password']]),
                        '='.join(['port', str(irods_config.database_config['db_port'])]),
                        '='.join(['host', irods_config.database_config['db_host']])
                    ]))
                f.flush()
                with open(os.path.join(irods_config.irods_directory, 'packaging', 'sql', 'mysql_functions.sql'), 'r') as sql_file:
                    lib.execute_command(
                        ['mysql', '='.join(['--defaults-file', f.name]), irods_config.database_config['db_name']],
                        stdin=sql_file)
        l.info('Creating database tables...')
        sql_files = [
                os.path.join(irods_config.irods_directory, 'packaging', 'sql', 'icatSysTables.sql'),
                os.path.join(irods_config.irods_directory, 'packaging', 'sql', 'icatSysInserts.sql')
            ]
        for sql_file in sql_files:
            try:
                execute_sql_file(sql_file, cursor, by_line=True)
            except IrodsError as e:
                six.reraise(IrodsError,
                        IrodsError('Database setup failed while running %s:\n%s' % (sql_file, str(e))),
                        sys.exc_info()[2])

def setup_database_values(irods_config, cursor=None, default_resource_directory=None):
    l = logging.getLogger(__name__)
    timestamp = '{0:011d}'.format(int(time.time()))

    def get_next_object_id():
        if irods_config.catalog_database_type == 'postgres':
            return execute_sql_statement(cursor, "select nextval('R_OBJECTID');").fetchone()[0]
        elif irods_config.catalog_database_type == 'mysql':
            return execute_sql_statement(cursor, "select R_OBJECTID_nextval();").fetchone()[0]
        elif irods_config.catalog_database_type == 'oracle':
            return execute_sql_statement(cursor, "select R_OBJECTID.nextval from DUAL;").fetchone()[0]
        else:
            raise IrodsError('no next object id function defined for %s' % irods_config.catalog_database_type)

    #zone
    zone_id = get_next_object_id()
    execute_sql_statement(cursor,
            "insert into R_ZONE_MAIN values (?,?,'local','','',?,?);",
            zone_id,
            irods_config.server_config['zone_name'],
            timestamp,
            timestamp)

    #groups
    admin_group_id = get_next_object_id()
    execute_sql_statement(cursor,
            "insert into R_USER_MAIN values (?,?,?,?,'','',?,?);",
            admin_group_id,
            'rodsadmin',
            'rodsgroup',
            irods_config.server_config['zone_name'],
            timestamp,
            timestamp)

    public_group_id = get_next_object_id()
    execute_sql_statement(cursor,
            "insert into R_USER_MAIN values (?,?,?,?,'','',?,?);",
            public_group_id,
            'public',
            'rodsgroup',
            irods_config.server_config['zone_name'],
            timestamp,
            timestamp)

    #users
    admin_user_id = get_next_object_id()
    execute_sql_statement(cursor,
            "insert into R_USER_MAIN values (?,?,?,?,'','',?,?);",
            admin_user_id,
            irods_config.server_config['zone_user'],
            'rodsadmin',
            irods_config.server_config['zone_name'],
            timestamp,
            timestamp)

    #group membership
    execute_sql_statement(cursor,
            "insert into R_USER_GROUP values (?,?,?,?);",
            admin_group_id,
            admin_user_id,
            timestamp,
            timestamp)
    execute_sql_statement(cursor,
            "insert into R_USER_GROUP values (?,?,?,?);",
            admin_user_id,
            admin_user_id,
            timestamp,
            timestamp)
    execute_sql_statement(cursor,
            "insert into R_USER_GROUP values (?,?,?,?);",
            public_group_id,
            admin_user_id,
            timestamp,
            timestamp)

    #password
    scrambled_password = password_obfuscation.scramble(irods_config.admin_password,
            key=irods_config.server_config.get('environment_variables', {}).get('IRODS_DATABASE_USER_PASSWORD_SALT', None))
    execute_sql_statement(cursor,
            "insert into R_USER_PASSWORD values (?,?,'9999-12-31-23.59.00',?,?);",
            admin_user_id,
            scrambled_password,
            timestamp,
            timestamp,
            log_params=False)

    #collections
    system_collections = [
            '/',
            '/'.join(['', irods_config.server_config['zone_name']]),
            '/'.join(['', irods_config.server_config['zone_name'], 'home']),
            '/'.join(['', irods_config.server_config['zone_name'], 'trash']),
            '/'.join(['', irods_config.server_config['zone_name'], 'trash', 'home'])
        ]
    public_collections = [
            '/'.join(['', irods_config.server_config['zone_name'], 'home', 'public']),
            '/'.join(['', irods_config.server_config['zone_name'], 'trash', 'home', 'public'])
        ]
    admin_collections = [
            '/'.join(['', irods_config.server_config['zone_name'], 'home', irods_config.server_config['zone_user']]),
            '/'.join(['', irods_config.server_config['zone_name'], 'trash', 'home', irods_config.server_config['zone_user']])
        ]
    for collection in itertools.chain(system_collections, public_collections, admin_collections):
        parent_collection = '/'.join(['', collection[1:].rpartition('/')[0]])
        collection_id = get_next_object_id()
        execute_sql_statement(cursor,
                "insert into R_COLL_MAIN values (?,?,?,?,?,0,'','','','','','',?,?);",
                collection_id,
                parent_collection,
                collection,
                irods_config.server_config['zone_user'],
                irods_config.server_config['zone_name'],
                timestamp,
                timestamp)

        execute_sql_statement(cursor,
                "insert into R_OBJT_ACCESS values (?,?,1200,?,?);",
                collection_id,
                public_group_id if collection in public_collections else admin_user_id,
                timestamp,
                timestamp)

    #bundle resource
    bundle_resc_id = get_next_object_id()
    execute_sql_statement(cursor,
            "insert into R_RESC_MAIN (resc_id,resc_name,zone_name,resc_type_name,resc_class_name,resc_net,resc_def_path,free_space,free_space_ts,resc_info,r_comment,resc_status,create_ts,modify_ts) values (?,'bundleResc',?,'unixfilesystem','bundle','localhost','/bundle','','','','','',?,?);",
            bundle_resc_id,
            irods_config.server_config['zone_name'],
            timestamp,
            timestamp)

    if default_resource_directory:
        default_resc_id = get_next_object_id()
        execute_sql_statement(cursor,
                "insert into R_RESC_MAIN (resc_id,resc_name,zone_name,resc_type_name,resc_class_name,resc_net,resc_def_path,free_space,free_space_ts,resc_info,r_comment,resc_status,create_ts,modify_ts) values (?,?,?,'unixfilesystem','cache',?,?,'','','','','',?,?);",
                default_resc_id,
                'demoResc',
                irods_config.server_config['zone_name'],
                lib.get_hostname(),
                default_resource_directory,
                timestamp,
                timestamp)
