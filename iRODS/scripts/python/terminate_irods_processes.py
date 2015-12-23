from __future__ import print_function
import subprocess
import os
import sys
import psutil
import itertools

#
# This script finds iRODS processes owned by the current user,
# confirms their full_path against this installation of iRODS,
# checks their executing state, then pauses and kills each one.
#

def execute_command_nonblocking(args, **kwargs):
    if 'env' in kwargs:
        kwargs_without_env = copy.copy(kwargs)
        kwargs_without_env['env'] = 'HIDDEN'
    else :
        kwargs_without_env = kwargs
    try :
        return subprocess.Popen(args, **kwargs)
    except OSError as e:
        irods_six.reraise(IrodsControllerError,
            IrodsControllerError('\n'.join([
                'Call to open process with {0} failed:'.format(
                    args),
                indent(
                    'Could not find the requested executable \'{0}\'; '
                    'please ensure \'{0}\' is installed and in the path.'.format(
                        args[0]))])),
            sys.exc_info()[2])

def execute_command_permissive(args, **kwargs):
    if 'stdout' not in kwargs:
        kwargs['stdout'] = subprocess.PIPE
    if 'stderr' not in kwargs:
        kwargs['stderr'] = subprocess.PIPE

    p = execute_command_nonblocking(args, **kwargs)

    out, err = [t.decode() for t in p.communicate()]
    return (out, err, p.returncode)

def get_pids_executing_binary_file(binary_file_path):
    username = ''
    try:
        usename = os.environ['USER']
    except KeyError:
        print('USER environment variable is not set')
        return []

    out, err, returncode = execute_command_permissive(['lsof', '-b', '-w', '-u', username])
    out = out if returncode == 0 else ''
    pid = parse_formatted_lsof_output(out, binary_file_path)
    return pid

def parse_formatted_lsof_output(output,binary_file_path):
    lines = output.split('\n')
    for line in lines:
        columns = line.split()
        for c in columns:
            if -1 != c.find(binary_file_path):
                for d in columns:
                    if -1 != d.find('txt'):
                        return [int(columns[1])]
    return []

p = subprocess.Popen(['which', 'lsof'],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)
out, err = p.communicate()
if p.returncode != 0:
    print('\n\t'.join([
            'Call to \'which lsof\' failed:',
            'Return code: {0}'.format(p.returncode),
            '\n\t\t'.join(itertools.chain(['Standard output:'], out.splitlines())),
            '\n\t\t'.join(itertools.chain(['Error output:'], err.splitlines()))]),
        'Please ensure lsof is installed and in your path.',
        file=sys.stderr, sep='\n')
    sys.exit(1)

top_level_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
for binary in ["irodsServer", "irodsReServer", "irodsXmsgServer", "irodsAgent"]:
    full_path = os.path.join(top_level_dir, "iRODS/server/bin/"+binary)
    for pid in get_pids_executing_binary_file(full_path):
        #print("killing %d %s" % (pid, full_path))
        p = psutil.Process(pid)
        p.suspend()
        p.terminate()
        p.kill()
