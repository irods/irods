import os
import re
import sys
import socket

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from .. import lib
from . import session


SessionsMixin = session.make_sessions_mixin([('otherrods', 'apass')], [('alice', 'password'), ('anonymous', None)])


class Test_Iticket(SessionsMixin, unittest.TestCase):

    def setUp(self):
        super(Test_Iticket, self).setUp()
        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]
        self.anon = self.user_sessions[1]

    def tearDown(self):
        super(Test_Iticket, self).tearDown()

    def test_iticket_bad_subcommand(self):
        self.admin.assert_icommand('iticket badsubcommand', 'STDOUT_SINGLELINE', 'unrecognized command')

    def test_iticket_get(self):
        filename = 'TicketTestFile'
        filepath = os.path.join(self.admin.local_session_dir, filename)
        lib.make_file(filepath, 1)
        collection = self.admin.session_collection + '/dir'
        data_obj = collection + '/' + filename

        self.admin.assert_icommand('imkdir ' + collection)
        self.admin.assert_icommand('iput ' + filepath + ' ' + data_obj)
        self.admin.assert_icommand('ils -l ' + collection, 'STDOUT')
        self.user.assert_icommand('ils -l ' + collection, 'STDERR')
        self.anon.assert_icommand('ils -l ' + collection, 'STDERR')
        self.ticket_get_on(data_obj, data_obj)
        self.ticket_get_on(collection, data_obj)

    def test_iticket_put(self):
        filename = 'TicketTestFile'
        filepath = os.path.join(self.admin.local_session_dir, filename)
        lib.make_file(filepath, 1)
        collection = self.admin.session_collection + '/dir'
        data_obj = collection + '/' + filename

        self.admin.assert_icommand('imkdir ' + collection)
        self.admin.assert_icommand('iput ' + filepath + ' ' + data_obj)
        self.admin.assert_icommand('ils -l ' + collection, 'STDOUT')
        self.user.assert_icommand('ils -l ' + collection, 'STDERR')
        self.anon.assert_icommand('ils -l ' + collection, 'STDERR')
        self.ticket_put_on(data_obj, data_obj, filepath)
        self.ticket_put_on(collection, data_obj, filepath)

    def ticket_get_on(self, ticket_target, data_obj):
        ticket = 'ticket'
        self.ticket_get_fail(ticket, data_obj)
        self.admin.assert_icommand('iticket create read ' + ticket_target + ' ' + ticket)
        self.admin.assert_icommand('iticket ls', 'STDOUT')
        self.ticket_get(ticket, data_obj)
        self.ticket_group_get(ticket, data_obj)
        self.ticket_user_get(ticket, data_obj)
        self.ticket_host_get(ticket, data_obj)
        self.ticket_expire_get(ticket, data_obj)

        self.admin.assert_icommand('iticket ls ' + ticket, 'STDOUT')

        #self.admin.assert_icommand('iticket delete ' + ticket)
        #self.admin.assert_icommand('iticket create read ' + ticket_target + ' ' + ticket)
        #self.ticket_uses_get(ticket, data_obj)
        #self.admin.assert_icommand('iticket delete ' + ticket)
        #self.admin.assert_icommand('iticket create read ' + ticket_target + ' ' + ticket)
        #self.ticket_uses_get(ticket, data_obj)

        self.admin.assert_icommand('iticket delete ' + ticket)

    def ticket_put_on(self, ticket_target, data_obj, filepath):
        ticket = 'faketicket'
        self.ticket_put_fail(ticket, data_obj, filepath)
        _, out, _ = self.admin.assert_icommand(['iticket', 'create', 'write', ticket_target], 'STDOUT_SINGLELINE', 'ticket:')
        ticket = out.rpartition(':')[2].rstrip('\n')
        assert len(ticket) == 15, ticket
        ticket_chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'
        for c in ticket:
            assert c in ticket_chars, c
        self.admin.assert_icommand('iticket mod ' + ticket + ' write-file 0')
        self.admin.assert_icommand('iticket ls', 'STDOUT')
        self.ticket_put(ticket, data_obj, filepath)
        self.ticket_group_put(ticket, data_obj, filepath)
        self.ticket_user_put(ticket, data_obj, filepath)
        self.ticket_host_put(ticket, data_obj, filepath)
        self.ticket_expire_put(ticket, data_obj, filepath)

        self.admin.assert_icommand('iticket ls ' + ticket, 'STDOUT')

        #self.admin.assert_icommand('iticket delete ' + ticket)
        #self.admin.assert_icommand('iticket create write ' + ticket_target + ' ' + ticket)
        #self.ticket_uses_put(ticket, data_obj, filepath)
        #self.admin.assert_icommand('iticket delete ' + ticket)
        #self.admin.assert_icommand('iticket create write ' + ticket_target + ' ' + ticket)
        #self.admin.assert_icommand('iticket delete ' + ticket)
        #self.admin.assert_icommand('iticket create write ' + ticket_target + ' ' + ticket)
        #self.ticket_uses_put(ticket, data_obj, filepath)
        #self.ticket_bytes_put(ticket, data_obj)

        self.admin.assert_icommand('iticket delete ' + ticket)

    def ticket_get(self, ticket, data_obj):
        self.user.assert_icommand('iget -t ' + ticket + ' ' + data_obj + ' -', 'STDOUT')
        self.anon.assert_icommand('iget -t ' + ticket + ' ' + data_obj + ' -', 'STDOUT')
        self.user.assert_icommand('iget -t ' + ticket + ' ' + data_obj + ' -', 'STDOUT')
        self.anon.assert_icommand('iget -t ' + ticket + ' ' + data_obj + ' -', 'STDOUT')

    def ticket_put(self, ticket, data_obj, filepath):
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.anon.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.anon.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)

    def ticket_get_fail(self, ticket, data_obj):
        self.user.assert_icommand('iget -t ' + ticket + ' ' + data_obj + ' -', 'STDERR')
        self.anon.assert_icommand('iget -t ' + ticket + ' ' + data_obj + ' -', 'STDERR')

    def ticket_put_fail(self, ticket, data_obj, filepath):
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj, 'STDERR')
        self.anon.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj, 'STDERR')

    def ticket_uses_get(self, ticket, data_obj):
        self.admin.assert_icommand('iticket mod ' + ticket + ' uses 2')
        self.anon.assert_icommand('iget ' + data_obj + ' -', 'STDERR')
        self.anon.assert_icommand('iget -t ' + ticket + ' ' + data_obj + ' -', 'STDOUT')
        self.anon.assert_icommand('iget -t ' + ticket + ' ' + data_obj + ' -', 'STDOUT')
        self.anon.assert_icommand('iget -t ' + ticket + ' ' + data_obj + ' -', 'STDERR')
        self.admin.assert_icommand('iticket ls', 'STDOUT')
        self.admin.assert_icommand('iticket mod ' + ticket + ' uses 0')

    def ticket_uses_put(self, ticket, data_obj, filepath):
        self.admin.assert_icommand('iticket mod ' + ticket + ' write-file 3')
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj, 'STDERR')
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj, 'STDERR')
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj, 'STDERR')
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj, 'STDERR')
        self.admin.assert_icommand('iticket mod ' + ticket + ' write-file 6')
        self.admin.assert_icommand('iticket ls', 'STDOUT')
        self.anon.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.anon.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.anon.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.anon.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj, 'STDERR')
        self.admin.assert_icommand('iticket mod ' + ticket + ' write-file 0')
        self.admin.assert_icommand('iticket mod ' + ticket + ' uses 3')
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj, 'STDERR')
        self.admin.assert_icommand('iticket mod ' + ticket + ' uses 6')
        self.admin.assert_icommand('iticket ls', 'STDOUT')
        self.anon.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.anon.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.anon.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.anon.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj, 'STDERR')
        self.admin.assert_icommand('iticket mod ' + ticket + ' uses 0')

    def ticket_group_get(self, ticket, data_obj):
        group = 'group'
        self.admin.assert_icommand('iadmin mkgroup ' + group)
        self.admin.assert_icommand('iadmin atg ' + group + ' ' + self.user.username)
        self.admin.assert_icommand('iticket mod ' + ticket + ' add group ' + group)
        self.user.assert_icommand('iget -t ' + ticket + ' ' + data_obj + ' -', 'STDOUT')
        self.anon.assert_icommand('iget -t ' + ticket + ' ' + data_obj + ' -', 'STDERR')
        self.user.assert_icommand('iget -t ' + ticket + ' ' + data_obj + ' -', 'STDOUT')
        self.anon.assert_icommand('iget -t ' + ticket + ' ' + data_obj + ' -', 'STDERR')
        self.admin.assert_icommand('iticket mod ' + ticket + ' remove group ' + group)
        self.admin.assert_icommand('iadmin rfg ' + group + ' ' + self.user.username)
        self.admin.assert_icommand(['iadmin', 'rmgroup', group])

    def ticket_group_put(self, ticket, data_obj, filepath):
        group = 'group'
        self.admin.assert_icommand('iadmin mkgroup ' + group)
        self.admin.assert_icommand('iadmin atg ' + group + ' ' + self.user.username)
        self.admin.assert_icommand('iticket mod ' + ticket + ' add group ' + group)
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.anon.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj, 'STDERR')
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.anon.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj, 'STDERR')
        self.admin.assert_icommand('iticket mod ' + ticket + ' remove group ' + group)
        self.admin.assert_icommand('iadmin rfg ' + group + ' ' + self.user.username)
        self.admin.assert_icommand('iadmin rmgroup ' + group)

    def ticket_user_get(self, ticket, data_obj):
        self.admin.assert_icommand('iticket mod ' + ticket + ' add user ' + self.user.username)
        self.user.assert_icommand('iget -t ' + ticket + ' ' + data_obj + ' -', 'STDOUT')
        self.anon.assert_icommand('iget -t ' + ticket + ' ' + data_obj + ' -', 'STDERR')
        self.user.assert_icommand('iget -t ' + ticket + ' ' + data_obj + ' -', 'STDOUT')
        self.anon.assert_icommand('iget -t ' + ticket + ' ' + data_obj + ' -', 'STDERR')
        self.admin.assert_icommand('iticket mod ' + ticket + ' remove user ' + self.user.username)

    def ticket_user_put(self, ticket, data_obj, filepath):
        self.admin.assert_icommand('iticket mod ' + ticket + ' add user ' + self.user.username)
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.anon.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj, 'STDERR')
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.anon.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj, 'STDERR')
        self.admin.assert_icommand('iticket mod ' + ticket + ' remove user ' + self.user.username)

    def ticket_host_get(self, ticket, data_obj):
        host = lib.get_hostname()
        not_host = '0.0.0.0'
        self.admin.assert_icommand('iticket mod ' + ticket + ' add host ' + not_host)
        self.ticket_get_fail(ticket, data_obj)
        self.admin.assert_icommand('iticket mod ' + ticket + ' add host ' + host)
        self.ticket_get(ticket, data_obj)
        self.admin.assert_icommand('iticket mod ' + ticket + ' remove host ' + not_host)
        self.ticket_get(ticket, data_obj)
        self.admin.assert_icommand('iticket mod ' + ticket + ' remove host ' + host)

    def ticket_host_put(self, ticket, data_obj, filepath):
        host = lib.get_hostname()
        not_host = '0.0.0.0'
        self.admin.assert_icommand('iticket mod ' + ticket + ' add host ' + not_host)
        self.ticket_put_fail(ticket, data_obj, filepath)
        self.admin.assert_icommand('iticket mod ' + ticket + ' add host ' + host)
        self.ticket_put(ticket, data_obj, filepath)
        self.admin.assert_icommand('iticket mod ' + ticket + ' remove host ' + not_host)
        self.ticket_put(ticket, data_obj, filepath)
        self.admin.assert_icommand('iticket mod ' + ticket + ' remove host ' + host)

    def ticket_expire_get(self, ticket, data_obj):
        past_date = "1970-01-01"
        future_date = "2040-12-12"
        self.admin.assert_icommand('iticket mod ' + ticket + ' expire ' + past_date)
        self.ticket_get_fail(ticket, data_obj)
        self.admin.assert_icommand('iticket mod ' + ticket + ' expire ' + future_date)
        self.ticket_get(ticket, data_obj)
        self.admin.assert_icommand('iticket mod ' + ticket + ' expire 0')
        self.ticket_get(ticket, data_obj)

    def ticket_expire_put(self, ticket, data_obj, filepath):
        past_date = "1970-01-01"
        future_date = "2040-12-12"
        self.admin.assert_icommand('iticket mod ' + ticket + ' expire ' + past_date)
        self.ticket_put_fail(ticket, data_obj, filepath)
        self.admin.assert_icommand('iticket mod ' + ticket + ' expire ' + future_date)
        self.ticket_put(ticket, data_obj, filepath)
        self.admin.assert_icommand('iticket mod ' + ticket + ' expire 0')
        self.ticket_put(ticket, data_obj, filepath)

    def ticket_bytes_put(self, ticket, data_obj, filepath):
        lib.make_file(filepath, 2)
        self.admin.assert_icommand('iticket mod ' + ticket + ' write-byte 6')
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj, 'STDERR')
        self.admin.assert_icommand('iticket mod ' + ticket + ' write-byte 8')
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj, 'STDERR')
        self.admin.assert_icommand('iticket mod ' + ticket + ' write-byte 0')
        self.user.assert_icommand('iput -ft ' + ticket + ' ' + filepath + ' ' + data_obj)

    def test_ticket_read_apply_recursively__issue_3299(self):
        collname_1 = 'ticket_read_apply_recursively__issue_3299'
        collname_2 = 'subdir'
        collname_3 = 'subsubdir'
        filename_1 = '3299_test_file_1'
        filename_2 = '3299_test_file_2'
        filename_3 = '3299_test_file_3'
        ticket = 'ticket_3299'
        lib.make_file(filename_1, 1024, 'arbitrary')
        lib.make_file(filename_2, 1024, 'arbitrary')
        lib.make_file(filename_3, 1024, 'arbitrary')
        self.admin.assert_icommand(['icd', self.admin.session_collection])
        self.admin.assert_icommand(['imkdir', collname_1])
        self.admin.assert_icommand(['icd', collname_1])
        self.admin.assert_icommand(['imkdir', collname_2])
        self.admin.assert_icommand(['iput', '-f', filename_1])
        self.admin.assert_icommand(['icd', collname_2])
        self.admin.assert_icommand(['imkdir', collname_3])
        self.admin.assert_icommand(['iput', '-f', filename_2])
        self.admin.assert_icommand(['icd', collname_3])
        self.admin.assert_icommand(['iput', '-f', filename_3])
        self.admin.assert_icommand(['icd', self.admin.session_collection])
        self.admin.assert_icommand(['iticket', 'create', 'read', collname_1, ticket])
        self.admin.assert_icommand(['ils', '-r', self.admin.session_collection + '/' + collname_1], 'STDOUT_SINGLELINE', filename_3)
        self.user.assert_icommand(['ils', '-r', self.admin.session_collection + '/' + collname_1, '-t', ticket], 'STDOUT_SINGLELINE', filename_3)

    def test_ticket_create_ticket_with_string_as_number__issue_3553(self):
        filename = '3553_test_file'
        lib.make_file(filename, 1024, 'arbitrary')
        ticketname = '23456'
        self.admin.assert_icommand('iput '+filename)
        self.admin.assert_icommand('iticket create write '+filename+' '+ticketname, 'STDERR_SINGLELINE', 'CAT_TICKET_INVALID')
        self.user.assert_icommand('iput '+filename)
        self.user.assert_icommand('iticket create write '+filename+' '+ticketname, 'STDERR_SINGLELINE', 'CAT_TICKET_INVALID')
