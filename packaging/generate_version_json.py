import json
import os
import subprocess
import platform

# initialize
irods_version = None
catalog_schema_version = None
outdata = {}

# get top level directory
irodstoplevel = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))

# get irods_version and catalog_schema_version
versionfile = irodstoplevel+"/VERSION"
jsonfile = versionfile+".json"
if os.path.isfile(jsonfile):
    # VERSION.json already exists, use it
    fh = open(jsonfile)
    data = json.load(fh)
    irods_version = data['irods_version']
    catalog_schema_version = data['catalog_schema_version']
else:
    # only VERSION exists, read in legacy format
    fh = open(versionfile)
    lines = fh.readlines()
    for line in lines:
        items = line.split("=")
        if items[0].strip() == "IRODSVERSION":
            irods_version = items[1].strip()
        if items[0].strip() == "CATALOG_SCHEMA_VERSION":
            catalog_schema_version = items[1].strip()
outdata['irods_version'] = irods_version
outdata['catalog_schema_version'] = int(catalog_schema_version)

# get commit_id
p = subprocess.Popen("git log | head -n1 | awk '{print $2}'",
                     stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
(commit_id, err) = p.communicate()
outdata['commit_id'] = commit_id.strip()

# get build_system_information
unameinfo = platform.uname()
outdata['build_system_information'] = " ".join(unameinfo).strip()

# get compile_time
p = subprocess.Popen('date -u +"%Y-%m-%dT%H:%M:%SZ"',
                     stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
(compile_time, err) = p.communicate()
outdata['compile_time'] = compile_time.strip()

# get compiler_version
p = subprocess.Popen('g++ --version | head -n1',
                     stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
(compiler_version, err) = p.communicate()
outdata['compiler_version'] = compiler_version.strip()

print json.dumps(outdata, indent=4, sort_keys=True)
