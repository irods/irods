from __future__ import print_function

from .. import lib
from . import session

import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

import os
import signal
import threading

import click
from flask import (
    Flask
)

cfg = """{
    "database_config.json": {
        "test": "configuration"
    },
    "hosts_config.json": {
        "test": "configuration"
    },
    "hosts_config_json_object": {
        "test": "configuration"
    },
    "server_config.json": {
        "test_key": "test_configuration"
    }
}"""

app = Flask('remote_configuration')
@app.route('/get_configuration')
def get_configuration():
    return cfg

def secho(text, file=None, nl=None, err=None, color=None, **styles):
    pass

def echo(text, file=None, nl=None, err=None, color=None, **styles):
    pass

click.echo = echo
click.secho = secho

def run_endpoint():
    app.run()

def stop_endpoint():
    os.kill(os.getpid(), signal.SIGTERM)




class TestRemoteServerProperties(session.make_sessions_mixin([], []), unittest.TestCase):

    def setUp(self):
        super(TestRemoteServerProperties, self).setUp()
        self.thread = threading.Thread(target=run_endpoint).start();

    def tearDown(self):
        super(TestRemoteServerProperties, self).tearDown()
        stop_endpoint()

    def test_server_properties_endpoint(self):
            os.environ['IRODS_SERVER_CONFIGURATION_API_KEY']  = 'default_api_key'
            os.environ['IRODS_SERVER_CONFIGURATION_ENDPOINT'] = 'http://localhost:5000/get_configuration'
            out, _ = lib.execute_command('irods_configuration_test_harness')
            assert(out == cfg+'\n')

