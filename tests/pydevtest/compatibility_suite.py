import sys
if (sys.version_info >= (2,7)):
  import unittest
else:
  import unittest2 as unittest
from resource_suite import ResourceBase
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd
import pydevtest_sessions as s
import commands

class Test_CompatibilitySuite(unittest.TestCase, ResourceBase):

  my_test_resource = {"setup":[],"teardown":[]}

  def setUp(self):
    ResourceBase.__init__(self)
    s.twousers_up()
    self.run_resource_setup()

  def tearDown(self):
    self.run_resource_teardown()
    s.twousers_down()

  def test_imeta_set(self):
    assertiCmd(s.adminsession,"iadmin lu","LIST","rods")
    assertiCmd(s.adminsession,"imeta ls -u " + s.users[1]["name"] + " att", "LIST", "None")
    assertiCmd(s.adminsession,"imeta add -u " + s.users[1]["name"] + " att val")
    assertiCmd(s.adminsession,"imeta ls -u " + s.users[1]["name"] + " att","LIST","attribute: att")
    assertiCmd(s.adminsession,"imeta set -u " + s.users[1]["name"] + " att newval")
    assertiCmd(s.adminsession,"imeta ls -u " + s.users[1]["name"] + " att","LIST","value: newval")
    assertiCmd(s.adminsession,"imeta set -u " + s.users[1]["name"] + " att newval someunit")
    assertiCmd(s.adminsession,"imeta ls -u " + s.users[1]["name"] + " att","LIST","units: someunit")
    assertiCmd(s.adminsession,"imeta set -u " + s.users[1]["name"] + " att verynewval")
    assertiCmd(s.adminsession,"imeta ls -u " + s.users[1]["name"] + " att","LIST","value: verynewval")
    assertiCmd(s.adminsession,"imeta ls -u " + s.users[1]["name"] + " att","LIST","units: someunit")
