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

import pydevtest_common


class RodsSession(object):
    def __init__(self, icommands_binaries_dir, environment_file_contents, password):
        self._icommands_binaries_dir = icommands_binaries_dir
        self._environment_file_contents = environment_file_contents
        self._password = password

        self._environment_file_invalid = True
        self._local_session_dir = tempfile.mkdtemp(prefix='irods-testing-')
        self._environment_file_path = os.path.join(self._local_session_dir, 'irods_environment.json')
        self._authentication_file_path = os.path.join(self._local_session_dir, 'irods_authentication')

        self._session_id = datetime.datetime.utcnow().strftime('%Y-%m-%dZ%H:%M:%S--') + os.path.basename(self._local_session_dir)

        self._write_environment_file()

    @property
    def environment_file_contents(self):
        self._environment_file_invalid = True
        return self._environment_file_contents

    @environment_file_contents.setter
    def environment_file_contents(self, value):
        self._environment_file_invalid = True
        self._environment_file_contents = value

    def delete_session_dir(self):
        '''To be called after self.runCmd('iexit').'''
        shutil.rmtree(self._local_session_dir)

    @property
    def zone_name(self):
        return self._environment_file_contents['irods_zone']

    @property
    def username(self):
        return self._environment_file_contents['irods_user_name']

    @property
    def default_resource(self):
        return self._environment_file_contents['irods_default_resource']

    @property
    def local_session_dir(self):
        return self._local_session_dir

    def run_icommand(self, *args, **kwargs):
        self._prepare_run_icommand(args[0], kwargs)
        return pydevtest_common.run_command(*args, **kwargs)

    def assert_icommand(self, *args, **kwargs):
        self._prepare_run_icommand(args[0], kwargs)
        pydevtest_common.assert_command(*args, **kwargs)

    def _prepare_run_icommand(self, arg, kwargs):
        self._log_run_icommand(arg)
        self._write_environment_file()
        if 'env' not in kwargs:
            environment = os.environ.copy()
            environment['IRODS_ENVIRONMENT_FILE'] = self._environment_file_path
            environment['IRODS_AUTHENTICATION_FILE'] = self._authentication_file_path
            kwargs['env'] = environment

    @staticmethod
    def _log_run_icommand(arg):
        valid_icommands = ['iinit', 'ienv', 'ihelp', 'ils', 'icd',
                           'imkdir', 'ichmod', 'imeta', 'iget', 'iput',
                           'imv', 'icp', 'irepl', 'iquest', 'irm',
                           'irmtrash', 'iexit', 'ilsresc', 'imiscsvrinfo',
                           'iuserinfo', 'ipwd', 'ierror', 'iexecmd', 'ips',
                           'iqstat', 'ichksum', 'itrim', 'iphymv', 'ibun',
                           'iphybun', 'ireg', 'imcoll', 'irsync', 'ixmsg',
                           'irule', 'iqdel', 'iticket', 'iapitest', 'iscan',
                           'isysmeta',]

        if isinstance(arg, list):
            icommand = arg[0]
            log_string = ' '.join(arg)
        else:
            icommand = shlex.split(arg)[0]
            log_string = arg
        assert icommand in valid_icommands, icommand
        pydevtest_common.write_to_log('server', ' --- run_icommand [{0}] --- \n'.format(log_string))

    def _write_environment_file(self):
        if self._environment_file_invalid:
            with open(self._environment_file_path, 'w') as f:
                json.dump(self._environment_file_contents, f)
            self._environment_file_invalid = False

    def interruptCmd(self, icommand, argList, filename, filesize):
        '''Runs an icommand with optional argument list but
        terminates the icommand subprocess once the filename
        reaches filesize bytes or larger.

        Returns  0 if subprocess was terminated.
        Returns -1 if subprocess completed normally.
        Returns -2 if timeout is reached.

        Not currently checking against allowed icommands.
        '''

        env = os.environ.copy()
        env['IRODS_ENVIRONMENT_FILE'] = self._environment_file_path
        env['IRODS_AUTHENTICATION_FILE'] = self._authentication_file_path

        cmdStr = "%s/%s" % (self._icommands_binaries_dir, icommand)
        argList = [cmdStr] + argList

        pydevtest_common.write_to_log('server', ' --- interrupt icommand [{0}] --- \n'.format(' '.join(argList)))

        p = subprocess.Popen(argList, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)

        # set timeout in seconds
        timeout = 60  # 1 minute
        begin = time.time()
        # wait for filename to get big enough to terminate subprocess
        granularity = 0.01
        while time.time() - begin < timeout and not os.path.exists(filename):
            # does not exist yet
            time.sleep(granularity)
        while time.time() - begin < timeout and os.stat(filename).st_size < filesize:
            # not big enough yet
            time.sleep(granularity)
        # if timeout was reached, return -2
        if (time.time() - begin) >= timeout:
            returncode = -2
        # else if subprocess did not complete by filesize threshold, we kill it
        elif p.poll() is None:
            p.terminate()
            # expected, so return 0
            returncode = 0
        # else the process finished before the filesize threshold was met
        else:
            # unexpected, so return -1
            returncode = -1

        return returncode

    def interruptCmdDelay(self, icommand, argList=[], delay=0):
        '''Runs an icommand with optional argument list but
        terminates the icommand subprocess after delay seconds.

        Returns 0  if subprocess was terminated.
        Returns -1 if subprocess completed normally.

        Not currently checking against allowed icommands.
        '''

        env = os.environ.copy()
        env['IRODS_ENVIRONMENT_FILE'] = self._environment_file_path
        env['IRODS_AUTHENTICATION_FILE'] = self._authentication_file_path

        cmdStr = os.path.join(self._icommands_binaries_dir, icommand)
        argList = [cmdStr] + argList

        pydevtest_common.write_to_log('server', ' --- interrupt icommand delay({0}) [{1}] --- \n'.format(delay, ' '.join(argList)))

        p = subprocess.Popen(argList, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)

        # wait for subprocess to complete
        granularity = 0.01
        t = 0.0
        while t < delay and p.poll() is None:
            time.sleep(granularity)
            t += granularity

        # if subprocess did not complete by delay, we kill it
        if p.poll() is None:
            p.terminate()
            # expected, so return 0
            returncode = 0
        # else the process finished before the delay expired
        else:
            # unexpected, so return -1
            returncode = -1

        return returncode

    def runCmd(self, icommand, argList=[], waitforresult=True, delay=0):
        '''Runs an icommand with optional argument list and
        returns tuple (stdout, stderr, ) from subprocess execution.

        Set of valid commands can be extended.
        '''
        valid_cmds = ['iinit', 'ienv', 'ihelp', 'ils', 'icd',
                      'imkdir', 'ichmod', 'imeta', 'iget', 'iput',
                      'imv', 'icp', 'irepl', 'iquest', 'irm',
                      'irmtrash', 'iexit', 'ilsresc', 'imiscsvrinfo',
                      'iuserinfo', 'ipwd', 'ierror', 'iexecmd', 'ips',
                      'iqstat', 'ichksum', 'itrim', 'iphymv', 'ibun',
                      'iphybun', 'ireg', 'imcoll', 'irsync', 'ixmsg',
                      'irule', 'iqdel', 'iticket', 'iapitest', 'iscan',
                      'isysmeta',]

        if icommand not in valid_cmds:
            # second value represents STDERR
            return ('', 'Invalid Command - "' + icommand + '" not allowed')

        env = os.environ.copy()
        env['IRODS_ENVIRONMENT_FILE'] = self._environment_file_path
        env['IRODS_AUTHENTICATION_FILE'] = self._authentication_file_path

        cmdStr = os.path.join(self._icommands_binaries_dir, icommand)
        argList = [cmdStr] + argList

        pydevtest_common.write_to_log('server', ' --- icommand [{0}] --- \n'.format(' '.join(argList)))

        if delay > 0:
            print '  runCmd: sleeping [' + str(delay) + '] seconds'
            time.sleep(delay)

        if waitforresult:
            return subprocess.Popen(argList, stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE, env=env).communicate()
        else:
            return subprocess.Popen(argList, stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE, env=env)

    def runAdminCmd(self, icommand, argList=[]):
        '''Runs the iadmin icommand with optional argument list and
        returns tuple (stdout, stderr) from subprocess execution.
        '''

        if icommand != 'iadmin':
            # second value represents STDERR
            return ("", "Invalid Admin Command - '" + icommand + "' not allowed")

        myenv = os.environ.copy()
        myenv['IRODS_ENVIRONMENT_FILE'] = self._environment_file_path
        myenv['IRODS_AUTHENTICATION_FILE'] = self._authentication_file_path

        cmdStr = "%s/%s" % (self._icommands_binaries_dir, icommand)
        argList = [cmdStr] + argList

        pydevtest_common.write_to_log('server', ' --- iadmin [{0}] --- \n'.format(' '.join(argList)))

        return subprocess.Popen(argList, stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                env=myenv).communicate("yes\n")
