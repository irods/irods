import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
from resource_suite import ResourceBase
import commands
import distutils.spawn
import os
import subprocess
import stat
import socket


class Test_FuseSuite(ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_FuseSuite, self).setUp()

    def tearDown(self):
        super(Test_FuseSuite, self).tearDown()

    def test_irodsFs_issue_2252(self):
        # =-=-=-=-=-=-=-
        # set up a fuse mount
        mount_point = "fuse_mount_point"

        if not os.path.isdir(mount_point):
            os.mkdir(mount_point)
        os.system("irodsFs " + mount_point)

        largefilename = "big_file.txt"
        output = commands.getstatusoutput('dd if=/dev/zero of=' + largefilename + ' bs=1M count=100')

        # =-=-=-=-=-=-=-
        # use system copy to put some data into the mount mount
        # and verify that it shows up in the ils
        cmd = "cp ./" + largefilename + " ./" + mount_point + "; ls ./" + mount_point + "/" + largefilename
        output = commands.getstatusoutput(cmd)
        out_str = str(output)
        print("results[" + out_str + "]")

        os.system("rm ./" + largefilename)
        os.system("rm ./" + mount_point + "/" + largefilename)

        # tear down the fuse mount
        os.system("fusermount -uz " + mount_point)
        if os.path.isdir(mount_point):
            os.rmdir(mount_point)

        assert(-1 != out_str.find(largefilename))

    def test_fusermount_permissions(self):
        # Check that fusermount is configured correctly for irodsFs
        fusermount_path = distutils.spawn.find_executable('fusermount')
        assert fusermount_path is not None, 'fusermount binary not found'
        assert os.stat(fusermount_path).st_mode & stat.S_ISUID, 'fusermount setuid bit not set'
        p = subprocess.Popen(['fusermount -V'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        stdoutdata, stderrdata = p.communicate()
        assert p.returncode == 0, '\n'.join(['fusermount not executable',
                                             'return code: ' + str(p.returncode),
                                             'stdout: ' + stdoutdata,
                                             'stderr: ' + stderrdata])

    def test_irodsFs(self):
        # =-=-=-=-=-=-=-
        # set up a fuse mount
        mount_point = "fuse_mount_point"

        if not os.path.isdir(mount_point):
            os.mkdir(mount_point)
        os.system("irodsFs " + mount_point)

        # =-=-=-=-=-=-=-
        # put some test data
        cmd = "iput README foo0"
        output = commands.getstatusoutput(cmd)

        # =-=-=-=-=-=-=-
        # see if the data object is actually in the mount point
        # using the system ls
        cmd = "ls -l " + mount_point
        output = commands.getstatusoutput(cmd)
        out_str = str(output)
        print("mount ls results [" + out_str + "]")
        assert(-1 != out_str.find("foo0"))

        # =-=-=-=-=-=-=-
        # use system copy to put some data into the mount mount
        # and verify that it shows up in the ils
        cmd = "cp README " + mount_point + "/baz ; ils -l baz"
        output = commands.getstatusoutput(cmd)
        out_str = str(output)
        print("results[" + out_str + "]")
        assert(-1 != out_str.find("baz"))

        # =-=-=-=-=-=-=-
        # now irm the file and verify that it is not visible
        # via the fuse mount
        cmd = "irm -f baz ; ils -l baz"
        output = commands.getstatusoutput(cmd)
        out_str = str(output)
        print("results[" + out_str + "]")
        assert(-1 != out_str.find("baz does not exist"))

        output = commands.getstatusoutput("ls -l " + mount_point)
        out_str = str(output)
        print("mount ls results [" + out_str + "]")
        assert(-1 != out_str.find("foo0"))

        # =-=-=-=-=-=-=-
        # now rm the foo0 file and then verify it doesnt show
        # up in the ils
        cmd = "rm " + mount_point + "/foo0; ils -l foo0"
        print("cmd: [" + cmd + "]")
        output = commands.getstatusoutput(cmd)
        out_str = str(output)
        print("results[" + out_str + "]")
        assert(-1 != out_str.find("foo0 does not exist"))

        # =-=-=-=-=-=-=-
        # now run bonnie++ and then verify it reports a summary
        if (os.path.isfile("/usr/sbin/bonnie++")):
            # ubuntu and centos
            bonniecmd = "/usr/sbin/bonnie++"
        else:
            # suse
            bonniecmd = "/usr/bin/bonnie++"
        cmd = bonniecmd + " -r 1024 -d " + mount_point
        print("cmd: [" + cmd + "]")
        output = commands.getstatusoutput(cmd)
        out_str = str(output)
        print("results[" + out_str + "]")
        assert(-1 != out_str.find("-Per Chr- --Block-- -Rewrite- -Per Chr- --Block-- --Seeks--"))
        assert(-1 != out_str.find("-Create-- --Read--- -Delete-- -Create-- --Read--- -Delete--"))

        # tear down the fuse mount
        os.system("fusermount -uz " + mount_point)
        if os.path.isdir(mount_point):
            os.rmdir(mount_point)
