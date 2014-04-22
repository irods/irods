import time
import os
from server_config import Server_Config
import subprocess

schema_directory = "schema_updates"

DEBUG = True
DEBUG = False


def get_current_schema_version(cfg):
    result = cfg.exec_sql_cmd( "select option_value \
                                 from r_grid_configuration \
                                 where namespace='database' \
                                 and option_name='schema_version'")
    if DEBUG:
        print(result)
    if ("relation \"r_grid_configuration\" does not exist" in result[2].decode('utf-8')):
        # create and populate configuration table
        result = cfg.exec_sql_cmd("create table R_GRID_CONFIGURATION ( \
                                     namespace varchar(2700), \
                                     option_name varchar(2700), \
                                     option_value varchar(2700) ); \
                                   create unique index idx_grid_configuration \
                                     on R_GRID_CONFIGURATION (namespace, option_name); \
                                   insert into R_GRID_CONFIGURATION VALUES ( \
                                     'database', 'schema_version', '1' );")
        if DEBUG:
            print(result)
        if (result[2].decode('utf-8') != ""):
            print(
                "ERROR: Creating Grid Configuration Table did not complete...")
            print(result[2])
            return
        # use default value
        current_schema_version = 1
    else:
        current_schema_version = int(
            result[1].decode('utf-8').split("\n")[2].strip())
    if DEBUG:
        print("current_schema_version: %d" % current_schema_version)
    return current_schema_version


def get_target_schema_version():
    # default
    target_schema_version = 2
    # read version value from /var/lib/irods/VERSION
    version_file = os.path.abspath("/var/lib/irods/VERSION")
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
    thesql = "update r_grid_configuration \
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
    cfg = Server_Config()
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
