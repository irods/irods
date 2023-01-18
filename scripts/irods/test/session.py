from __future__ import print_function

import contextlib
import datetime
import errno
import hashlib
import itertools
import json
import os
import shutil
import tempfile
import time

from .. import test
from .. import lib
from .. import paths
from . import settings
from ..configuration import IrodsConfig
from .command import assert_command, assert_command_fail

from .. import six

def make_session_for_existing_user(username, password, hostname, zone):
    env_dict = lib.make_environment_dict(username, hostname, zone, use_ssl=test.settings.USE_SSL)
    return IrodsSession(env_dict, password, False)

def make_session_for_existing_admin():
    irods_config = IrodsConfig()
    if irods_config.version_tuple < (4, 1, 0):
        client_environment = open_and_load_pre410_env_file(os.path.join(irods_config.home_directory, '.irods', '.irodsEnv'))
    else:
        client_environment = irods_config.client_environment
    username = client_environment['irods_user_name']
    zone_name = client_environment['irods_zone_name']
    env_dict = lib.make_environment_dict(
        username, test.settings.ICAT_HOSTNAME, zone_name, use_ssl=test.settings.USE_SSL)
    return IrodsSession(env_dict, test.settings.PREEXISTING_ADMIN_PASSWORD, False)

def mkuser_and_return_session(user_type, username, password, hostname):
    irods_config = IrodsConfig()
    if irods_config.version_tuple < (4, 1, 0):
        client_environment = open_and_load_pre410_env_file(os.path.join(irods_config.home_directory, '.irods', '.irodsEnv'))
    else:
        client_environment = irods_config.client_environment
    zone_name = client_environment['irods_zone_name']
    with make_session_for_existing_admin() as admin_session:
        admin_session.assert_icommand(
            ['iadmin', 'mkuser', username, user_type])
        if password is not None:
            admin_session.assert_icommand(
                ['iadmin', 'moduser', username, 'password', password])
        manage_data = password != None
        env_dict = lib.make_environment_dict(username, hostname, zone_name, use_ssl=test.settings.USE_SSL)
        return IrodsSession(env_dict, password, manage_data)

def mkgroup_and_add_users(group_name, usernames):
    with make_session_for_existing_admin() as admin_session:
        admin_session.assert_icommand(['iadmin', 'mkgroup', group_name])
        for username in usernames:
            admin_session.assert_icommand(
                ['iadmin', 'atg', group_name, username])

def get_data_id(session, collection_name, data_name):
    rc, out, err = session.assert_icommand(['iquest', "select DATA_ID where COLL_NAME = '{0}' and DATA_NAME = '{1}'".format(collection_name, data_name)], 'STDOUT_SINGLELINE', 'DATA_ID')
    assert rc == 0, rc
    assert err == '', err
    lines = out.split()
    assert len(lines) == 4, lines # make sure genquery only returned one result
    return int(lines[2])

def make_sessions_mixin(rodsadmin_name_password_list, rodsuser_name_password_list):
    class SessionsMixin(object):
        def setUp(self):
            with make_session_for_existing_admin() as admin_session:
                self.admin_sessions = [mkuser_and_return_session('rodsadmin', name, password, lib.get_hostname())
                                       for name, password in rodsadmin_name_password_list]
                self.user_sessions = [mkuser_and_return_session('rodsuser', name, password, lib.get_hostname())
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
        self._environment_file_path = os.path.join(self._local_session_dir, 'irods_environment.json')
        self._authentication_file_path = os.path.join(self._local_session_dir, 'irods_authentication')
        self._session_id = datetime.datetime.utcnow().strftime('%Y-%m-%dZ%H:%M:%S--') + os.path.basename(self._local_session_dir)

        if self._password is not None:
            self.assert_icommand('iinit', 'STDOUT_SINGLELINE',
                                 input=f'{self._password}\n')
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
    def qualified_username(self):
        return self.username + '#' + self._environment_file_contents.get('irods_zone_name', '<undefined>')

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

    def remote_home_collection(self, remote_zone_name):
        return '/{0}/home/{1}#{2}'.format(remote_zone_name, self.username, self.zone_name)

    def run_icommand(self, *args, **kwargs):
        self._prepare_run_icommand(args[0], kwargs)
        return lib.execute_command_permissive(*args, **kwargs)

    def assert_icommand(self, *args, **kwargs):
        self._prepare_run_icommand(args[0], kwargs)
        return assert_command(*args, **kwargs)

    def assert_icommand_fail(self, *args, **kwargs):
        self._prepare_run_icommand(args[0], kwargs)
        return assert_command_fail(*args, **kwargs)

    def assert_irule(self, rule_contents, *args, **kwargs):
        with contextlib.closing(tempfile.NamedTemporaryFile(mode='wt', suffix='.r', dir=self.local_session_dir)) as f:
            print(rule_contents, end='', file=f)
            f.flush()
            self.assert_icommand("irule -F %s" % (f.name), *args, **kwargs)

    def _prepare_run_icommand(self, arg, kwargs):
        self._log_run_icommand(arg)
        self._write_environment_file()
        if 'env' not in kwargs:
            kwargs['env'] = os.environ.copy()
        if IrodsConfig().version_tuple < (4, 1, 0):
            kwargs['env']['irodsEnvFile'] = self._environment_file_path
            kwargs['env']['irodsAuthFileName'] = self._authentication_file_path
        else:
            kwargs['env']['IRODS_ENVIRONMENT_FILE'] = self._environment_file_path
            kwargs['env']['IRODS_AUTHENTICATION_FILE'] = self._authentication_file_path

    def _log_run_icommand(self, arg):
        if isinstance(arg, six.string_types):
            icommand = lib.safe_shlex_split_for_2_6(arg)[0]
            log_string = arg
        else:
            icommand = arg[0]
            log_string = ' '.join(arg)

        message = ' --- IrodsSession: icommand executed by [{0}] [{1}] --- \n'.format(
            self.qualified_username, log_string)
        if IrodsConfig().version_tuple < (4, 2, 0):
            server_log_dir = os.path.join(paths.irods_directory(), 'iRODS', 'server', 'log')
            server_log_path = sorted([os.path.join(server_log_dir, name)
                    for name in os.listdir(server_log_dir)
                    if name.startswith('rodsLog')],
                key=lambda path: os.path.getctime(path))[-1]
        else:
            server_log_path = paths.server_log_path()
        lib.write_to_log(server_log_path, message)
        print(message, end='')

    def _write_environment_file(self):
        if self._environment_file_invalid:
            with open(self._environment_file_path, 'w') as f:
                if IrodsConfig().version_tuple < (4, 1, 0):
                    for key, value in self._environment_file_contents.items():
                        if key in json_env_map:
                            env_line = '{setting} {value}\n'.format(setting=json_env_map[key], value=value)
                            f.write(env_line)
                else:
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
        parameters = lib.safe_shlex_split_for_2_6(fullcmd)
        print("\n")
        print("INTERRUPTING iCMD")
        print("running icommand: " + self.username + "[" + fullcmd + "]")
        print("  filename set to: [" + filename + "]")
        print("  filesize set to: [" + str(filesize) + "] bytes")

        lib.write_to_log(
            IrodsConfig().server_log_path, ' --- interrupt icommand [{0}] --- \n'.format(fullcmd))

        env = os.environ.copy()
        env['IRODS_ENVIRONMENT_FILE'] = self._environment_file_path
        env['IRODS_AUTHENTICATION_FILE'] = self._authentication_file_path
        self._write_environment_file()

        timeout = 30
        begin = time.time()
        granularity = 0.005

        p = lib.execute_command_nonblocking(parameters, env=env)

        while time.time() - begin < timeout and (not os.path.exists(filename) or os.stat(filename).st_size < filesize):
            time.sleep(granularity)
        if (time.time() - begin) >= timeout:
            print(execute_command(['ls', '-l', os.path.dirname(filename)])[1])
            out, err = p.communicate()
            print(out, err)
            print(self.run_icommand(['ils', '-l'])[0])
            assert False
        elif p.poll() is None:
            p.terminate()
        else:
            assert False

        return 0

    def get_entries_in_collection(self, collection):
        out, _, _ = self.run_icommand(['ils', collection])
        raw = out.strip().split('\n')
        collection = raw[0]
        entries = [entry.strip() for entry in raw[1:]]
        return entries

    def get_vault_path(self, resource='demoResc'):
        out, err, rc = self.run_icommand(
            ['iquest', '%s', "select RESC_VAULT_PATH where RESC_NAME = '{0}'".format(resource)])
        if err != '':
            raise OSError(
                err, 'iquest wrote to stderr when called from get_vault_path()')
        return out.rstrip('\n')

    def get_vault_session_path(self, resource='demoResc'):
        return os.path.join(self.get_vault_path(resource),
                            "home",
                            self.username,
                            self._session_id)

# Two-way mapping of the new (json) and old iRODS environment setting names
json_env_map = {'irods_host': 'irodsHost',
        'irods_port': 'irodsPort',
        'irods_default_resource': 'irodsDefResource',
        'irods_home': 'irodsHome',
        'irods_cwd': 'irodsCwd',
        'irods_user_name': 'irodsUserName',
        'irods_zone_name': 'irodsZone',
        'irods_client_server_negotiation': 'irodsClientServerNegotiation',
        'irods_client_server_policy': 'irodsClientServerPolicy',
        'irods_encryption_salt_size': 'irodsEncryptionSaltSize',
        'irods_encryption_num_hash_rounds': 'irodsEncryptionNumHashRounds',
        'irods_encryption_algorithm': 'irodsEncryptionAlgorithm',
        'irods_default_hash_scheme': 'irodsDefaultHashScheme',
        'irods_match_hash_policy': 'irodsMatchHashPolicy'}
json_env_map.update(dict([(val, key) for key, val in json_env_map.items()]))

def open_and_load_pre410_env_file(filename):

    #A very brittle parsing takes place here:
    #Each line of .irodsEnv is split into tokens.
    #If the first token matches a key in our old-new setting map
    #we use the corresponding json setting, and the second token as value

    irods_env = {}
    with open(filename) as env_file:
        for line in env_file.readlines():
            tokens = line.strip().split()
            if len(tokens) > 1 and tokens[0] in json_env_map:
                irods_env[json_env_map[tokens[0]]] = tokens[1].strip("'")

    return irods_env
