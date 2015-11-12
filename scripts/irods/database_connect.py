from __future__ import print_function

import itertools
import json
import logging
import os
import pprint
import sys

from . import pypyodbc

from . import six

from . import lib
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
                raise IrodsError('Multiple sections named %s in %s.' % (section, f.name))
            odbc_dict[section] = {}
        elif section is None:
                raise IrodsError('Section headings of the form [section] must precede entries in %s.' % (f.name))
        elif '=' in line:
            key, _, value = [e.strip() for e in line.partition('=')]
            if key in odbc[section]:
                raise IrodsError('Multiple entries titled \'%s\' in the section titled %s in %s' % (key, section, f.name))
            odbc[section][key] = value
        else:
            raise IrodsError('Invalid line in %s. All lines must be section headings of the form [section], '
                    'entries containing an \'=\', or a blank line.' % (f.name))

    return odbc_dict

def dump_odbc_ini(odbc_dict, f):
    for section in odbc_dict:
        print('[%s]' % (section), file=f)
        for key in odbc_dict[section]:
            if odbc_dict[section][key] is None:
                raise IrodsError('Value for key \'%s\' is None.', key)
            print('%s=%s' % (key, odbc_dict[section][key]), file=f)
        print('', file=f)

def get_odbc_entry(db_config):
    odbc_entry = get_odbc_template(db_config['catalog_database_type'])
    odbc_entry['Driver'] = db_config['db_odbc_driver']

    odbc_entry['Database'] = db_config['db_name']
    odbc_entry['Server'] = db_config['db_host']
    odbc_entry['Servername'] = db_config['db_host']
    odbc_entry['Port'] = str(db_config['db_port'])
    return odbc_entry

def get_odbc_template(db_type):
    if db_type == 'postgres':
        return {
            'Description': 'iRODS Catalog',
            'Driver': None,
            'Trace': 'No',
            'Debug': '0',
            'CommLog': '0',
            'TraceFile': '',
            'Database': None,
            'Servername': None,
            'Port': None,
            'ReadOnly': 'No',
            'Ksqo': '0',
            'RowVersioning': 'No',
            'ShowSystemTables': 'No',
            'ShowOidColumn': 'No',
            'FakeOidIndex': 'No',
            'ConnSettings': ''
        }
    elif db_type == 'mysql':
        return {
            'Description': 'iRODS catalog',
            'Option': '2',
            'Driver': None,
            'Server': None,
            'Port': None,
            'Database': None
        }
    elif db_type == 'oracle':
        return {
            'Description': 'iRODS catalog',
            'Driver': None,
        }
    else:
        raise IrodsError('No odbc template exists for %s' % (db_type))

def get_installed_odbc_drivers():
    out, _, code = lib.execute_command_permissive(['odbcinst', '-q', '-d'])
    return [] if code else [s[1:-1] for s in out.splitlines() if s]

def get_odbc_drivers_for_db_type(db_type):
    return [d for d in get_installed_odbc_drivers() if db_type in d.lower()]

def get_odbc_driver_paths(db_type):
    if db_type == 'postgres':
        return sorted(unique_list(itertools.chain(
                #lib.find_shared_object('psqlodbc.so'),
                #lib.find_shared_object('psqlodbca.so'),
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
                    lib.find_shared_object('libsqora\.so.*', regex=True))),
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

#oracle completely ignores all settings in the odbc.ini file (though
#the unixODBC driver will pick up Driver and Password), so we have
#to set TWO_TASK to '//<host>:<port>/<service_name>' as well.
def get_two_task_for_oracle(db_config):
    return '//%s:%d/%s' % (db_config['db_host'],
            db_config['db_port'],
            db_config['db_name'])

def get_connection_string(db_config):
    odbc_dict = {}
    #odbc_dict['Driver'] = db_config['db_odbc_driver']
    odbc_dict['Password'] = db_config['db_password']
    odbc_dict['PWD'] = db_config['db_password']
    odbc_dict['Username'] = db_config['db_username']
    odbc_dict['User'] = db_config['db_username']
    odbc_dict['UID'] = db_config['db_username']
    keys = [k for k in odbc_dict.keys()]
    #keys[keys.index('Driver')] = keys[0]
    #keys[0] = 'Driver'

    return ';'.join(itertools.chain(['DSN=iRODS Catalog'], ['%s=%s' % (k, odbc_dict[k]) for k in keys]))

def get_connection(db_config):
    l = logging.getLogger(__name__)
    if db_config['catalog_database_type'] == 'oracle':
        os.environ['TWO_TASK'] = get_two_task_for_oracle(db_config)
        l.debug('set TWO_TASK For oracle to "%s"', os.environ['TWO_TASK'])

    connection_string = get_connection_string(db_config)
    l.info("Connecting with connection string:\n%s", connection_string)
    sync_odbc_ini(db_config)
    return pypyodbc.connect(connection_string.encode('ascii'), ansi=True)

def sync_odbc_ini(db_config):
    odbc_dict = get_odbc_entry(db_config)

    #The 'Driver' keyword must be first
    keys = [k for k in odbc_dict.keys()]
    keys[keys.index('Driver')] = keys[0]
    keys[0] = 'Driver'

    template = '\n'.join(itertools.chain(['[iRODS Catalog]'], ['%s=%s' % (k, odbc_dict[k]) for k in keys]))
    lib.execute_command(['odbcinst', '-i', '-s', '-h', '-r'], input=template)

def get_odbc_ini_path():
    if 'ODBCINI' in os.environ:
        return os.environ['ODBCINI']
    return os.path.join(os.path.expanduser('~'), '.odbc.ini')

def execute_sql_file(filepath, cursor):
    l = logging.getLogger(__name__)
    l.debug('Executing SQL in %s', filepath)
    with open(filepath, 'r') as f:
        cursor.execute(f.read())
    l.debug('Committing...')
    cursor.commit()

#def main():
#    l = logging.getLogger(__name__)
#    logging.getLogger().setLevel(logging.NOTSET)
#
#    irods_logging.register_tty_handler(sys.stderr, logging.WARNING, None)
#
#    db_config = IrodsConfig().database_config
#    odbc_ini_path = get_odbc_ini_path()
#    os.unlink(odbc_ini_path)
#    with open(odbc_ini_path, mode='w', flags=(os.O_EXCL | os.O_CREAT)) as f:
#        os.chmod(f.name, 0o600)
#        dump_odbc_ini({db_config['catalog_database_type']: get_odbc_entry(db_config)})
#    if db_config['catalog_database_type'] == 'oracle':
#        log.info('Oracle will also require that the \'TWO_TASK\' environment variable be set to \'%s\' to connect.', get_two_task_for_oracle(db_config))

