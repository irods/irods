import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

from . import session
from . import resource_suite

#class Test_IUserinfo(SessionsMixin, unittest.TestCase):
class Test_IUserinfo(resource_suite.ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_IUserinfo, self).setUp()

        # create a remote user with the same name as a local one
        self.admin_session = session.make_session_for_existing_admin()
        test_session = self.admin_session
        self.local_user = test_session.username
        self.local_zone = test_session.zone_name
        self.remote_user = test_session.username
        self.remote_zone = 'remoteZone'

        self.admin_session.assert_icommand(['iadmin', 'mkzone', self.remote_zone, 'remote'])
        self.admin_session.assert_icommand(['iadmin', 'mkuser', '{self.remote_user}#{self.remote_zone}'.format(**locals()), 'rodsadmin'])

    def tearDown(self):
        self.admin_session.assert_icommand(['iadmin', 'rmuser', '{self.remote_user}#{self.remote_zone}'.format(**locals())])
        self.admin_session.assert_icommand(['iadmin', 'rmzone', self.remote_zone])
        super(Test_IUserinfo, self).tearDown()

    def test_iuserinfo_with_zone(self):
        test_session = self.admin_session

        # STDOUT_MULTILINE requires ordered output which is not guaranteed,
        # so capture output in a string and compare against expected output
        expected_groups = ['member of group: rods', 'member of group: public']
        _,out,_ = test_session.assert_icommand(['iuserinfo', '{self.local_user}#{self.local_zone}'.format(**locals())], 'STDOUT', ['zone: ' + self.local_zone])
        self.assertTrue(self.local_zone in out)
        self.assertTrue(self.remote_zone not in out)
        for group in expected_groups:
            self.assertTrue(group in out)

        _,out,_ = test_session.assert_icommand(['iuserinfo', '{self.remote_user}#{self.remote_zone}'.format(**locals())], 'STDOUT', ['zone: ' + self.remote_zone])
        self.assertTrue(expected_groups[-1] not in out)
        self.assertTrue(self.local_zone not in out)
        self.assertTrue(self.remote_zone in out)
        for group in expected_groups[:-1]:
            self.assertTrue(group in out)

    def test_iuserinfo_no_zone(self):
        test_session = self.admin_session
        user = test_session.username
        zone = test_session.zone_name
        _,out,_ = test_session.assert_icommand(['iuserinfo', user], 'STDOUT', 'name: ' +user)
        self.assertTrue(zone in out)
        self.assertTrue(self.remote_zone not in out)

    def test_iuserinfo_no_input(self):
        test_session = self.admin_session
        user = test_session.username
        zone = test_session.zone_name
        _,out,_ = test_session.assert_icommand(['iuserinfo'], 'STDOUT', 'name: ' + user)
        self.assertTrue(zone in out)
        self.assertTrue(self.remote_zone not in out)

    def test_iuserinfo_invalid_user_and_zone(self):
        test_session = self.admin_session
        test_session.assert_icommand(['iuserinfo', 'invalidUser#{self.local_zone}'.format(**locals())], 'STDOUT',
                                     'User invalidUser#{self.local_zone} does not exist.'.format(**locals()))
        test_session.assert_icommand(['iuserinfo', '{self.local_user}#invalidZone'.format(**locals())], 'STDOUT',
                                     'User {self.local_user}#invalidZone does not exist.'.format(**locals()))

    def test_iuserinfo_bad_input(self):
        self.admin_session.assert_icommand(['iuserinfo', 'this#should#fail'], 'STDOUT', 'Failed parsing input')

    def test_iuserinfo_group(self):
        self.admin_session.assert_icommand(['iuserinfo', 'public'], 'STDOUT', 'member of group: public')

    def test_iuserinfo_does_not_cause_an_error_when_the_username_includes_the_at_symbol__issue_5467(self):
        username = 'yak@shaving.com'

        try:
            user = session.mkuser_and_return_session('rodsuser', username, 'rods', 'localhost')
            user.assert_icommand(['iuserinfo'], 'STDOUT', 'name: ' + username)
            user.assert_icommand(['iuserinfo', username], 'STDOUT', 'name: ' + username)
        finally:
            user.assert_icommand(['irm', '-rf', user.session_collection])
            self.admin_session.assert_icommand(['iadmin', 'rmuser', username])
