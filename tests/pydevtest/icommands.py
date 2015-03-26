import datetime
import json
import os
import psutil
import shutil
import subprocess
import sys
import tempfile
import time

import pydevtest_common


def write_to_server_log(message):
    server_log_dir = os.path.join(pydevtest_common.get_irods_top_level_dir(), 'iRODS/server/log')
    proc = subprocess.Popen('ls -t ' + server_log_dir + '/rodsLog* | head -n1', stdout=subprocess.PIPE, shell=True)
    log_file_path, err = proc.communicate()
    with open(log_file_path.rstrip(), 'a') as f:
        f.write(message)

class RodsSession(object):
    '''A set of methods to start, close, and manage multiple
    iRODS client sessions at the same time, using icommands.
    '''

    def __init__(self, icommands_binaries_dir, environment_dict, password):
        self._icommands_binaries_dir = icommands_binaries_dir
        self._environment_dict = environment_dict
        self._password = password

        self._session_dir = tempfile.mkdtemp(prefix='irods-testing-')
        self._environment_file_path = os.path.join(self._session_dir, 'irods_environment.json')
        self._authentication_file_path = os.path.join(self._session_dir, 'irods_authentication')
        self._write_environment_file()

        self._session_id = datetime.datetime.utcnow().strftime('%Y-%m-%dZ%H:%M:%S--') + os.path.basename(self._session_dir)

    def _write_environment_file(self):
        with open(self._environment_file_path, 'w') as f:
            json.dump(self._environment_dict, f)

    def update_environment_dict(self, environment_dict):
        self._environment_dict.update(environment_dict)
        self._write_environment_file()

    def delete_session_dir(self):
        '''To be called after self.runCmd('iexit').'''
        shutil.rmtree(self._session_dir)

    def get_zone_name(self):
        return self._environment_dict['irods_zone']

    def get_username(self):
        return self._environment_dict['irods_user_name']

    def get_default_resource(self):
        return self._environment_dict['irods_default_resource']

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

        write_to_server_log(' --- interrupt icommand [{0}] --- \n'.format(' '.join(argList)))

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

        write_to_server_log(' --- interrupt icommand delay({0}) [{1}] --- \n'.format(delay, ' '.join(argList)))

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
                      'irule', 'iqdel', 'iticket', 'iapitest',
                      'isysmeta',]

        if icommand not in valid_cmds:
            # second value represents STDERR
            return ('', 'Invalid Command - "' + icommand + '" not allowed')

        env = os.environ.copy()
        env['IRODS_ENVIRONMENT_FILE'] = self._environment_file_path
        env['IRODS_AUTHENTICATION_FILE'] = self._authentication_file_path

        cmdStr = os.path.join(self._icommands_binaries_dir, icommand)
        argList = [cmdStr] + argList

        write_to_server_log(' --- icommand [{0}] --- \n'.format(' '.join(argList)))

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

        write_to_server_log(' --- iadmin [{0}] --- \n'.format(' '.join(argList)))

        return subprocess.Popen(argList, stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                env=myenv).communicate("yes\n")
