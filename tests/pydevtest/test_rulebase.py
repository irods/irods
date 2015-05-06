import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import os
import socket
import time  # remove once file hash fix is commited #2279
import lib

from resource_suite import ResourceBase


class Test_Rulebase(ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_Rulebase, self).setUp()

    def tearDown(self):
        super(Test_Rulebase, self).tearDown()

    def test_acPostProcForPut_replicate_to_multiple_resources(self):
        # create new resources
        hostname = socket.gethostname()
        self.admin.assert_icommand("iadmin mkresc r1 unixfilesystem " + hostname + ":/tmp/irods/r1", 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand("iadmin mkresc r2 unixfilesystem " + hostname + ":/tmp/irods/r2", 'STDOUT_SINGLELINE', "Creating")

        # save original core.re
        corefile = os.path.join(lib.get_core_re_dir(), "core.re")
        origcorefile = os.path.join(lib.get_core_re_dir(), "core.re.orig")
        os.system("cp "+corefile+" "+origcorefile)

        # add acPostProcForPut replication rule
        part1="sed -e '/^acPostProcForPut/i acPostProcForPut { replicateMultiple( \"r1,r2\" ); }' "
        part2=part1+corefile+' > '+origcorefile
        os.system(part2)
        time.sleep(1)  # remove once file hash fix is commited #2279
        os.system("cp "+origcorefile+" "+corefile) 
        time.sleep(1)  # remove once file hash fix is commited #2279

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
        with open('/tmp/irods/newrule', 'w') as f:
            f.write(newrule)
        os.system("cat "+corefile+" /tmp/irods/newrule > "+origcorefile)
        time.sleep(1)  # remove once file hash fix is commited #2279
        os.system("cp "+origcorefile+" "+corefile) 
        time.sleep(1)  # remove once file hash fix is commited #2279

        # put data
        tfile = "rulebasetestfile"
        os.system("touch " + tfile)
        self.admin.assert_icommand("iput " + tfile)

        # check replicas
        self.admin.assert_icommand("ils -L " + tfile, 'STDOUT_SINGLELINE', " demoResc ")
        self.admin.assert_icommand("ils -L " + tfile, 'STDOUT_SINGLELINE', " r1 ")
        self.admin.assert_icommand("ils -L " + tfile, 'STDOUT_SINGLELINE', " r2 ")

        # clean up and remove new resources
        self.admin.assert_icommand("irm -rf " + tfile)
        self.admin.assert_icommand("iadmin rmresc r1")
        self.admin.assert_icommand("iadmin rmresc r2")

        # restore core.re
        time.sleep(1)  # remove once file hash fix is commited #2279
        os.system("cp " + origcorefile + " " + corefile)
        time.sleep(1)  # remove once file hash fix is commited #2279

    def test_dynamic_pep_with_rscomm_usage(self):
        # save original core.re
        corefile = os.path.join(lib.get_core_re_dir(), "core.re")
        origcorefile = os.path.join(lib.get_core_re_dir(), "core.re.orig")
        os.system("cp "+corefile+" "+origcorefile)

        # add dynamic PEP with rscomm usage
        time.sleep(1)  # remove once file hash fix is commited #2279
        os.system('''echo "pep_resource_open_pre(*OUT) { msiGetSystemTime( *junk, '' ); }" >> '''+corefile)
        time.sleep(1)  # remove once file hash fix is commited #2279

        # check rei functioning
        self.admin.assert_icommand("iget " + self.testfile + " - ", 'STDOUT_SINGLELINE', self.testfile)

        # restore core.re
        time.sleep(1)  # remove once file hash fix is commited #2279
        os.system("cp "+origcorefile+" "+corefile)
        time.sleep(1)  # remove once file hash fix is commited #2279
