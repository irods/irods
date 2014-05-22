# Verifies that required system configurations are correct before the RPM
# package is installed.

import sys
import os
import pipes
import subprocess
import tempfile

xinetd_authd_config = '/etc/xinetd.d/auth'
authd_runlevel = 3


def check_xinetd_authd_config(config_fn):
    """
    Check the server_args attribute in the xinetd authd system configuration
    file (specified by config_fn), and confirm that it does not include the -E
    option.  Returns 1 if the -E is present (which should be corrected before
    installing the iRODS RPM), or 0 if the file cannot be found or if the
    file does not include the -E option for server_args.
    """
    if not os.path.exists(config_fn):
        return 0
    error = 0
    in_auth = 0
    config_file = open(config_fn)
    while True:
        line = config_file.readline().strip()
        if not line:
            break
        if line.startswith('service'):
            split_service = line.split()
            if len(split_service) >= 2 and split_service[1] == 'auth':
                in_auth = 1
        if line.startswith('}'):
            in_auth = 0
        if in_auth and line.startswith('server_args'):
            split_attr = line.split('=')
            if len(split_attr) >= 2:
                split_value = split_attr[1].split()
                if '-E' in split_value:
                    error = 1
    config_file.close()
    return error


def check_authd_in_runlevel(runlevel):
    """
    Check the specified runlevel and confirm that authd is enabled.  Returns 0
    if authd is confirmed to be enabled (as required by iRODS), or 1
    otherwise.
    """
    error = 1
    (temp_file, temp_fn) = (
        tempfile.mkstemp(prefix='irods_pre_rpm-', dir='/tmp'))
    subprocess.call(
        ['/sbin/chkconfig',
            '--list',
            '--level=' + str(runlevel),
            'auth'],
        stdout=temp_file)
    os.close(temp_file)
    temp_file = open(temp_fn)
    line = temp_file.readline().strip()
    split = line.split()
    if split[0] == 'auth' and split[1] == 'on':
        error = 0
    temp_file.close()
    os.remove(temp_fn)
    return error


def pre_rpm_checks():
    """
    Calls checks required before the iRODS RPM is installed, and outputs any
    failures to standard error.  Exits with a return code of 0 if all checks
    were successful, or 1 if the xinetd authd configuration check failed, or 2
    if the authd runlevel check failed.
    """
    error = 0
    if check_xinetd_authd_config(xinetd_authd_config):
        sys.stderr.write('ERROR: The auth server_args attribute in ' +
                         xinetd_authd_config +
                         ' includes -E\n')
        error = 1
    if check_authd_in_runlevel(authd_runlevel):
        sys.stderr.write(
            'ERROR: The authd server is not enabled in runlevel ' +
            str(authd_runlevel) + '.\n')
        error = 2
    if error != 0:
        sys.exit(error)
    else:
        sys.stderr.write('OK\n')

pre_rpm_checks()
