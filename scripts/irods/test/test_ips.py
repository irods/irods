import unittest
from threading import Event, Thread

from . import resource_suite
from .. import lib

class Test_Ips(resource_suite.ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Ips, self).setUp()

    def tearDown(self):
        super(Test_Ips, self).tearDown()

    @staticmethod
    def ips_with_command(cmd):
        def run_command(e, cmd):
            e.wait()
            lib.execute_command(cmd)

        def run_ips(e, cmd, output):
            e.wait()
            out,_ = lib.execute_command(['ips', '-a'])
            output.append(out)

        threads = []
        execute_command_output = [None]
        e = Event()
        # Set up a thread for the command and a thread for ips (ips must come last)
        threads.append(Thread(target=run_command, args=[e, cmd]))
        threads.append(Thread(target=run_ips, args=[e, cmd, execute_command_output]))
        # Need to set an event so that command is still running when ips runs
        for t in threads:
            t.start()
        e.set()
        for t in threads:
            t.join()

        # Make sure the name of the icommand appears in the output
        return execute_command_output.pop()

    def test_icommand_process_names(self):
        commands = ['izonereport', 'iapitest', 'iclienthints']
        for cmd in commands:
            self.assertTrue(cmd in self.ips_with_command(cmd))
