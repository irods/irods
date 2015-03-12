from __future__ import print_function

import json
import os
import re
import subprocess
import tempfile
import time


def get_install_dir():
    return os.path.dirname(os.path.dirname(os.path.realpath(__file__)))


class CompatibilityDict(dict):

    def __init__(self, compatibility_listing, *args, **kwargs):
        super(CompatibilityDict, self).__init__(*args, **kwargs)
        key_dict = {}
        for old, new in compatibility_listing:
            key_dict[old] = new
            key_dict[new] = old
        self.key_dict = key_dict

    def __missing__(self, key):
        compat_key = self.key_dict[key]
        if compat_key in self.keys():
            return self[compat_key]
        else:
            raise KeyError(key)


class ServerConfig(object):

    def __init__(self):
        self.combined_config_dict = CompatibilityDict(compatibility_listing=[('DBPassword', 'db_password'),
                                                                             ('DBUsername', 'db_username')])

        # Binary installation
        if os.path.isfile('/var/lib/irods/packaging/binary_installation.flag'):
            self.capture('/etc/irods/server_config.json')
            self.capture('/etc/irods/database_config.json')
        # Run in place
        elif os.path.isfile(get_install_dir() + '/iRODS/server/config/server_config.json'):
            self.capture(get_install_dir() + '/iRODS/server/config/server_config.json')
            self.capture(get_install_dir() + '/iRODS/server/config/database_config.json')
        # Support deprecated pre-json config files through 4.1.x
        else:
            thefile = '/etc/irods/server.config'
            # binary
            if os.path.isfile(thefile):
                cfg_file = thefile
            # run-in-place
            else:
                cfg_file = get_install_dir() + '/iRODS/server/config/server.config'
            self.capture(cfg_file, ' ')

        self.capture(os.path.join(os.environ['HOME'], '.odbc.ini'), '=')

    def get(self, key):
        return self.combined_config_dict[key]

    def capture(self, cfg_file, sep=None):
        # NOTE:: we want to make this programmatically detected
        cfg_file = os.path.abspath(cfg_file)
        name, ext = os.path.splitext(cfg_file)
        with open(cfg_file, 'r') as f:
            if '.json' == ext:
                self.combined_config_dict.update(json.load(f))
            else:
                for i, row in enumerate(f):
                    columns = row.split(sep)
                    # print columns
                    if columns[0] == '#':
                        continue
                    elif len(columns) > 1:
                        self.combined_config_dict[columns[0].rstrip()] = columns[1].rstrip()

    def get_db_password(self):
        try:
            db_key = self.get('DBKey')
        except KeyError:
            return self.get('db_password')

        obfuscated_password = self.get('db_password')
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
        db_user = self.get('DBUsername')
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
        db_user = self.get('DBUsername').split('@')[0]
        db_host = self.get('DBUsername').split('@')[1]
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
