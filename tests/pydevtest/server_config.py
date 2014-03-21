
import time
import os
import subprocess

class Server_Config:
    values = {}
    def __init__(self):
        cfg_file = "/etc/irods/server.config"
        self.capture(cfg_file,' ')
        
        cfg_file = "/var/lib/irods/.odbc.ini"
        self.capture(cfg_file,'=')

    def capture(self,cfg_file,sep):
        # NOTE:: we want to make this programmatically detected
        cfg_file = os.path.abspath( cfg_file )
        with open( cfg_file ) as f:
            for i, row in enumerate( f ):
                columns = row.split(sep)
                #print columns
                col_0 = columns[0]
                if( col_0[0] == '#' ):
                    continue
                elif len(columns) > 1:
                    self.values[ columns[0] .rstrip()] = columns[1].rstrip()

    def exec_pgsql_cmd(self,sql):
        p = subprocess.Popen(
                "../../packaging/find_bin_postgres.sh", 
                shell=True, 
                stdout=subprocess.PIPE, 
                stderr=subprocess.PIPE)

        psql = ""
        for line in p.stdout.readlines():
            psql = line.rstrip() + "/psql"
        retval = p.wait()
        if retval != 0:
            print "find_bin_postgres.sh failed"
            return

        db_host = self.values[ 'Servername' ]
        db_name = self.values[ 'Database' ]
        if db_host == 'localhost':
            run_str = psql + " " + db_name + " -c \"" + sql + "\""
        else:
            run_str = psql + " -h " + db_host + " " + db_name + " -c \"" + sql + "\""
       
        res = os.system( run_str )

    def exec_pgsql_file(self,sql):
        p = subprocess.Popen(
                "../../packaging/find_bin_postgres.sh", 
                shell=True, 
                stdout=subprocess.PIPE, 
                stderr=subprocess.PIPE)

        psql = ""
        for line in p.stdout.readlines():
            psql = line.rstrip() + "/psql"
        retval = p.wait()
        if retval != 0:
            print "find_bin_postgres.sh failed"
            return

        db_host = self.values[ 'Servername' ]
        db_name = self.values[ 'Database' ]
        if db_host == 'localhost':
            run_str = psql + " " + db_name + " < " + sql
        else:
            run_str = psql + " -h " + db_host + " " + db_name + " < " + sql
       
        res = os.system( run_str )


    def exec_sql_file(self,sql):
        if self.values[ 'catalog_database_type' ] == 'postgres':
            self.exec_pgsql_file(sql)

    def exec_sql_cmd(self,sql):
        if self.values[ 'catalog_database_type' ] == 'postgres':
            self.exec_pgsql_cmd(sql)


#def main():
#    cfg = Server_Config()
#    #print cfg.values
#
#    secs = int( time.time() )
#
#    cfg.exec_sql_cmd( "insert into r_server_load_digest values ('rescA', 50, %s)" % secs )
#    cfg.exec_sql_cmd( "insert into r_server_load_digest values ('rescB', 75, %s)" % secs )
#    cfg.exec_sql_cmd( "insert into r_server_load_digest values ('rescC', 95, %s)" % secs )
#    cfg.exec_sql_cmd( "select * from r_server_load_digest" )
#    cfg.exec_sql_cmd( "update r_server_load_digest set load_factor=15 where resc_name='rescA'")
#    cfg.exec_sql_cmd( "select * from r_server_load_digest" )
#    cfg.exec_sql_cmd( "delete from r_server_load_digest where resc_name='rescA'")
#    cfg.exec_sql_cmd( "delete from r_server_load_digest where resc_name='rescB'")
#    cfg.exec_sql_cmd( "delete from r_server_load_digest where resc_name='rescC'")
#    cfg.exec_sql_cmd( "select * from r_server_load_digest" )
#
#
#if  __name__ =='__main__':
#    main()
