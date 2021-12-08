#!/usr/bin/python3
from __future__ import print_function
import logging
import sys

from irods import json_validation
from irods.exceptions import IrodsError, IrodsWarning
import irods.log

if __name__ == '__main__':
    logging.getLogger().setLevel(logging.NOTSET)
    l = logging.getLogger(__name__)

    irods.log.register_tty_handler(sys.stdout, logging.INFO, logging.WARNING)
    irods.log.register_tty_handler(sys.stderr, logging.WARNING, None)
    if len(sys.argv) != 3:
        l.error('Usage: %s <configuration_file> <schema_url>', sys.argv[0])
        sys.exit(1)

    config_file = sys.argv[1]
    schema_uri = sys.argv[2]

    try:
        json_validation.load_and_validate(config_file, schema_uri)
    except IrodsWarning as e:
        l.warning('Encountered a warning in validation.', exc_info=True)
        sys.exit(2)
    except IrodsError as e:
        l.error('Encountered an error in validation.', exc_info=True)
        sys.exit(1)
    sys.exit(0)
