from __future__ import print_function
import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import os
import stat
import datetime
import filecmp
import time
import shutil
import random
import subprocess
import ustrings

from . import session
from .. import test
from . import settings
from .resource_suite import ResourceBase
from .. import lib


class ChunkyDevTest(ResourceBase):

    def test_beginning_from_devtest(self):
        # build expected variables with similar devtest names
        test_file = os.path.join(self.admin.local_session_dir, 'test_file')
        lib.make_file(test_file, 4000, contents='arbitrary')
        myssize = str(os.stat(test_file).st_size)
        username = self.admin.username
        irodszone = self.admin.zone_name
        testuser1 = self.user0.username
        irodshome = self.admin.session_collection
        irodsdefresource = self.admin.default_resource
        dir_w = "."
        sfile2 = dir_w + "/sfile2"
        cat_file_into_file_n_times(test_file, sfile2, 2)
        mysdir = "/tmp/irodssdir"
        myldir = dir_w + "/ldir"
        if os.path.exists(myldir):
            shutil.rmtree(myldir)
        self.admin.assert_icommand("imkdir " + irodshome + "/icmdtest")

        # test basic informational icommands
        self.admin.assert_icommand("iinit -l", 'STDOUT_SINGLELINE', self.admin.username)
        self.admin.assert_icommand("iinit -l", 'STDOUT_SINGLELINE', self.admin.zone_name)
        self.admin.assert_icommand("iinit -l", 'STDOUT_SINGLELINE', self.admin.default_resource)

        # begin original devtest
        self.admin.assert_icommand(['ilsresc', '--ascii'], 'STDOUT_SINGLELINE', self.testresc)
        self.admin.assert_icommand("ilsresc -l", 'STDOUT_SINGLELINE', self.testresc)
        self.admin.assert_icommand("imiscsvrinfo", 'STDOUT_SINGLELINE', ["relVersion"])
        self.admin.assert_icommand("iuserinfo", 'STDOUT_SINGLELINE', "name: " + username)
        self.admin.assert_icommand("ienv", 'STDOUT_SINGLELINE', "irods_zone_name")
        self.admin.assert_icommand("ipwd", 'STDOUT_SINGLELINE', "home")
        self.admin.assert_icommand("ihelp ils", 'STDOUT_SINGLELINE', "ils")
        self.admin.assert_icommand("ierror -14000", 'STDOUT_SINGLELINE', "SYS_API_INPUT_ERR")
        self.admin.assert_icommand("iexecmd hello", 'STDOUT_SINGLELINE', "Hello world")
        self.admin.assert_icommand("ips -v", 'STDOUT_SINGLELINE', "ips")
        self.admin.assert_icommand("iqstat", 'STDOUT_SINGLELINE', "No delayed rules pending for user " + self.admin.username)

        # put and list basic file information
        self.admin.assert_icommand("ils -AL", 'STDOUT_SINGLELINE', "home")  # debug
        self.admin.assert_icommand("iput -K --wlock " + test_file + " " + irodshome + "/icmdtest/foo1")
        self.admin.assert_icommand("ichksum -f " + irodshome + "/icmdtest/foo1", 'STDOUT_SINGLELINE', "    sha2:")
        self.admin.assert_icommand("iput -kf " + test_file + " " + irodshome + "/icmdtest/foo1")
        self.admin.assert_icommand("ils " + irodshome + "/icmdtest/foo1", 'STDOUT_SINGLELINE', "foo1")
        self.admin.assert_icommand("ils -l " + irodshome + "/icmdtest/foo1", 'STDOUT_SINGLELINE', ["foo1", myssize])
        self.admin.assert_icommand("ils " + irodshome + "/icmdtest", 'STDOUT_SINGLELINE', "foo1")
        self.admin.assert_icommand("ils -A " + irodshome + "/icmdtest/foo1",
                                   'STDOUT_SINGLELINE', username + "#" + irodszone + ":own")
        self.admin.assert_icommand("ichmod read " + testuser1 + " " + irodshome + "/icmdtest/foo1")
        self.admin.assert_icommand("ils -A " + irodshome + "/icmdtest/foo1",
                                   'STDOUT_SINGLELINE', testuser1 + "#" + irodszone + ":read")
        # basic replica
        self.admin.assert_icommand("irepl -B -R " + self.testresc + " --rlock " + irodshome + "/icmdtest/foo1")
        self.admin.assert_icommand("ils -l " + irodshome + "/icmdtest/foo1", 'STDOUT_SINGLELINE', self.testresc)

        # overwrite a copy
        self.admin.assert_icommand("itrim -S " + irodsdefresource + " -N1 " + irodshome + "/icmdtest/foo1", 'STDOUT_SINGLELINE', "files trimmed")
        self.admin.assert_icommand_fail("ils -L " + irodshome + "/icmdtest/foo1", 'STDOUT_SINGLELINE', [irodsdefresource])
        # TODO: this must be skipped because iphymv requires a leaf resource for source and destination
        #self.admin.assert_icommand("iphymv -S " + self.testresc + " -R " + irodsdefresource + " " + irodshome + "/icmdtest/foo1")
        #self.admin.assert_icommand("ils -l " + irodshome + "/icmdtest/foo1", 'STDOUT_SINGLELINE', irodsdefresource[0:19])
        # basic metadata shuffle
        self.admin.assert_icommand("imeta add -d " + irodshome + "/icmdtest/foo1 testmeta1 180 cm")
        self.admin.assert_icommand("imeta ls -d " + irodshome + "/icmdtest/foo1", 'STDOUT_SINGLELINE', ["testmeta1"])
        self.admin.assert_icommand("imeta ls -d " + irodshome + "/icmdtest/foo1", 'STDOUT_SINGLELINE', ["180"])
        self.admin.assert_icommand("imeta ls -d " + irodshome + "/icmdtest/foo1", 'STDOUT_SINGLELINE', ["cm"])
        self.admin.assert_icommand("icp -K -R " + self.testresc + " " +
                                   irodshome + "/icmdtest/foo1 " + irodshome + "/icmdtest/foo2")

        # test imeta -v
        imeta_popen = subprocess.Popen(
            'echo "ls -d ' + irodshome + '/icmdtest/foo1" | imeta -v', shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        imeta_output = imeta_popen.communicate()[0].decode()
        assert imeta_output.find('testmeta1') > -1
        assert imeta_output.find('180') > -1
        assert imeta_output.find('cm') > -1

        # new file mode check
        self.admin.assert_icommand("iget -fK --rlock " + irodshome + "/icmdtest/foo2 /tmp/")
        assert stat.S_IMODE(os.stat("/tmp/foo2").st_mode) == 0o640
        os.unlink("/tmp/foo2")

        self.admin.assert_icommand("ils " + irodshome + "/icmdtest/foo2", 'STDOUT_SINGLELINE', "foo2")
        self.admin.assert_icommand("imv " + irodshome + "/icmdtest/foo2 " + irodshome + "/icmdtest/foo4")
        self.admin.assert_icommand("ils -l " + irodshome + "/icmdtest/foo4", 'STDOUT_SINGLELINE', "foo4")
        self.admin.assert_icommand("imv " + irodshome + "/icmdtest/foo4 " + irodshome + "/icmdtest/foo2")
        self.admin.assert_icommand("ils -l " + irodshome + "/icmdtest/foo2", 'STDOUT_SINGLELINE', "foo2")
        self.admin.assert_icommand("ichksum " + irodshome + "/icmdtest/foo2", 'STDOUT_SINGLELINE', "foo2")
        self.admin.assert_icommand("imeta add -d " + irodshome + "/icmdtest/foo2 testmeta1 180 cm")
        self.admin.assert_icommand("imeta add -d " + irodshome + "/icmdtest/foo1 testmeta2 hello")
        self.admin.assert_icommand("imeta ls -d " + irodshome + "/icmdtest/foo1", 'STDOUT_SINGLELINE', ["testmeta1"])
        self.admin.assert_icommand("imeta ls -d " + irodshome + "/icmdtest/foo1", 'STDOUT_SINGLELINE', ["hello"])
        self.admin.assert_icommand("imeta qu -d testmeta1 = 180", 'STDOUT_SINGLELINE', "foo1")
        self.admin.assert_icommand("imeta qu -d testmeta2 = hello", 'STDOUT_SINGLELINE', "dataObj: foo1")
        self.admin.assert_icommand("iget -f -K --rlock " + irodshome + "/icmdtest/foo2 " + dir_w)
        assert myssize == str(os.stat(dir_w + "/foo2").st_size)
        os.unlink(dir_w + "/foo2")

        # we have foo1 in $irodsdefresource and foo2 in testresource

        # cleanup
        os.unlink(sfile2)

    def test_iput_ibun_gzip_bzip2_from_devtest(self):
        # build expected variables with similar devtest names
        with session.make_session_for_existing_admin() as rods_admin:
            rods_admin.assert_icommand(['ichmod', 'own', self.admin.username, '/' + self.admin.zone_name])

        test_file = os.path.join(self.admin.local_session_dir, 'test_file')
        lib.make_file(test_file, 4000, contents='arbitrary')
        myssize = str(os.stat(test_file).st_size)
        username = self.admin.username
        irodszone = self.admin.zone_name
        testuser1 = self.user0.username
        irodshome = self.admin.session_collection
        irodsdefresource = self.admin.default_resource


        dir_w = "."
        sfile2 = dir_w + "/sfile2"
        cat_file_into_file_n_times(test_file, sfile2, 2)
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
            shutil.copyfile(test_file, mysfile)

        # we put foo1 in $irodsdefresource and foo2 in testresource
        self.admin.assert_icommand("iput -K --wlock " + test_file + " " + irodshome + "/icmdtest/foo1")
        self.admin.assert_icommand("icp -K -R " + self.testresc + " " +
                                   irodshome + "/icmdtest/foo1 " + irodshome + "/icmdtest/foo2")

        self.admin.assert_icommand("irepl -B -R " + self.testresc + " " + irodshome + "/icmdtest/foo1")
        phypath = dir_w + "/" + "foo1." + str(random.randrange(10000000))
        self.admin.assert_icommand("iput -kfR " + irodsdefresource + " " + sfile2 + " " + irodshome + "/icmdtest/foo1")
        # show have 2 different copies
        self.admin.assert_icommand("ils -l " + irodshome + "/icmdtest/foo1", 'STDOUT_SINGLELINE', ["foo1", myssize])
        self.admin.assert_icommand("ils -l " + irodshome + "/icmdtest/foo1",
                                   'STDOUT_SINGLELINE', ["foo1", str(os.stat(sfile2).st_size)])
        # update all old copies
        self.admin.assert_icommand("irepl -a " + irodshome + "/icmdtest/foo1")
        # make sure the old size is not there
        self.admin.assert_icommand_fail("ils -l " + irodshome + "/icmdtest/foo1", 'STDOUT_SINGLELINE', myssize)
        self.admin.assert_icommand("itrim -S " + irodsdefresource + " " + irodshome + "/icmdtest/foo1", 'STDOUT_SINGLELINE', "files trimmed")
        # bulk test
        self.admin.assert_icommand("iput -bIvPKr " + mysdir + " " + irodshome + "/icmdtest", 'STDOUT_SINGLELINE', "Bulk upload")
        # iput with a lot of options
        rsfile = dir_w + "/rsfile"
        if os.path.isfile(rsfile):
            os.unlink(rsfile)
        self.admin.assert_icommand("iput -PkITr -X " + rsfile + " --retries 10 " +
                                   mysdir + " " + irodshome + "/icmdtestw", 'STDOUT_SINGLELINE', "Processing")
        self.admin.assert_icommand("imv " + irodshome + "/icmdtestw " + irodshome + "/icmdtestw1")
        self.admin.assert_icommand("ils -lr " + irodshome + "/icmdtestw1", 'STDOUT_SINGLELINE', "sfile10")
        self.admin.assert_icommand("ils -Ar " + irodshome + "/icmdtestw1", 'STDOUT_SINGLELINE', "sfile10")
        self.admin.assert_icommand("irm -rvf " + irodshome + "/icmdtestw1", 'STDOUT_SINGLELINE', "num files done")
        if os.path.isfile(rsfile):
            os.unlink(rsfile)
        self.admin.assert_icommand("iget -vIKPfr -X rsfile --retries 10 " +
                                   irodshome + "/icmdtest " + dir_w + "/testx", 'STDOUT_SINGLELINE', "opened")
        if os.path.isfile(rsfile):
            os.unlink(rsfile)
        lib.execute_command(['tar', 'chf', os.path.join(dir_w, 'testx.tar'), '-C', os.path.join(dir_w, 'testx'), '.'])
        self.admin.assert_icommand("iput " + dir_w + "/testx.tar " + irodshome + "/icmdtestx.tar")
        self.admin.assert_icommand("ibun -x " + irodshome + "/icmdtestx.tar " + irodshome + "/icmdtestx")
        self.admin.assert_icommand("ils -lr " + irodshome + "/icmdtestx", 'STDOUT_SINGLELINE', ["foo2"])
        self.admin.assert_icommand("ils -lr " + irodshome + "/icmdtestx", 'STDOUT_SINGLELINE', ["sfile10"])
        self.admin.assert_icommand("ibun -cDtar " + irodshome + "/icmdtestx1.tar " + irodshome + "/icmdtestx")
        self.admin.assert_icommand("ils -l " + irodshome + "/icmdtestx1.tar", 'STDOUT_SINGLELINE', "testx1.tar")
        if os.path.exists(dir_w + "/testx1"):
            shutil.rmtree(dir_w + "/testx1")
        os.mkdir(dir_w + "/testx1")
        if os.path.isfile(dir_w + "/testx1.tar"):
            os.unlink(dir_w + "/testx1.tar")
        self.admin.assert_icommand("iget " + irodshome + "/icmdtestx1.tar " + dir_w + "/testx1.tar")
        lib.execute_command(['tar', 'xvf', os.path.join(dir_w, 'testx1.tar'), '-C', os.path.join(dir_w, 'testx1')])
        compare_dirs = filecmp.dircmp(os.path.join(dir_w, 'testx'), os.path.join(dir_w, 'testx1', 'icmdtestx'))
        assert (not compare_dirs.right_only and not compare_dirs.left_only and not compare_dirs.diff_files), "Directories differ"

        # test ibun with gzip
        self.admin.assert_icommand("ibun -cDgzip " + irodshome + "/icmdtestx1.tar.gz " + irodshome + "/icmdtestx")
        self.admin.assert_icommand("ibun -x " + irodshome + "/icmdtestx1.tar.gz " + irodshome + "/icmdtestgz")
        if os.path.isfile("icmdtestgz"):
            os.unlink("icmdtestgz")
        self.admin.assert_icommand("iget -vr " + irodshome + "/icmdtestgz " + dir_w + "", 'STDOUT_SINGLELINE', "icmdtestgz")
        compare_dirs = filecmp.dircmp(os.path.join(dir_w, 'testx'), os.path.join(dir_w, 'icmdtestgz', 'icmdtestx'))
        assert (not compare_dirs.right_only and not compare_dirs.left_only and not compare_dirs.diff_files), "Directories differ"
        shutil.rmtree(dir_w + "/icmdtestgz")
        self.admin.assert_icommand("ibun --add " + irodshome + "/icmdtestx1.tar.gz " + irodshome + "/icmdtestgz")
        self.admin.assert_icommand("irm -rf " + irodshome + "/icmdtestx1.tar.gz " + irodshome + "/icmdtestgz")

        # test ibun with zip
        self.admin.assert_icommand("ibun -cDzip " + irodshome + "/icmdtestx1.zip " + irodshome + "/icmdtestx")
        self.admin.assert_icommand("ibun -x " + irodshome + "/icmdtestx1.zip " + irodshome + "/icmdtestzip")
        if os.path.isfile("icmdtestzip"):
            os.unlink("icmdtestzip")
        self.admin.assert_icommand("iget -vr " + irodshome + "/icmdtestzip " + dir_w + "", 'STDOUT_SINGLELINE', "icmdtestzip")
        compare_dirs = filecmp.dircmp(os.path.join(dir_w, 'testx'), os.path.join(dir_w, 'icmdtestzip', 'icmdtestx'))
        assert (not compare_dirs.right_only and not compare_dirs.left_only and not compare_dirs.diff_files), "Directories differ"
        shutil.rmtree(dir_w + "/icmdtestzip")
        self.admin.assert_icommand("ibun --add " + irodshome + "/icmdtestx1.zip " + irodshome + "/icmdtestzip")
        self.admin.assert_icommand("irm -rf " + irodshome + "/icmdtestx1.zip " + irodshome + "/icmdtestzip")

        # test ibun with bzip2
        self.admin.assert_icommand("ibun -cDbzip2 " + irodshome + "/icmdtestx1.tar.bz2 " + irodshome + "/icmdtestx")
        self.admin.assert_icommand("ibun -xb " + irodshome + "/icmdtestx1.tar.bz2 " + irodshome + "/icmdtestbz2")
        if os.path.isfile("icmdtestbz2"):
            os.unlink("icmdtestbz2")
        self.admin.assert_icommand("iget -vr " + irodshome + "/icmdtestbz2 " + dir_w + "", 'STDOUT_SINGLELINE', "icmdtestbz2")
        compare_dirs = filecmp.dircmp(os.path.join(dir_w, 'testx'), os.path.join(dir_w, 'icmdtestbz2', 'icmdtestx'))
        assert (not compare_dirs.right_only and not compare_dirs.left_only and not compare_dirs.diff_files), "Directories differ"
        shutil.rmtree(dir_w + "/icmdtestbz2")
        self.admin.assert_icommand("irm -rf " + irodshome + "/icmdtestx1.tar.bz2")

        # Issue 3835 - implement a phybun test suite
        #self.admin.assert_icommand("iphybun -R " + self.anotherresc + " -Dbzip2 " + irodshome + "/icmdtestbz2")
        #self.admin.assert_icommand("itrim -N1 -S " + self.testresc + " -r " + irodshome + "/icmdtestbz2", 'STDOUT_SINGLELINE', "Total size trimmed")
        #self.admin.assert_icommand("itrim -N1 -S " + irodsdefresource + " -r " + irodshome + "/icmdtestbz2", 'STDOUT_SINGLELINE', "Total size trimmed")

        # get the name of bundle file
        #path, test_collection = os.path.split(irodshome)
        #out, _ = lib.execute_command(['ils', '-L', os.path.join(irodshome, 'icmdtestbz2', 'icmdtestx', 'foo1')])
        #out, _ = self.admin.assert_icommand_fail(['ils', os.path.join('/tempZone', 'bundle', 'home', username, test_collection)], 'STDOUT_SINGLELINE', 'bundle')
        #print(out)
        #bunfile = out.split()[-1]
        #print(bunfile)
        #self.admin.assert_icommand("ils --bundle " + bunfile, 'STDOUT_SINGLELINE', "Subfiles")
        #self.admin.assert_icommand("irm -f --empty " + bunfile)
        self.admin.assert_icommand("irm -rf " + irodshome + "/icmdtestbz2")

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
        test_file = os.path.join(self.admin.local_session_dir, 'test_file')
        lib.make_file(test_file, 4000, contents='arbitrary')
        myssize = str(os.stat(test_file).st_size)
        username = self.admin.username
        irodszone = self.admin.zone_name
        testuser1 = self.user0.username
        irodshome = self.admin.session_collection
        irodsdefresource = self.admin.default_resource
        dir_w = "."
        sfile2 = dir_w + "/sfile2"
        cat_file_into_file_n_times(test_file, sfile2, 2)
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
            shutil.copyfile(test_file, mysfile)

        shutil.move(sfile2, '/tmp/sfile2')
        shutil.copyfile('/tmp/sfile2', '/tmp/sfile2r')
        # <-- FAILING - REASON FOR SKIPPING
        self.admin.assert_icommand("ireg -KR " + self.testresc + " /tmp/sfile2 " + irodshome + "/foo5")
        shutil.copyfile('/tmp/sfile2', '/tmp/sfile2r')
        self.admin.assert_icommand("ireg -KkR " + self.anotherresc + " --repl /tmp/sfile2r  " + irodshome + "/foo5")
        self.admin.assert_icommand("iget -fK " + irodshome + "/foo5 " + dir_w + "/foo5")
        assert filecmp.cmp('/tmp/sfile2', os.path.join(dir_w, 'foo5')), "Files differed"
        self.admin.assert_icommand("ireg -KCR " + self.testresc + " " + mysdir + " " + irodshome + "/icmdtesta")
        if os.path.exists(dir_w + "/testa"):
            shutil.rmtree(dir_w + "/testa")
        self.admin.assert_icommand("iget -fvrK " + irodshome + "/icmdtesta " + dir_w + "/testa", 'STDOUT_SINGLELINE', "testa")
        compare_dirs = filecmp.dircmp(mysdir, os.path.join(dir_w, 'testa'))
        assert (not compare_dirs.right_only and not compare_dirs.left_only and not compare_dirs.diff_files), "Directories differ"
        shutil.rmtree(dir_w + "/testa")
        # test ireg with normal user
        testuser2home = "/" + irodszone + "/home/" + self.user1.username
        shutil.copyfile('/tmp/sfile2', '/tmp/sfile2c')
        self.user1.assert_icommand("ireg -KR " + self.testresc + " /tmp/sfile2c " +
                                   testuser2home + "/foo5", 'STDERR_SINGLELINE', "PATH_REG_NOT_ALLOWED")
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
        test_file = os.path.join(self.admin.local_session_dir, 'test_file')
        lib.make_file(test_file, 4000, contents='arbitrary')
        myssize = str(os.stat(test_file).st_size)
        username = self.admin.username
        irodszone = self.admin.zone_name
        testuser1 = self.user0.username
        irodshome = self.admin.session_collection
        irodsdefresource = self.admin.default_resource
        dir_w = "."
        sfile2 = dir_w + "/sfile2"
        cat_file_into_file_n_times(test_file, sfile2, 2)
        mysdir = "/tmp/irodssdir"
        myldir = dir_w + "/ldir"
        if os.path.exists(myldir):
            shutil.rmtree(myldir)

        # make a directory containing 20 small files
        if not os.path.isdir(mysdir):
            os.mkdir(mysdir)
        for i in range(20):
            mysfile = mysdir + "/sfile" + str(i)
            shutil.copyfile(test_file, mysfile)

        self.admin.assert_icommand("imkdir icmdtest")
        # we put foo1 in $irodsdefresource and foo2 in testresource
        self.admin.assert_icommand("iput -K --wlock " + test_file + " " + irodshome + "/icmdtest/foo1")
        self.admin.assert_icommand("icp -K -R " + self.testresc + " " +
                                   irodshome + "/icmdtest/foo1 " + irodshome + "/icmdtest/foo2")

        # prepare icmdtesta
        self.admin.assert_icommand("ireg -KCR " + self.testresc + " " + mysdir + " " + irodshome + "/icmdtesta")

        # mcoll test
        self.admin.assert_icommand("imcoll -m link " + irodshome + "/icmdtesta " + irodshome + "/icmdtestb")
        self.admin.assert_icommand("ils -lr " + irodshome + "/icmdtestb", 'STDOUT_SINGLELINE', "icmdtestb")
        if os.path.exists(dir_w + "/testb"):
            shutil.rmtree(dir_w + "/testb")
        self.admin.assert_icommand("iget -fvrK " + irodshome + "/icmdtestb " + dir_w + "/testb", 'STDOUT_SINGLELINE', "testb")
        compare_dirs = filecmp.dircmp(mysdir, os.path.join(dir_w, 'testb'))
        assert (not compare_dirs.right_only and not compare_dirs.left_only and not compare_dirs.diff_files), "Directories differ"
        self.admin.assert_icommand("imcoll -U " + irodshome + "/icmdtestb")
        self.admin.assert_icommand("irm -rf " + irodshome + "/icmdtestb")
        shutil.rmtree(dir_w + "/testb")
        self.admin.assert_icommand("imkdir " + irodshome + "/icmdtestm")
        self.admin.assert_icommand("imcoll -m filesystem -R " +
                                   self.testresc + " " + mysdir + " " + irodshome + "/icmdtestm")
        self.admin.assert_icommand("imkdir " + irodshome + "/icmdtestm/testmm")
        self.admin.assert_icommand("iput " + test_file + " " + irodshome + "/icmdtestm/testmm/foo1")
        self.admin.assert_icommand("iput " + test_file + " " + irodshome + "/icmdtestm/testmm/foo11")
        self.admin.assert_icommand("imv " + irodshome +
                                   "/icmdtestm/testmm/foo1 " + irodshome + "/icmdtestm/testmm/foo2")
        self.admin.assert_icommand("imv " + irodshome + "/icmdtestm/testmm " + irodshome + "/icmdtestm/testmm1")

        # mv to normal collection
        self.admin.assert_icommand("imv " + irodshome + "/icmdtestm/testmm1/foo2 " + irodshome + "/icmdtest/foo100")
        self.admin.assert_icommand("ils -l " + irodshome + "/icmdtest/foo100", 'STDOUT_SINGLELINE', "foo100")
        self.admin.assert_icommand("imv " + irodshome + "/icmdtestm/testmm1 " + irodshome + "/icmdtest/testmm1")
        self.admin.assert_icommand("ils -lr " + irodshome + "/icmdtest/testmm1", 'STDOUT_SINGLELINE', "foo11")
        self.admin.assert_icommand("irm -rf " + irodshome + "/icmdtest/testmm1 " + irodshome + "/icmdtest/foo100")
        if os.path.exists(dir_w + "/testm"):
            shutil.rmtree(dir_w + "/testm")
        self.admin.assert_icommand("iget -fvrK " + irodshome + "/icmdtesta " + dir_w + "/testm", 'STDOUT_SINGLELINE', "testm")
        compare_dirs = filecmp.dircmp(mysdir, os.path.join(dir_w, 'testm'))
        assert (not compare_dirs.right_only and not compare_dirs.left_only and not compare_dirs.diff_files), "Directories differ"
        self.admin.assert_icommand("imcoll -U " + irodshome + "/icmdtestm")
        self.admin.assert_icommand("irm -rf " + irodshome + "/icmdtestm")
        shutil.rmtree(dir_w + "/testm")
        self.admin.assert_icommand("imkdir " + irodshome + "/icmdtestt_mcol")
        # added so icmdtestx.tar exists
        self.admin.assert_icommand("ibun -c " + irodshome + "/icmdtestx.tar " + irodshome + "/icmdtest")
        self.admin.assert_icommand("imcoll -m tar " + irodshome + "/icmdtestx.tar " + irodshome + "/icmdtestt_mcol")
        self.admin.assert_icommand("ils -lr " + irodshome + "/icmdtestt_mcol", 'STDOUT_SINGLELINE', ["foo2"])
        self.admin.assert_icommand("ils -lr " + irodshome + "/icmdtestt_mcol", 'STDOUT_SINGLELINE', ["foo1"])
        if os.path.exists(dir_w + "/testt"):
            shutil.rmtree(dir_w + "/testt")
        if os.path.exists(dir_w + "/testx"):
            shutil.rmtree(dir_w + "/testx")
        self.admin.assert_icommand("iget -vr " + irodshome + "/icmdtest  " + dir_w + "/testx", 'STDOUT_SINGLELINE', "testx")
        self.admin.assert_icommand("iget -vr " + irodshome +
                                   "/icmdtestt_mcol/icmdtest  " + dir_w + "/testt", 'STDOUT_SINGLELINE', "testt")
        compare_dirs = filecmp.dircmp(os.path.join(dir_w, 'testx'), os.path.join(dir_w, 'testt'))
        assert (not compare_dirs.right_only and not compare_dirs.left_only and not compare_dirs.diff_files), "Directories differ"
        self.admin.assert_icommand("imkdir " + irodshome + "/icmdtestt_mcol/mydirtt")
        self.admin.assert_icommand("iput " + test_file + " " + irodshome + "/icmdtestt_mcol/mydirtt/foo1mt")
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
        test_file = os.path.join(self.admin.local_session_dir, 'test_file')
        lib.make_file(test_file, 4000, contents='arbitrary')
        myssize = str(os.stat(test_file).st_size)
        username = self.admin.username
        irodszone = self.admin.zone_name
        testuser1 = self.user0.username
        irodshome = self.admin.session_collection
        irodsdefresource = self.admin.default_resource
        dir_w = "."
        sfile2 = dir_w + "/sfile2"
        cat_file_into_file_n_times(test_file, sfile2, 2)
        mysdir = "/tmp/irodssdir"
        myldir = dir_w + "/ldir"
        if os.path.exists(myldir):
            shutil.rmtree(myldir)
        self.admin.assert_icommand("imkdir icmdtest")

        # we put foo1 in $irodsdefresource and foo2 in testresource
        self.admin.assert_icommand("iput -K --wlock " + test_file + " " + irodshome + "/icmdtest/foo1")
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
        with open(lfile, 'wt') as f:
            print('012345678901234567890123456789012345678901234567890123456789012', file=f)
        for i in range(6):
            cat_file_into_file_n_times(lfile, lfile1, 9)
            shutil.move(lfile1, lfile)
        os.mkdir(myldir)
        for i in range(1, 3):
            mylfile = myldir + "/lfile" + str(i)
            mysfile = myldir + "/sfile" + str(i)
            if i != 2:
                shutil.copyfile(lfile, mylfile)
            else:
                shutil.move(lfile, mylfile)
            shutil.copyfile(test_file, mysfile)

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

    @unittest.skip('FIXME')
    def test_phybun_from_devtest(self):
        with session.make_session_for_existing_admin() as rods_admin:
            rods_admin.run_icommand(['ichmod', 'own', '-r', self.admin.username, '/' + self.admin.zone_name])

        # build expected variables with similar devtest names
        test_file = os.path.join(self.admin.local_session_dir, 'test_file')
        lib.make_file(test_file, 4000, contents='arbitrary')
        myssize = str(os.stat(test_file).st_size)
        username = self.admin.username
        irodszone = self.admin.zone_name
        testuser1 = self.user0.username
        irodshome = self.admin.session_collection
        irodsdefresource = self.admin.default_resource
        dir_w = "."
        sfile2 = dir_w + "/sfile2"
        cat_file_into_file_n_times(test_file, sfile2, 2)
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
            shutil.copyfile(test_file, mysfile)

        # make a directory containing 20 small files
        if not os.path.isdir(mysdir):
            os.mkdir(mysdir)
        for i in range(20):
            mysfile = mysdir + "/sfile" + str(i)
            shutil.copyfile(progname, mysfile)

        # iphybun test
        self.admin.assert_icommand("iput -rR " + self.testresc + " " + mysdir + " " + irodshome + "/icmdtestp", "STDOUT_SINGLELINE", ustrings.recurse_ok_string())
        self.admin.assert_icommand("iphybun -KR " + self.anotherresc + " " + irodshome + "/icmdtestp")
        self.admin.assert_icommand("itrim -rS " + self.testresc + " -N1 " +
                                   irodshome + "/icmdtestp", 'STDOUT_SINGLELINE', "files trimmed")
        out, _ = lib.execute_command(['ils', '-L', os.path.join(irodshome, 'icmdtestp', 'sfile1')])
        bunfile = out.split()[-1]
        print(bunfile)
        self.admin.assert_icommand("irepl --purgec -R " + self.anotherresc + " " + bunfile, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')
        self.admin.assert_icommand("itrim -rS " + self.testresc + " -N1 " +
                                   irodshome + "/icmdtestp", 'STDOUT_SINGLELINE', "files trimmed")
        # get the name of bundle file
        self.admin.assert_icommand("irm -f --empty " + bunfile)
        # should not be able to remove it because it is not empty
        self.admin.assert_icommand("ils " + bunfile, 'STDOUT_SINGLELINE', bunfile)
        self.admin.assert_icommand("irm -rvf " + irodshome + "/icmdtestp", 'STDOUT_SINGLELINE', "num files done")
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
        test_file = os.path.join(self.admin.local_session_dir, 'test_file')
        lib.make_file(test_file, 4000, contents='arbitrary')
        myssize = str(os.stat(test_file).st_size)
        username = self.admin.username
        irodszone = self.admin.zone_name
        testuser1 = self.user0.username
        irodshome = self.admin.session_collection
        irodsdefresource = self.admin.default_resource
        dir_w = "."
        sfile2 = dir_w + "/sfile2"
        cat_file_into_file_n_times(test_file, sfile2, 2)
        mysdir = "/tmp/irodssdir"
        myldir = dir_w + "/ldir"
        if os.path.exists(myldir):
            shutil.rmtree(myldir)
        self.admin.assert_icommand("imkdir icmdtest")

        # testing irsync
        self.admin.assert_icommand("irsync " + test_file + " i:" + irodshome + "/icmdtest/foo100")
        self.admin.assert_icommand("irsync i:" + irodshome + "/icmdtest/foo100 " + dir_w + "/foo100")
        self.admin.assert_icommand("irsync i:" + irodshome + "/icmdtest/foo100 i:" + irodshome + "/icmdtest/foo200")
        self.admin.assert_icommand("irm -f " + irodshome + "/icmdtest/foo100 " + irodshome + "/icmdtest/foo200")
        self.admin.assert_icommand("iput -R " + self.testresc + " " + test_file + " " + irodshome + "/icmdtest/foo100")
        self.admin.assert_icommand("irsync " + test_file + " i:" + irodshome + "/icmdtest/foo100")
        self.admin.assert_icommand("iput -R " + self.testresc + " " + test_file + " " + irodshome + "/icmdtest/foo200")
        self.admin.assert_icommand("irsync i:" + irodshome + "/icmdtest/foo100 i:" + irodshome + "/icmdtest/foo200")
        os.unlink(dir_w + "/foo100")

        # cleanup
        os.unlink(sfile2)

    def test_xml_protocol_from_devtest(self):
        # build expected variables with similar devtest names
        test_file = os.path.join(self.admin.local_session_dir, 'test_file')
        lib.make_file(test_file, 4000, contents='arbitrary')
        myssize = str(os.stat(test_file).st_size)
        username = self.admin.username
        irodszone = self.admin.zone_name
        testuser1 = self.user0.username
        irodshome = self.admin.session_collection
        irodsdefresource = self.admin.default_resource
        dir_w = "."
        sfile2 = dir_w + "/sfile2"
        cat_file_into_file_n_times(test_file, sfile2, 2)
        mysdir = "/tmp/irodssdir"
        myldir = dir_w + "/ldir"
        if os.path.exists(myldir):
            shutil.rmtree(myldir)
        self.admin.assert_icommand("imkdir icmdtest")

        lrsfile = dir_w + "/lrsfile"
        rsfile = dir_w + "/rsfile"

        # do test using xml protocol
        os.environ['irodsProt'] = "1"
        self.admin.assert_icommand(['ilsresc', '--ascii'], 'STDOUT_SINGLELINE', self.testresc)
        self.admin.assert_icommand("imiscsvrinfo", 'STDOUT_SINGLELINE', "relVersion")
        self.admin.assert_icommand("iuserinfo", 'STDOUT_SINGLELINE', "name: " + username)
        self.admin.assert_icommand("ienv", 'STDOUT_SINGLELINE', "irods_version")
        self.admin.assert_icommand("icd " + irodshome)
        self.admin.assert_icommand("ipwd", 'STDOUT_SINGLELINE', "home")
        self.admin.assert_icommand("ihelp ils", 'STDOUT_SINGLELINE', "ils")
        self.admin.assert_icommand("ierror -14000", 'STDOUT_SINGLELINE', "SYS_API_INPUT_ERR")
        self.admin.assert_icommand("iexecmd hello", 'STDOUT_SINGLELINE', "Hello world")
        self.admin.assert_icommand("ips -v", 'STDOUT_SINGLELINE', "ips")
        self.admin.assert_icommand("iqstat", 'STDOUT_SINGLELINE', "No delayed rules")
        self.admin.assert_icommand("imkdir " + irodshome + "/icmdtest1")
        # make a directory of large files
        self.admin.assert_icommand("iput -kf  " + test_file + "  " + irodshome + "/icmdtest1/foo1")
        self.admin.assert_icommand("ils -l " + irodshome + "/icmdtest1/foo1", 'STDOUT_SINGLELINE', ["foo1", myssize])
        self.admin.assert_icommand("ils " + irodshome + "/icmdtest1", 'STDOUT_SINGLELINE', "foo1")
        self.admin.assert_icommand("ichmod read " + self.user0.username + " " + irodshome + "/icmdtest1/foo1")
        self.admin.assert_icommand("ils -A " + irodshome + "/icmdtest1/foo1",
                                   'STDOUT_SINGLELINE', self.user0.username + "#" + irodszone + ":read")
        self.admin.assert_icommand("irepl -B -R " + self.testresc + " " + irodshome + "/icmdtest1/foo1")
        # overwrite a copy
        self.admin.assert_icommand("itrim -S  " + irodsdefresource + " -N1 " + irodshome + "/icmdtest1/foo1", 'STDOUT_SINGLELINE', "files trimmed")
        # TODO: this must be skipped because iphymv requires a leaf resource for source and destination
        #self.admin.assert_icommand("iphymv -S " + self.testresc + " -R " + irodsdefresource + " " + irodshome + "/icmdtest/foo1")
        self.admin.assert_icommand("imeta add -d " + irodshome + "/icmdtest1/foo1 testmeta1 180 cm")
        self.admin.assert_icommand("imeta ls -d " + irodshome + "/icmdtest1/foo1", 'STDOUT_SINGLELINE', "testmeta1")
        self.admin.assert_icommand("imeta ls -d " + irodshome + "/icmdtest1/foo1", 'STDOUT_SINGLELINE', "180")
        self.admin.assert_icommand("imeta ls -d " + irodshome + "/icmdtest1/foo1", 'STDOUT_SINGLELINE', "cm")
        self.admin.assert_icommand("icp -K -R " + self.testresc + " " +
                                   irodshome + "/icmdtest1/foo1 " + irodshome + "/icmdtest1/foo2")
        self.admin.assert_icommand("imv " + irodshome + "/icmdtest1/foo2 " + irodshome + "/icmdtest1/foo4")
        self.admin.assert_icommand("imv " + irodshome + "/icmdtest1/foo4 " + irodshome + "/icmdtest1/foo2")
        self.admin.assert_icommand("ichksum -K " + irodshome + "/icmdtest1/foo2")
        self.admin.assert_icommand("iget -f -K " + irodshome + "/icmdtest1/foo2 " + dir_w)
        os.unlink(dir_w + "/foo2")
        self.admin.assert_icommand("irsync " + test_file + " i:" + irodshome + "/icmdtest1/foo1")
        self.admin.assert_icommand("irsync i:" + irodshome + "/icmdtest1/foo1 /tmp/foo1")
        self.admin.assert_icommand("irsync i:" + irodshome + "/icmdtest1/foo1 i:" + irodshome + "/icmdtest1/foo2")
        os.unlink("/tmp/foo1")
        os.environ['irodsProt'] = "0"

        # cleanup
        os.unlink(sfile2)

    def test_large_files_from_devtest(self):
        # build expected variables with similar devtest names
        test_file = os.path.join(self.admin.local_session_dir, 'test_file')
        lib.make_file(test_file, 4000, contents='arbitrary')
        myssize = str(os.stat(test_file).st_size)
        username = self.admin.username
        irodszone = self.admin.zone_name
        testuser1 = self.user0.username
        irodshome = self.admin.session_collection
        irodsdefresource = self.admin.default_resource
        dir_w = "."
        sfile2 = dir_w + "/sfile2"
        cat_file_into_file_n_times(test_file, sfile2, 2)
        mysdir = "/tmp/irodssdir"
        myldir = dir_w + "/ldir"
        if os.path.exists(myldir):
            shutil.rmtree(myldir)
        self.admin.assert_icommand("imkdir icmdtest")

        # make a directory of 2 large files and 2 small files
        lfile = dir_w + "/lfile"
        lfile1 = dir_w + "/lfile1"
        with open(lfile, 'wt') as f:
            print('012345678901234567890123456789012345678901234567890123456789012', file=f)
        for i in range(6):
            cat_file_into_file_n_times(lfile, lfile1, 9)
            shutil.move(lfile1, lfile)
        os.mkdir(myldir)
        for i in range(1, 3):
            mylfile = myldir + "/lfile" + str(i)
            mysfile = myldir + "/sfile" + str(i)
            if i != 2:
                shutil.copyfile(lfile, mylfile)
            else:
                shutil.move(lfile, mylfile)
            shutil.copyfile(test_file, mysfile)

        # do the large files tests
        lrsfile = dir_w + "/lrsfile"
        rsfile = dir_w + "/rsfile"
        if os.path.isfile(lrsfile):
            os.unlink(lrsfile)
        if os.path.isfile(rsfile):
            os.unlink(rsfile)
        self.admin.assert_icommand("iput -vbPr --retries 10 --wlock -X " + rsfile + " --lfrestart " +
                                   lrsfile + " -N 2 " + myldir + " " + irodshome + "/icmdtest/testy", 'STDOUT_SINGLELINE', "New restartFile")
        if os.path.isfile(lrsfile):
            os.unlink(lrsfile)
        if os.path.isfile(rsfile):
            os.unlink(rsfile)
        self.admin.assert_icommand("irepl -BvrPT -R " + self.testresc + " --rlock " +
                                   irodshome + "/icmdtest/testy", 'STDOUT_SINGLELINE', "icmdtest/testy")
        self.admin.assert_icommand("itrim -vrS " + irodsdefresource + " --dryrun --age 1 -N 1 " +
                                   irodshome + "/icmdtest/testy", 'STDOUT_SINGLELINE', "This is a DRYRUN")
        self.admin.assert_icommand("itrim -vrS " + irodsdefresource + " -N 1 " +
                                   irodshome + "/icmdtest/testy", 'STDOUT_SINGLELINE', "a copy trimmed")
        self.admin.assert_icommand("icp -vKPTr -N 2 " + irodshome + "/icmdtest/testy " +
                                   irodshome + "/icmdtest/testz", 'STDOUT_SINGLELINE', "Processing lfile1")
        self.admin.assert_icommand("irsync -r i:" + irodshome + "/icmdtest/testy i:" + irodshome + "/icmdtest/testz")
        self.admin.assert_icommand("irm -vrf " + irodshome + "/icmdtest/testy")
        # TODO: this must be skipped because iphymv requires a leaf resource for source and destination
        #self.admin.assert_icommand("iphymv -vrS " + irodsdefresource + " -R " +
                                   #self.testresc + " " + irodshome + "/icmdtest/testz", 'STDOUT_SINGLELINE', "icmdtest/testz")

        if os.path.isfile(lrsfile):
            os.unlink(lrsfile)
        if os.path.isfile(rsfile):
            os.unlink(rsfile)
        if os.path.exists(dir_w + "/testz"):
            shutil.rmtree(dir_w + "/testz")
        self.admin.assert_icommand("iget -vPKr --retries 10 -X " + rsfile + " --lfrestart " + lrsfile +
                                   " --rlock -N 2 " + irodshome + "/icmdtest/testz " + dir_w + "/testz", 'STDOUT_SINGLELINE', "testz")
        self.admin.assert_icommand("irsync -r " + dir_w + "/testz i:" + irodshome + "/icmdtest/testz", "STDOUT_SINGLELINE", ustrings.recurse_ok_string())
        self.admin.assert_icommand("irsync -r i:" + irodshome + "/icmdtest/testz " + dir_w + "/testz")
        if os.path.isfile(lrsfile):
            os.unlink(lrsfile)
        if os.path.isfile(rsfile):
            os.unlink(rsfile)
        compare_dirs = filecmp.dircmp(os.path.join(dir_w, 'testz'), myldir)
        assert (not compare_dirs.right_only and not compare_dirs.left_only and not compare_dirs.diff_files), "Directories differ"
        # test -N0 transfer
        self.admin.assert_icommand("iput -N0 -R " + self.testresc + " " +
                                   myldir + "/lfile1 " + irodshome + "/icmdtest/testz/lfoo100")
        if os.path.isfile(dir_w + "/lfoo100"):
            os.unlink(dir_w + "/lfoo100")
        self.admin.assert_icommand("iget -N0 " + irodshome + "/icmdtest/testz/lfoo100 " + dir_w + "/lfoo100")
        assert filecmp.cmp(os.path.join(myldir, 'lfile1'), os.path.join(dir_w, 'lfoo100'))
        shutil.rmtree(dir_w + "/testz")
        os.unlink(dir_w + "/lfoo100")
        self.admin.assert_icommand("irm -vrf " + irodshome + "/icmdtest/testz")

        # cleanup
        os.unlink(sfile2)
        if os.path.exists(myldir):
            shutil.rmtree(myldir)

    @unittest.skipIf(test.settings.USE_SSL, 'RBUDP does not support encryption')
    def test_large_files_with_RBUDP_from_devtest(self):
        # build expected variables with similar devtest names
        test_file = os.path.join(self.admin.local_session_dir, 'test_file')
        lib.make_file(test_file, 4000, contents='arbitrary')
        myssize = str(os.stat(test_file).st_size)
        username = self.admin.username
        irodszone = self.admin.zone_name
        testuser1 = self.user0.username
        irodshome = self.admin.session_collection
        irodsdefresource = self.admin.default_resource
        dir_w = "."
        sfile2 = dir_w + "/sfile2"
        cat_file_into_file_n_times(test_file, sfile2, 2)
        mysdir = "/tmp/irodssdir"
        myldir = dir_w + "/ldir"
        if os.path.exists(myldir):
            shutil.rmtree(myldir)
        self.admin.assert_icommand("imkdir icmdtest")

        # make a directory of 2 large files and 2 small files
        lfile = dir_w + "/lfile"
        lfile1 = dir_w + "/lfile1"
        with open(lfile, 'wt') as f:
            print('012345678901234567890123456789012345678901234567890123456789012', file=f)
        for i in range(6):
            cat_file_into_file_n_times(lfile, lfile1, 9)
            shutil.move(lfile1, lfile)
        os.mkdir(myldir)
        for i in range(1, 3):
            mylfile = myldir + "/lfile" + str(i)
            mysfile = myldir + "/sfile" + str(i)
            if i != 2:
                shutil.copyfile(lfile, mylfile)
            else:
                shutil.move(lfile, mylfile)
            shutil.copyfile(test_file, mysfile)

        # do the large files tests using RBUDP
        lrsfile = dir_w + "/lrsfile"
        rsfile = dir_w + "/rsfile"
        if os.path.isfile(lrsfile):
            os.unlink(lrsfile)
        if os.path.isfile(rsfile):
            os.unlink(rsfile)
        self.admin.assert_icommand("iput -vQPKr --retries 10 -X " + rsfile + " --lfrestart " +
                                   lrsfile + " " + myldir + " " + irodshome + "/icmdtest/testy", 'STDOUT_SINGLELINE', "icmdtest/testy")
        self.admin.assert_icommand("irepl -BQvrPT -R " + self.testresc + " " +
                                   irodshome + "/icmdtest/testy", 'STDOUT_SINGLELINE', "icmdtest/testy")
        self.admin.assert_icommand("itrim -vrS " + irodsdefresource + " -N 1 " +
                                   irodshome + "/icmdtest/testy", 'STDOUT_SINGLELINE', "a copy trimmed")
        self.admin.assert_icommand("icp -vQKPTr " + irodshome + "/icmdtest/testy " +
                                   irodshome + "/icmdtest/testz", 'STDOUT_SINGLELINE', "Processing sfile1")
        self.admin.assert_icommand("irm -vrf " + irodshome + "/icmdtest/testy")
        if os.path.isfile(lrsfile):
            os.unlink(lrsfile)
        if os.path.isfile(rsfile):
            os.unlink(rsfile)
        if os.path.exists(dir_w + "/testz"):
            shutil.rmtree(dir_w + "/testz")
        self.admin.assert_icommand("iget -vQPKr --retries 10 -X " + rsfile + " --lfrestart " + lrsfile +
                                   " " + irodshome + "/icmdtest/testz " + dir_w + "/testz", 'STDOUT_SINGLELINE', "Processing sfile2")
        if os.path.isfile(lrsfile):
            os.unlink(lrsfile)
        if os.path.isfile(rsfile):
            os.unlink(rsfile)
        compare_dirs = filecmp.dircmp(os.path.join(dir_w, 'testz'), myldir)
        assert (not compare_dirs.right_only and not compare_dirs.left_only and not compare_dirs.diff_files), "Directories differ"
        shutil.rmtree(dir_w + "/testz")
        self.admin.assert_icommand("irm -vrf " + irodshome + "/icmdtest/testz")
        shutil.rmtree(myldir)

        # cleanup
        os.unlink(sfile2)
        if os.path.exists(myldir):
            shutil.rmtree(myldir)

def cat_file_into_file_n_times(inpath, outpath, n=1):
    with open(outpath, 'wt') as outfile:
        with open(inpath, 'r') as infile:
            for i in range(0, 2):
                infile.seek(0)
                while True:
                    data = infile.read(2**20)
                    if data:
                        print(data, file=outfile, end='')
                    else:
                        break
