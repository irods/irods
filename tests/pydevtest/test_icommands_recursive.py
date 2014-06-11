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
    output = commands.getstatusoutput('dd if="' + source + '" of="' + f_name + '" count=' + str(f_size) + ' iflag=count_bytes')
    if output[0] != 0:
        raise OSError(output[0], "dd returned non-zero")

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

    def test_iput_r(self):
        base_name = "test_iput_r_dir"
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        os.mkdir(local_dir)
        n_files = 1000
        file_size = 100
        for i in range(n_files):
            make_file(os.path.join(local_dir, "junk"+str(i).zfill(4)),
                      file_size)
        local_files = set(os.listdir(local_dir))
        assert len(local_files) == n_files, "dd loop did not make all " + str(n_files) + " files"
        alice_session = s.sessions[1]
        assertiCmd(alice_session, "iput -r " + local_dir, "EMPTY")
        rods_files = set(runCmd_ils_to_entries(alice_session.runCmd("ils", [base_name])))
        self.assertTrue(local_files == rods_files,
                        msg="Files missing from rods:\n" + str(local_files-rods_files) + "\n\n" + \
                            "Extra files in rods:\n" + str(rods_files-local_files))
        vault_files = set(os.listdir(os.path.join(get_vault_session_path(alice_session),
                                                  base_name)))
        self.assertTrue(local_files == vault_files,
                        msg="Files missing from vault:\n" + str(local_files-vault_files) + "\n\n" + \
                            "Extra files in vault:\n" + str(vault_files-local_files))

    def test_irm_r(self):
        base_name = "test_irm_r_dir"
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        os.mkdir(local_dir)
        n_files = 1000
        file_size = 100
        for i in range(n_files):
            make_file(os.path.join(local_dir, "junk"+str(i).zfill(4)),
                      file_size)
        local_files = set(os.listdir(local_dir))
        assert len(local_files) == n_files, "dd loop did not make all " + str(n_files) + " files"
        alice_session = s.sessions[1]
        assertiCmd(alice_session, "iput -r " + local_dir, "EMPTY")
        rods_files_pre_irm = set(runCmd_ils_to_entries(alice_session.runCmd("ils", [base_name])))
        self.assertTrue(local_files == rods_files_pre_irm,
                        msg="Files missing from rods:\n" + str(local_files-rods_files_pre_irm) + "\n\n" + \
                            "Extra files in rods:\n" + str(rods_files_pre_irm-local_files))
        vault_files_pre_irm = set(os.listdir(os.path.join(get_vault_session_path(alice_session),
                                                  base_name)))
        self.assertTrue(local_files == vault_files_pre_irm,
                        msg="Files missing from vault:\n" + str(local_files-vault_files_pre_irm) + "\n\n" + \
                            "Extra files in vault:\n" + str(vault_files_pre_irm-local_files))

        # Files have been successfully added, try removing them
        assertiCmd(alice_session, "irm -r " + base_name, "EMPTY")
        assertiCmd(alice_session, "ils " + base_name, "ERROR", "does not exist")
        
        vault_files_post_irm = set(os.listdir(os.path.join(get_vault_session_path(alice_session),
                                                  base_name)))
        self.assertTrue(len(vault_files_post_irm) == 0,
                        msg="Files not removed from vault:\n" + str(vault_files_post_irm))
        

    def test_imv_r(self):
        base_name_source = "test_imv_r_dir_source"
        local_dir = os.path.join(self.testing_tmp_dir, base_name_source)
        os.mkdir(local_dir)
        n_files = 1000
        file_size = 100
        for i in range(n_files):
            make_file(os.path.join(local_dir, "junk"+str(i).zfill(4)),
                      file_size)
        local_files = set(os.listdir(local_dir))
        assert len(local_files) == n_files, "dd loop did not make all " + str(n_files) + " files"
        alice_session = s.sessions[1]
        assertiCmd(alice_session, "iput -r " + local_dir, "EMPTY")
        rods_files_pre_imv = set(runCmd_ils_to_entries(alice_session.runCmd("ils", [base_name_source])))
        self.assertTrue(local_files == rods_files_pre_imv,
                        msg="Files missing from rods:\n" + str(local_files-rods_files_pre_imv) + "\n\n" + \
                            "Extra files in rods:\n" + str(rods_files_pre_imv-local_files))
        vault_files_pre_irm = set(os.listdir(os.path.join(get_vault_session_path(alice_session),
                                                          base_name_source)))
        self.assertTrue(local_files == vault_files_pre_irm,
                        msg="Files missing from vault:\n" + str(local_files-vault_files_pre_irm) + "\n\n" + \
                            "Extra files in vault:\n" + str(vault_files_pre_irm-local_files))

        # Files have been successfully added, try moving them
        base_name_target = "test_imv_r_dir_target"
        assertiCmd(alice_session, "imv " + base_name_source + " " + base_name_target, "EMPTY")
        assertiCmd(alice_session, "ils " + base_name_source, "ERROR", "does not exist")
        assertiCmd(alice_session, "ils", "LIST", base_name_target)
        rods_files_post_imv = set(runCmd_ils_to_entries(alice_session.runCmd("ils", [base_name_target])))
        self.assertTrue(local_files == rods_files_post_imv,
                        msg="Files missing from rods:\n" + str(local_files-rods_files_post_imv) + "\n\n" + \
                            "Extra files in rods:\n" + str(rods_files_post_imv-local_files))
        
        vault_files_post_irm = set(os.listdir(os.path.join(get_vault_session_path(alice_session),
                                                           base_name_target)))
        self.assertTrue(local_files == vault_files_post_irm,
                        msg="Files missing from vault:\n" + str(local_files-vault_files_post_irm) + "\n\n" + \
                            "Extra files in vault:\n" + str(vault_files_post_irm-local_files))



        
        
                      

        





