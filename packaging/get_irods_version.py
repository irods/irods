import os
import json

irodstoplevel = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
#print irodstoplevel
fh = open(irodstoplevel+"/VERSION.json")
data = json.load(fh)
print data['irods_version']

