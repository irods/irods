from __future__ import print_function
import base64
import collections
import contextlib
import copy
import errno
import getpass
import grp
import hashlib
import itertools
import json
import logging
import mmap
import os
import platform
import pprint
import psutil
import pwd
import re
import shlex
import shutil
import socket
import subprocess
import sys
import tempfile
import time

from . import six

from .exceptions import IrodsError, IrodsWarning
from . import paths

# get the fully qualified domain name
#(no, really, getfqdn() is insufficient)
def get_hostname():
    return socket.getaddrinfo(
        socket.gethostname(), 0, 0, 0, 0, socket.AI_CANONNAME)[0][3]

def safe_shlex_split_for_2_6(args):
    if not isinstance(args, str) and isinstance(args, six.text_type):
        args = args.encode('ascii')
    return shlex.split(args)

def execute_command_nonblocking(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, use_unsafe_shell=False, **kwargs):
    l = logging.getLogger(__name__)
    if not use_unsafe_shell and isinstance(args, six.string_types):
        args = safe_shlex_split_for_2_6(args)
    kwargs['stdout'] = stdout
    kwargs['stderr'] = stderr
    kwargs['shell'] = use_unsafe_shell
    if 'env' in kwargs:
        kwargs_without_env = copy.copy(kwargs)
        kwargs_without_env['env'] = 'HIDDEN'
    else :
        kwargs_without_env = kwargs
    l.debug('Calling %s with options:\n%s', args, pprint.pformat(kwargs_without_env))
    try :
        return subprocess.Popen(args, **kwargs)
    except OSError as e:
        six.reraise(IrodsError,
            IrodsError('\n'.join([
                'Call to open process with {0} failed:'.format(
                    args),
                indent(
                    'Could not find the requested executable \'{0}\'; '
                    'please ensure \'{0}\' is installed and in the path.'.format(
                        args[0]))])),
            sys.exc_info()[2])

def execute_command_timeout(args, timeout=10, **kwargs):
    p = execute_command_nonblocking(args, **kwargs)
    start_time = time.time()
    while time.time() < start_time + timeout:
        if p.poll() is not None:
            out, err = communicate_and_log(p, args)
            check_command_return(args, out, err, p.returncode, **kwargs)
            break
        time.sleep(0.3)
    else:
        try:
            if p.poll() is None:
                p.kill()
        except OSError:
            pass
        raise IrodsError(
            'The call {0} did not complete within'
            ' {1} seconds.'.format(args, timeout))

def execute_command_permissive(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, input=None, **kwargs):
    if input is not None:
        if 'stdin' in kwargs and kwargs['stdin'] != subprocess.PIPE:
            raise IrodsError('\'input\' option is mutually exclusive with a \'stdin\' '
                    'option that is not equal to \'subprocess.PIPE\'.')
        kwargs['stdin'] = subprocess.PIPE
    p = execute_command_nonblocking(args, stdout=stdout, stderr=stderr, **kwargs)

    out, err = communicate_and_log(p, args, input)
    return (out, err, p.returncode)

def check_command_return(args, out, err, returncode, **kwargs):
    if returncode is not None and returncode != 0:
        if 'env' in kwargs:
            kwargs_without_env = copy.copy(kwargs)
            kwargs_without_env['env'] = 'HIDDEN'
        else :
            kwargs_without_env = kwargs
        raise IrodsError('\n'.join([
            'Call to open process with {0} returned an error:'.format(
                args),
            indent(
                'Options passed to Popen:',
                indent(*['{0}: {1}'.format(k, v) for k, v in kwargs_without_env.items()]),
                'Return code: {0}'.format(returncode),
                'Standard output:',
                indent(out),
                'Error output:',
                indent(err))]))

def communicate_and_log(p, args, input=None):
    l = logging.getLogger(__name__)
    out, err = [t.decode('utf_8') for t in p.communicate(input=(input.encode('ascii') if input is not None else None))]
    message = ['Command %s returned with code %s.' % (args, p.returncode)]
    if input:
        message.append('stdin:\n%s' % indent(input))
    if out:
        message.append('stdout:\n%s' % indent(out))
    if err:
        message.append('stderr:\n%s' % indent(err))
    l.debug('\n'.join(message))
    return (out, err)

def execute_command(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, **kwargs):
    out, err, returncode = execute_command_permissive(args, stdout=stdout, stderr=stderr, **kwargs)
    check_command_return(args, out, err, returncode, **kwargs)

    return (out, err)

def indent(*text, **kwargs):
    if 'indentation' in kwargs:
        indentation = kwargs['indentation']
    else:
        indentation='  '
    return '\n'.join([
        ''.join([indentation, '\n{0}'.format(indentation).join(lines.splitlines())])
            for lines in text])

def get_pids_executing_binary_file(binary_file_path):
    def get_exe(process):
        if psutil.version_info >= (2,0):
            return process.exe()
        return process.exe
    abspath = os.path.abspath(binary_file_path)
    pids = []
    for p in psutil.process_iter():
        try:
            if abspath == get_exe(p):
                pids.append(p.pid)
        except (psutil.NoSuchProcess, psutil.AccessDenied):
            pass
    return pids

def kill_pid(pid):
    p = psutil.Process(pid)
    p.suspend()
    p.terminate()
    p.kill()

def find_shared_object(so_name, regex=False, additional_directories=[]):
    paths = []
    if regex:
        so_regex = re.compile(so_name)

    if 'LD_PRELOAD' in os.environ:
        for path in os.environ['LD_PRELOAD'].split(':'):
            if path not in paths and os.path.exists(path) and (
                    (regex and so_regex.match(os.path.basename(path))) or
                    (not regex and os.path.basename(path) == so_name)):
                paths.append(path)

    if 'LD_LIBRARY_PATH' in os.environ:
        for directory in os.environ['LD_LIBRARY_PATH'].split(':'):
            if regex and os.path.exists(directory):
                for name in os.listdir(directory):
                    if so_regex.match(name) and os.path.join(directory, name) not in paths:
                        paths.append(os.path.join(directory, name))
            elif os.path.exists(os.path.join(directory, so_name)) and os.path.join(directory, so_name) not in paths:
                paths.append(os.path.join(directory, so_name))

    env = os.environ.copy()
    env['PATH'] = ':'.join([env['PATH'], '/sbin'])
    out, _ = execute_command(['ldconfig', '-vNX'], env=env)
    for directory in [d.rstrip(':') for d in out.splitlines() if d and d[0] == '/']:
        if regex and os.path.exists(directory):
            for name in os.listdir(directory):
                if so_regex.match(name) and os.path.join(directory, name) not in paths:
                    paths.append(os.path.join(directory, name))
        elif os.path.exists(os.path.join(directory, so_name)) and os.path.join(directory, so_name) not in paths:
            paths.append(os.path.join(directory, so_name))

    for (directory, _, names) in itertools.chain(*[os.walk(d) for d in itertools.chain(additional_directories, ['/usr/lib/'])]):
        if regex:
            for name in names:
                if so_regex.match(os.path.basename(name)) and os.path.join(directory, name) not in paths:
                    paths.append(os.path.join(directory, name))
        elif os.path.exists(os.path.join(directory, so_name)) and os.path.join(directory, so_name) not in paths:
            paths.append(os.path.join(directory, so_name))

    return paths

def file_digest(filename, hash_type, encoding='hex'):
    block_size = pow(2, 20)
    hasher = hashlib.new(hash_type)
    with open(filename, 'rb') as f:
        while True:
            data = f.read(block_size)
            if not data:
                break
            hasher.update(data)
    if encoding == 'hex':
        return hasher.hexdigest()
    if encoding == 'base64':
        return base64.b64encode(hasher.digest()).decode()
    if encoding is None or encoding == 'none':
        return hasher.digest()
    raise IrodsError('Unknown encoding %s for digest' % (encoding))


def re_shm_exists():
    possible_shm_locations = ['/var/run/shm', '/dev/shm']
    for l in possible_shm_locations:
        try:
            files = os.listdir(l)
            for f in files:
                if 'irods' in f.lower():
                    return os.path.join(l, f)
        except OSError:
            pass
    return False

def json_object_hook_list(l):
    rv = []
    for i in l:
        if not isinstance(i, str) and isinstance(i, six.text_type):
            i = i.encode('ascii')
        elif isinstance(i, list):
            i = json_object_hook_list(i)
        rv.append(i)
    return rv

def json_object_hook_dict(d):
    rv = {}
    for k, v in d.items():
        if not isinstance(k, str) and isinstance(k, six.text_type):
            k = k.encode('ascii')
        if not isinstance(v, str) and isinstance(v, six.text_type):
            v = v.encode('ascii')
        elif isinstance(v, list):
            v = json_object_hook_list(v)
        rv[k] = v
    return rv

def open_and_load_json(filename):
    with open(filename, 'rt') as f:
        return json.load(f, object_hook=json_object_hook_dict)

def update_json_file_from_dict(filename, update_dict):
    env = open_and_load_json(filename)
    env.update(update_dict)
    with open(filename, 'wt') as f:
        json.dump(env, f, indent=4)

def create_directory_of_small_files(directory_name_suffix, file_count):
    if not os.path.exists(directory_name_suffix):
        os.mkdir(directory_name_suffix)
    for i in range(file_count):
        with open('{0}/{1}'.format(directory_name_suffix, i), 'wt') as f:
            print("iglkg3fqfhwpwpo-" + "A" * i, file=f, end='')

def create_local_testfile(filename):
    filepath = os.path.abspath(filename)
    with open(filepath, 'wt') as f:
        print('TESTFILE -- [' + filepath + ']', file=f, end='')
    return filepath

def touch(fname, times=None):
    with open(fname, 'at'):
        os.utime(fname, times)

def cat(fname, string):
    with open(fname, 'at') as f:
        print(string, file=f, end='')

def make_file(f_name, f_size, contents='zero', block_size_in_bytes=1000):
    if contents not in ['arbitrary', 'random', 'zero']:
        raise AssertionError
    if contents == 'arbitrary' or f_size == 0:
        execute_command(['truncate', '-s', str(f_size), f_name])
        return

    source = {'zero': '/dev/zero',
              'random': '/dev/urandom'}[contents]

    count = f_size / block_size_in_bytes
    if count > 0:
        execute_command(['dd', 'if='+source, 'of='+f_name, 'count='+str(count), 'bs='+str(block_size_in_bytes)])
        leftover_size = f_size % block_size_in_bytes
        if leftover_size > 0:
            execute_command(['dd', 'if='+source, 'of='+f_name, 'count=1', 'bs='+str(leftover_size), 'oflag=append', 'conv=notrunc'])
    else:
        execute_command(['dd', 'if='+source, 'of='+f_name, 'count=1', 'bs='+str(f_size)])


def make_dir_p(directory):
    try:
        os.makedirs(directory)
    except OSError as e:
        if e.errno == errno.EEXIST and os.path.isdir(directory):
            pass
        else:
            raise

def make_large_local_tmp_dir(dir_name, file_count, file_size):
    os.makedirs(dir_name)
    for i in range(file_count):
        make_file(os.path.join(dir_name, "junk" + str(i).zfill(4)),
                  file_size)
    local_files = os.listdir(dir_name)
    if len(local_files) != file_count:
        raise AssertionError("dd loop did not make all " + str(file_count) + " files")
    return local_files

def make_deep_local_tmp_dir(root_name, depth=10, files_per_level=50, file_size=100):
    # output
    directories = {}

    current_dir_name = root_name
    for d in range(depth):
        # make subdir and files
        files = make_large_local_tmp_dir(current_dir_name, files_per_level, file_size)

        # add to output
        directories[current_dir_name] = files

        # next level down
        current_dir_name = os.path.join(current_dir_name, 'sub'+str(d))

    return directories

def files_in_dir(path):
    for file in os.listdir(path):
        if os.path.isfile(os.path.join(path, file)):
            yield file

def get_user_env(user):
    out, _ = execute_command(['su', '-', user, '-c',
        'python -c "from __future__ import print_function; import os; import json; print(json.dumps(dict(os.environ)))"'])
    return json.loads(out)

def switch_user(user, group=None):
    user_env = get_user_env(user)

    pw_record = pwd.getpwnam(user)
    os.environ['HOME'] = pw_record.pw_dir
    os.environ['LOGNAME'] = pw_record.pw_name
    os.environ['USER'] = pw_record.pw_name
    os.setgid(pw_record.pw_gid if group is None else grp.getgrnam(group).gr_gid)
    os.setuid(pw_record.pw_uid)

@contextlib.contextmanager
def file_backed_up(filename):
    with tempfile.NamedTemporaryFile(prefix=os.path.basename(filename)) as f:
        shutil.copyfile(filename, f.name)
        try:
            yield filename
        finally:
            shutil.copyfile(f.name, filename)

@contextlib.contextmanager
def directory_deleter(dirname):
    try:
        yield dirname
    finally:
        shutil.rmtree(dirname)

def is_jsonschema_installed():
    try:
        import jsonschema
        return True
    except ImportError:
        return False

def prepend_string_to_file(string, filename):
    with open(filename, 'r') as f:
        contents = f.read()
    with open(filename, 'wt') as f:
        print(string, file=f, end='')
        print(contents, file=f, end='')

def make_environment_dict(username, hostname, zone_name, use_ssl=True):
    irods_home = os.path.join('/', zone_name, 'home', username)

    environment = {
        'irods_host': hostname,
        'irods_port': 1247,
        'irods_default_resource': 'demoResc',
        'irods_home': irods_home,
        'irods_cwd': irods_home,
        'irods_user_name': username,
        'irods_zone_name': zone_name,
        'irods_client_server_negotiation': 'request_server_negotiation',
        'irods_client_server_policy': 'CS_NEG_REFUSE',
        'irods_encryption_key_size': 32,
        'irods_encryption_salt_size': 8,
        'irods_encryption_num_hash_rounds': 16,
        'irods_encryption_algorithm': 'AES-256-CBC',
        'irods_default_hash_scheme': 'SHA256',
        'irods_maximum_size_for_single_buffer_in_megabytes': 32,
        'irods_default_number_of_transfer_threads': 4,
        'irods_maximum_number_of_transfer_threads': 64,
        'irods_transfer_buffer_size_for_parallel_transfer_in_megabytes': 4,
        'irods_connection_pool_refresh_time_in_seconds': 300

    }
    if use_ssl:
        environment.update({
            'irods_client_server_policy': 'CS_NEG_REQUIRE',
            'irods_ssl_verify_server': 'cert',
            'irods_ssl_ca_certificate_file': '/etc/irods/server.crt',
        })
    return environment

def get_os_distribution():
    return platform.linux_distribution()[0]

def get_os_distribution_version_major():
    return platform.linux_distribution()[1].split('.')[0]

def get_object_names_from_entries(ils_out):
    if isinstance(ils_out, six.string_types):
        ils_out = ils_out.strip().split()
    for item in ils_out:
        # strip collections
        if not item.startswith('C- /'):
            yield item

def get_file_size_by_path(path):
    return os.stat(path).st_size

def write_to_log(log_path, message):
    with open(log_path, 'at') as f:
        print(message, file=f, end='')

def count_occurrences_of_regexp_in_log(log_path, pattern, start_index=0):
    occurrences = []
    target = None
    if isinstance(pattern,(tuple,list)):
        pattern = [pattern[0].encode('ascii')] + list(pattern[1:])
    elif isinstance(pattern,re._pattern_type):
        target = pattern
    else:
        pattern = [pattern.encode('ascii')]  # assume string-like
    if target is None: target=re.compile(*pattern)
    with open(log_path) as f:
        m = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
        occurrences.extend(j for j in target.finditer(m[start_index:]))
        m.close()
    return occurrences

def count_occurrences_of_string_in_log(log_path, string, start_index=0):
    with open(log_path) as f:
        m = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
        n = 0
        target = string.encode('ascii')
        i = m.find(target, start_index)
        while i != -1:
            n += 1
            i = m.find(target, i + 1)
        m.close()
        return n

def version_string_to_tuple(version_string):
    return tuple(map(int, version_string.split('.')))

def hostname_resolves_to_local_address(hostname):
    _, err, ret = execute_command_permissive([
        os.path.join( paths.server_bin_directory(), 'hostname_resolves_to_local_address'),
        hostname])
    if ret == 0:
        return True
    elif ret == 1:
        return False
    raise IrodsError('Error encountered in hostname_resolves_to_local_address for hostname [{0}]:\n{1}'.format(hostname, err))

def get_header(message):
    lines = [l.strip() for l in message.splitlines()]
    length = 0
    for line in lines:
        length = max(length, len(line))
    edge = '+' + '-' * (length + 2) + '+'
    format_string = '{0:<' + str(length) + '}'
    header_lines = ['', edge]
    for line in lines:
        header_lines.append('| ' + format_string.format(line) + ' |')
    header_lines.append(edge)
    header_lines.append('')
    return '\n'.join(header_lines)

def nested_update(d, u):
    for k, v in u.items():
        d[k] = nested_update(d.get(k, {}), v) if isinstance(v, collections.Mapping) else u[k]
    return d

def prompt(*args, **kwargs):
    echo = kwargs.get('echo', True)
    end = kwargs.get('end', ': ')
    input_filter = kwargs.get('input_filter', lambda x: x)

    l = logging.getLogger(__name__)
    message = ''.join([args[0] % tuple(args[1:]), end])
    while True:
        l.debug(message)
        if echo:
            print(message, end='')
            sys.stdout.flush()
            user_input = sys.stdin.readline().rstrip('\n')
            l.debug('User input: %s', user_input)
        else:
            if sys.stdin.isatty():
                user_input = getpass.getpass(message)
            else:
                print('Warning: Cannot control echo output on the terminal (stdin is not a tty). Input may be echoed.', file=sys.stderr)
                user_input = sys.stdin.readline().rstrip('\n')
        if not sys.stdin.isatty():
            print('\n', end='')
        try :
            return input_filter(user_input)
        except InputFilterError as e:
            l.debug('Error encountered in user input:', exc_info=sys.exc_info())
            l.warning(e.args[0] if len(e.args) else "User input error.")

def default_prompt(*args, **kwargs):
    l = logging.getLogger(__name__)
    default = kwargs.pop('default', [])
    input_filter = kwargs.pop('input_filter', lambda x: x)

    while True:
        if default:
            if len(default) == 1:
                message = ''.join([
                    args[0] % tuple(args[1:]),
                    ' [%s]' % (default[0])])
                user_input = prompt(message, **kwargs)
                if not user_input:
                    user_input = default[0]
            else:
                message = ''.join([
                    args[0] % tuple(args[1:]), ':\n',
                    '\n'.join(['%d. %s' % (i + 1, default[i]) for i in range(0, len(default))]),
                    '\nPlease select a number or choose 0 to enter a new value'])
                user_input = default_prompt(message, default=[1], **kwargs)
                try:
                    i = int(user_input) - 1
                except (TypeError, ValueError):
                    i = -1
                if i in range(0, len(default)):
                    user_input = default[i]
                else:
                    user_input = prompt('New value', **kwargs)
        else:
            user_input = prompt(*args, **kwargs)
        try :
            return input_filter(user_input)
        except InputFilterError as e:
            l.debug('Error encountered in user input:', exc_info=sys.exc_info())
            l.warning(e.args[0] if len(e.args) else "User input error.")

def int_filter(field='Input'):
    def f(x):
        try:
            return int(x)
        except ValueError as e:
            irods.six.reraise(InputFilterError, InputFilterError('%s must be an integer.' % (field)), sys.exc_info()[2])
    return f

def set_filter(set_, field='Input'):
    def f(x):
        if x in set_:
            return x
        raise InputFilterError('%s must be chosen from %s' % (field, list(set_)))
    return f

def character_count_filter(minimum=None, maximum=None, field='Input'):
    def f(x):
        if (minimum is None or len(x) >= minimum) and (maximum is None or len(x) <= maximum):
            return x
        if minimum is not None and minimum < 0:
            new_minimum = 0
        else:
            new_minimum = minimum
        if new_minimum is not None and maximum is not None:
            if new_minimum == maximum:
                raise InputFilterError('%s must be exactly %s character%s in length.' % (field, maximum, '' if maximum == 1 else 's'))
            if new_minimum < maximum:
                raise InputFilterError('%s must be between %s and %s characters in length.' % (field, new_minimum, maximum))
            raise IrodsError('Minimum character count %s must not be greater than maximum character count %s.' % (new_minimum, maximum))
        if new_minimum is not None:
            raise InputFilterError('%s must be at least %s character%s in length.' % (field, new_minimum, '' if new_minimum == 1 else 's'))
        raise InputFilterError('%s may be at most %s character%s in length.' % (field, maximum, '' if maximum == 1 else 's'))
    return f

class InputFilterError(Exception):
    pass

class callback_on_change_dict(dict):
    def __init__(self, *args, **kwargs):
        if args:
            self.callback = args[0]
            args = args[1:]
        else:
            self.callback = lambda: None
        super(callback_on_change_dict, self).__init__(*args, **kwargs)

    def __setitem__(self, *args, **kwargs):
        super(callback_on_change_dict, self).__setitem__(*args, **kwargs)
        self.callback()

    def __delitem__(self, *args, **kwargs):
        super(callback_on_change_dict, self).__delitem__(*args, **kwargs)
        self.callback()

    def update(self, *args, **kwargs):
        super(callback_on_change_dict, self).update(*args, **kwargs)
        self.callback()

    def clear(self, *args, **kwargs):
        super(callback_on_change_dict, self).clear(*args, **kwargs)
        self.callback()

    def pop(self, *args, **kwargs):
        super(callback_on_change_dict, self).pop(*args, **kwargs)
        self.callback()

    def popitem(self, *args, **kwargs):
        super(callback_on_change_dict, self).popitem(*args, **kwargs)
        self.callback()

    def setdefault(self, *args, **kwargs):
        super(callback_on_change_dict, self).setdefault(*args, **kwargs)
        self.callback()

def delayAssert(a, interval=1, maxrep=100):
    for _ in range(maxrep):
        time.sleep(interval)  # wait for test to fire
        if a():
            break
    if not a():
        raise AssertionError

def log_message_occurrences_equals_count(msg, count=1, server_log_path=None, start_index=0):
    if server_log_path is None:
        server_log_path=paths.server_log_path()
    return count == count_occurrences_of_string_in_log(server_log_path, msg, start_index)

def log_message_occurrences_greater_than_count(msg, count=1, server_log_path=None, start_index=0):
    if server_log_path is None:
        server_log_path=paths.server_log_path()
    return count_occurrences_of_string_in_log(server_log_path, msg, start_index) > count

def log_message_occurrences_fewer_than_count(msg, count=1, server_log_path=None, start_index=0):
    if server_log_path is None:
        server_log_path=paths.server_log_path()
    return count_occurrences_of_string_in_log(server_log_path, msg, start_index) < count

def log_message_occurrences_is_one_of_list_of_counts(msg, expected_value_list=None, server_log_path=None, start_index=0):
    if server_log_path is None:
        server_log_path=paths.server_log_path()
    return count_occurrences_of_string_in_log(server_log_path, msg, start_index) in expected_value_list

def metadata_attr_with_value_exists(session, attr, value):
    print('looking for attr:[{0}] value:[{1}]'.format(attr, value))
    out, _, _ = session.run_icommand(['iquest', '%s',
        '"select META_DATA_ATTR_VALUE where META_DATA_ATTR_NAME = \'{}\'"'.format(attr)])
    print(out)
    return value in out

def create_ufs_resource(resource_name, user, hostname=None):
    vault_name = resource_name + '_vault'
    vault_directory = os.path.join(user.local_session_dir, vault_name)

    if not hostname:
        hostname = socket.gethostname()

    vault = hostname + ':' + vault_directory
    user.assert_icommand(['iadmin', 'mkresc', resource_name, 'unixfilesystem', vault], 'STDOUT', [resource_name])

def create_replication_resource(resource_name, user):
    user.assert_icommand(['iadmin', 'mkresc', resource_name, 'replication'], 'STDOUT', [resource_name])

def create_passthru_resource(resource_name, user):
    user.assert_icommand(['iadmin', 'mkresc', resource_name, 'passthru'], 'STDOUT', [resource_name])

def remove_resource(resource_name, user):
    user.assert_icommand(['iadmin', 'rmresc', resource_name])

def add_child_resource(parent_resource_name, child_resource_name, user):
    user.assert_icommand(['iadmin', 'addchildtoresc', parent_resource_name, child_resource_name])

def remove_child_resource(parent_resource_name, child_resource_name, user):
    user.assert_icommand(['iadmin', 'rmchildfromresc', parent_resource_name, child_resource_name])

def get_user_type(session, user_name, zone_name=None):
    """Get the type field of the user indicated by user_name and zone_name."""
    gql = "select USER_TYPE where USER_NAME = '{}' and USER_ZONE = '{}'"
    return session.assert_icommand(['iquest', '%s',
        gql.format(user_name, zone_name or session.zone_name)], 'STDOUT')[1].strip()


def get_user_zone(session, user_name, zone_name=None):
    """Get the zone field of the user indicated by user_name and zone_name."""
    gql = "select USER_ZONE where USER_NAME = '{}' and USER_ZONE = '{}'"
    return session.assert_icommand(['iquest', '%s',
        gql.format(user_name, zone_name or session.zone_name)], 'STDOUT')[1].strip()


def get_user_comment(session, user_name, zone_name=None):
    """Get the comment field of the user indicated by user_name and zone_name."""
    gql = "select USER_COMMENT where USER_NAME = '{}' and USER_ZONE = '{}'"
    return session.assert_icommand(['iquest', '%s',
        gql.format(user_name, zone_name or session.zone_name)], 'STDOUT')[1].strip()


def get_user_info(session, user_name, zone_name=None):
    """Get the info field of the user indicated by user_name and zone_name."""
    gql = "select USER_INFO where USER_NAME = '{}' and USER_ZONE = '{}'"
    return session.assert_icommand(['iquest', '%s',
        gql.format(user_name, zone_name or session.zone_name)], 'STDOUT')[1].strip()
