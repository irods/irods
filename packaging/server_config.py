from __future__ import print_function

import json
import os
import re
import subprocess
import tempfile
import time


def get_install_dir():
    return os.path.dirname(os.path.dirname(os.path.realpath(__file__)))

def is_binary_installation():
    return os.path.isfile(os.path.join(get_install_dir(), 'packaging', 'binary_installation.flag'))

def get_config_dir():
    if is_binary_installation():
        return '/etc/irods'
    return os.path.join(get_install_dir(), 'iRODS', 'server', 'config')

def load_odbc_ini(filename):
    rv = {}
    with open(filename, 'r') as f:
        for line in f:
            if len(line) < 1:
                continue
            if line[0] in [';', '#']:
                continue
            if line[0] == '[':
                continue
            columns = line.split('=')
            if len(columns) != 2:
                continue
            key = columns[0].strip()
            value = columns[1].strip()
            rv[key] = value
    return rv

class ServerConfig(object):
    def __init__(self):
        self.combined_config_dict = {}

        config_files = ['server_config.json', 'database_config.json']
        for file_ in config_files:
            fullpath = os.path.join(get_config_dir(), file_)
            if not os.path.isfile(fullpath):
                raise RuntimeError('Missing {0}\n{1}'.format(fullpath, os.listdir(get_config_dir())))
            self.capture(fullpath)

        self.capture(os.path.join(os.environ['HOME'], '.odbc.ini'), '=')

    def get(self, key):
        return self.combined_config_dict[key]

    def capture(self, cfg_file, sep=None):
        _, ext = os.path.splitext(cfg_file)
        if ext == '.json':
            with open(cfg_file, 'r') as f:
                update = json.load(f)
        else:
            update = load_odbc_ini(cfg_file)
        self.combined_config_dict.update(update)

    def get_db_password(self):
        return self.get('db_password')

    # =-=-=-=-=-=-=-=-=-=-=-=-=-
    # POSTGRES
    # =-=-=-=-=-=-=-=-=-=-=-=-=-
    def exec_pgsql_cmd(self, sql):
        sql = sql.strip()
        if not sql.endswith(';'):
            sql += ';'

        sqlfile = 'tmpsqlfile'
        with open(sqlfile, 'w') as f:
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
        db_host = self.get('Servername')
        db_port = self.get('Port')
        db_name = self.get('Database')
        db_user = self.get('db_username')
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
        if not os.path.isfile(fbp):
            fbp = get_install_dir() + '/plugins/database/packaging/find_bin_mysql.sh'
        p = subprocess.Popen(fbp, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()
        if p.returncode != 0:
            raise RuntimeError('Failed to find mysql binary. stdout[{0}] stderr[{1}]'.format(stdout, stderr))
        sqlclient = stdout.strip().decode('utf-8').rstrip() + '/mysql'
        db_host = self.get('Server')
        db_port = self.get('Port')
        db_name = self.get('Database')
        db_user = self.get('db_username')
        db_pass = self.get_db_password()

        with tempfile.NamedTemporaryFile() as f:
            cnf_file_contents = """
[client]
user={db_user}
password={db_pass}
host={db_host}
port={db_port}
""".format(**vars())
            f.write(cnf_file_contents)
            f.flush()

            run_str = '{sqlclient} --defaults-file={defaults_file} {db_name} < {sql_filename}'.format(
                defaults_file=f.name, **vars())
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
        if not os.path.isfile(fbp):
            fbp = get_install_dir() + '/plugins/database/packaging/find_bin_oracle.sh'
        p = subprocess.Popen(fbp, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()
        if p.returncode != 0:
            raise RuntimeError('Failed to find oracle binary. stdout[{0}] stderr[{1}]'.format(stdout, stderr))
        sqlclient = stdout.strip().decode('utf-8').rstrip()
        db_port = self.get('Port')
        db_user = self.get('db_username').split('@')[0]
        db_host = self.get('db_username').split('@')[1]
        db_pass = self.get_db_password()
        run_str = '{sqlclient} {db_user}/{db_pass}@{db_host} < {sql_filename}'.format(**vars())
        p = subprocess.Popen(run_str, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        myout, myerr = p.communicate()
        return (p.returncode, myout, myerr)

    # =-=-=-=-=-=-=-=-=-=-=-=-=-
    # GENERIC
    # =-=-=-=-=-=-=-=-=-=-=-=-=-
    def exec_sql_cmd(self, sql):
        if self.get('catalog_database_type') == 'postgres':
            return self.exec_pgsql_cmd(sql)
        if self.get('catalog_database_type') == 'mysql':
            return self.exec_mysql_cmd(sql)
        if self.get('catalog_database_type') == 'oracle':
            return self.exec_oracle_cmd(sql)
        print('exec_sql_cmd: unknown database type [{0}]'.format(self.get('catalog_database_type')))

    def exec_sql_file(self, sql):
        if self.get('catalog_database_type') == 'postgres':
            return self.exec_pgsql_file(sql)
        if self.get('catalog_database_type') == 'mysql':
            return self.exec_mysql_file(sql)
        if self.get('catalog_database_type') == 'oracle':
            return self.exec_oracle_file(sql)
        print('exec_sql_file: unknown database type [{0}]'.format(self.get('catalog_database_type')))
