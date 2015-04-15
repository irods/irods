import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import commands
import os
import stat
import datetime
import time
import shutil
import random
import subprocess

from resource_suite import ResourceBase
import lib

class ChunkyDevTest(ResourceBase):

    def test_beginning_from_devtest(self):
        # build expected variables with similar devtest names
        progname = __file__
        myssize = str(os.stat(progname).st_size)
        username = self.admin.username
        irodszone = self.admin.zone_name
        testuser1 = self.user0.username
        irodshome = self.admin.session_collection
        irodsdefresource = self.admin.default_resource
        dir_w = "."
        sfile2 = dir_w + "/sfile2"
        commands.getstatusoutput("cat " + progname + " " + progname + " > " + sfile2)
        mysdir = "/tmp/irodssdir"
        myldir = dir_w + "/ldir"
        if os.path.exists(myldir):
            shutil.rmtree(myldir)
        self.admin.assert_icommand("imkdir " + irodshome + "/icmdtest")

        # test basic informational commands
        self.admin.assert_icommand("iinit -l", 'STDOUT', self.admin.username)
        self.admin.assert_icommand("iinit -l", 'STDOUT', self.admin.zone_name)
        self.admin.assert_icommand("iinit -l", 'STDOUT', self.admin.default_resource)
        res = self.admin.run_icommand(['ils', '-V'])
        assert res[1].count('irods_host') == 1
        assert res[1].count('irods_port') == 1
        assert res[1].count('irods_default_resource') == 1

        # begin original devtest
        self.admin.assert_icommand("ilsresc", 'STDOUT', self.testresc)
        self.admin.assert_icommand("ilsresc -l", 'STDOUT', self.testresc)
        self.admin.assert_icommand("imiscsvrinfo", 'STDOUT', ["relVersion"])
        self.admin.assert_icommand("iuserinfo", 'STDOUT', "name: " + username)
        self.admin.assert_icommand("ienv", 'STDOUT', "irods_zone")
        self.admin.assert_icommand("ipwd", 'STDOUT', "home")
        self.admin.assert_icommand("ihelp ils", 'STDOUT', "ils")
        self.admin.assert_icommand("ierror -14000", 'STDOUT', "SYS_API_INPUT_ERR")
        self.admin.assert_icommand("iexecmd hello", 'STDOUT', "Hello world")
        self.admin.assert_icommand("ips -v", 'STDOUT', "ips")
        self.admin.assert_icommand("iqstat", 'STDOUT', "No delayed rules pending for user " + self.admin.username)

        # put and list basic file information
        self.admin.assert_icommand("ils -AL", 'STDOUT', "home")  # debug
        self.admin.assert_icommand("iput -K --wlock " + progname + " " + irodshome + "/icmdtest/foo1")
        self.admin.assert_icommand("ichksum -f " + irodshome + "/icmdtest/foo1", 'STDOUT', "performed = 1")
        self.admin.assert_icommand("iput -kf " + progname + " " + irodshome + "/icmdtest/foo1")
        self.admin.assert_icommand("ils " + irodshome + "/icmdtest/foo1", 'STDOUT', "foo1")
        self.admin.assert_icommand("ils -l " + irodshome + "/icmdtest/foo1", 'STDOUT', ["foo1", myssize])
        self.admin.assert_icommand("iadmin ls " + irodshome + "/icmdtest", 'STDOUT', "foo1")
        self.admin.assert_icommand("ils -A " + irodshome + "/icmdtest/foo1",
                   'STDOUT', username + "#" + irodszone + ":own")
        self.admin.assert_icommand("ichmod read " + testuser1 + " " + irodshome + "/icmdtest/foo1")
        self.admin.assert_icommand("ils -A " + irodshome + "/icmdtest/foo1",
                   'STDOUT', testuser1 + "#" + irodszone + ":read")
        # basic replica
        self.admin.assert_icommand("irepl -B -R " + self.testresc + " --rlock " + irodshome + "/icmdtest/foo1")
        self.admin.assert_icommand("ils -l " + irodshome + "/icmdtest/foo1", 'STDOUT', self.testresc)

        # overwrite a copy
        self.admin.assert_icommand("itrim -S " + irodsdefresource + " -N1 " + irodshome + "/icmdtest/foo1")
        self.admin.assert_icommand_fail("ils -L " + irodshome + "/icmdtest/foo1", 'STDOUT', [irodsdefresource])
        self.admin.assert_icommand("iphymv -R " + irodsdefresource + " " + irodshome + "/icmdtest/foo1")
        self.admin.assert_icommand("ils -l " + irodshome + "/icmdtest/foo1", 'STDOUT', irodsdefresource[0:19])
        # basic metadata shuffle
        self.admin.assert_icommand("imeta add -d " + irodshome + "/icmdtest/foo1 testmeta1 180 cm")
        self.admin.assert_icommand("imeta ls -d " + irodshome + "/icmdtest/foo1", 'STDOUT', ["testmeta1"])
        self.admin.assert_icommand("imeta ls -d " + irodshome + "/icmdtest/foo1", 'STDOUT', ["180"])
        self.admin.assert_icommand("imeta ls -d " + irodshome + "/icmdtest/foo1", 'STDOUT', ["cm"])
        self.admin.assert_icommand("icp -K -R " + self.testresc + " " +
                   irodshome + "/icmdtest/foo1 " + irodshome + "/icmdtest/foo2")

        # test imeta -v
        imeta_popen = subprocess.Popen(
            'echo "ls -d ' + irodshome + '/icmdtest/foo1" | imeta -v', shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        imeta_output, imeta_err = imeta_popen.communicate()
        assert imeta_output.find('testmeta1') > -1
        assert imeta_output.find('180') > -1
        assert imeta_output.find('cm') > -1

        # new file mode check
        self.admin.assert_icommand("iget -fK --rlock " + irodshome + "/icmdtest/foo2 /tmp/")
        assert oct(stat.S_IMODE(os.stat("/tmp/foo2").st_mode)) == '0640'
        os.unlink("/tmp/foo2")

        self.admin.assert_icommand("ils " + irodshome + "/icmdtest/foo2", 'STDOUT', "foo2")
        self.admin.assert_icommand("imv " + irodshome + "/icmdtest/foo2 " + irodshome + "/icmdtest/foo4")
        self.admin.assert_icommand("ils -l " + irodshome + "/icmdtest/foo4", 'STDOUT', "foo4")
        self.admin.assert_icommand("imv " + irodshome + "/icmdtest/foo4 " + irodshome + "/icmdtest/foo2")
        self.admin.assert_icommand("ils -l " + irodshome + "/icmdtest/foo2", 'STDOUT', "foo2")
        self.admin.assert_icommand("ichksum " + irodshome + "/icmdtest/foo2", 'STDOUT', "foo2")
        self.admin.assert_icommand("imeta add -d " + irodshome + "/icmdtest/foo2 testmeta1 180 cm")
        self.admin.assert_icommand("imeta add -d " + irodshome + "/icmdtest/foo1 testmeta2 hello")
        self.admin.assert_icommand("imeta ls -d " + irodshome + "/icmdtest/foo1", 'STDOUT', ["testmeta1"])
        self.admin.assert_icommand("imeta ls -d " + irodshome + "/icmdtest/foo1", 'STDOUT', ["hello"])
        self.admin.assert_icommand("imeta qu -d testmeta1 = 180", 'STDOUT', "foo1")
        self.admin.assert_icommand("imeta qu -d testmeta2 = hello", 'STDOUT', "dataObj: foo1")
        self.admin.assert_icommand("iget -f -K --rlock " + irodshome + "/icmdtest/foo2 " + dir_w)
        assert myssize == str(os.stat(dir_w + "/foo2").st_size)
        os.unlink(dir_w + "/foo2")

        # we have foo1 in $irodsdefresource and foo2 in testresource

        # cleanup
        os.unlink(sfile2)

    def test_iput_ibun_gzip_bzip2_from_devtest(self):
        # build expected variables with similar devtest names
        with lib.make_session_for_existing_admin() as rods_admin:
            rods_admin.assert_icommand(['ichmod', 'own', self.admin.username, '/' + self.admin.zone_name])

        progname = __file__
        myssize = str(os.stat(progname).st_size)
        username = self.admin.username
        irodszone = self.admin.zone_name
        testuser1 = self.user0.username
        irodshome = self.admin.session_collection
        irodsdefresource = self.admin.default_resource
        dir_w = "."
        sfile2 = dir_w + "/sfile2"
        commands.getstatusoutput("cat " + progname + " " + progname + " > " + sfile2)
        mysdir = "/tmp/irodssdir"
        myldir = dir_w + "/ldir"
        if os.path.exists(myldir):
            shutil.rmtree(myldir)
        self.admin.assert_icommand("imkdir icmdtest")

        # make a directory containing 20 small files
        if not os.path.isdir(mysdir):
            os.mkdir(mysdir)
        for i in range(20):
            mysfile = mysdir + "/sfile" + str(i)
            shutil.copyfile(progname, mysfile)

        # we put foo1 in $irodsdefresource and foo2 in testresource
        self.admin.assert_icommand("iput -K --wlock " + progname + " " + irodshome + "/icmdtest/foo1")
        self.admin.assert_icommand("icp -K -R " + self.testresc + " " +
                   irodshome + "/icmdtest/foo1 " + irodshome + "/icmdtest/foo2")

        self.admin.assert_icommand("irepl -B -R " + self.testresc + " " + irodshome + "/icmdtest/foo1")
        phypath = dir_w + "/" + "foo1." + str(random.randrange(10000000))
        self.admin.assert_icommand("iput -kfR " + irodsdefresource + " " + sfile2 + " " + irodshome + "/icmdtest/foo1")
        # show have 2 different copies
        self.admin.assert_icommand("ils -l " + irodshome + "/icmdtest/foo1", 'STDOUT', ["foo1", myssize])
        self.admin.assert_icommand("ils -l " + irodshome + "/icmdtest/foo1",
                   'STDOUT', ["foo1", str(os.stat(sfile2).st_size)])
        # update all old copies
        self.admin.assert_icommand("irepl -U " + irodshome + "/icmdtest/foo1")
        # make sure the old size is not there
        self.admin.assert_icommand_fail("ils -l " + irodshome + "/icmdtest/foo1", 'STDOUT', myssize)
        self.admin.assert_icommand("itrim -S " + irodsdefresource + " " + irodshome + "/icmdtest/foo1")
        # bulk test
        self.admin.assert_icommand("iput -bIvPKr " + mysdir + " " + irodshome + "/icmdtest", 'STDOUT', "Bulk upload")
        # iput with a lot of options
        rsfile = dir_w + "/rsfile"
        if os.path.isfile(rsfile):
            os.unlink(rsfile)
        self.admin.assert_icommand("iput -PkITr -X " + rsfile + " --retries 10 " +
                   mysdir + " " + irodshome + "/icmdtestw", 'STDOUT', "Processing")
        self.admin.assert_icommand("imv " + irodshome + "/icmdtestw " + irodshome + "/icmdtestw1")
        self.admin.assert_icommand("ils -lr " + irodshome + "/icmdtestw1", 'STDOUT', "sfile10")
        self.admin.assert_icommand("ils -Ar " + irodshome + "/icmdtestw1", 'STDOUT', "sfile10")
        self.admin.assert_icommand("irm -rvf " + irodshome + "/icmdtestw1", 'STDOUT', "num files done")
        if os.path.isfile(rsfile):
            os.unlink(rsfile)
        self.admin.assert_icommand("iget -vIKPfr -X rsfile --retries 10 " +
                   irodshome + "/icmdtest " + dir_w + "/testx", 'STDOUT', "opened")
        if os.path.isfile(rsfile):
            os.unlink(rsfile)
        commands.getstatusoutput("tar -chf " + dir_w + "/testx.tar -C " + dir_w + "/testx .")
        self.admin.assert_icommand("iput " + dir_w + "/testx.tar " + irodshome + "/icmdtestx.tar")
        self.admin.assert_icommand("ibun -x " + irodshome + "/icmdtestx.tar " + irodshome + "/icmdtestx")
        self.admin.assert_icommand("ils -lr " + irodshome + "/icmdtestx", 'STDOUT', ["foo2"])
        self.admin.assert_icommand("ils -lr " + irodshome + "/icmdtestx", 'STDOUT', ["sfile10"])
        self.admin.assert_icommand("ibun -cDtar " + irodshome + "/icmdtestx1.tar " + irodshome + "/icmdtestx")
        self.admin.assert_icommand("ils -l " + irodshome + "/icmdtestx1.tar", 'STDOUT', "testx1.tar")
        if os.path.exists(dir_w + "/testx1"):
            shutil.rmtree(dir_w + "/testx1")
        os.mkdir(dir_w + "/testx1")
        if os.path.isfile(dir_w + "/testx1.tar"):
            os.unlink(dir_w + "/testx1.tar")
        self.admin.assert_icommand("iget " + irodshome + "/icmdtestx1.tar " + dir_w + "/testx1.tar")
        commands.getstatusoutput("tar -xvf " + dir_w + "/testx1.tar -C " + dir_w + "/testx1")
        output = commands.getstatusoutput("diff -r " + dir_w + "/testx " + dir_w + "/testx1/icmdtestx")
        print "output is [" + str(output) + "]"
        assert output[0] == 0
        assert output[1] == "", "diff output was not empty..."

        # test ibun with gzip
        self.admin.assert_icommand("ibun -cDgzip " + irodshome + "/icmdtestx1.tar.gz " + irodshome + "/icmdtestx")
        self.admin.assert_icommand("ibun -x " + irodshome + "/icmdtestx1.tar.gz " + irodshome + "/icmdtestgz")
        if os.path.isfile("icmdtestgz"):
            os.unlink("icmdtestgz")
        self.admin.assert_icommand("iget -vr " + irodshome + "/icmdtestgz " + dir_w + "", 'STDOUT', "icmdtestgz")
        output = commands.getstatusoutput("diff -r " + dir_w + "/testx " + dir_w + "/icmdtestgz/icmdtestx")
        print "output is [" + str(output) + "]"
        assert output[0] == 0
        assert output[1] == "", "diff output was not empty..."
        shutil.rmtree(dir_w + "/icmdtestgz")
        self.admin.assert_icommand("ibun --add " + irodshome + "/icmdtestx1.tar.gz " + irodshome + "/icmdtestgz")
        self.admin.assert_icommand("irm -rf " + irodshome + "/icmdtestx1.tar.gz " + irodshome + "/icmdtestgz")

        # test ibun with bzip2
        self.admin.assert_icommand("ibun -cDbzip2 " + irodshome + "/icmdtestx1.tar.bz2 " + irodshome + "/icmdtestx")
        self.admin.assert_icommand("ibun -xb " + irodshome + "/icmdtestx1.tar.bz2 " + irodshome + "/icmdtestbz2")
        if os.path.isfile("icmdtestbz2"):
            os.unlink("icmdtestbz2")
        self.admin.assert_icommand("iget -vr " + irodshome + "/icmdtestbz2 " + dir_w + "", 'STDOUT', "icmdtestbz2")
        output = commands.getstatusoutput("diff -r " + dir_w + "/testx " + dir_w + "/icmdtestbz2/icmdtestx")
        print "output is [" + str(output) + "]"
        assert output[0] == 0
        assert output[1] == "", "diff output was not empty..."
        shutil.rmtree(dir_w + "/icmdtestbz2")
        self.admin.assert_icommand("irm -rf " + irodshome + "/icmdtestx1.tar.bz2")
        self.admin.assert_icommand("iphybun -R " + self.anotherresc + " -Dbzip2 " + irodshome + "/icmdtestbz2")
        self.admin.assert_icommand("itrim -N1 -S " + self.testresc + " -r " + irodshome + "/icmdtestbz2", 'STDOUT', "Total size trimmed")
        self.admin.assert_icommand("itrim -N1 -S " + irodsdefresource + " -r " + irodshome + "/icmdtestbz2", 'STDOUT', "Total size trimmed")

        # get the name of bundle file
        output = commands.getstatusoutput(
            "ils -L " + irodshome + "/icmdtestbz2/icmdtestx/foo1 | tail -n1 | awk '{ print $NF }'")
        print output[1]
        bunfile = output[1]
        self.admin.assert_icommand("ils --bundle " + bunfile, 'STDOUT', "Subfiles")
        self.admin.assert_icommand("irm -rf " + irodshome + "/icmdtestbz2")
        self.admin.assert_icommand("irm -f --empty " + bunfile)

        # cleanup
        os.unlink(dir_w + "/testx1.tar")
        os.unlink(dir_w + "/testx.tar")
        shutil.rmtree(dir_w + "/testx1")
        shutil.rmtree(dir_w + "/testx")
        os.unlink(sfile2)
        if os.path.exists(myldir):
            shutil.rmtree(myldir)
        if os.path.exists(mysdir):
            shutil.rmtree(mysdir)

    def test_ireg_from_devtest(self):
        # build expected variables with similar devtest names
        progname = __file__
        myssize = str(os.stat(progname).st_size)
        username = self.admin.username
        irodszone = self.admin.zone_name
        testuser1 = self.user0.username
        irodshome = self.admin.session_collection
        irodsdefresource = self.admin.default_resource
        dir_w = "."
        sfile2 = dir_w + "/sfile2"
        commands.getstatusoutput("cat " + progname + " " + progname + " > " + sfile2)
        mysdir = "/tmp/irodssdir"
        myldir = dir_w + "/ldir"
        if os.path.exists(myldir):
            shutil.rmtree(myldir)
        self.admin.assert_icommand("imkdir icmdtest")

        # make a directory containing 20 small files
        if not os.path.isdir(mysdir):
            os.mkdir(mysdir)
        for i in range(20):
            mysfile = mysdir + "/sfile" + str(i)
            shutil.copyfile(progname, mysfile)

        commands.getstatusoutput("mv " + sfile2 + " /tmp/sfile2")
        commands.getstatusoutput("cp /tmp/sfile2 /tmp/sfile2r")
        # <-- FAILING - REASON FOR SKIPPING
        self.admin.assert_icommand("ireg -KR " + self.testresc + " /tmp/sfile2 " + irodshome + "/foo5")
        commands.getstatusoutput("cp /tmp/sfile2 /tmp/sfile2r")
        self.admin.assert_icommand("ireg -KR " + self.anotherresc + " --repl /tmp/sfile2r  " + irodshome + "/foo5")
        self.admin.assert_icommand("iget -fK " + irodshome + "/foo5 " + dir_w + "/foo5")
        output = commands.getstatusoutput("diff /tmp/sfile2  " + dir_w + "/foo5")
        print "output is [" + str(output) + "]"
        assert output[0] == 0
        assert output[1] == "", "diff output was not empty..."
        self.admin.assert_icommand("ireg -KCR " + self.testresc + " " + mysdir + " " + irodshome + "/icmdtesta")
        if os.path.exists(dir_w + "/testa"):
            shutil.rmtree(dir_w + "/testa")
        self.admin.assert_icommand("iget -fvrK " + irodshome + "/icmdtesta " + dir_w + "/testa", 'STDOUT', "testa")
        output = commands.getstatusoutput("diff -r " + mysdir + " " + dir_w + "/testa")
        print "output is [" + str(output) + "]"
        assert output[0] == 0
        assert output[1] == "", "diff output was not empty..."
        shutil.rmtree(dir_w + "/testa")
        # test ireg with normal user
        testuser2home = "/" + irodszone + "/home/" + self.user1.username
        commands.getstatusoutput("cp /tmp/sfile2 /tmp/sfile2c")
        self.user1.assert_icommand("ireg -KR " + self.testresc + " /tmp/sfile2c " +
                   testuser2home + "/foo5", 'STDERR', "PATH_REG_NOT_ALLOWED")
        self.user1.assert_icommand("iput -R " + self.testresc + " /tmp/sfile2c " + testuser2home + "/foo5")
        self.user1.assert_icommand("irm -f " + testuser2home + "/foo5")

        # cleanup
        os.unlink("/tmp/sfile2c")
        os.unlink(dir_w + "/foo5")

        if os.path.exists(myldir):
            shutil.rmtree(myldir)
        if os.path.exists(mysdir):
            shutil.rmtree(mysdir)

    def test_mcoll_from_devtest(self):
        # build expected variables with similar devtest names
        progname = __file__
        myssize = str(os.stat(progname).st_size)
        username = self.admin.username
        irodszone = self.admin.zone_name
        testuser1 = self.user0.username
        irodshome = self.admin.session_collection
        irodsdefresource = self.admin.default_resource
        dir_w = "."
        sfile2 = dir_w + "/sfile2"
        commands.getstatusoutput("cat " + progname + " " + progname + " > " + sfile2)
        mysdir = "/tmp/irodssdir"
        myldir = dir_w + "/ldir"
        if os.path.exists(myldir):
            shutil.rmtree(myldir)

        # make a directory containing 20 small files
        if not os.path.isdir(mysdir):
            os.mkdir(mysdir)
        for i in range(20):
            mysfile = mysdir + "/sfile" + str(i)
            shutil.copyfile(progname, mysfile)

        self.admin.assert_icommand("imkdir icmdtest")
        # we put foo1 in $irodsdefresource and foo2 in testresource
        self.admin.assert_icommand("iput -K --wlock " + progname + " " + irodshome + "/icmdtest/foo1")
        self.admin.assert_icommand("icp -K -R " + self.testresc + " " +
                   irodshome + "/icmdtest/foo1 " + irodshome + "/icmdtest/foo2")

        # prepare icmdtesta
        self.admin.assert_icommand("ireg -KCR " + self.testresc + " " + mysdir + " " + irodshome + "/icmdtesta")

        # mcoll test
        self.admin.assert_icommand("imcoll -m link " + irodshome + "/icmdtesta " + irodshome + "/icmdtestb")
        self.admin.assert_icommand("ils -lr " + irodshome + "/icmdtestb", 'STDOUT', "icmdtestb")
        if os.path.exists(dir_w + "/testb"):
            shutil.rmtree(dir_w + "/testb")
        self.admin.assert_icommand("iget -fvrK " + irodshome + "/icmdtestb " + dir_w + "/testb", 'STDOUT', "testb")
        output = commands.getstatusoutput("diff -r " + mysdir + " " + dir_w + "/testb")
        print "output is [" + str(output) + "]"
        assert output[0] == 0
        assert output[1] == "", "diff output was not empty..."
        self.admin.assert_icommand("imcoll -U " + irodshome + "/icmdtestb")
        self.admin.assert_icommand("irm -rf " + irodshome + "/icmdtestb")
        shutil.rmtree(dir_w + "/testb")
        self.admin.assert_icommand("imkdir " + irodshome + "/icmdtestm")
        self.admin.assert_icommand("imcoll -m filesystem -R " +
                   self.testresc + " " + mysdir + " " + irodshome + "/icmdtestm")
        self.admin.assert_icommand("imkdir " + irodshome + "/icmdtestm/testmm")
        self.admin.assert_icommand("iput " + progname + " " + irodshome + "/icmdtestm/testmm/foo1")
        self.admin.assert_icommand("iput " + progname + " " + irodshome + "/icmdtestm/testmm/foo11")
        self.admin.assert_icommand("imv " + irodshome +
                   "/icmdtestm/testmm/foo1 " + irodshome + "/icmdtestm/testmm/foo2")
        self.admin.assert_icommand("imv " + irodshome + "/icmdtestm/testmm " + irodshome + "/icmdtestm/testmm1")

        # mv to normal collection
        self.admin.assert_icommand("imv " + irodshome + "/icmdtestm/testmm1/foo2 " + irodshome + "/icmdtest/foo100")
        self.admin.assert_icommand("ils -l " + irodshome + "/icmdtest/foo100", 'STDOUT', "foo100")
        self.admin.assert_icommand("imv " + irodshome + "/icmdtestm/testmm1 " + irodshome + "/icmdtest/testmm1")
        self.admin.assert_icommand("ils -lr " + irodshome + "/icmdtest/testmm1", 'STDOUT', "foo11")
        self.admin.assert_icommand("irm -rf " + irodshome + "/icmdtest/testmm1 " + irodshome + "/icmdtest/foo100")
        if os.path.exists(dir_w + "/testm"):
            shutil.rmtree(dir_w + "/testm")
        self.admin.assert_icommand("iget -fvrK " + irodshome + "/icmdtesta " + dir_w + "/testm", 'STDOUT', "testm")
        output = commands.getstatusoutput("diff -r " + mysdir + " " + dir_w + "/testm")
        print "output is [" + str(output) + "]"
        assert output[0] == 0
        assert output[1] == "", "diff output was not empty..."
        self.admin.assert_icommand("imcoll -U " + irodshome + "/icmdtestm")
        self.admin.assert_icommand("irm -rf " + irodshome + "/icmdtestm")
        shutil.rmtree(dir_w + "/testm")
        self.admin.assert_icommand("imkdir " + irodshome + "/icmdtestt_mcol")
        # added so icmdtestx.tar exists
        self.admin.assert_icommand("ibun -c " + irodshome + "/icmdtestx.tar " + irodshome + "/icmdtest")
        self.admin.assert_icommand("imcoll -m tar " + irodshome + "/icmdtestx.tar " + irodshome + "/icmdtestt_mcol")
        self.admin.assert_icommand("ils -lr " + irodshome + "/icmdtestt_mcol", 'STDOUT', ["foo2"])
        self.admin.assert_icommand("ils -lr " + irodshome + "/icmdtestt_mcol", 'STDOUT', ["foo1"])
        if os.path.exists(dir_w + "/testt"):
            shutil.rmtree(dir_w + "/testt")
        if os.path.exists(dir_w + "/testx"):
            shutil.rmtree(dir_w + "/testx")
        self.admin.assert_icommand("iget -vr " + irodshome + "/icmdtest  " + dir_w + "/testx", 'STDOUT', "testx")
        self.admin.assert_icommand("iget -vr " + irodshome +
                   "/icmdtestt_mcol/icmdtest  " + dir_w + "/testt", 'STDOUT', "testt")
        output = commands.getstatusoutput("diff -r  " + dir_w + "/testx " + dir_w + "/testt")
        print "output is [" + str(output) + "]"
        assert output[0] == 0
        assert output[1] == "", "diff output was not empty..."
        self.admin.assert_icommand("imkdir " + irodshome + "/icmdtestt_mcol/mydirtt")
        self.admin.assert_icommand("iput " + progname + " " + irodshome + "/icmdtestt_mcol/mydirtt/foo1mt")
        self.admin.assert_icommand("imv " + irodshome + "/icmdtestt_mcol/mydirtt/foo1mt " +
                   irodshome + "/icmdtestt_mcol/mydirtt/foo1mtx")

        # unlink
        self.admin.assert_icommand("imcoll -U " + irodshome + "/icmdtestt_mcol")

        # cleanup
        os.unlink(sfile2)
        shutil.rmtree(dir_w + "/testt")
        shutil.rmtree(dir_w + "/testx")
        if os.path.exists(mysdir):
            shutil.rmtree(mysdir)

    def test_large_dir_and_mcoll_from_devtest(self):
        # build expected variables with similar devtest names
        progname = __file__
        myssize = str(os.stat(progname).st_size)
        username = self.admin.username
        irodszone = self.admin.zone_name
        testuser1 = self.user0.username
        irodshome = self.admin.session_collection
        irodsdefresource = self.admin.default_resource
        dir_w = "."
        sfile2 = dir_w + "/sfile2"
        commands.getstatusoutput("cat " + progname + " " + progname + " > " + sfile2)
        mysdir = "/tmp/irodssdir"
        myldir = dir_w + "/ldir"
        if os.path.exists(myldir):
            shutil.rmtree(myldir)
        self.admin.assert_icommand("imkdir icmdtest")

        # we put foo1 in $irodsdefresource and foo2 in testresource
        self.admin.assert_icommand("iput -K --wlock " + progname + " " + irodshome + "/icmdtest/foo1")
        self.admin.assert_icommand("icp -K -R " + self.testresc + " " +
                   irodshome + "/icmdtest/foo1 " + irodshome + "/icmdtest/foo2")

        # added so icmdtestx.tar exists
        self.admin.assert_icommand("ibun -c " + irodshome + "/icmdtestx.tar " + irodshome + "/icmdtest")
        self.admin.assert_icommand("imkdir " + irodshome + "/icmdtestt_large")
        self.admin.assert_icommand("imcoll -m tar " + irodshome + "/icmdtestx.tar " + irodshome + "/icmdtestt_large")
        self.admin.assert_icommand("imkdir " + irodshome + "/icmdtestt_large/mydirtt")

        # make a directory of 2 large files and 2 small files
        lfile = dir_w + "/lfile"
        lfile1 = dir_w + "/lfile1"
        commands.getstatusoutput("echo 012345678901234567890123456789012345678901234567890123456789012 > " + lfile)
        for i in range(6):
            commands.getstatusoutput("cat " + lfile + " " + lfile + " " + lfile + " " + lfile +
                                     " " + lfile + " " + lfile + " " + lfile + " " + lfile + " " + lfile + " > " + lfile1)
            os.rename(lfile1, lfile)
        os.mkdir(myldir)
        for i in range(1, 3):
            mylfile = myldir + "/lfile" + str(i)
            mysfile = myldir + "/sfile" + str(i)
            if i != 2:
                shutil.copyfile(lfile, mylfile)
            else:
                os.rename(lfile, mylfile)
            shutil.copyfile(progname, mysfile)

        # test adding a large file to a mounted collection
        self.admin.assert_icommand("iput " + myldir + "/lfile1 " + irodshome + "/icmdtestt_large/mydirtt")
        self.admin.assert_icommand("iget " + irodshome + "/icmdtestt_large/mydirtt/lfile1 " + dir_w + "/testt")
        self.admin.assert_icommand("irm -rf " + irodshome + "/icmdtestt_large/mydirtt")
        self.admin.assert_icommand("imcoll -s " + irodshome + "/icmdtestt_large")
        self.admin.assert_icommand("imcoll -p " + irodshome + "/icmdtestt_large")
        self.admin.assert_icommand("imcoll -U " + irodshome + "/icmdtestt_large")
        self.admin.assert_icommand("irm -rf " + irodshome + "/icmdtestt_large")
        os.unlink(dir_w + "/testt")

        # cleanup
        os.unlink(sfile2)
        if os.path.exists(myldir):
            shutil.rmtree(myldir)

    def test_phybun_from_devtest(self):
        with lib.make_session_for_existing_admin() as rods_admin:
            rods_admin.run_icommand(['ichmod', 'own', self.admin.username, '/' + self.admin.zone_name])

        # build expected variables with similar devtest names
        progname = __file__
        myssize = str(os.stat(progname).st_size)
        username = self.admin.username
        irodszone = self.admin.zone_name
        testuser1 = self.user0.username
        irodshome = self.admin.session_collection
        irodsdefresource = self.admin.default_resource
        dir_w = "."
        sfile2 = dir_w + "/sfile2"
        commands.getstatusoutput("cat " + progname + " " + progname + " > " + sfile2)
        mysdir = "/tmp/irodssdir"
        myldir = dir_w + "/ldir"
        if os.path.exists(myldir):
            shutil.rmtree(myldir)
        self.admin.assert_icommand("imkdir icmdtest")

        # make a directory containing 20 small files
        if not os.path.isdir(mysdir):
            os.mkdir(mysdir)
        for i in range(20):
            mysfile = mysdir + "/sfile" + str(i)
            shutil.copyfile(progname, mysfile)

        # iphybun test
        self.admin.assert_icommand("iput -rR " + self.testresc + " " + mysdir + " " + irodshome + "/icmdtestp")
        self.admin.assert_icommand("iphybun -KR " + self.anotherresc + " " + irodshome + "/icmdtestp")
        self.admin.assert_icommand("itrim -rS " + self.testresc + " -N1 " +
                   irodshome + "/icmdtestp", 'STDOUT', "files trimmed")
        output = commands.getstatusoutput("ils -L " + irodshome + "/icmdtestp/sfile1 | tail -n1 | awk '{ print $NF }'")
        print output[1]
        bunfile = output[1]
        self.admin.assert_icommand("irepl --purgec -R " + self.anotherresc + " " + bunfile)
        self.admin.assert_icommand("itrim -rS " + self.testresc + " -N1 " +
                   irodshome + "/icmdtestp", 'STDOUT', "files trimmed")
        # get the name of bundle file
        self.admin.assert_icommand("irm -f --empty " + bunfile)
        # should not be able to remove it because it is not empty
        self.admin.assert_icommand("ils " + bunfile, 'STDOUT', bunfile)
        self.admin.assert_icommand("irm -rvf " + irodshome + "/icmdtestp", 'STDOUT', "num files done")
        self.admin.assert_icommand("irm -f --empty " + bunfile)
        if os.path.exists(dir_w + "/testp"):
            shutil.rmtree(dir_w + "/testp")
        shutil.rmtree(mysdir)

        # cleanup
        os.unlink(sfile2)
        if os.path.exists(myldir):
            shutil.rmtree(myldir)
        if os.path.exists(mysdir):
            shutil.rmtree(mysdir)

    def test_irsync_from_devtest(self):
        # build expected variables with similar devtest names
        progname = __file__
        myssize = str(os.stat(progname).st_size)
        username = self.admin.username
        irodszone = self.admin.zone_name
        testuser1 = self.user0.username
        irodshome = self.admin.session_collection
        irodsdefresource = self.admin.default_resource
        dir_w = "."
        sfile2 = dir_w + "/sfile2"
        commands.getstatusoutput("cat " + progname + " " + progname + " > " + sfile2)
        mysdir = "/tmp/irodssdir"
        myldir = dir_w + "/ldir"
        if os.path.exists(myldir):
            shutil.rmtree(myldir)
        self.admin.assert_icommand("imkdir icmdtest")

        # testing irsync
        self.admin.assert_icommand("irsync " + progname + " i:" + irodshome + "/icmdtest/foo100")
        self.admin.assert_icommand("irsync i:" + irodshome + "/icmdtest/foo100 " + dir_w + "/foo100")
        self.admin.assert_icommand("irsync i:" + irodshome + "/icmdtest/foo100 i:" + irodshome + "/icmdtest/foo200")
        self.admin.assert_icommand("irm -f " + irodshome + "/icmdtest/foo100 " + irodshome + "/icmdtest/foo200")
        self.admin.assert_icommand("iput -R " + self.testresc + " " + progname + " " + irodshome + "/icmdtest/foo100")
        self.admin.assert_icommand("irsync " + progname + " i:" + irodshome + "/icmdtest/foo100")
        self.admin.assert_icommand("iput -R " + self.testresc + " " + progname + " " + irodshome + "/icmdtest/foo200")
        self.admin.assert_icommand("irsync i:" + irodshome + "/icmdtest/foo100 i:" + irodshome + "/icmdtest/foo200")
        os.unlink(dir_w + "/foo100")

        # cleanup
        os.unlink(sfile2)

    def test_xml_protocol_from_devtest(self):
        # build expected variables with similar devtest names
        progname = __file__
        myssize = str(os.stat(progname).st_size)
        username = self.admin.username
        irodszone = self.admin.zone_name
        testuser1 = self.user0.username
        irodshome = self.admin.session_collection
        irodsdefresource = self.admin.default_resource
        dir_w = "."
        sfile2 = dir_w + "/sfile2"
        commands.getstatusoutput("cat " + progname + " " + progname + " > " + sfile2)
        mysdir = "/tmp/irodssdir"
        myldir = dir_w + "/ldir"
        if os.path.exists(myldir):
            shutil.rmtree(myldir)
        self.admin.assert_icommand("imkdir icmdtest")

        lrsfile = dir_w + "/lrsfile"
        rsfile = dir_w + "/rsfile"

        # do test using xml protocol
        os.environ['irodsProt'] = "1"
        self.admin.assert_icommand("ilsresc", 'STDOUT', self.testresc)
        self.admin.assert_icommand("imiscsvrinfo", 'STDOUT', "relVersion")
        self.admin.assert_icommand("iuserinfo", 'STDOUT', "name: " + username)
        self.admin.assert_icommand("ienv", 'STDOUT', "Release Version")
        self.admin.assert_icommand("icd " + irodshome)
        self.admin.assert_icommand("ipwd", 'STDOUT', "home")
        self.admin.assert_icommand("ihelp ils", 'STDOUT', "ils")
        self.admin.assert_icommand("ierror -14000", 'STDOUT', "SYS_API_INPUT_ERR")
        self.admin.assert_icommand("iexecmd hello", 'STDOUT', "Hello world")
        self.admin.assert_icommand("ips -v", 'STDOUT', "ips")
        self.admin.assert_icommand("iqstat", 'STDOUT', "No delayed rules")
        self.admin.assert_icommand("imkdir " + irodshome + "/icmdtest1")
        # make a directory of large files
        self.admin.assert_icommand("iput -kf  " + progname + "  " + irodshome + "/icmdtest1/foo1")
        self.admin.assert_icommand("ils -l " + irodshome + "/icmdtest1/foo1", 'STDOUT', ["foo1", myssize])
        self.admin.assert_icommand("iadmin ls " + irodshome + "/icmdtest1", 'STDOUT', "foo1")
        self.admin.assert_icommand("ichmod read " + self.user0.username + " " + irodshome + "/icmdtest1/foo1")
        self.admin.assert_icommand("ils -A " + irodshome + "/icmdtest1/foo1",
                   'STDOUT', self.user0.username + "#" + irodszone + ":read")
        self.admin.assert_icommand("irepl -B -R " + self.testresc + " " + irodshome + "/icmdtest1/foo1")
        # overwrite a copy
        self.admin.assert_icommand("itrim -S  " + irodsdefresource + " -N1 " + irodshome + "/icmdtest1/foo1")
        self.admin.assert_icommand("iphymv -R  " + irodsdefresource + " " + irodshome + "/icmdtest1/foo1")
        self.admin.assert_icommand("imeta add -d " + irodshome + "/icmdtest1/foo1 testmeta1 180 cm")
        self.admin.assert_icommand("imeta ls -d " + irodshome + "/icmdtest1/foo1", 'STDOUT', "testmeta1")
        self.admin.assert_icommand("imeta ls -d " + irodshome + "/icmdtest1/foo1", 'STDOUT', "180")
        self.admin.assert_icommand("imeta ls -d " + irodshome + "/icmdtest1/foo1", 'STDOUT', "cm")
        self.admin.assert_icommand("icp -K -R " + self.testresc + " " +
                   irodshome + "/icmdtest1/foo1 " + irodshome + "/icmdtest1/foo2")
        self.admin.assert_icommand("imv " + irodshome + "/icmdtest1/foo2 " + irodshome + "/icmdtest1/foo4")
        self.admin.assert_icommand("imv " + irodshome + "/icmdtest1/foo4 " + irodshome + "/icmdtest1/foo2")
        self.admin.assert_icommand("ichksum -K " + irodshome + "/icmdtest1/foo2", 'STDOUT', "foo2")
        self.admin.assert_icommand("iget -f -K " + irodshome + "/icmdtest1/foo2 " + dir_w)
        os.unlink(dir_w + "/foo2")
        self.admin.assert_icommand("irsync " + progname + " i:" + irodshome + "/icmdtest1/foo1")
        self.admin.assert_icommand("irsync i:" + irodshome + "/icmdtest1/foo1 /tmp/foo1")
        self.admin.assert_icommand("irsync i:" + irodshome + "/icmdtest1/foo1 i:" + irodshome + "/icmdtest1/foo2")
        os.unlink("/tmp/foo1")
        os.environ['irodsProt'] = "0"

        # cleanup
        os.unlink(sfile2)

    def test_large_files_from_devtest(self):
        # build expected variables with similar devtest names
        progname = __file__
        myssize = str(os.stat(progname).st_size)
        username = self.admin.username
        irodszone = self.admin.zone_name
        testuser1 = self.user0.username
        irodshome = self.admin.session_collection
        irodsdefresource = self.admin.default_resource
        dir_w = "."
        sfile2 = dir_w + "/sfile2"
        commands.getstatusoutput("cat " + progname + " " + progname + " > " + sfile2)
        mysdir = "/tmp/irodssdir"
        myldir = dir_w + "/ldir"
        if os.path.exists(myldir):
            shutil.rmtree(myldir)
        self.admin.assert_icommand("imkdir icmdtest")

        # make a directory of 2 large files and 2 small files
        lfile = dir_w + "/lfile"
        lfile1 = dir_w + "/lfile1"
        commands.getstatusoutput("echo 012345678901234567890123456789012345678901234567890123456789012 > " + lfile)
        for i in range(6):
            commands.getstatusoutput("cat " + lfile + " " + lfile + " " + lfile + " " + lfile +
                                     " " + lfile + " " + lfile + " " + lfile + " " + lfile + " " + lfile + " > " + lfile1)
            os.rename(lfile1, lfile)
        os.mkdir(myldir)
        for i in range(1, 3):
            mylfile = myldir + "/lfile" + str(i)
            mysfile = myldir + "/sfile" + str(i)
            if i != 2:
                shutil.copyfile(lfile, mylfile)
            else:
                os.rename(lfile, mylfile)
            shutil.copyfile(progname, mysfile)

        # do the large files tests
        lrsfile = dir_w + "/lrsfile"
        rsfile = dir_w + "/rsfile"
        if os.path.isfile(lrsfile):
            os.unlink(lrsfile)
        if os.path.isfile(rsfile):
            os.unlink(rsfile)
        self.admin.assert_icommand("iput -vbPKr --retries 10 --wlock -X " + rsfile + " --lfrestart " +
                   lrsfile + " -N 2 " + myldir + " " + irodshome + "/icmdtest/testy", 'STDOUT', "New restartFile")
        self.admin.assert_icommand("ichksum -rK " + irodshome + "/icmdtest/testy", 'STDOUT', "Total checksum performed")
        if os.path.isfile(lrsfile):
            os.unlink(lrsfile)
        if os.path.isfile(rsfile):
            os.unlink(rsfile)
        self.admin.assert_icommand("irepl -BvrPT -R " + self.testresc + " --rlock " +
                   irodshome + "/icmdtest/testy", 'STDOUT', "icmdtest/testy")
        self.admin.assert_icommand("itrim -vrS " + irodsdefresource + " --dryrun --age 1 -N 1 " +
                   irodshome + "/icmdtest/testy", 'STDOUT', "This is a DRYRUN")
        self.admin.assert_icommand("itrim -vrS " + irodsdefresource + " -N 1 " +
                   irodshome + "/icmdtest/testy", 'STDOUT', "a copy trimmed")
        self.admin.assert_icommand("icp -vKPTr -N 2 " + irodshome + "/icmdtest/testy " +
                   irodshome + "/icmdtest/testz", 'STDOUT', "Processing lfile1")
        self.admin.assert_icommand("irsync -r i:" + irodshome + "/icmdtest/testy i:" + irodshome + "/icmdtest/testz")
        self.admin.assert_icommand("irm -vrf " + irodshome + "/icmdtest/testy")
        self.admin.assert_icommand("iphymv -vrS " + irodsdefresource + " -R " +
                   self.testresc + " " + irodshome + "/icmdtest/testz", 'STDOUT', "icmdtest/testz")

        if os.path.isfile(lrsfile):
            os.unlink(lrsfile)
        if os.path.isfile(rsfile):
            os.unlink(rsfile)
        if os.path.exists(dir_w + "/testz"):
            shutil.rmtree(dir_w + "/testz")
        self.admin.assert_icommand("iget -vPKr --retries 10 -X " + rsfile + " --lfrestart " + lrsfile +
                   " --rlock -N 2 " + irodshome + "/icmdtest/testz " + dir_w + "/testz", 'STDOUT', "testz")
        self.admin.assert_icommand("irsync -r " + dir_w + "/testz i:" + irodshome + "/icmdtest/testz")
        self.admin.assert_icommand("irsync -r i:" + irodshome + "/icmdtest/testz " + dir_w + "/testz")
        if os.path.isfile(lrsfile):
            os.unlink(lrsfile)
        if os.path.isfile(rsfile):
            os.unlink(rsfile)
        output = commands.getstatusoutput("diff -r " + dir_w + "/testz " + myldir)
        print "output is [" + str(output) + "]"
        assert output[0] == 0
        assert output[1] == "", "diff output was not empty..."
        # test -N0 transfer
        self.admin.assert_icommand("iput -N0 -R " + self.testresc + " " +
                   myldir + "/lfile1 " + irodshome + "/icmdtest/testz/lfoo100")
        if os.path.isfile(dir_w + "/lfoo100"):
            os.unlink(dir_w + "/lfoo100")
        self.admin.assert_icommand("iget -N0 " + irodshome + "/icmdtest/testz/lfoo100 " + dir_w + "/lfoo100")
        output = commands.getstatusoutput("diff " + myldir + "/lfile1 " + dir_w + "/lfoo100")
        print "output is [" + str(output) + "]"
        assert output[0] == 0
        assert output[1] == "", "diff output was not empty..."
        shutil.rmtree(dir_w + "/testz")
        os.unlink(dir_w + "/lfoo100")
        self.admin.assert_icommand("irm -vrf " + irodshome + "/icmdtest/testz")

        # cleanup
        os.unlink(sfile2)
        if os.path.exists(myldir):
            shutil.rmtree(myldir)

    def test_large_files_with_RBUDP_from_devtest(self):
        # build expected variables with similar devtest names
        progname = __file__
        myssize = str(os.stat(progname).st_size)
        username = self.admin.username
        irodszone = self.admin.zone_name
        testuser1 = self.user0.username
        irodshome = self.admin.session_collection
        irodsdefresource = self.admin.default_resource
        dir_w = "."
        sfile2 = dir_w + "/sfile2"
        commands.getstatusoutput("cat " + progname + " " + progname + " > " + sfile2)
        mysdir = "/tmp/irodssdir"
        myldir = dir_w + "/ldir"
        if os.path.exists(myldir):
            shutil.rmtree(myldir)
        self.admin.assert_icommand("imkdir icmdtest")

        # make a directory of 2 large files and 2 small files
        lfile = dir_w + "/lfile"
        lfile1 = dir_w + "/lfile1"
        commands.getstatusoutput("echo 012345678901234567890123456789012345678901234567890123456789012 > " + lfile)
        for i in range(6):
            commands.getstatusoutput("cat " + lfile + " " + lfile + " " + lfile + " " + lfile +
                                     " " + lfile + " " + lfile + " " + lfile + " " + lfile + " " + lfile + " > " + lfile1)
            os.rename(lfile1, lfile)
        os.mkdir(myldir)
        for i in range(1, 3):
            mylfile = myldir + "/lfile" + str(i)
            mysfile = myldir + "/sfile" + str(i)
            if i != 2:
                shutil.copyfile(lfile, mylfile)
            else:
                os.rename(lfile, mylfile)
            shutil.copyfile(progname, mysfile)

        # do the large files tests using RBUDP
        lrsfile = dir_w + "/lrsfile"
        rsfile = dir_w + "/rsfile"
        if os.path.isfile(lrsfile):
            os.unlink(lrsfile)
        if os.path.isfile(rsfile):
            os.unlink(rsfile)
        self.admin.assert_icommand("iput -vQPKr --retries 10 -X " + rsfile + " --lfrestart " +
                   lrsfile + " " + myldir + " " + irodshome + "/icmdtest/testy", 'STDOUT', "icmdtest/testy")
        self.admin.assert_icommand("irepl -BQvrPT -R " + self.testresc + " " +
                   irodshome + "/icmdtest/testy", 'STDOUT', "icmdtest/testy")
        self.admin.assert_icommand("itrim -vrS " + irodsdefresource + " -N 1 " +
                   irodshome + "/icmdtest/testy", 'STDOUT', "a copy trimmed")
        self.admin.assert_icommand("icp -vQKPTr " + irodshome + "/icmdtest/testy " +
                   irodshome + "/icmdtest/testz", 'STDOUT', "Processing sfile1")
        self.admin.assert_icommand("irm -vrf " + irodshome + "/icmdtest/testy")
        if os.path.isfile(lrsfile):
            os.unlink(lrsfile)
        if os.path.isfile(rsfile):
            os.unlink(rsfile)
        if os.path.exists(dir_w + "/testz"):
            shutil.rmtree(dir_w + "/testz")
        self.admin.assert_icommand("iget -vQPKr --retries 10 -X " + rsfile + " --lfrestart " + lrsfile +
                   " " + irodshome + "/icmdtest/testz " + dir_w + "/testz", 'STDOUT', "Processing sfile2")
        if os.path.isfile(lrsfile):
            os.unlink(lrsfile)
        if os.path.isfile(rsfile):
            os.unlink(rsfile)
        output = commands.getstatusoutput("diff -r " + dir_w + "/testz " + myldir)
        print "output is [" + str(output) + "]"
        assert output[0] == 0
        assert output[1] == "", "diff output was not empty..."
        shutil.rmtree(dir_w + "/testz")
        self.admin.assert_icommand("irm -vrf " + irodshome + "/icmdtest/testz")
        shutil.rmtree(myldir)

        # cleanup
        os.unlink(sfile2)
        if os.path.exists(myldir):
            shutil.rmtree(myldir)
