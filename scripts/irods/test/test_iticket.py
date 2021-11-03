import os
import re
import sys
import socket
import tempfile

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from .. import lib
from . import session
from .. import test

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

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Skipping for now - needs investigation - issue 4931")
    def test_iticket_get_small(self):
        # Single buffer transfer
        self.ticket_get_test(1)

    @unittest.skip('This test does not work in CI, but passes locally')
    def test_iticket_get_large(self):
        # Triggers parallel transfer
        self.ticket_get_test(40000001)

    def ticket_get_test(self, size):
        filename = 'TicketTestFile'
        filepath = os.path.join(self.admin.local_session_dir, filename)
        lib.make_file(filepath, size)
        collection = self.admin.session_collection + '/dir'
        data_obj = collection + '/' + filename

        self.admin.assert_icommand('imkdir ' + collection)
        self.admin.assert_icommand('iput ' + filepath + ' ' + data_obj)
        self.admin.assert_icommand('ils -l ' + collection, 'STDOUT')
        self.user.assert_icommand('ils -l ' + collection, 'STDERR')
        self.anon.assert_icommand('ils -l ' + collection, 'STDERR')
        self.ticket_get_on(data_obj, data_obj)
        self.ticket_get_on(collection, data_obj)

        os.unlink(filepath)

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Skipping for now - needs investigation - issue 4931")
    def test_iticket_put_small(self):
        # Single buffer transfer
        self.ticket_put_test(1)

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Skipping for now - needs investigation - issue 4931")
    def test_iticket_put_large(self):
        # Triggers parallel transfer
        self.ticket_put_test(40000001)

    def ticket_put_test(self, size):
        filename = 'TicketTestFile'
        filepath = os.path.join(self.admin.local_session_dir, filename)
        lib.make_file(filepath, size)
        collection = self.admin.session_collection + '/dir'
        data_obj = collection + '/' + filename

        self.admin.assert_icommand('imkdir ' + collection)
        self.admin.assert_icommand('iput ' + filepath + ' ' + data_obj)
        self.admin.assert_icommand('ils -l ' + collection, 'STDOUT')
        self.user.assert_icommand('ils -l ' + collection, 'STDERR')
        self.anon.assert_icommand('ils -l ' + collection, 'STDERR')
        self.ticket_put_on(data_obj, data_obj, filepath)
        self.ticket_put_on(collection, data_obj, filepath)

        os.unlink(filepath)

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
        self.admin.assert_icommand(['iticket', 'delete', ticket])

    def test_ticket_create_ticket_with_string_as_number__issue_3553(self):
        filename = '3553_test_file'
        lib.make_file(filename, 1024, 'arbitrary')
        ticketname = '23456'
        self.admin.assert_icommand('iput '+filename)
        self.admin.assert_icommand('iticket create write '+filename+' '+ticketname, 'STDERR_SINGLELINE', 'CAT_TICKET_INVALID')
        self.user.assert_icommand('iput '+filename)
        self.user.assert_icommand('iticket create write '+filename+' '+ticketname, 'STDERR_SINGLELINE', 'CAT_TICKET_INVALID')

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Skipping for now - needs investigation - issue 4931")
    def test_iticket_get__issue_4387(self):
        filename = 'TicketTestFile'
        filepath = os.path.join(self.admin.local_session_dir, filename)
        lib.make_file(filepath, 1)
        parent_collection = self.admin.session_collection + '/inner'
        collection = parent_collection + '/outer'
        data_obj = collection + '/' + filename

        self.admin.assert_icommand('imkdir -p ' + collection)
        self.admin.assert_icommand('iput ' + filepath + ' ' + data_obj)
        self.admin.assert_icommand('ils -l ' + collection, 'STDOUT')
        self.user.assert_icommand('ils -l ' + collection, 'STDERR')
        self.anon.assert_icommand('ils -l ' + collection, 'STDERR')
        self.ticket_get_on(parent_collection, data_obj)

    def test_data_size_is_updated_after_overwriting_data_object_using_ticket__issue_4744(self):
        ticket = 'ticket_issue_4744'

        try:
            resc_name = 'issue_4744_resc'
            vault_path = tempfile.mkdtemp(suffix='_issue_4744')
            self.admin.assert_icommand(['iadmin', 'mkresc', resc_name, 'unixfilesystem', lib.get_hostname() + ':' + vault_path], 'STDOUT', ['unixfilesystem'])

            # Create a new data object.
            data_object = 'issue_4744.txt'
            self.user.assert_icommand(['istream', 'write', '-R', resc_name, data_object], input='foo123')
            self.user.assert_icommand(['iquest', "select DATA_SIZE where COLL_NAME = '{0}' and DATA_NAME = '{1}'".format(self.user.session_collection, data_object)],
                                      'STDOUT', ['DATA_SIZE = 6'])

            # Create a ticket for writing to the data object.
            self.user.assert_icommand(['iticket', 'create', 'write', data_object, ticket])

            # Read the contents of the data object using the ticket and verify that the returned
            # contents matches the expected sequence of bytes.
            out, _, ec = self.user.run_icommand(['iget', '-t', ticket, data_object, '-'])
            self.assertEqual(ec, 0)
            self.assertEqual('666f6f313233', out.encode('hex'))

            def overwrite_and_verify(data, data_hex_encoding):
                # Create a new file. This will be used to overwrite the data object.
                f = tempfile.NamedTemporaryFile(delete=False)
                f.write(data)
                f.close()

                # Overwrite the data object with new data and verify that the size is what we expect.
                self.user.assert_icommand(['iput', '-ft', ticket, '-R', resc_name, f.name, data_object])
                self.user.assert_icommand(['iquest', "select DATA_SIZE where COLL_NAME = '{0}' and DATA_NAME = '{1}'".format(self.user.session_collection, data_object)],
                                          'STDOUT', ['DATA_SIZE = ' + str(len(data))])

                # Read the contents of the data object using the ticket and verify that the returned
                # contents matches the expected sequence of bytes.
                out, _, ec = self.user.run_icommand(['iget', '-t', ticket, data_object, '-'])
                self.assertEqual(ec, 0)
                self.assertEqual(data_hex_encoding, out.encode('hex'))

                # Get the physical path of the replica and compare its contents to the expected hex encoding.
                gql = "select DATA_PATH where COLL_NAME = '{0}' and DATA_NAME = '{1}'".format(self.user.session_collection, data_object)
                out, _, ec = self.user.run_icommand(['iquest', '%s', gql])
                self.assertEqual(ec, 0)
                with open(out.strip(), 'r') as f:
                    self.assertEqual(data_hex_encoding, f.read().encode('hex'))

            # Overwrite the data object with a smaller file.
            temp_file_contents = 'bar'
            overwrite_and_verify(temp_file_contents, '626172')

            # Overwrite the data object with a larger file.
            temp_file_contents = 'barbar123'
            overwrite_and_verify(temp_file_contents, '626172626172313233')
        finally:
            self.user.run_icommand(['iticket', 'delete', ticket])
            self.user.run_icommand(['irm', '-f', data_object])
            self.admin.run_icommand(['iadmin', 'rmresc', resc_name])

    def test_write_byte_count_is_updated_when_data_size_does_not_change__issue_2719(self):
        def do_test_write_byte_count_updated(self, filesize):
            name = 'issue_2719'
            ticket = 'ticket_' + name
            local_file = os.path.join(self.admin.local_session_dir, name + '.txt')
            logical_path = os.path.join(self.admin.session_collection, name)

            try:
                write_byte_limit = filesize * 2

                if not os.path.exists(local_file):
                    lib.make_file(local_file, filesize)

                # Create a new data object.
                self.admin.assert_icommand(['iput', local_file, logical_path])
                self.admin.assert_icommand(['iquest', '%s', "select DATA_SIZE where COLL_NAME = '{0}' and DATA_NAME = '{1}'".format(
                                           os.path.dirname(logical_path), os.path.basename(logical_path))],
                                           'STDOUT', str(filesize))

                # Create a ticket for writing to the data object.
                self.admin.assert_icommand(['iticket', 'create', 'write', logical_path, ticket])
                self.admin.assert_icommand(['iticket', 'mod', ticket, 'write-bytes', str(write_byte_limit)])

                # verify ticket information in the catalog
                out, err, ec = self.admin.run_icommand(['iquest', '%s...%s', "select TICKET_WRITE_BYTE_COUNT, TICKET_WRITE_BYTE_LIMIT where TICKET_STRING = '{}'".format(ticket)])
                self.assertEqual(ec, 0)
                self.assertEqual(len(err), 0)

                write_byte_count = out.split('...')[0]
                stored_write_byte_limit = out.split('...')[1]
                self.assertEqual(int(write_byte_count), 0)
                self.assertEqual(int(stored_write_byte_limit), write_byte_limit)

                # overwrite with the same file and ensure that the write byte count updates
                self.user.assert_icommand(['iput', '-f', '-t', ticket, local_file, logical_path])
                out, err, ec = self.admin.run_icommand(['iquest', '%s...%s', "select TICKET_WRITE_FILE_COUNT, TICKET_WRITE_BYTE_COUNT where TICKET_STRING = '{}'".format(ticket)])
                self.assertEqual(ec, 0)
                self.assertEqual(len(err), 0)

                write_file_count = out.split('...')[0]
                write_byte_count = out.split('...')[1]
                self.assertEqual(int(write_file_count), 1)
                self.assertEqual(int(write_byte_count), filesize)

            finally:
                self.admin.run_icommand(['iticket', 'delete', ticket])
                self.admin.run_icommand(['irm', '-f', logical_path])
                if os.path.exists(local_file):
                    os.unlink(local_file)

        do_test_write_byte_count_updated(self, 1024) # 1 KiB
        do_test_write_byte_count_updated(self, 40 * 1024 * 1024) #40 MiB

    def test_iticket_allows_rodsadmins_to_delete_tickets_created_by_other_users__issue_5933(self):
        # Create a ticket for a data object.
        data_object = 'foo.issue_5933'
        self.user.assert_icommand(['itouch', data_object])
        _, out, _ = self.user.assert_icommand(['iticket', 'create', 'read', data_object], 'STDOUT', ['ticket:'])

        # Capture the actual ticket value.
        ticket_string = out.strip().split(':')[1]

        # Show that without -M (i.e. the admin flag), no user can delete another user's ticket.
        self.admin.assert_icommand(['iticket', 'delete', ticket_string], 'STDERR', ['-890000 CAT_TICKET_INVALID'])

        # Show that including -M allows a rodsadmin to delete tickets created by other users.
        self.admin.assert_icommand(['iticket', '-M', 'delete', ticket_string])

        # Show that the ticket was actually deleted.
        self.user.assert_icommand(['iticket', 'ls', ticket_string])
        self.user.assert_icommand(['iticket', 'ls'])

    def test_iticket_allows_rodsadmins_to_modify_tickets_created_by_other_users__issue_5933(self):
        # Create a ticket for a data object.
        data_object = 'foo.issue_5933'
        self.user.assert_icommand(['itouch', data_object])
        _, out, _ = self.user.assert_icommand(['iticket', 'create', 'read', data_object], 'STDOUT', ['ticket:'])

        # Capture the actual ticket value.
        ticket_string = out.strip().split(':')[1]

        # Change the use limit.
        self.user.assert_icommand(['iticket', 'ls', ticket_string], 'STDOUT', ['uses limit: 0'])
        self.admin.assert_icommand(['iticket', '-M', 'mod', ticket_string, 'uses', '999'])
        self.user.assert_icommand(['iticket', 'ls', ticket_string], 'STDOUT', ['uses limit: 999'])

        # Change the expiration timestamp.
        self.user.assert_icommand(['iticket', 'ls', ticket_string], 'STDOUT', ['expire time: none'])
        self.admin.assert_icommand(['iticket', '-M', 'mod', ticket_string, 'expire', '2100-01-01.23:00:00'])
        self.user.assert_icommand(['iticket', 'ls', ticket_string], 'STDOUT', ['expire time: 2100-01-01.23:00:00'])

        # Change the expiration timestamp using seconds since epoch.
        self.admin.assert_icommand(['iticket', '-M', 'mod', ticket_string, 'expire', '7258201200'])
        self.user.assert_icommand(['iticket', 'ls', ticket_string], 'STDOUT', ['expire time: 2200-01-01.23:00:00'])

        # Change the write-file limit.
        self.user.assert_icommand(['iticket', 'ls', ticket_string], 'STDOUT', ['write file limit: 10'])
        self.admin.assert_icommand(['iticket', '-M', 'mod', ticket_string, 'write-file', '999'])
        self.user.assert_icommand(['iticket', 'ls', ticket_string], 'STDOUT', ['write file limit: 999'])

        # Change the write-bytes limit.
        self.user.assert_icommand(['iticket', 'ls', ticket_string], 'STDOUT', ['write byte limit: 0'])
        self.admin.assert_icommand(['iticket', '-M', 'mod', ticket_string, 'write-bytes', '999'])
        self.user.assert_icommand(['iticket', 'ls', ticket_string], 'STDOUT', ['write byte limit: 999'])

        # Add an allowed user to the ticket.
        self.user.assert_icommand(['iticket', 'ls', ticket_string], 'STDOUT', ['No user restrictions'])
        self.admin.assert_icommand(['iticket', '-M', 'mod', ticket_string, 'add', 'user', self.admin.username])
        self.user.assert_icommand(['iticket', 'ls', ticket_string], 'STDOUT', ['restricted-to user: ' + self.admin.username])

        # Remove the allowed user from the ticket.
        self.admin.assert_icommand(['iticket', '-M', 'mod', ticket_string, 'remove', 'user', self.admin.username])
        self.user.assert_icommand(['iticket', 'ls', ticket_string], 'STDOUT', ['No user restrictions'])

        # Add an allowed host to the ticket.
        self.user.assert_icommand(['iticket', 'ls', ticket_string], 'STDOUT', ['No host restrictions'])
        self.admin.assert_icommand(['iticket', '-M', 'mod', ticket_string, 'add', 'host', 'irods.org'])
        # We don't check the value associated with the host value because the server resolves the hostname
        # to an IP address and IP addresses can change.
        self.user.assert_icommand(['iticket', 'ls', ticket_string], 'STDOUT', ['restricted-to host: '])

        # Remove the allowed host from the ticket.
        self.admin.assert_icommand(['iticket', '-M', 'mod', ticket_string, 'remove', 'host', 'irods.org'])
        self.user.assert_icommand(['iticket', 'ls', ticket_string], 'STDOUT', ['No host restrictions'])

        # Add an allowed group to the ticket.
        self.user.assert_icommand(['iticket', 'ls', ticket_string], 'STDOUT', ['No group restrictions'])
        self.admin.assert_icommand(['iticket', '-M', 'mod', ticket_string, 'add', 'group', 'rodsadmin'])
        self.user.assert_icommand(['iticket', 'ls', ticket_string], 'STDOUT', ['restricted-to group: rodsadmin'])

        # Remove the allowed group from the ticket.
        self.admin.assert_icommand(['iticket', '-M', 'mod', ticket_string, 'remove', 'group', 'rodsadmin'])
        self.user.assert_icommand(['iticket', 'ls', ticket_string], 'STDOUT', ['No group restrictions'])

