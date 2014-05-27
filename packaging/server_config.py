import time
import os
import subprocess


class Server_Config:
    values = {}

    def __init__(self):
        thefile = "/etc/irods/server.config"
        if os.path.isfile(thefile):
                cfg_file = thefile
        else:
                cfg_file = os.path.dirname(
                    os.path.dirname(os.path.realpath(__file__))) + "/iRODS/server/config/server.config"
        self.capture(cfg_file, ' ')

        thefile = "/var/lib/irods/.odbc.ini"
        if os.path.isfile(thefile):
                cfg_file = thefile
        else:
                cfg_file = os.environ['HOME'] + "/.odbc.ini"
        self.capture(cfg_file, '=')

    def capture(self, cfg_file, sep):
        # NOTE:: we want to make this programmatically detected
        cfg_file = os.path.abspath(cfg_file)
        f = open(cfg_file, 'r')
        try:
            for i, row in enumerate(f):
                columns = row.split(sep)
                # print columns
                col_0 = columns[0]
                if(col_0[0] == '#'):
                    continue
                elif len(columns) > 1:
                    self.values[columns[0] .rstrip()] = columns[1].rstrip()
        finally:
            f.close()

    # =-=-=-=-=-=-=-=-=-=-=-=-=-
    # POSTGRES
    # =-=-=-=-=-=-=-=-=-=-=-=-=-
    def exec_pgsql_cmd(self, sql):
        fbp = os.path.dirname(
            os.path.realpath(__file__)) + "/find_bin_postgres.sh"
        if( not os.path.isfile(fbp) ):
                fbp = os.path.dirname( os.path.dirname(
                        os.path.realpath(__file__))) + "/plugins/database/packaging/find_bin_postgres.sh"
        p = subprocess.Popen(
            fbp,
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)

        sqlclient = ""
        for line in p.stdout:
            sqlclient = line.decode('utf-8').rstrip() + "/psql"
        retval = p.wait()
        if retval != 0:
            print("find_bin_postgres.sh failed")
            return

        db_host = self.values['Servername']
        db_port = self.values['Port']
        db_name = self.values['Database']
        if db_host == 'localhost':
            run_str = sqlclient + " -p " + db_port + \
                " " + db_name + " -c \"" + sql + "\""
        else:
            run_str = sqlclient + " -h " + db_host + " -p " + \
                db_port + " " + db_name + " -c \"" + sql + "\""

        p = subprocess.Popen(
            run_str, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (myout, myerr) = p.communicate()
        return (p.returncode, myout, myerr)

    def exec_pgsql_file(self, sql):
        fbp = os.path.dirname(
            os.path.realpath(__file__)) + "/find_bin_postgres.sh"
        if( not os.path.isfile(fbp) ):
                fbp = os.path.dirname( os.path.dirname(
                        os.path.realpath(__file__))) + "/plugins/database/packaging/find_bin_postgres.sh"
        p = subprocess.Popen(
            fbp,
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)

        sqlclient = ""
        for line in p.stdout:
            sqlclient = line.decode('utf-8').rstrip() + "/psql"
        retval = p.wait()
        if retval != 0:
            print("find_bin_postgres.sh failed")
            return

        db_host = self.values['Servername']
        db_port = self.values['Port']
        db_name = self.values['Database']
        if db_host == 'localhost':
            run_str = sqlclient + " -p " + db_port + " " + db_name + " < " + sql
        else:
            run_str = sqlclient + " -h " + db_host + " -p " + \
                db_port + " " + db_name + " < " + sql

        p = subprocess.Popen(
            run_str, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (myout, myerr) = p.communicate()
        return (p.returncode, myout, myerr)

    # =-=-=-=-=-=-=-=-=-=-=-=-=-
    # MySQL
    # =-=-=-=-=-=-=-=-=-=-=-=-=-
    def exec_mysql_cmd(self, sql):
        fbp = os.path.dirname(
            os.path.realpath(__file__)) + "/find_bin_mysql.sh"
        p = subprocess.Popen(
            fbp,
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)

        sqlclient = ""
        for line in p.stdout:
            sqlclient = line.decode('utf-8').rstrip() + "/mysql"
        retval = p.wait()
        if retval != 0:
            print("find_bin_mysql.sh failed")
            return

        db_host = self.values['Servername']
        db_port = self.values['Port']
        db_name = self.values['Database']
        db_user = self.values['DBUsername']
        if db_host == 'localhost':
            run_str = sqlclient + \
                " -u " + db_user + \
                " -P " + db_port + \
                " " + db_name + \
                " -e \"" + sql + "\""
        else:
            run_str = sqlclient + \
                " -h " + db_host + \
                " -u " + db_user + \
                " -P " + db_port + \
                " " + db_name + \
                " -e \"" + sql + "\""

        p = subprocess.Popen(
            run_str, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (myout, myerr) = p.communicate()
        return (p.returncode, myout, myerr)

    def exec_mysql_file(self, sql):
        fbp = os.path.dirname(
            os.path.realpath(__file__)) + "/find_bin_mysql.sh"
        p = subprocess.Popen(
            fbp,
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)

        sqlclient = ""
        for line in p.stdout:
            sqlclient = line.decode('utf-8').rstrip() + "/mysql"
        retval = p.wait()
        if retval != 0:
            print("find_bin_mysql.sh failed")
            return

        db_host = self.values['Servername']
        db_port = self.values['Port']
        db_name = self.values['Database']
        db_user = self.values['DBUsername']
        if db_host == 'localhost':
            run_str = sqlclient + \
                " -u " + db_user + \
                " -P " + db_port + \
                " " + db_name + \
                " < " + sql
        else:
            run_str = sqlclient + \
                " -h " + db_host + \
                " -u " + db_user + \
                " -P " + db_port + \
                " " + db_name + \
                " < " + sql

        p = subprocess.Popen(
            run_str, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (myout, myerr) = p.communicate()
        return (p.returncode, myout, myerr)

    # =-=-=-=-=-=-=-=-=-=-=-=-=-
    # GENERIC
    # =-=-=-=-=-=-=-=-=-=-=-=-=-
    def exec_sql_cmd(self, sql):
        if self.values['catalog_database_type'] == 'postgres':
            return self.exec_pgsql_cmd(sql)
        if self.values['catalog_database_type'] == 'mysql':
            return self.exec_mysql_cmd(sql)
        print( "exec_sql_cmd: unknown database type [%s]", self.values['catalog_database_type'])
        return

    def exec_sql_file(self, sql):
        if self.values['catalog_database_type'] == 'postgres':
            return self.exec_pgsql_file(sql)
        if self.values['catalog_database_type'] == 'mysql':
            return self.exec_mysql_file(sql)
        print( "exec_sql_file: unknown determine database type [%s]", self.values['catalog_database_type'])
        return
