from __future__ import print_function
from contextlib import contextmanager
import os

from .configuration import IrodsConfig
from .exceptions import IrodsError
from . import lib

IRODS_RULE_LANGUAGE_RULE_ENGINE_PLUGIN_NAME = 'irods_rule_engine_plugin-irods_rule_language'
PYTHON_RULE_ENGINE_PLUGIN_NAME = 'irods_rule_engine_plugin-python'

class CoreFile:
    def __init__(self, plugin_name=None):
        irods_config = IrodsConfig()
        self.plugin_name = plugin_name if plugin_name is not None else irods_config.default_rule_engine_plugin
        try:
            filename = {
                    IRODS_RULE_LANGUAGE_RULE_ENGINE_PLUGIN_NAME: 'core.re',
                    PYTHON_RULE_ENGINE_PLUGIN_NAME: 'core.py'
                    }[self.plugin_name]
        except KeyError:
            raise IrodsError('unsupported rule engine for testing: {0}'.format(self.plugin_name))
        self.filepath = os.path.join(irods_config.core_re_directory, filename)

    def add_rule(self, rule_text):
        if self.plugin_name == IRODS_RULE_LANGUAGE_RULE_ENGINE_PLUGIN_NAME:
            lib.prepend_string_to_file(rule_text, self.filepath)
        if self.plugin_name == PYTHON_RULE_ENGINE_PLUGIN_NAME:
            with open(self.filepath, 'a') as f:
                print(rule_text, file=f)

@contextmanager
def temporary_core_file(plugin_name=None):
    core = CoreFile(plugin_name)
    with lib.file_backed_up(core.filepath):
        yield core
