import sys
if (sys.version_info >= (2,7)):
    import unittest
else:
    import unittest2 as unittest
from resource_suite import ResourceBase
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd
import pydevtest_sessions as s
import commands
import os
import socket

class Test_RulebaseSuite(unittest.TestCase, ResourceBase):

    my_test_resource = {"setup":[],"teardown":[]}

    def setUp(self):
        ResourceBase.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

    def test_acPostProcForPut_replicate_to_multiple_resources(self):
        # create new resources
        hostname = socket.gethostname()
        assertiCmd(s.adminsession,"iadmin mkresc r1 unixfilesystem "+hostname+":/tmp/irods/r1", "STDOUT", "Creating")
        assertiCmd(s.adminsession,"iadmin mkresc r2 unixfilesystem "+hostname+":/tmp/irods/r2", "STDOUT", "Creating")

        # save original core.re
        os.system("cp /etc/irods/core.re /etc/irods/core.re.orig")

        # add acPostProcForPut replication rule
        os.system('''sed -e '/^acPostProcForPut/i acPostProcForPut { replicateMultiple( "r1,r2" ); }' /etc/irods/core.re > /tmp/irods/core.re''')
        os.system("cp /tmp/irods/core.re /etc/irods/core.re")

        # add new rule to end of core.re
        newrule = """

                        # multiple replication rule
                        replicateMultiple(*destRgStr) {
                            *destRgList = split(*destRgStr, ',');
                                writeLine("serverLog", " acPostProcForPut multiple replicate $objPath $filePath -> *destRgStr");
                            foreach (*destRg in *destRgList) {
                                writeLine("serverLog", " acPostProcForPut replicate $objPath $filePath -> *destRg");
                                *e = errorcode(msiSysReplDataObj(*destRg,"null"));
                                if (*e != 0) {
                                if(*e == -808000) {
                                    writeLine("serverLog", "$objPath cannot be found");
                                    $status = 0;
                                    succeed;
                                } else {
                                    fail(*e);
                                }
                                }
                            }
                        }

                """
        f = open("/tmp/irods/newrule","w")
        f.write(newrule)
        f.close()
        os.system("cat /etc/irods/core.re /tmp/irods/newrule > /tmp/irods/core.re")
        os.system("cp /tmp/irods/core.re /etc/irods/core.re")

        # put data
        tfile = "rulebasetestfile"
        os.system("touch "+tfile)
        assertiCmd(s.adminsession,"iput "+tfile)

        # check replicas
        assertiCmd(s.adminsession,"ils -L "+tfile,"STDOUT"," demoResc ")
        assertiCmd(s.adminsession,"ils -L "+tfile,"STDOUT"," r1 ")
        assertiCmd(s.adminsession,"ils -L "+tfile,"STDOUT"," r2 ")

        # clean up and remove new resources
        assertiCmd(s.adminsession,"irm -rf "+tfile)
        assertiCmd(s.adminsession,"iadmin rmresc r1")
        assertiCmd(s.adminsession,"iadmin rmresc r2")

        # restore core.re
        os.system("cp /etc/irods/core.re.orig /etc/irods/core.re")