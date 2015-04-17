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
commit_id, err = p.communicate()
outdata['commit_id'] = commit_id.strip().decode('ascii')

# get build_system_information
unameinfo = platform.uname()
outdata['build_system_information'] = ' '.join(unameinfo).strip()

# get compile_time
p = subprocess.Popen('date -u +"%Y-%m-%dT%H:%M:%SZ"',
                     stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
(compile_time, err) = p.communicate()
outdata['compile_time'] = compile_time.strip().decode('ascii')

# get compiler_version
p = subprocess.Popen('g++ --version | head -n1',
                     stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
(compiler_version, err) = p.communicate()
outdata['compiler_version'] = compiler_version.strip().decode('ascii')

print(json.dumps(outdata, indent=4, sort_keys=True))
