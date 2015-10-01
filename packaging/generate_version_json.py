from __future__ import absolute_import
from __future__ import print_function
from __future__ import unicode_literals

import json
import os
import subprocess
import platform

# get top level directory
irodstoplevel = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))

# load the template
templatefile = os.path.join(irodstoplevel, 'VERSION.json.dist')
with open(templatefile) as fh:
    outdata = json.load(fh)

# get commit_id
p = subprocess.Popen("git log | head -n1 | awk '{print $2}'",
                     stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
commit_id = p.communicate()[0].decode()
outdata['commit_id'] = commit_id.strip()

# get build_system_information
unameinfo = platform.uname()
outdata['build_system_information'] = ' '.join(unameinfo).strip()

# get compile_time
p = subprocess.Popen('date -u +"%Y-%m-%dT%H:%M:%SZ"',
                     stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
compile_time = p.communicate()[0].decode()
outdata['compile_time'] = compile_time.strip()

# get compiler_version
p = subprocess.Popen('g++ --version | head -n1',
                     stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
compiler_version = p.communicate()[0].decode()
outdata['compiler_version'] = compiler_version.strip()

print(json.dumps(outdata, indent=4, sort_keys=True))
