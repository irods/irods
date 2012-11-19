import pydevtest_sessions as s
from nose.tools import with_setup
from nose.plugins.skip import SkipTest
from pydevtest_common import assertiCmd, assertiCmdFail
import commands

@with_setup(s.adminonly_up,s.adminonly_down)
def test_iquest_totaldatasize():
    assertiCmd(s.adminsession,"iquest \"select sum(DATA_SIZE) where COLL_NAME like '/tempZone/home/%'\"","LIST","DATA_SIZE") # selects total data size
    
@with_setup(s.adminonly_up,s.adminonly_down)
def test_iquest_bad_format():
    assertiCmd(s.adminsession,"iquest \"bad formatting\"","ERROR","INPUT_ARG_NOT_WELL_FORMED_ERR") # bad request
