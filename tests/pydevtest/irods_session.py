import datetime
import json
import os
import psutil
import shlex
import shutil
import subprocess
import sys
import tempfile
import time

import lib


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
    def session_collection(self):
        return os.path.join('/', self.zone_name, 'home', self.username, self._session_id)

    @property
    def home_collection(self):
        return os.path.join('/', self.zone_name, 'home', self.username)

    def run_icommand(self, *args, **kwargs):
        self._prepare_run_icommand(args[0], kwargs)
        return lib.run_command(*args, **kwargs)

    def assert_icommand(self, *args, **kwargs):
        self._prepare_run_icommand(args[0], kwargs)
        lib.assert_command(*args, **kwargs)

    def assert_icommand_fail(self, *args, **kwargs):
        self._prepare_run_icommand(args[0], kwargs)
        lib.assert_command_fail(*args, **kwargs)

    def _prepare_run_icommand(self, arg, kwargs):
        self._log_run_icommand(arg)
        self._write_environment_file()
        if 'env' not in kwargs:
            environment = os.environ.copy()
            environment['IRODS_ENVIRONMENT_FILE'] = self._environment_file_path
            environment['IRODS_AUTHENTICATION_FILE'] = self._authentication_file_path
            kwargs['env'] = environment

    def _log_run_icommand(self, arg):
        valid_icommands = ['iinit', 'ienv', 'ihelp', 'ils', 'icd',
                           'imkdir', 'ichmod', 'imeta', 'iget', 'iput',
                           'imv', 'icp', 'irepl', 'iquest', 'irm',
                           'irmtrash', 'iexit', 'ilsresc', 'imiscsvrinfo',
                           'iuserinfo', 'ipwd', 'ierror', 'iexecmd', 'ips',
                           'iqstat', 'ichksum', 'itrim', 'iphymv', 'ibun',
                           'iphybun', 'ireg', 'imcoll', 'irsync', 'ixmsg',
                           'irule', 'iqdel', 'iticket', 'iapitest', 'iscan',
                           'isysmeta', 'iadmin',]

        if isinstance(arg, basestring):
            icommand = shlex.split(arg)[0]
            log_string = arg
        else:
            icommand = arg[0]
            log_string = ' '.join(arg)
        assert icommand in valid_icommands, icommand
        lib.write_to_log('server', ' --- run_icommand by [{0}] [{1}] --- \n'.format(self.username, log_string))

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
        self.assert_icommand('iexit full')
        shutil.rmtree(self._local_session_dir)

    def interrupt_icommand(self, fullcmd, filename, filesize):
        ''' Runs an icommand, but does not let it complete.

        This function terminates the icommand once filename reaches (>=)
        filesize in bytes.

        Asserts that the icommand was successfully terminated early.
        '''
        filename = os.path.abspath(filename)
        parameters = shlex.split(fullcmd)
        print "\n"
        print "INTERRUPTING iCMD"
        print "running icommand: " + self.username + "[" + fullcmd + "]"
        print "  filename set to: [" + filename + "]"
        print "  filesize set to: [" + str(filesize) + "] bytes"

        lib.write_to_log('server', ' --- interrupt icommand [{0}] --- \n'.format(fullcmd))

        env = os.environ.copy()
        env['IRODS_ENVIRONMENT_FILE'] = self._environment_file_path
        env['IRODS_AUTHENTICATION_FILE'] = self._authentication_file_path
        self._write_environment_file()

        timeout = 30
        begin = time.time()
        granularity = 0.005

        p = subprocess.Popen(parameters, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)

        while time.time() - begin < timeout and (not os.path.exists(filename) or os.stat(filename).st_size < filesize):
            time.sleep(granularity)
        if (time.time() - begin) >= timeout:
            print lib.run_command(['ls', '-l', os.path.dirname(filename)])[1]
            out, err = p.communicate()
            print out, err
            print self.run_icommand(['ils', '-l'])[1]
            assert False
        elif p.poll() is None:
            p.terminate()
        else:
            assert False

        return 0
