#!/usr/bin/python2
from __future__ import print_function

import contextlib
import sys
import logging

from irods.configuration import IrodsConfig
import irods.log


def get_current_schema_version(irods_config=None, cursor=None):
    if irods_config is None:
        irods_config = IrodsConfig()
    return irods_config.get_schema_version_in_database()

if __name__ == '__main__':
    logging.getLogger().setLevel(logging.NOTSET)
    l = logging.getLogger(__name__)
    irods.log.register_tty_handler(sys.stdout, logging.INFO, logging.WARNING)
    irods.log.register_tty_handler(sys.stderr, logging.WARNING, None)

    irods_config = IrodsConfig()
    irods.log.register_file_handler(irods_config.control_log_path)

    print(get_current_schema_version(irods_config))
