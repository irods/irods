import time
import os
import subprocess


class Server_Config:
    values = {}

    def __init__(self):
        cfg_file = "/etc/irods/server.config"
        self.capture(cfg_file, ' ')

        cfg_file = "/var/lib/irods/.odbc.ini"
        self.capture(cfg_file, '=')

    def capture(self, cfg_file, sep):
        # NOTE:: we want to make this programmatically detected
        cfg_file = os.path.abspath(cfg_file)
        with open(cfg_file) as f:
            for i, row in enumerate(f):
                columns = row.split(sep)
                # print columns
                col_0 = columns[0]
                if(col_0[0] == '#'):
                    continue
                elif len(columns) > 1:
                    self.values[columns[0] .rstrip()] = columns[1].rstrip()

    def exec_pgsql_cmd(self, sql):
        fbp = os.path.dirname(
            os.path.realpath(__file__)) + "/find_bin_postgres.sh"
        p = subprocess.Popen(
            fbp,
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)

        psql = ""
        for line in p.stdout:
            psql = line.decode('utf-8').rstrip() + "/psql"
        retval = p.wait()
        if retval != 0:
            print("find_bin_postgres.sh failed")
            return

        db_host = self.values[ 'Servername' ]
        db_port = self.values[ 'Port' ]
        db_name = self.values[ 'Database' ]
        if db_host == 'localhost':
            run_str = psql + " -p " + db_port + " " + db_name + " -c \"" + sql + "\""
        else:
            run_str = psql + " -h " + db_host + " -p " + db_port + " " + db_name + " -c \"" + sql + "\""

        p = subprocess.Popen(
            run_str, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (myout, myerr) = p.communicate()
        return (p.returncode, myout, myerr)

    def exec_pgsql_file(self, sql):
        fbp = os.path.dirname(
            os.path.realpath(__file__)) + "/find_bin_postgres.sh"
        p = subprocess.Popen(
            fbp,
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)

        psql = ""
        for line in p.stdout:
            psql = line.decode('utf-8').rstrip() + "/psql"
        retval = p.wait()
        if retval != 0:
            print("find_bin_postgres.sh failed")
            return

        db_host = self.values[ 'Servername' ]
        db_port = self.values[ 'Port' ]
        db_name = self.values[ 'Database' ]
        if db_host == 'localhost':
            run_str = psql + " -p " + db_port + " " + db_name + " < " + sql
        else:
            run_str = psql + " -h " + db_host + " -p " + db_port + " " + db_name + " < " + sql
       
        p = subprocess.Popen( run_str, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE )
        (myout, myerr) = p.communicate()
        return (p.returncode, myout, myerr)

    def exec_sql_file(self, sql):
        if self.values['catalog_database_type'] == 'postgres':
            return self.exec_pgsql_file(sql)

    def exec_sql_cmd(self, sql):
        if self.values['catalog_database_type'] == 'postgres':
            return self.exec_pgsql_cmd(sql)
