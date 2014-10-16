import time
import os
from server_config import ServerConfig
import subprocess

schema_directory = "schema_updates"

DEBUG = True
DEBUG = False

def get_current_schema_version(cfg):
    dbtype = cfg.values['catalog_database_type']
    result = cfg.exec_sql_cmd( "select option_value \
                                from R_GRID_CONFIGURATION \
                                where namespace='database' \
                                and option_name='schema_version';")

    # use default value
    current_schema_version = 1

    err_idx = 2;
    if( dbtype == "oracle" ):
        err_idx = 1

    if DEBUG:
        print(cfg.values)
        print(result)
    if (
        (dbtype == "postgres" and
        "relation \"r_grid_configuration\" does not exist" in result[2].decode('utf-8'))
        or
        (dbtype == "mysql" and
        ("Table '%s.R_GRID_CONFIGURATION' doesn't exist" % cfg.values['Database']) in result[2].decode('utf-8'))
        or
        (dbtype == "oracle" and
        "table or view does not exist" in result[1].decode('utf-8')) # sqlplus puts the output into stdout
        ):

        # create and populate configuration table
        indexstring = ""
        if (dbtype == "mysql"):
            indexstring = "(767)"
        result = cfg.exec_sql_cmd("create table R_GRID_CONFIGURATION ( \
                                   namespace varchar(2700), \
                                   option_name varchar(2700), \
                                   option_value varchar(2700) );" )
        if DEBUG:
            print(result)

        result = cfg.exec_sql_cmd("create unique index idx_grid_configuration \
                                   on R_GRID_CONFIGURATION (namespace %s, option_name %s);" % ( indexstring, indexstring ) )
        if DEBUG:
            print(result)

        result = cfg.exec_sql_cmd("insert into R_GRID_CONFIGURATION VALUES ( \
                                   'database', 'schema_version', '1' );" )
        if DEBUG:
            print(result)
 
        # special error case for oracle as it prints out '1 row created' on success
        if ( dbtype == "oracle" ):
            if( result[err_idx].decode('utf-8').find("row created") == -1 ):
                print("ERROR: Creating Grid Configuration Table did not complete...")
                print(result[err_idx])
                return
        else:
            if( result[err_idx].decode('utf-8') != "" ):
                print("ERROR: Creating Grid Configuration Table did not complete...")
                print(result[err_idx])
                return

        # use default value
        current_schema_version = 1
    else:
        sql_output_lines = result[1].decode('utf-8').split('\n')
        for i, line in enumerate(sql_output_lines):
            if 'option_value' in line.lower():
                result_line = i+1
                # oracle and postgres have line of "------" separating column title from entry
                if '-' in sql_output_lines[result_line]:
                    result_line += 1
                break
        else:
            raise RuntimeError('get_current_schema_version: failed to parse schema_version\n\n' + '\n'.join(sql_output_lines))

        try:
            current_schema_version = int(sql_output_lines[result_line])
        except ValueError:
            print('Failed to convert [' + sql_output_lines[result_line] + '] to an int')
            raise RuntimeError('get_current_schema_version: failed to parse schema_version\n\n' + '\n'.join(sql_output_lines))
    if DEBUG:
        print("current_schema_version: %d" % current_schema_version)

    return current_schema_version


def get_target_schema_version():
    # default
    target_schema_version = 2
    # read version value from VERSION file
    if os.path.isfile( "/var/lib/irods/VERSION" ):
        version_file = os.path.abspath("/var/lib/irods/VERSION")
    else:
        version_file = os.path.dirname(os.path.dirname(os.path.realpath(__file__))) + "/VERSION"
    lines = [line.rstrip('\n') for line in open(version_file)]
    for line in lines:
        (name, value) = line.split("=")
        if (name.strip() == "CATALOG_SCHEMA_VERSION"):
            if DEBUG:
                print("CATALOG_SCHEMA_VERSION found in %s..." % version_file)
            target_schema_version = int(value.strip())
            if DEBUG:
                print("CATALOG_SCHEMA_VERSION: %d" % target_schema_version)
    # return it
    if DEBUG:
        print("target_schema_version: %d" % target_schema_version)
    return target_schema_version


def update_schema_version(cfg, version):
    if DEBUG:
        print("Updating schema_version...")
    # update the database
    thesql = "update R_GRID_CONFIGURATION \
              set option_value = '%d' \
              where namespace = 'database' \
              and option_name = 'schema_version';" % version
    result = cfg.exec_sql_cmd(thesql)
    if (result[2].decode('utf-8') != ""):
        print("ERROR: Updating schema_version did not complete...")
        print(result[2])
        return
    # success
    if DEBUG:
        print("SUCCESS, updated to schema_version %d" % version)


def get_update_files(version, dbtype):
    # list all files
    sd = os.path.dirname(os.path.realpath(__file__)) + "/" + schema_directory
    if ( not os.path.exists( sd )):
        sd = os.path.dirname(
            os.path.dirname(os.path.realpath(__file__))) + "/plugins/database/packaging/" + schema_directory
    mycmd = "find %s -name %s" % (sd, "*." + dbtype + ".*sql")
    if DEBUG:
        print("finding files: %s" % mycmd)
    p = subprocess.Popen(
        mycmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (myout, myerr) = p.communicate()
    if (p.returncode == 0):
        return myout.decode('utf-8').strip().split("\n")
    else:
        print("ERROR: %s" % myerr.decode('utf-8'))
        return


def update_database_to_latest_version():
    # get config
    cfg = ServerConfig()
    # get current version
    current_schema_version = get_current_schema_version(cfg)
    # get target version
    target_schema_version = get_target_schema_version()
    # check if any work to be done
    if (current_schema_version > target_schema_version):
        print("Catalog Schema Version is from the future (current=%d > target=%d)." % (
            current_schema_version, target_schema_version))
        return
    if (current_schema_version == target_schema_version):
        print("Catalog Schema Version is already up to date (version=%d)." %
              target_schema_version)
        return
    # read possible update sql files
    foundfiles = get_update_files(
        current_schema_version, cfg.values['catalog_database_type'])
    if DEBUG:
        print("files: %s" % foundfiles)
    updatefiles = {}
    for f in foundfiles:
        version = int(f.split("/")[-1].split(".")[0])
        updatefiles[version] = f
    if DEBUG:
        print("updatefiles: %s" % updatefiles)
    # check all necessary update files exist
    for version in range(current_schema_version + 1, target_schema_version + 1):
        if DEBUG:
            print("checking... %d" % version)
        if version not in updatefiles.keys():
            print(
                "ERROR: SQL Update File Not Found for Schema Version %d" % version)
            return
    # run each update sql file
    for version in range(current_schema_version + 1, target_schema_version + 1):
        print("Updating to Catalog Schema... %d" % version)
        if DEBUG:
            print("running: %s" % updatefiles[version])
        result = cfg.exec_sql_file(updatefiles[version])
        if (result[2].decode('utf-8') != ""):
            print("ERROR: SQL did not complete...")
            print(result[2])
            return
        if DEBUG:
            print("sql result...")
        if DEBUG:
            print(result)
        # update schema_version in database
        update_schema_version(cfg, version)
    # done
    print("Done.")


def main():

    if DEBUG:
        print("-------------------- DEBUG IS ON --------------------")
    update_database_to_latest_version()
    if DEBUG:
        print("-------------------- DEBUG IS ON --------------------")

if __name__ == '__main__':
    main()
