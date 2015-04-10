from __future__ import print_function

import os
import datetime
import socket

import icommands
import pydevtest_common

_user_creation_information = [
    {'name': 'rods',
     'password': 'rods',
 },
    {'name': 'alice',
     'password': 'apass',
 },
    {'name': 'bobby',
     'password': 'bpass',
 },
]


def make_environment_dict(username):
    environment = {
        'irods_host': socket.gethostname(),
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
    if pydevtest_common.irods_test_constants.USE_SSL:
        environment.update({
            'irods_client_server_policy': 'CS_NEG_REQUIRE',
            'irods_ssl_verify_server': 'cert',
            'irods_ssl_ca_certificate_file': '/etc/irods/server.crt',
        })
    return environment

######################################################
# generic standup functions for admin and user
#   called by more specific functions down below
######################################################
def admin_up():
    global adminsession
    adminsession = icommands.RodsSession('/usr/bin', make_environment_dict(_user_creation_information[0]['name']), _user_creation_information[0]['password'])
    adminsession.runCmd('iinit', [adminsession._password])
    adminsession.runCmd('imkdir', [adminsession._session_id])
    adminsession.runCmd('icd', [adminsession._session_id])
    print('admin session created: user[' + adminsession.username + '] zone[' + adminsession.zone_name + ']')

    # set sessions[0] as adminsession
    global sessions
    sessions = []
    sessions.append(adminsession)

    # users, passwords, and groups
    global testgroup
    testgroup = "pydevtest_user_group"
    if not pydevtest_common.irods_test_constants.TOPOLOGY_FROM_RESOURCE_SERVER:
        adminsession.runAdminCmd('iadmin', ['mkgroup', testgroup])
        for u in _user_creation_information[1:]:
            adminsession.runAdminCmd('iadmin', ['mkuser', u['name'], 'rodsuser'])
            adminsession.runAdminCmd('iadmin', ['moduser', u['name'], 'password', u['password']])
            adminsession.runAdminCmd('iadmin', ['atg', testgroup, u['name']])

    # get back into the proper directory
    adminsession.runCmd('icd', [adminsession._session_id])

def admin_down():
    # trash
    adminsession.runCmd('irmtrash', ['-M'])  # removes all trash for all users (admin mode)

    # users
    if not pydevtest_common.irods_test_constants.TOPOLOGY_FROM_RESOURCE_SERVER:
        for u in _user_creation_information[1:]:
            adminsession.runAdminCmd('iadmin', ['rfg', testgroup, u['name']])
            adminsession.runAdminCmd('iadmin', ['rmuser', u['name']])
        # groups
        adminsession.runAdminCmd('iadmin', ['rmgroup', testgroup])
        adminsession.runAdminCmd('iadmin', ['rum'])

    print('admin session exiting: user[' + adminsession.username + '] zone[' + adminsession.zone_name + ']')
    adminsession.runCmd('iexit', ['full'])
    adminsession.delete_session_dir()

def user_up(user):
    # set up single user session
    user_session = icommands.RodsSession('/usr/bin', make_environment_dict(user['name']), user['password'])
    user_session.runCmd('iinit', [user_session._password])
    user_session.runCmd('imkdir', [user_session._session_id])
    user_session.runCmd('icd', [user_session._session_id])
    print('user session created: user[' + user_session.username + '] zone[' + user_session.zone_name + ']')
    sessions.append(user_session)

def user_down(usersession):
    # tear down user session
    usersession.runCmd('icd')
    usersession.runCmd('irm', ['-rf', usersession._session_id])
    print('user session exiting: user[' + usersession.username + '] zone[' + usersession.zone_name + ']')
    usersession.runCmd('iexit', ['full'])
    usersession.delete_session_dir()

#################################################################
# routines to be called from with_setup
#   designed this way since with_setup cannot pass variables
#################################################################

def adminonly_up():
    admin_up()

def adminonly_down():
    admin_down()

def oneuser_up():
    admin_up()
    user_up(_user_creation_information[1])

def oneuser_down():
    user_down(sessions[1])
    admin_down()

def twousers_up():
    admin_up()
    user_up(_user_creation_information[1])
    user_up(_user_creation_information[2])

def twousers_down():
    user_down(sessions[2])
    user_down(sessions[1])
    admin_down()
