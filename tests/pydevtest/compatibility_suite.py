import sys
import string
import os
if (sys.version_info >= (2, 7)):
    import unittest
else:
    import unittest2 as unittest
from resource_suite import ResourceBase
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd, getiCmdOutput
import pydevtest_sessions as s
import commands
import psutil


class Test_CompatibilitySuite(unittest.TestCase, ResourceBase):

    my_test_resource = {"setup": [], "teardown": []}

    def setUp(self):
        ResourceBase.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

    def test_imeta_set(self):
        assertiCmd(s.adminsession, 'iadmin lu', 'STDOUT', 'rods')
        assertiCmd(s.adminsession, 'imeta ls -u ' + s.sessions[1].username + ' att', 'STDOUT', 'None')
        assertiCmd(s.adminsession, 'imeta add -u ' + s.sessions[1].username + ' att val')
        assertiCmd(s.adminsession, 'imeta ls -u ' + s.sessions[1].username + ' att', 'STDOUT', 'attribute: att')
        assertiCmd(s.adminsession, 'imeta set -u ' + s.sessions[1].username + ' att newval')
        assertiCmd(s.adminsession, 'imeta ls -u ' + s.sessions[1].username + ' att', 'STDOUT', 'value: newval')
        assertiCmd(s.adminsession, 'imeta set -u ' + s.sessions[1].username + ' att newval someunit')
        assertiCmd(s.adminsession, 'imeta ls -u ' + s.sessions[1].username + ' att', 'STDOUT', 'units: someunit')
        assertiCmd(s.adminsession, 'imeta set -u ' + s.sessions[1].username + ' att verynewval')
        assertiCmd(s.adminsession, 'imeta ls -u ' + s.sessions[1].username + ' att', 'STDOUT', 'value: verynewval')
        assertiCmd(s.adminsession, 'imeta ls -u ' + s.sessions[1].username + ' att', 'STDOUT', 'units: someunit')

    def test_iphybun_n(self):
        assertiCmd(s.adminsession, "imkdir testColl")
        assertiCmd(s.adminsession, "icd testColl")
        filenames = []
        for i in range(0, 8):
            f = "empty" + str(i) + ".txt"
            filenames.append(f)
            pydevtest_common.cat(f, str(i))
            assertiCmd(s.adminsession, "iput " + f)
            os.unlink(f)
        assertiCmd(s.adminsession, "icd ..")
        assertiCmd(s.adminsession, "iphybun -N3 -SdemoResc -RdemoResc testColl")
        coll_dir = getiCmdOutput(
            s.adminsession, "ils /tempZone/bundle/home/rods")[0].split('\n')[-2].lstrip(string.printable.translate(None, "/"))
        after = getiCmdOutput(s.adminsession, "ils " + coll_dir)
        after = after[0].split('\n')
        assert(len(after) == 2 + 3)
        assertiCmd(s.adminsession, "irm -rf testColl")
