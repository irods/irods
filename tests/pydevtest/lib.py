from __future__ import print_function

import commands
import contextlib
import datetime
import hashlib
import itertools
import json
import mmap
import os
import platform
import psutil
import re
import shlex
import shutil
import socket
import subprocess
import sys
import tempfile
import time

import configuration


def md5_hex_file(filename):
    block_size = pow(2, 20)
    md5 = hashlib.md5()
    with open(filename, 'rb') as f:
        while True:
            data = f.read(block_size)
            if not data:
                break
            md5.update(data)
        return md5.hexdigest()

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

def update_json_file_from_dict(filename, update_dict):
    with open(filename) as f:
        env = json.load(f)
    env.update(update_dict)
    with open(filename, 'w') as f:
        json.dump(env, f, indent=4)

def get_hostname():
    return socket.gethostname()

def get_irods_top_level_dir():
    return os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

def get_irods_config_dir():
    if os.path.isfile('/etc/irods/server_config.json'):
        return '/etc/irods'
    return os.path.join(get_irods_top_level_dir(), 'iRODS/server/config')

def get_core_re_dir():
    top_lvl = get_irods_top_level_dir()
    bin_flg = os.path.join(top_lvl, 'packaging/binary_installation.flag')
    if os.path.isfile(bin_flg):
        return '/etc/irods'
    return os.path.join(get_irods_config_dir(), 'reConfigs')

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
    with open(fname, 'a'):
        os.utime(fname, times)

def cat(fname, string, times=None):
    with open(fname, 'a') as f:
        f.write(string)

def make_file(f_name, f_size, contents='zero'):
    assert contents in ['arbitrary', 'random', 'zero']
    if contents == 'arbitrary' or f_size == 0:
        subprocess.check_call(['truncate', '-s', str(f_size), f_name])
        return

    source = {'zero': '/dev/zero',
              'random': '/dev/urandom'}[contents]

    subprocess.check_call(['dd', 'if='+source, 'of='+f_name, 'count=1', 'bs='+str(f_size)])

def ils_output_to_entries(stdout):
    raw = stdout.strip().split('\n')
    collection = raw[0]
    entries = [entry.strip() for entry in raw[1:]]
    return entries

def get_vault_path(session):
    cmdout = session.run_icommand(
        ['iquest', '%s', "select RESC_VAULT_PATH where RESC_NAME = 'demoResc'"])
    if cmdout[2] != '':
        raise OSError(
            cmdout[2], 'iquest wrote to stderr when called from get_vault_path()')
    return cmdout[1].rstrip('\n')

def get_vault_session_path(session):
    return os.path.join(get_vault_path(session),
                        "home",
                        session.username,
                        session._session_id)

def make_large_local_tmp_dir(dir_name, file_count, file_size):
    os.makedirs(dir_name)
    for i in range(file_count):
        make_file(os.path.join(dir_name, "junk" + str(i).zfill(4)),
                  file_size)
    local_files = os.listdir(dir_name)
    assert len(local_files) == file_count, "dd loop did not make all " + \
        str(file_count) + " files"
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

def files_in_ils_output(ils_out):
    for item in ils_out:
        # strip collections
        if not item.startswith('C- /'):
            yield item

def files_in_dir(path):
    for file in os.listdir(path):
        if os.path.isfile(os.path.join(path, file)):
            yield file

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
    with open(filename, 'w') as f:
        f.write(string)
        f.write(contents)

def get_log_path(log_source):
    log_prefix_dict = {
        'server': 'rodsLog',
        're': 'reLog',
    }
    assert log_source in log_prefix_dict, log_source

    log_prefix = log_prefix_dict[log_source]
    server_log_dir = os.path.join(
        get_irods_top_level_dir(), 'iRODS/server/log')
    command_str = 'ls -t {0}/{1}* | head -n1'.format(
        server_log_dir, log_prefix)
    proc = subprocess.Popen(command_str, stdout=subprocess.PIPE, shell=True)
    stdout, stderr = proc.communicate()
    if proc.returncode != 0 or stdout == '':
        raise subprocess.CalledProcessError(
            proc.returncode, command_str, 'stdout [{0}] stderr[{1}]'.format(stdout, stderr))
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
            i = m.find(string, i + 1)
        m.close()
        return n

def run_command(command_arg, check_rc=False, stdin_string='', use_unsafe_shell=False, env=None, cwd=None):
    if not use_unsafe_shell and isinstance(command_arg, basestring):
        command_arg = shlex.split(command_arg)
    p = subprocess.Popen(
        command_arg, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, env=env, shell=use_unsafe_shell, cwd=cwd)
    stdout, stderr = p.communicate(input=stdin_string)
    rc = p.returncode
    if check_rc:
        if rc != 0:
            raise subprocess.CalledProcessError(
                rc, command_arg, stdout + '\n\n' + stderr)
    return rc, stdout, stderr

def check_run_command_output(command_arg, stdout, stderr, check_type='EMPTY', expected_results='', use_regex=False):
    assert check_type in ['EMPTY', 'STDOUT', 'STDERR', 'STDOUT_SINGLELINE',
                          'STDERR_SINGLELINE', 'STDOUT_MULTILINE', 'STDERR_MULTILINE'], check_type

    if isinstance(expected_results, basestring):
        expected_results = [expected_results]

    regex_msg = 'regex ' if use_regex else ''

    print('Expecting {0}: {1}{2}'.format(
        check_type, regex_msg, expected_results))
    print('  stdout:')
    print('    | ' + '\n    | '.join(stdout.splitlines()))
    print('  stderr:')
    print('    | ' + '\n    | '.join(stderr.splitlines()))

    if check_type not in ['STDERR', 'STDERR_SINGLELINE', 'STDERR_MULTILINE'] and stderr != '':
        print('Unexpected output on stderr\n')
        return False

    if check_type in ['STDOUT', 'STDERR']:
        output = stdout if check_type == 'STDOUT' else stderr
        for er in expected_results:
            regex_pattern = er if use_regex else re.escape(er)
            if not re.search(regex_pattern, output):
                print('Output not found\n')
                return False
        print('Output found\n')
        return True
    elif check_type in ['STDOUT_SINGLELINE', 'STDERR_SINGLELINE', 'STDOUT_MULTILINE', 'STDERR_MULTILINE']:
        lines = stdout.splitlines() if check_type in [
            'STDOUT_SINGLELINE', 'STDOUT_MULTILINE'] else stderr.splitlines()

        if check_type in ['STDOUT_MULTILINE', 'STDERR_MULTILINE']:
            for er in expected_results:
                regex_pattern = er if use_regex else re.escape(er)
                for line in lines:
                    if re.search(regex_pattern, line.rstrip('\n')):
                        break
                else:
                    print(
                        '    --> stopping search - expected result not found')
                    break
            else:
                print('Output found\n')
                return True
            print('Output not found\n')
            return False
        else:
            for line in lines:
                found_count = 0
                for er in expected_results:
                    regex_pattern = er if use_regex else re.escape(er)
                    if re.search(regex_pattern, line.rstrip('\n')):
                        found_count += 1
                if found_count == len(expected_results):
                    print('Output found\n')
                    return True
            print('Output not found\n')
            return False
    elif check_type == 'EMPTY':
        if stdout == '':
            print('Output found\n')
            return True
        print('Unexpected output on stdout\n')
        return False
    assert False, check_type

def extract_function_kwargs(func, kwargs):
    args = func.func_code.co_varnames[:func.func_code.co_argcount]
    args_dict = {}
    for k, v in kwargs.items():
        if k in args:
            args_dict[k] = v
    return args_dict

def assert_command(*args, **kwargs):
    return _assert_helper(*args, should_fail=False, **kwargs)

def assert_command_fail(*args, **kwargs):
    return _assert_helper(*args, should_fail=True, **kwargs)

def _assert_helper(command_arg, check_type='EMPTY', expected_results='', should_fail=False, **kwargs):
    run_command_arg_dict = extract_function_kwargs(run_command, kwargs)
    rc, stdout, stderr = run_command(command_arg, **run_command_arg_dict)

    fail_string = ' FAIL' if should_fail else ''
    if isinstance(command_arg, basestring):
        print('Assert{0} Command: {1}'.format(fail_string, command_arg))
    else:
        print('Assert{0} Command: {1}'.format(
            fail_string, ' '.join(command_arg)))

    check_run_command_output_arg_dict = extract_function_kwargs(
        check_run_command_output, kwargs)
    result = should_fail != check_run_command_output(
        command_arg, stdout, stderr, check_type=check_type, expected_results=expected_results, **check_run_command_output_arg_dict)

    desired_rc = kwargs.get('desired_rc', None)
    if desired_rc is not None:
        print(
            'Checking return code: actual [{0}] desired [{1}]'.format(rc, desired_rc))
        if desired_rc != rc:
            print('RETURN CODE CHECK FAILED')
            result = False

    if not result:
        print('FAILED TESTING ASSERTION\n\n')
    assert result
    return rc, stdout, stderr

def stop_irods_server():
    assert_command([os.path.join(get_irods_top_level_dir(), 'iRODS/irodsctl'), 'stop'],
                   'STDOUT_SINGLELINE', 'Success')

def start_irods_server(env=None):
    def is_jsonschema_available():
        try:
            import jsonschema
            return True
        except ImportError:
            return False

    if is_jsonschema_available():
        assert_command('{0} graceful_start'.format(os.path.join(get_irods_top_level_dir(), 'iRODS/irodsctl')),
                       'STDOUT_SINGLELINE', 'Success', env=env)
    else:
        assert_command('{0} graceful_start'.format(os.path.join(get_irods_top_level_dir(), 'iRODS/irodsctl')),
                       'STDERR_SINGLELINE', 'jsonschema not installed', desired_rc=0, env=env)
    with make_session_for_existing_admin() as admin_session:
        admin_session.assert_icommand('ils', 'STDOUT_SINGLELINE', admin_session.zone_name)

def restart_irods_server(env=None):
    stop_irods_server()
    start_irods_server(env=env)

def make_environment_dict(username, hostname, zone_name):
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
        'irods_transfer_buffer_size_for_parallel_transfer_in_megabytes': 4

    }
    if configuration.USE_SSL:
        environment.update({
            'irods_client_server_policy': 'CS_NEG_REQUIRE',
            'irods_ssl_verify_server': 'cert',
            'irods_ssl_ca_certificate_file': '/etc/irods/server.crt',
        })
    return environment

def json_object_hook_ascii_list(l):
    rv = []
    for i in l:
        if isinstance(i, unicode):
            i = i.encode('ascii')
        elif isinstance(i, list):
            i = json_object_hook_ascii_list(i)
        rv.append(i)
    return rv

def json_object_hook_ascii_dict(d):
    rv = {}
    for k, v in d.items():
        if isinstance(k, unicode):
            k = k.encode('ascii')
        if isinstance(v, unicode):
            v = v.encode('ascii')
        elif isinstance(v, list):
            v = json_object_hook_ascii_list(v)
        rv[k] = v
    return rv

def open_and_load_json_ascii(filename):
    with open(filename) as f:
        return json.load(f, object_hook=json_object_hook_ascii_dict)

def get_service_account_environment_file_contents():
    return open_and_load_json_ascii(os.path.expanduser('~/.irods/irods_environment.json'))

def make_session_for_existing_user(username, password, hostname, zone):
    env_dict = make_environment_dict(username, hostname, zone)
    return IrodsSession(env_dict, password, False)

def make_session_for_existing_admin():
    service_env = get_service_account_environment_file_contents()
    username = service_env['irods_user_name']
    zone_name = service_env['irods_zone_name']
    env_dict = make_environment_dict(
        username, configuration.ICAT_HOSTNAME, zone_name)
    return IrodsSession(env_dict, configuration.PREEXISTING_ADMIN_PASSWORD, False)

def mkuser_and_return_session(user_type, username, password, hostname):
    service_env = get_service_account_environment_file_contents()
    zone_name = service_env['irods_zone_name']
    with make_session_for_existing_admin() as admin_session:
        admin_session.assert_icommand(
            ['iadmin', 'mkuser', username, user_type])
        if password is not None:
            admin_session.assert_icommand(
                ['iadmin', 'moduser', username, 'password', password])
        manage_data = password != None
        env_dict = make_environment_dict(username, hostname, zone_name)
        return IrodsSession(env_dict, password, manage_data)

def mkgroup_and_add_users(group_name, usernames):
    with make_session_for_existing_admin() as admin_session:
        admin_session.assert_icommand(['iadmin', 'mkgroup', group_name])
        for username in usernames:
            admin_session.assert_icommand(
                ['iadmin', 'atg', group_name, username])

def get_os_distribution():
    return platform.linux_distribution()[0]

def get_os_distribution_version_major():
    return platform.linux_distribution()[1].split('.')[0]

def make_sessions_mixin(rodsadmin_name_password_list, rodsuser_name_password_list):
    class SessionsMixin(object):
        def setUp(self):
            with make_session_for_existing_admin() as admin_session:
                self.admin_sessions = [mkuser_and_return_session('rodsadmin', name, password, get_hostname())
                                       for name, password in rodsadmin_name_password_list]
                self.user_sessions = [mkuser_and_return_session('rodsuser', name, password, get_hostname())
                                      for name, password in rodsuser_name_password_list]
            super(SessionsMixin, self).setUp()

        def tearDown(self):
            with make_session_for_existing_admin() as admin_session:
                for session in itertools.chain(self.admin_sessions, self.user_sessions):
                    session.__exit__()
                    admin_session.assert_icommand(
                        ['iadmin', 'rmuser', session.username])
            super(SessionsMixin, self).tearDown()
    return SessionsMixin

class IrodsSession(object):
    def __init__(self, environment_file_contents, password, manage_irods_data):
        self._environment_file_contents = environment_file_contents
        self._password = password
        self._manage_irods_data = manage_irods_data

        self._environment_file_invalid = True
        self._local_session_dir = tempfile.mkdtemp(prefix='irods-testing-')
        self._environment_file_path = os.path.join(
            self._local_session_dir, 'irods_environment.json')
        self._authentication_file_path = os.path.join(
            self._local_session_dir, 'irods_authentication')
        self._session_id = datetime.datetime.utcnow().strftime(
            '%Y-%m-%dZ%H:%M:%S--') + os.path.basename(self._local_session_dir)

        if self._password is not None:
            self.assert_icommand(['iinit', self._password])
        if self._manage_irods_data:
            self.assert_icommand(['imkdir', self.session_collection])
            self.assert_icommand(['icd', self.session_collection])

    @property
    def environment_file_contents(self):
        self._environment_file_invalid = True
        return self._environment_file_contents

    @environment_file_contents.setter
    def environment_file_contents(self, value):
        self._environment_file_invalid = True
        self._environment_file_contents = value

    @property
    def zone_name(self):
        return self._environment_file_contents['irods_zone_name']

    @property
    def username(self):
        return self._environment_file_contents['irods_user_name']

    @property
    def password(self):
        return self._password

    @property
    def default_resource(self):
        return self._environment_file_contents['irods_default_resource']

    @property
    def local_session_dir(self):
        return self._local_session_dir

    @property
    def home_collection(self):
        return os.path.join('/', self.zone_name, 'home', self.username)

    @property
    def session_collection(self):
        return os.path.join(self.home_collection, self._session_id)

    @property
    def session_collection_trash(self):
        return self.session_collection.replace('/home/', '/trash/home/', 1)

    def run_icommand(self, *args, **kwargs):
        self._prepare_run_icommand(args[0], kwargs)
        return run_command(*args, **kwargs)

    def assert_icommand(self, *args, **kwargs):
        self._prepare_run_icommand(args[0], kwargs)
        return assert_command(*args, **kwargs)

    def assert_icommand_fail(self, *args, **kwargs):
        self._prepare_run_icommand(args[0], kwargs)
        return assert_command_fail(*args, **kwargs)

    def _prepare_run_icommand(self, arg, kwargs):
        self._log_run_icommand(arg)
        self._write_environment_file()
        if 'env' not in kwargs:
            kwargs['env'] = os.environ.copy()
        kwargs['env']['IRODS_ENVIRONMENT_FILE'] = self._environment_file_path
        kwargs['env']['IRODS_AUTHENTICATION_FILE'] = self._authentication_file_path

    def _log_run_icommand(self, arg):
        valid_icommands = ['iadmin', 'iapitest', 'ibun', 'icd',
                            'ichksum', 'ichmod', 'icp', 'ienv',
                            'ierror', 'iexecmd', 'iexit', 'ifsck',
                            'iget', 'ihelp', 'iinit', 'ils',
                            'ilsresc', 'imcoll', 'imeta',
                            'imiscsvrinfo', 'imkdir', 'imv',
                            'iphybun', 'iphymv', 'ips', 'iput',
                            'ipwd', 'iqdel', 'iqstat', 'iquest',
                            'ireg', 'irepl', 'irm', 'irmtrash',
                            'irodsFs', 'irods-grid', 'irsync',
                            'irule', 'iscan', 'isysmeta', 'iticket',
                            'itrim', 'iuserinfo', 'ixmsg',
                            'izonereport' ]

        if isinstance(arg, basestring):
            icommand = shlex.split(arg)[0]
            log_string = arg
        else:
            icommand = arg[0]
            log_string = ' '.join(arg)
        assert icommand in valid_icommands, icommand
        message = ' --- IrodsSession: icommand executed by [{0}] [{1}] --- \n'.format(
            self.username, log_string)
        write_to_log('server', message)
        print(message, end='')

    def _write_environment_file(self):
        if self._environment_file_invalid:
            with open(self._environment_file_path, 'w') as f:
                json.dump(self._environment_file_contents, f)
            self._environment_file_invalid = False

    def __enter__(self):
        return self

    def __exit__(self, exc_type=None, exc_value=None, traceback=None):
        if self._manage_irods_data:
            self.assert_icommand('icd')
            self.assert_icommand(['irm', '-rf', self.session_collection])
            self.assert_icommand('irmtrash')
        shutil.rmtree(self._local_session_dir)

    def interrupt_icommand(self, fullcmd, filename, filesize):
        ''' Runs an icommand, but does not let it complete.

        This function terminates the icommand once filename reaches (>=)
        filesize in bytes.

        Asserts that the icommand was successfully terminated early.
        '''
        filename = os.path.abspath(filename)
        parameters = shlex.split(fullcmd)
        print("\n")
        print("INTERRUPTING iCMD")
        print("running icommand: " + self.username + "[" + fullcmd + "]")
        print("  filename set to: [" + filename + "]")
        print("  filesize set to: [" + str(filesize) + "] bytes")

        write_to_log(
            'server', ' --- interrupt icommand [{0}] --- \n'.format(fullcmd))

        env = os.environ.copy()
        env['IRODS_ENVIRONMENT_FILE'] = self._environment_file_path
        env['IRODS_AUTHENTICATION_FILE'] = self._authentication_file_path
        self._write_environment_file()

        timeout = 30
        begin = time.time()
        granularity = 0.005

        p = subprocess.Popen(
            parameters, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)

        while time.time() - begin < timeout and (not os.path.exists(filename) or os.stat(filename).st_size < filesize):
            time.sleep(granularity)
        if (time.time() - begin) >= timeout:
            print(run_command(['ls', '-l', os.path.dirname(filename)])[1])
            out, err = p.communicate()
            print(out, err)
            print(self.run_icommand(['ils', '-l'])[1])
            assert False
        elif p.poll() is None:
            p.terminate()
        else:
            assert False

        return 0
