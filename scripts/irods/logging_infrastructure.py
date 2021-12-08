#! /usr/bin/python3

from __future__ import print_function

import os
import sys
import distro
import subprocess
import shutil
import logging

from .log import register_tty_handler

def rsyslog_config_path():
    return '/etc/rsyslog.d/00-irods.conf'

def logrotate_config_path():
    return '/etc/logrotate.d/irods'

def setup_rsyslog_and_logrotate(register_tty=True):
    l = logging.getLogger(__name__)
    l.setLevel(logging.INFO)

    if register_tty:
        register_tty_handler(sys.stdout, logging.INFO, logging.WARNING)

    # Copy rsyslog configuration file into place if it does not exist
    # and restart the rsyslog daemon so that the configuration is loaded.
    dst = rsyslog_config_path()
    if not os.path.isfile(dst):
        l.info('Configuring rsyslog ...')
        shutil.copyfile('/var/lib/irods/packaging/irods.rsyslog', dst)
        l.info('done.')

        l.info('Restarting rsyslog ...')
        if 'ubuntu' == distro.id():
            subprocess.call(['service', 'rsyslog', 'restart'])
        else:
            subprocess.call(['systemctl', 'restart', 'rsyslog'])
        l.info('done.')
    else:
        l.info('rsyslog already configured.')

    # Copy logrotate configuration file into place if it does not exist.
    dst = logrotate_config_path()
    if not os.path.isfile(dst):
        l.info('Configuring logrotate ...')
        shutil.copyfile('/var/lib/irods/packaging/irods.logrotate', dst)
        l.info('done.')
    else:
        l.info('logrotate already configured.')
