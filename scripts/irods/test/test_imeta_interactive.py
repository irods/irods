from __future__ import print_function
import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
from .. import lib
from . import session
from subprocess import PIPE
from fcntl import fcntl, F_SETFL, F_GETFL
from time import process_time
from os import O_NONBLOCK
from array import array

'''
This class takes the performance benefit that comes from using setUpClass/tearDownClass methods
and just throws it away.

imeta doesn't have a way outside of stdout and stderr to tell us when it's done with a command,
so after we issue a command, we have to wait until imeta is done printing to stdout and stderr.
However, our test is faster than imeta, so we have to give it time to do its thing, so instead
of waiting for stdout and stderr to run out (it's probably still empty when we start looking at
it), we keep trying to pull characters out until a certain amount of time has passed since the
last character.
Some solutions of similar problems on stack overflow look for the prompt string ("imeta>"), but
this isn't an applicable solution for us, since the prompt string is one of the things we're
testing, and since it might not actually be there for some of our test cases.

And yes, we do have to pull stdout/stderr out one character at a time. We have no idea how much
stdout/stderr there is ahead of us, and trying to pull more than is available will cause what's
available to be lost, even if the exception is caught.
'''

def read_stream(stream, timeout):
    '''Reads from a byte stream and returns an array of bytes after a timeout or end of stream.

    Arguments:
    stream -- a file-like object from which bytes can be read
    timeout -- a float representing the amount of time to wait for the stream to start
    '''
    arr = array('b')
    timestamp = None
    while True:
        char = stream.read(1)
        # The output may not have started yet so we wait until it starts or times out.
        if char is None:
            if timestamp is None:
                timestamp = process_time()
            elif process_time() - timestamp > timeout:
                break
            continue
        # The end of the stream is indicated by an empty string so break the loop here.
        if char.decode() == '':
            break
        arr.frombytes(char)
        timestamp = None
    return arr

class Test_Imeta_Interactive(unittest.TestCase):

    default_timeout = 0.5

    def get_output(self, timeout=default_timeout):
        return (read_stream(self.imeta_p.stdout, timeout).tobytes().decode(),
                read_stream(self.imeta_p.stderr, timeout).tobytes().decode())

    @classmethod
    def setUpClass(cls):
        cls.admin = session.mkuser_and_return_session('rodsadmin', 'otherrods', 'rods', lib.get_hostname())
        cls.test_data_path_base = cls.admin.session_collection + '/imeta_test_data'
        cls.test_data_paths_wildcard = cls.test_data_path_base + '%'
        cls.test_univ_attr = 'some_attr'
        cls.test_data_qty = 5
        for n in range(cls.test_data_qty):
            data_path = cls.test_data_path_base + str(n)
            cls.admin.assert_icommand(['itouch', data_path])
            cls.admin.assert_icommand(['imeta', 'add', '-d', data_path, cls.test_univ_attr, str(n * 5)])

    @classmethod
    def tearDownClass(cls):
        with session.make_session_for_existing_admin() as admin_session:
            cls.admin.__exit__()
            admin_session.assert_icommand(['iadmin', 'rmuser', cls.admin.username])

    # Since imeta could terminate during the tests, each test gets its own instance
    def setUp(self):
        kwargs = dict()
        self.admin._prepare_run_icommand('imeta', kwargs)
        self.imeta_p = lib.execute_command_nonblocking('imeta', stdout=PIPE, stderr=PIPE, stdin=PIPE, **kwargs)
        self.assertIsNone(self.imeta_p.poll())

        # Don't block when we run out of stdout/stderr
        fcntl(self.imeta_p.stdout, F_SETFL, fcntl(self.imeta_p.stdout, F_GETFL) | O_NONBLOCK)
        fcntl(self.imeta_p.stderr, F_SETFL, fcntl(self.imeta_p.stderr, F_GETFL) | O_NONBLOCK)

    def tearDown(self):
        if self.imeta_p.poll() is None:
            self.imeta_p.terminate()
        if self.imeta_p.poll() is None:
            self.imeta_p.communicate()

    def test_prompt(self):
        (out, err) = self.get_output()
        self.assertEqual(out, 'imeta>')
        self.assertEqual(err, '')

    def test_empty_cmd(self):
        self.get_output()
        self.imeta_p.stdin.write(b'\n')
        self.imeta_p.stdin.flush()
        (out, err) = self.get_output()
        self.assertEqual(out, 'imeta>')
        self.assertEqual(err, '')
        self.imeta_p.stdin.write(b'\n')
        self.imeta_p.stdin.flush()
        (out, err) = self.get_output()
        self.assertEqual(out, 'imeta>')
        self.assertEqual(err, '')
        # ensure imeta stays open
        timestamp = process_time()
        while process_time() - timestamp < self.default_timeout:
            self.assertIsNone(self.imeta_p.poll())

    def test_space_cmd(self):
        self.get_output()
        self.imeta_p.stdin.write(b'         \n')
        self.imeta_p.stdin.flush()
        (out, err) = self.get_output()
        self.assertEqual(out, 'imeta>')
        self.assertEqual(err, '')
        # ensure imeta stays open
        timestamp = process_time()
        while process_time() - timestamp < self.default_timeout:
            self.assertIsNone(self.imeta_p.poll())

    def test_quit(self):
        self.get_output()
        self.imeta_p.stdin.write(b'quit\n')
        self.imeta_p.stdin.flush()
        (out, err) = self.get_output()
        self.assertEqual(out, '')
        self.assertEqual(err, '')
        # ensure imeta closes
        timestamp = process_time()
        while self.imeta_p.poll() is None and process_time() - timestamp < self.default_timeout:
            pass
        self.assertEqual(self.imeta_p.poll(), 0)

    def test_eot(self):
        self.get_output()
        self.imeta_p.stdin.close()
        (out, err) = self.get_output()
        self.assertEqual(out, '\n')
        self.assertEqual(err, '')
        # ensure imeta closes
        timestamp = process_time()
        while self.imeta_p.poll() is None and process_time() - timestamp < self.default_timeout:
            pass
        self.assertEqual(self.imeta_p.poll(), 0)

    def test_bad_cmd(self):
        self.get_output()
        self.imeta_p.stdin.write(b'badcmd\n')
        self.imeta_p.stdin.flush()
        (out, err) = self.get_output()
        self.assertEqual(out, 'imeta>')
        self.assertRegexpMatches(err, '^Unrecognized subcommand ')
        # ensure imeta stays open
        timestamp = process_time()
        while process_time() - timestamp < self.default_timeout:
            self.assertIsNone(self.imeta_p.poll())

    def test_ls_no_args(self):
        self.get_output()
        self.imeta_p.stdin.write(b'ls\n')
        self.imeta_p.stdin.flush()
        (out, err) = self.get_output()
        self.assertEqual(out, 'imeta>')
        self.assertRegexpMatches(err, '^Error: No object type descriptor ')
        # ensure imeta stays open
        timestamp = process_time()
        while process_time() - timestamp < self.default_timeout:
            self.assertIsNone(self.imeta_p.poll())

    def test_imeta_quoted_arguments__5518(self):
        self.get_output()
        data_name = self.test_data_path_base + '1'
        A = 'test 5518 quoted arguments with spaces'
        V = 'c\td'
        U = 'e \tf'
        self.imeta_p.stdin.write('adda -d {data_name} "{A}" "{V}" "{U}"\n'.format(**locals()).encode())
        self.imeta_p.stdin.flush()
        (out, _) = self.get_output()
        self.assertEqual(out, 'imeta>')
        self.admin.assert_icommand(['iquest','--no-page', '%s/%s/%s',
                                    "select META_DATA_ATTR_NAME, META_DATA_ATTR_VALUE, META_DATA_ATTR_UNITS where DATA_NAME = '{0}' "
                                    " and META_DATA_ATTR_NAME = '{1}'".format( data_name.split('/')[-1], A)
                                   ],
                                   'STDOUT_SINGLELINE', '^{A}/{V}/{U}$'.format(**locals()), use_regex=True)
        # ensure imeta stays open
        timestamp = process_time()
        while process_time() - timestamp < self.default_timeout:
            self.assertIsNone(self.imeta_p.poll())

    def test_ls_d(self):
        self.get_output()
        self.imeta_p.stdin.write('ls -d {}1\n'.format(self.test_data_path_base).encode())
        self.imeta_p.stdin.flush()
        (out, err) = self.get_output()
        self.assertRegexpMatches(out, '.+\nimeta>$')
        self.assertEqual(err, '')
        # ensure imeta stays open
        timestamp = process_time()
        while process_time() - timestamp < self.default_timeout:
            self.assertIsNone(self.imeta_p.poll())

