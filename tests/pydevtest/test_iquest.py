from nose.plugins.skip import SkipTest
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd
import pydevtest_sessions as s
import commands

class Test_iquest(object):

    ###################
    # iquest
    ###################

    def test_iquest_totaldatasize(self):
        assertiCmd(s.adminsession,"iquest \"select sum(DATA_SIZE) where COLL_NAME like '/tempZone/home/%'\"","LIST","DATA_SIZE") # selects total data size

    def test_iquest_bad_format(self):
        assertiCmd(s.adminsession,"iquest \"bad formatting\"","ERROR","INPUT_ARG_NOT_WELL_FORMED_ERR") # bad request

