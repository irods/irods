from __future__ import print_function

import json
import os
import re
import subprocess
import time

def get_install_dir():
    return os.path.dirname(os.path.dirname(os.path.realpath(__file__)))

class ServerConfig(object):
    def __init__(self):
        self.values = {}
        if os.path.isfile('/etc/irods/server_config.json'):
            self.capture('/etc/irods/server_config.json')
            self.capture('/etc/irods/database_config.json')
        # Run in place
        elif os.path.isfile(get_install_dir() + '/iRODS/server/config/server_config.json'):
            self.capture(get_install_dir() + '/iRODS/server/config/server_config.json')
            self.capture(get_install_dir() + '/iRODS/server/config/database_config.json')
        # Support deprecated pre-json config files through 4.1.x
        else:
            thefile = '/etc/irods/server.config'
            if os.path.isfile(thefile):
                cfg_file = thefile
            else:
                cfg_file = get_install_dir() + '/iRODS/server/config/server.config'
            self.capture(cfg_file, ' ')

        self.capture(os.path.join(os.environ['HOME'], '.odbc.ini'), '=')

    def get(self, key):
        # old-key to new-key map
        map_deprecated_keys = {'DBPassword': 'db_password', 'DBUsername': 'db_username'}
        if key in self.values:
            return self.values[key]
        elif key in map_deprecated_keys:
            return self.values[map_deprecated_keys[key]]
        else:
            raise KeyError(key)

    def capture(self, cfg_file, sep=None):
        # NOTE:: we want to make this programmatically detected
        cfg_file = os.path.abspath(cfg_file)
        name, ext = os.path.splitext(cfg_file)
        with open(cfg_file, 'r') as f:
            if '.json' == ext:
                self.values.update(json.load(f))
            else:
                for i, row in enumerate(f):
                    columns = row.split(sep)
                    # print columns
                    if columns[0] == '#':
                        continue
                    elif len(columns) > 1:
                        self.values[columns[0].rstrip()] = columns[1].rstrip()

    def get_db_password(self):
        try:
            db_key = self.values['DBKey']
        except KeyError:
            return self.values['db_password']

        obfuscated_password = self.values['db_password']
        p = subprocess.Popen(['iadmin', obfuscated_password, db_key],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        db_pass, db_err = p.communicate()
        if p.returncode != 0:
            raise RuntimeError('Failed to decode db password. stdout[{0}], stderr[{1}]'.format(stdout, stderr))
        return db_pass.split(':')[1].rstrip()

    # =-=-=-=-=-=-=-=-=-=-=-=-=-
    # POSTGRES
    # =-=-=-=-=-=-=-=-=-=-=-=-=-
    def exec_pgsql_cmd(self, sql):
        sql = sql.strip()
        if not sql.endswith(';'):
            sql += ';'

        sqlfile = 'tmpsqlfile'
        with open(sqlfile, 'w+') as f:
            f.write(sql)
        returncode, myout, myerr = self.exec_pgsql_file(sqlfile)
        os.unlink(sqlfile)
        return (returncode, myout, myerr)

    def exec_pgsql_file(self, sql_filename):
        fbp = os.path.dirname(os.path.realpath(__file__)) + '/find_bin_postgres.sh'
        if not os.path.isfile(fbp):
            fbp = get_install_dir() + '/plugins/database/packaging/find_bin_postgres.sh'
        p = subprocess.Popen(fbp, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()
        if p.returncode != 0:
            raise RuntimeError('Failed to find postgres binary. stdout[{0}] stderr[{1}]'.format(stdout, stderr))
        sqlclient = stdout.strip().decode('utf-8').rstrip() + '/psql'
        db_host = self.values['Servername']
        db_port = self.values['Port']
        db_name = self.values['Database']
        db_user = self.values['db_username']
        # suse12 ident auth for default pgsql install fails if localhost given explicitly
        if db_host == 'localhost':
            run_str = '{sqlclient} -p {db_port} -U {db_user} {db_name} < {sql_filename}'.format(**vars())
        # this works for cen6, ub12-14
        else:
            run_str = '{sqlclient} -h {db_host} -p {db_port} -U {db_user} {db_name} < {sql_filename}'.format(**vars())
        p = subprocess.Popen(run_str, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        myout, myerr = p.communicate()
        return (p.returncode, myout, myerr)

    # =-=-=-=-=-=-=-=-=-=-=-=-=-
    # MYSQL
    # =-=-=-=-=-=-=-=-=-=-=-=-=-
    def exec_mysql_cmd(self, sql):
        sql = sql.strip()
        if not sql.endswith(';'):
            sql += ';'

        sqlfile = 'tmpsqlfile'
        with open(sqlfile, 'w+') as f:
            f.write(sql)
        returncode, myout, myerr = self.exec_mysql_file(sqlfile)
        os.unlink(sqlfile)
        return (returncode, myout, myerr)

    def exec_mysql_file(self, sql_filename):
        fbp = os.path.dirname(os.path.realpath(__file__)) + '/find_bin_mysql.sh'
        p = subprocess.Popen(fbp, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()
        if p.returncode != 0:
            raise RuntimeError('Failed to find mysql binary. stdout[{0}] stderr[{1}]'.format(stdout, stderr))
        sqlclient = stdout.strip().decode('utf-8').rstrip() + '/mysql'
        db_host = self.values['Server']
        db_port = self.values['Port']
        db_name = self.values['Database']
        db_user = self.values['DBUsername']
        db_pass = self.get_db_password()
        run_str = '{sqlclient} -h {db_host} -u {db_user} --password={db_pass} -P {db_port} {db_name} < {sql_filename}'.format(**vars())
        p = subprocess.Popen(run_str, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        myout, myerr = p.communicate()
        return (p.returncode, myout, myerr)

    # =-=-=-=-=-=-=-=-=-=-=-=-=-
    # ORACLE
    # =-=-=-=-=-=-=-=-=-=-=-=-=-
    def exec_oracle_cmd(self, sql):
        sql = sql.strip()
        if not sql.endswith(';'):
            sql += ';'

        sqlfile = 'tmpsqlfile'
        with open(sqlfile, 'w+') as f:
            f.write(sql)
        returncode, myout, myerr = self.exec_oracle_file(sqlfile)
        os.unlink(sqlfile)
        return (returncode, myout, myerr)

    def exec_oracle_file(self, sql_filename):
        fbp = os.path.dirname(os.path.realpath(__file__)) + '/find_bin_oracle.sh'
        p = subprocess.Popen(fbp, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()
        if p.returncode != 0:
            raise RuntimeError('Failed to find oracle binary. stdout[{0}] stderr[{1}]'.format(stdout, stderr))
        sqlclient = stdout.strip().decode('utf-8').rstrip()
        db_port = self.values['Port']
        db_user = self.values['DBUsername'].split('@')[0]
        db_host = self.values['DBUsername'].split('@')[1]
        db_pass = self.get_db_password()
        run_str = '{sqlclient} {db_user}/{db_pass}@{db_host} < {sql_filename}'.format(**vars())
        p = subprocess.Popen(run_str, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        myout, myerr = p.communicate()
        return (p.returncode, myout, myerr)

    # =-=-=-=-=-=-=-=-=-=-=-=-=-
    # GENERIC
    # =-=-=-=-=-=-=-=-=-=-=-=-=-
    def exec_sql_cmd(self, sql):
        if self.values['catalog_database_type'] == 'postgres':
            return self.exec_pgsql_cmd(sql)
        if self.values['catalog_database_type'] == 'mysql':
            return self.exec_mysql_cmd(sql)
        if self.values['catalog_database_type'] == 'oracle':
            return self.exec_oracle_cmd(sql)
        print('exec_sql_cmd: unknown database type [{0}]'.format(self.values['catalog_database_type']))

    def exec_sql_file(self, sql):
        if self.values['catalog_database_type'] == 'postgres':
            return self.exec_pgsql_file(sql)
        if self.values['catalog_database_type'] == 'mysql':
            return self.exec_mysql_file(sql)
        if self.values['catalog_database_type'] == 'oracle':
            return self.exec_oracle_file(sql)
        print('exec_sql_file: unknown database type [{0}]'.format(self.values['catalog_database_type']))
