from __future__ import print_function

import itertools
import os

import configuration
import irods_session
import pydevtest_common


def make_environment_dict(username, hostname):
    environment = {
        'irods_host': hostname,
        'irods_port': 1247,
        'irods_default_resource': 'demoResc',
        'irods_home': os.path.join('/tempZone/home', username),
        'irods_cwd': os.path.join('/tempZone/home', username),
        'irods_user_name': username,
        'irods_zone': 'tempZone',
        'irods_client_server_negotiation': 'request_server_negotiation',
        'irods_client_server_policy': 'CS_NEG_REFUSE',
        'irods_encryption_key_size': 32,
        'irods_encryption_salt_size': 8,
        'irods_encryption_num_hash_rounds': 16,
        'irods_encryption_algorithm': 'AES-256-CBC',
        'irods_default_hash_scheme': 'SHA256',
    }
    if configuration.USE_SSL:
        environment.update({
            'irods_client_server_policy': 'CS_NEG_REQUIRE',
            'irods_ssl_verify_server': 'cert',
            'irods_ssl_ca_certificate_file': '/etc/irods/server.crt',
        })
    return environment

def make_session_for_existing_admin():
    env_dict = make_environment_dict(configuration.PREEXISTING_ADMIN_USERNAME,
                                     configuration.ICAT_HOSTNAME)
    return irods_session.IrodsSession('/usr/bin', env_dict, configuration.PREEXISTING_ADMIN_PASSWORD, False)

def mkuser_and_return_session(user_type, username, password, hostname):
    with make_session_for_existing_admin() as admin_session:
        admin_session.assert_icommand(['iadmin', 'mkuser', username, user_type])
        admin_session.assert_icommand(['iadmin', 'moduser', username, 'password', password])
        env_dict = make_environment_dict(username, hostname)
        return irods_session.IrodsSession('/usr/bin', env_dict, password, True)

def mkgroup_and_add_users(group_name, usernames):
    with make_session_for_existing_admin() as admin_session:
        admin_session.assert_icommand(['iadmin', 'mkgroup', group_name])
        for username in usernames:
            admin_session.assert_icommand(['iadmin', 'atg', group_name, username])

def rmgroup(group_name):
    with make_session_for_existing_admin() as admin_session:
        admin_session.assert_icommand(['iadmin', 'rmgroup', group_name])

def rmuser(username):
    with make_session_for_existing_admin() as admin_session:
        admin_session.assert_icommand(['iadmin', 'rmuser', username])

def make_sessions_mixin(rodsadmin_name_password_list, rodsuser_name_password_list):
    class SessionsMixin(object):
        def setUp(self):
            with make_session_for_existing_admin() as admin_session:
                self.admin_sessions = [mkuser_and_return_session('rodsadmin', name, password, pydevtest_common.get_hostname())
                                           for name, password in rodsadmin_name_password_list]
                self.user_sessions = [mkuser_and_return_session('rodsuser', name, password, pydevtest_common.get_hostname())
                                           for name, password in rodsuser_name_password_list]
            super(SessionsMixin, self).setUp()

        def tearDown(self):
            with make_session_for_existing_admin() as admin_session:
                for session in itertools.chain(self.admin_sessions, self.user_sessions):
                    session.__exit__()
                    admin_session.assert_icommand(['iadmin', 'rmuser', session.username])
            super(SessionsMixin, self).tearDown()
    return SessionsMixin
