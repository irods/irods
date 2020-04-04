#!/env/python

# This script is designed to be run on a newly upgraded (from 4.1.x to 4.2.x)
# Catalog Provider before upgrading the Catalog Consumers in the same Zone from 4.1.x.
#
# This script makes no changes to the iRODS iCAT itself.
#
# This script generates two sets of iadmin commands to be run on the Catalog Provider.
#
# This script is not required, but provides additional assurance that no
# resources will be deleted due to any confusion associated with the
# packaged preremove.sh script included in 4.1 (including and up to 4.1.12).

import socket
import subprocess

# GATHER - get the resources and their hostname values from the catalog
resources = {}
cmd = ['iquest','%s %s',"select RESC_NAME, RESC_LOC where RESC_NAME != 'bundleResc'"]
process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=None)
output = process.communicate()
for line in output[0].splitlines():
    name, host = line.split()
    resources[name] = host

# BEFORE - temporarily swap resource hostnames to provider's hostname
myhost = socket.gethostname()
print('# Run these on the catalog provider BEFORE upgrading all catalog consumers')
for r in resources:
    print('iadmin modresc {0} host {1}'.format(r, myhost))

# AFTER - swap resource hostnames back to their original values
print('')
print('# Run these on the catalog provider AFTER upgrading all catalog consumers')
for k,v in resources.items():
    print('iadmin modresc {0} host {1}'.format(k, v))

