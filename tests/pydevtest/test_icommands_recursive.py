import sys
if (sys.version_info >= (2,7)):
    import unittest
else:
    import unittest2 as unittest
import commands
import os
import shutil
import psutil
import pydevtest_sessions as s
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd, getiCmdOutput
from resource_suite import ResourceBase

def make_file(f_name, f_size, source='/dev/zero'):
    output = commands.getstatusoutput('dd if="' + source + '" of="' + f_name + '" count=' + str(f_size) + ' bs=1')
    if output[0] != 0:
        sys.stderr.write(output[1] + '\n')
        raise OSError(output[0], "call to dd returned non-zero")

def runCmd_ils_to_entries(runCmd_output):
    raw = runCmd_output[0].strip().split('\n')
    collection = raw[0]
    entries = [entry.strip() for entry in raw[1:]]
    return entries

def get_vault_path(session):
    cmdout = session.runCmd("iquest", ["%s", "select RESC_VAULT_PATH where RESC_NAME = 'demoResc'"])
    if cmdout[1] != "":
        raise OSError(cmdout[1], "iquest wrote to stderr when called from get_vault_path()")
    return cmdout[0].rstrip('\n')

def get_vault_session_path(session):
    return os.path.join(get_vault_path(session),
                        "home",
                        session.getUserName(),
                        session.sessionId)

def make_large_local_tmp_dir(dir_name, file_count, file_size):
    os.mkdir(dir_name)
    for i in range(file_count):
        make_file(os.path.join(dir_name, "junk"+str(i).zfill(4)),
                  file_size)
    local_files = os.listdir(dir_name)
    assert len(local_files) == file_count, "dd loop did not make all " + str(file_count) + " files"
    return local_files


class Test_ICommands_Recursive(unittest.TestCase, ResourceBase):

    my_test_resource = {"setup":[],"teardown":[]}

    def setUp(self):
        ResourceBase.__init__(self)
        s.twousers_up()
        self.run_resource_setup()
        self.testing_tmp_dir = '/tmp/irods-test-icommands-recursive'
        shutil.rmtree(self.testing_tmp_dir, ignore_errors=True)
        os.mkdir(self.testing_tmp_dir)

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()
        shutil.rmtree(self.testing_tmp_dir)

    def iput_r_large_collection(self, session, base_name, file_count, file_size):
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        local_files = make_large_local_tmp_dir(local_dir, file_count, file_size)
        assertiCmd(session, "iput -r " + local_dir, "EMPTY")
        rods_files = set(runCmd_ils_to_entries(session.runCmd("ils", [base_name])))
        self.assertTrue(set(local_files) == rods_files,
                        msg="Files missing from rods:\n" + str(set(local_files)-rods_files) + "\n\n" + \
                            "Extra files in rods:\n" + str(rods_files-set(local_files)))
        vault_files = set(os.listdir(os.path.join(get_vault_session_path(session),
                                                  base_name)))
        self.assertTrue(set(local_files) == vault_files,
                        msg="Files missing from vault:\n" + str(set(local_files)-vault_files) + "\n\n" + \
                            "Extra files in vault:\n" + str(vault_files-set(local_files)))

        return (local_dir, local_files)

    def test_iput_r(self):
        self.iput_r_large_collection(s.sessions[1], "test_iput_r_dir", file_count=1000, file_size=100)

    def test_irm_r(self):
        base_name = "test_irm_r_dir"
        user_session = s.sessions[1]
        self.iput_r_large_collection(user_session, base_name, file_count=1000, file_size=100)

        assertiCmd(user_session, "irm -r " + base_name, "EMPTY")
        assertiCmd(user_session, "ils " + base_name, "ERROR", "does not exist")
        
        vault_files_post_irm = os.listdir(os.path.join(get_vault_session_path(user_session),
                                                       base_name))
        self.assertTrue(len(vault_files_post_irm) == 0,
                        msg="Files not removed from vault:\n" + str(vault_files_post_irm))
        

    def test_imv_r(self):
        base_name_source = "test_imv_r_dir_source"
        user_session = s.sessions[1]
        file_names = set(self.iput_r_large_collection(user_session, base_name_source, file_count=1000, file_size=100)[1])

        base_name_target = "test_imv_r_dir_target"
        assertiCmd(user_session, "imv " + base_name_source + " " + base_name_target, "EMPTY")
        assertiCmd(user_session, "ils " + base_name_source, "ERROR", "does not exist")
        assertiCmd(user_session, "ils", "LIST", base_name_target)
        rods_files_post_imv = set(runCmd_ils_to_entries(user_session.runCmd("ils", [base_name_target])))
        self.assertTrue(file_names == rods_files_post_imv,
                        msg="Files missing from rods:\n" + str(file_names-rods_files_post_imv) + "\n\n" + \
                            "Extra files in rods:\n" + str(rods_files_post_imv-file_names))
        
        vault_files_post_irm_source = set(os.listdir(os.path.join(get_vault_session_path(user_session),
                                                                  base_name_source)))
        self.assertTrue(len(vault_files_post_irm_source) == 0)

        vault_files_post_irm_target = set(os.listdir(os.path.join(get_vault_session_path(user_session),
                                                           base_name_target)))
        self.assertTrue(file_names == vault_files_post_irm_target,
                        msg="Files missing from vault:\n" + str(file_names-vault_files_post_irm_target) + "\n\n" + \
                            "Extra files in vault:\n" + str(vault_files_post_irm_target-file_names))

    def test_icp_r(self):
        base_name_source = "test_icp_r_dir_source"
        user_session = s.sessions[1]
        file_names = set(self.iput_r_large_collection(user_session, base_name_source, file_count=1000, file_size=100)[1])

        base_name_target = "test_icp_r_dir_target"
        assertiCmd(user_session, "icp -r " + base_name_source + " " + base_name_target, "EMPTY")
        assertiCmd(user_session, "ils", "LIST", base_name_target)
        rods_files_source = set(runCmd_ils_to_entries(user_session.runCmd("ils", [base_name_source])))
        self.assertTrue(file_names == rods_files_source,
                        msg="Files missing from rods source:\n" + str(file_names-rods_files_source) + "\n\n" + \
                            "Extra files in rods source:\n" + str(rods_files_source-file_names))

        rods_files_target = set(runCmd_ils_to_entries(user_session.runCmd("ils", [base_name_target])))
        self.assertTrue(file_names == rods_files_target,
                        msg="Files missing from rods target:\n" + str(file_names-rods_files_target) + "\n\n" + \
                            "Extra files in rods target:\n" + str(rods_files_target-file_names))     

        vault_files_post_icp_source = set(os.listdir(os.path.join(get_vault_session_path(user_session),
                                                                  base_name_source)))

        self.assertTrue(file_names == vault_files_post_icp_source,
                        msg="Files missing from vault:\n" + str(file_names-vault_files_post_icp_source) + "\n\n" + \
                            "Extra files in vault:\n" + str(vault_files_post_icp_source-file_names))


        vault_files_post_icp_target = set(os.listdir(os.path.join(get_vault_session_path(user_session),
                                                                  base_name_target)))
        self.assertTrue(file_names == vault_files_post_icp_target,
                        msg="Files missing from vault:\n" + str(file_names-vault_files_post_icp_target) + "\n\n" + \
                            "Extra files in vault:\n" + str(vault_files_post_icp_target-file_names))

    def test_irsync_r_dir_to_coll(self):
        base_name = "test_irsync_r_dir_to_coll"
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        file_names = set(make_large_local_tmp_dir(local_dir, file_count=1000, file_size=100))
        user_session = s.sessions[1]

        assertiCmd(user_session, "irsync -r " + local_dir + " i:" + base_name, "EMPTY")
        assertiCmd(user_session, "ils", "LIST", base_name)
        rods_files = set(runCmd_ils_to_entries(user_session.runCmd("ils", [base_name])))
        self.assertTrue(file_names == rods_files,
                        msg="Files missing from rods:\n" + str(file_names-rods_files) + "\n\n" + \
                            "Extra files in rods:\n" + str(rods_files-file_names))

        vault_files_post_irsync = set(os.listdir(os.path.join(get_vault_session_path(user_session),
                                                              base_name)))

        self.assertTrue(file_names == vault_files_post_irsync,
                        msg="Files missing from vault:\n" + str(file_names-vault_files_post_irsync) + "\n\n" + \
                            "Extra files in vault:\n" + str(vault_files_post_irsync-file_names))

    def test_irsync_r_coll_to_coll(self):
        base_name_source = "test_irsync_r_coll_to_coll_source"
        user_session = s.sessions[1]
        file_names = set(self.iput_r_large_collection(user_session, base_name_source, file_count=1000, file_size=100)[1])
        base_name_target = "test_irsync_r_coll_to_coll_target"
        assertiCmd(user_session, "irsync -r i:" + base_name_source + " i:" + base_name_target, "EMPTY")
        assertiCmd(user_session, "ils", "LIST", base_name_source)
        assertiCmd(user_session, "ils", "LIST", base_name_target)
        rods_files_source = set(runCmd_ils_to_entries(user_session.runCmd("ils", [base_name_source])))
        self.assertTrue(file_names == rods_files_source,
                        msg="Files missing from rods source:\n" + str(file_names-rods_files_source) + "\n\n" + \
                            "Extra files in rods source:\n" + str(rods_files_source-file_names))

        rods_files_target = set(runCmd_ils_to_entries(user_session.runCmd("ils", [base_name_target])))
        self.assertTrue(file_names == rods_files_target,
                        msg="Files missing from rods target:\n" + str(file_names-rods_files_target) + "\n\n" + \
                            "Extra files in rods target:\n" + str(rods_files_target-file_names))

        vault_files_post_irsync_source = set(os.listdir(os.path.join(get_vault_session_path(user_session),
                                                                     base_name_source)))

        self.assertTrue(file_names == vault_files_post_irsync_source,
                        msg="Files missing from vault:\n" + str(file_names-vault_files_post_irsync_source) + "\n\n" + \
                            "Extra files in vault:\n" + str(vault_files_post_irsync_source-file_names))

        vault_files_post_irsync_target = set(os.listdir(os.path.join(get_vault_session_path(user_session),
                                                                     base_name_target)))

        self.assertTrue(file_names == vault_files_post_irsync_target,
                        msg="Files missing from vault:\n" + str(file_names-vault_files_post_irsync_target) + "\n\n" + \
                            "Extra files in vault:\n" + str(vault_files_post_irsync_target-file_names))

    def test_irsync_r_coll_to_dir(self):
        base_name_source = "test_irsync_r_coll_to_dir_source"
        user_session = s.sessions[1]
        file_names = set(self.iput_r_large_collection(user_session, base_name_source, file_count=1000, file_size=100)[1])
        local_dir = os.path.join(self.testing_tmp_dir, "test_irsync_r_coll_to_dir_target")
        assertiCmd(user_session, "irsync -r i:" + base_name_source + " " + local_dir, "EMPTY")
        assertiCmd(user_session, "ils", "LIST", base_name_source)
        rods_files_source = set(runCmd_ils_to_entries(user_session.runCmd("ils", [base_name_source])))
        self.assertTrue(file_names == rods_files_source,
                        msg="Files missing from rods source:\n" + str(file_names-rods_files_source) + "\n\n" + \
                            "Extra files in rods source:\n" + str(rods_files_source-file_names))

        local_files = set(os.listdir(local_dir))
        self.assertTrue(file_names == local_files,
                        msg="Files missing from local dir:\n" + str(file_names-local_files) + "\n\n" + \
                            "Extra files in local dir:\n" + str(local_files-file_names))

        vault_files_post_irsync_source = set(os.listdir(os.path.join(get_vault_session_path(user_session),
                                                                     base_name_source)))

        self.assertTrue(file_names == vault_files_post_irsync_source,
                        msg="Files missing from vault:\n" + str(file_names-vault_files_post_irsync_source) + "\n\n" + \
                            "Extra files in vault:\n" + str(vault_files_post_irsync_source-file_names))

