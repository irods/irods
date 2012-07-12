import sessions as s
from nose import with_setup
from zonetests_common import assertiCmd, assertiCmdFail


@with_setup(s.admin_session_up,s.admin_session_down)
def test_attempt_bad_icommands():
  assertiCmdFail(s.adminsession,"idoesnotexist","LIST","nope")

@with_setup(s.admin_session_up,s.admin_session_down)
def test_bad_test_formatting():
  assertiCmdFail(s.adminsession,"ils -L","badformat")

#@with_setup(three_sessions_up,three_sessions_down)
#def test_user1put_with_user2get():
#  # local setup
#  datafilename = "twousers.txt"
#  output = commands.getstatusoutput( 'echo "i am a twouser testfile" > '+datafilename )
#  # assertions
#  assertiCmdFail(gardenuser1session,"ils -L","LIST",datafilename) # should not be listed yet
#  assertiCmd(gardenuser1session,"iput -fK "+datafilename) # iput
#  assertiCmd(gardenuser1session,"ils -LA "+datafilename,"LIST",datafilename) # should be listed
#  assertiCmd(gardenuser2session,"icd") # home directory
#  assertiCmd(gardenuser2session,"icd ../gardenuser1/"+garden1sessionid) # common directory
#  assertiCmd(gardenuser2session,"ils -LA "+datafilename,"LIST",datafilename) # should be listed
#  assertiCmdFail(gardenuser2session,"iget -Kf "+datafilename) # iget should fail (no permissions)
#  assertiCmd(gardenuser1session,"ichmod read gardenuser2 "+datafilename) # ichmod
#  assertiCmd(gardenuser2session,"iget -Kf "+datafilename) # iget
#  # local cleanup
#  output = commands.getstatusoutput( 'rm '+datafilename )
#
#@with_setup(three_sessions_up,three_sessions_down)
#def test_user1put_with_user2ACLCheck():
#  # local setup
#  datafilename = "twousersACL.txt"
#  output = commands.getstatusoutput( 'echo "i am a twouser testfile" > '+datafilename )
#  # assertions
#  assertiCmdFail(gardenuser1session,"ils -L","LIST",datafilename) # should not be listed yet
#  assertiCmd(gardenuser1session,"iput -fK "+datafilename) # iput
#  assertiCmd(gardenuser1session,"ils -LA "+datafilename,"LIST",datafilename) # should be listed
#  assertiCmd(gardenuser2session,"icd") # home directory
#  assertiCmd(gardenuser2session,"icd ../gardenuser1/"+garden1sessionid) # common directory
#  assertiCmd(gardenuser2session,"ils -LA "+datafilename,"LIST",datafilename) # should be listed
#  assertiCmdFail(gardenuser2session,"ils -LA "+datafilename,"LIST","read object") # should fail, no read perms
#  assertiCmd(gardenuser1session,"ichmod read gardenuser2 "+datafilename) # ichmod
#  assertiCmd(gardenuser2session,"ils -LA "+datafilename,"LIST","read object") # should have read perms
#  # local cleanup                                                                              
#  output = commands.getstatusoutput( 'rm '+datafilename )
#
