import commands
import contextlib
import json
import mmap
import os
import psutil
import re
import shlex
import shutil
import socket
import subprocess
import tempfile
import time


def update_json_file_from_dict(filename, update_dict):
    with open(filename) as f:
        env = json.load(f)
    env.update(update_dict)
    with open(filename, 'w') as f:
        json.dump(env, f, indent=4)

def get_hostname():
    return socket.gethostname()

def get_irods_top_level_dir():
    if os.path.isfile('/etc/irods/server_config.json'):
        return '/var/lib/irods'
    return os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

def get_irods_config_dir():
    if os.path.isfile('/etc/irods/server_config.json'):
        return '/etc/irods'
    return os.path.join(get_irods_top_level_dir(), 'iRODS/config')

def create_directory_of_small_files(directory_name_suffix, file_count):
    if not os.path.exists(directory_name_suffix):
        os.mkdir(directory_name_suffix)
    for i in range(file_count):
        with open('{0}/{1}'.format(directory_name_suffix, i), 'w') as f:
            f.write("iglkg3fqfhwpwpo-" + "A" * i)

def create_local_testfile(filename):
    filepath = os.path.abspath(filename)
    with open(filepath, 'wb') as f:
        f.write('TESTFILE -- [' + filepath + ']')
    return filepath

def create_local_largefile(filename):
    filepath = os.path.abspath(filename)
    os.system('dd if=/dev/zero of=' + filepath + ' bs=1M count=64')
    return filepath

def touch(fname, times=None):
    with file(fname, 'a'):
        os.utime(fname, times)

def cat(fname, string, times=None):
    with file(fname, 'a') as f:
        f.write(string)

def make_file(f_name, f_size, source='/dev/zero'):
    output = commands.getstatusoutput('dd if="' + source + '" of="' + f_name + '" count=1 bs=' + str(f_size))
    if output[0] != 0:
        sys.stderr.write(output[1] + '\n')
        raise OSError(output[0], "call to dd returned non-zero")

def ils_output_to_entries(stdout):
    raw = stdout.strip().split('\n')
    collection = raw[0]
    entries = [entry.strip() for entry in raw[1:]]
    return entries

def get_vault_path(session):
    cmdout = session.run_icommand(['iquest', '%s', "select RESC_VAULT_PATH where RESC_NAME = 'demoResc'"])
    if cmdout[2] != '':
        raise OSError(cmdout[2], 'iquest wrote to stderr when called from get_vault_path()')
    return cmdout[1].rstrip('\n')

def get_vault_session_path(session):
    return os.path.join(get_vault_path(session),
                        "home",
                        session.username,
                        session._session_id)

def make_large_local_tmp_dir(dir_name, file_count, file_size):
    os.mkdir(dir_name)
    for i in range(file_count):
        make_file(os.path.join(dir_name, "junk" + str(i).zfill(4)),
                  file_size)
    local_files = os.listdir(dir_name)
    assert len(local_files) == file_count, "dd loop did not make all " + str(file_count) + " files"
    return local_files

@contextlib.contextmanager
def core_re_backed_up():
    with tempfile.NamedTemporaryFile(prefix='core.re_backup') as f:
        shutil.copyfile('/etc/irods/core.re', f.name)
        yield
        shutil.copyfile(f.name, '/etc/irods/core.re')

def prepend_string_to_core_re(string):
    core_re_path = '/etc/irods/core.re'
    with open(core_re_path, 'r') as f: contents = f.read()
    with open(core_re_path, 'w') as f:
        f.write(string)
        f.write(contents)

def get_log_path(log_source):
    log_prefix_dict = {
        'server': 'rodsLog',
        're': 'reLog',
    }
    assert log_source in log_prefix_dict, log_source

    log_prefix = log_prefix_dict[log_source]
    server_log_dir = os.path.join(get_irods_top_level_dir(), 'iRODS/server/log')
    command_str = 'ls -t {0}/{1}* | head -n1'.format(server_log_dir, log_prefix)
    proc = subprocess.Popen(command_str, stdout=subprocess.PIPE, shell=True)
    stdout, stderr = proc.communicate()
    if proc.returncode != 0 or stdout == '':
        raise subprocess.CalledProcessError(proc.returncode, command_str, 'stdout [{0}] stderr[{1}]'.format(stdout, stderr))
    log_file_path = stdout.rstrip()
    return log_file_path

def get_log_size(log_source):
    return os.stat(get_log_path(log_source)).st_size

def write_to_log(log_source, message):
    with open(get_log_path(log_source), 'a') as f:
        f.write(message)

def count_occurrences_of_string_in_log(log_source, string, start_index=0):
    with open(get_log_path(log_source)) as f:
        m = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
        n = 0
        i = m.find(string, start_index)
        while i != -1:
            n += 1
            i = m.find(string, i+1)
        return n

def run_command(command_arg, check_rc=False, stdin_string='', use_unsafe_shell=False, env=None, cwd=None):
    if not use_unsafe_shell and isinstance(command_arg, str):
        command_arg = shlex.split(command_arg)
    p = subprocess.Popen(command_arg, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env, shell=use_unsafe_shell, cwd=cwd)
    stdout, stderr = p.communicate(input=stdin_string)
    rc = p.returncode
    if check_rc:
        if rc != 0:
            raise subprocess.CalledProcessError(rc, command_arg, stdout + '\n\n' + stderr)
    return rc, stdout, stderr

def check_run_command_output(command_arg, stdout, stderr, check_type='EMPTY', expected_results='', use_regex=False):
    assert check_type in ['EMPTY', 'STDOUT', 'STDERR', 'STDOUT_MULTILINE', 'STDERR_MULTILINE'], check_type

    if isinstance(expected_results, str):
        expected_results = [expected_results]

    regex_msg = 'regex ' if use_regex else ''


    print 'Expecting {0}: {1}{2}'.format(check_type, regex_msg, expected_results)
    print '  stdout:'
    print '    | ' + '\n    | '.join(stdout.splitlines())
    print '  stderr:'
    print '    | ' + '\n    | '.join(stderr.splitlines())

    if check_type not in ['STDERR', 'STDERR_MULTILINE'] and stderr != '':
        print 'FAILURE: Unexpected output on stderr\n'
        return False

    if check_type in ['STDOUT', 'STDERR', 'STDOUT_MULTILINE', 'STDERR_MULTILINE']:
        lines = stdout.splitlines() if check_type in ['STDOUT', 'STDOUT_MULTILINE'] else stderr.splitlines()

        if check_type in ['STDOUT_MULTILINE', 'STDERR_MULTILINE']:
            for er in expected_results:
                regex_pattern = er if use_regex else re.escape(er)
                for line in lines:
                    if re.search(regex_pattern, line.rstrip('\n')):
                        break
                else:
                    print '    --> stopping search - expected result not found'
                    break
            else:
                print 'SUCCESS\n'
                return True
            print 'FAILURE: Not found\n'
            return False
        else:
            for line in lines:
                found_count = 0
                for er in expected_results:
                    regex_pattern = er if use_regex else re.escape(er)
                    if re.search(regex_pattern, line.rstrip('\n')):
                        found_count += 1
                if found_count == len(expected_results):
                    print 'SUCCESS\n'
                    return True
            print 'FAILURE: Not found\n'
            return False
    elif check_type == 'EMPTY':
        if stdout == '':
            print 'SUCCESS\n'
            return True
        print 'FAILURE: Unexpected output on stdout\n'
        return False
    assert False

def extract_function_kwargs(func, kwargs):
    args = func.func_code.co_varnames
    args_dict = {}
    for k, v in kwargs.items():
        if k in args:
            args_dict[k] = v
    return args_dict

def assert_command(*args, **kwargs):
    _assert_helper(*args, should_fail=False, **kwargs)

def assert_command_fail(*args, **kwargs):
    _assert_helper(*args, should_fail=True, **kwargs)

def _assert_helper(command_arg, check_type='EMPTY', expected_results='', should_fail=False, **kwargs):
    run_command_arg_dict = extract_function_kwargs(run_command, kwargs)
    _, stdout, stderr = run_command(command_arg, **run_command_arg_dict)

    fail_string = ' FAIL' if should_fail else ''
    if isinstance(command_arg, list):
        print 'Assert{0}Command: {1}'.format(fail_string, ' '.join(command_arg))
    else:
        print 'Assert{0} Command: {1}'.format(fail_string, command_arg)

    check_run_command_output_arg_dict = extract_function_kwargs(check_run_command_output, kwargs)
    result = should_fail != check_run_command_output(command_arg, stdout, stderr, check_type=check_type, expected_results=expected_results, **check_run_command_output_arg_dict)
    if not result:
        print 'ASSERT COMMAND FAILED'
    assert result

def restart_irods_server(env=None):
    assert_command('{0} restart'.format(os.path.join(get_irods_top_level_dir(), 'iRODS/irodsctl')), 'STDOUT_MULTILINE', ['Stopping iRODS server', 'Starting iRODS server'], env=env)
