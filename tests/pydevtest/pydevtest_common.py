import commands
import contextlib
import json
import mmap
import os
import psutil
import re
import shlex
import shutil
import socket
import subprocess
import tempfile
import time


class irods_test_constants(object):
    RUN_IN_TOPOLOGY = False
    TOPOLOGY_FROM_RESOURCE_SERVER = False
    HOSTNAME_1 = HOSTNAME_2 = HOSTNAME_3 = socket.gethostname()
    USE_SSL = False

def update_json_file_from_dict(filename, update_dict):
    with open(filename) as f:
        env = json.load(f)
    env.update(update_dict)
    with open(filename, 'w') as f:
        json.dump(env, f, indent=4)

def get_hostname():
    return socket.gethostname()

def get_irods_top_level_dir():
    if os.path.isfile('/etc/irods/server_config.json'):
        return '/var/lib/irods'
    return os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

def get_irods_config_dir():
    if os.path.isfile('/etc/irods/server_config.json'):
        return '/etc/irods'
    return os.path.join(get_irods_top_level_dir(), 'iRODS/config')

def create_directory_of_small_files(directory_name_suffix, file_count):
    if not os.path.exists(directory_name_suffix):
        os.mkdir(directory_name_suffix)
    for i in range(file_count):
        with open('{0}/{1}'.format(directory_name_suffix, i), 'w') as f:
            f.write("iglkg3fqfhwpwpo-" + "A" * i)

def create_local_testfile(filename):
    filepath = os.path.abspath(filename)
    with open(filepath, 'wb') as f:
        f.write('TESTFILE -- [' + filepath + ']')
    return filepath

def create_local_largefile(filename):
    filepath = os.path.abspath(filename)
    os.system('dd if=/dev/zero of=' + filepath + ' bs=1M count=64')
    return filepath

def check_icmd_outputtype(fullcmd, outputtype):
    allowed_outputtypes = ["LIST", "EMPTY", "ERROR", "", 'STDOUT', 'STDERR', 'STDOUT_MULTILINE', 'STDERR_MULTILINE']
    if outputtype not in allowed_outputtypes:
        print "  full command: [" + fullcmd + "]"
        print "  allowed outputtypes: " + str(allowed_outputtypes)
        print "  unknown outputtype requested: [" + outputtype + "]"
        assert False, "hard fail, bad icommand output format requested"

def getiCmdOutput(mysession, fullcmd):
    parameters = shlex.split(fullcmd)  # preserves quoted substrings
    print "running icommand: " + mysession.get_username() + "[" + fullcmd + "]"
    if parameters[0] == "iadmin":
        output = mysession.runAdminCmd(parameters[0], parameters[1:])
    else:
        output = mysession.runCmd(parameters[0], parameters[1:])
    # return output array
    #   [0] is stdout
    #   [1] is stderr
    return output

def getiCmdBoolean(mysession, fullcmd, outputtype="", expectedresults="", use_regex=False):
    result = False  # should start as failing, then get set to pass
    parameters = shlex.split(fullcmd)  # preserves quoted substrings
    # expectedresults needs to be a list
    if isinstance(expectedresults, str):  # converts a string to a list
        expectedresults = [expectedresults]
    # get output from icommand
    output = getiCmdOutput(mysession, fullcmd)
    # allow old outputtype identifiers
    if outputtype == "LIST":
        outputtype = "STDOUT"
    elif outputtype == "ERROR":
        outputtype = "STDERR"

    if use_regex:
        regex_msg = 'regex '
    else:
        regex_msg = ''

    # check result listing for expected results
    if outputtype in ['STDOUT', 'STDERR', 'STDOUT_MULTILINE', 'STDERR_MULTILINE']:
        print "  Expecting " + outputtype + ": " + regex_msg + str(expectedresults)
        print "  stdout:"
        print "    | " + "\n    | ".join(output[0].splitlines())
        print "  stderr: [" + output[1].rstrip('\n') + "]"
        # generate lines based on outputtype
        if outputtype in ['STDOUT', 'STDOUT_MULTILINE']:
            lines = output[0].splitlines()
        else:
            lines = output[1].splitlines()
        # look for expected results in the output lines
        if outputtype in ['STDOUT_MULTILINE', 'STDERR_MULTILINE']:
            for er in expectedresults:
                if use_regex:
                    regex_pattern = er
                else:
                    regex_pattern = re.escape(er)
                for line in lines:
                    print '  searching for ' + regex_msg + '[' + er + '] in [' + line.rstrip('\n') + '] ...',
                    if re.search(regex_pattern, line.rstrip('\n')):
                        print "FOUND"
                        break
                    else:
                        print "NOTFOUND"
                else:
                    print "    --> stopping search - expected result not found"
                    break
            else:
                print "    --> stopping search - expected result(s) found"
                result = True
        else:
            for line in lines:
                foundcount = 0
                for er in expectedresults:
                    if use_regex:
                        regex_pattern = er
                    else:
                        regex_pattern = re.escape(er)
                    print '  searching for ' + regex_msg + '[' + er + '] in [' + line.rstrip('\n') + ']...',
                    if re.search(regex_pattern, line.rstrip('\n')):
                        foundcount += 1
                        print "found (" + str(foundcount) + " of " + str(len(expectedresults)) + ")"
                    else:
                        print "NOTFOUND"
                if foundcount == len(expectedresults):
                    print "    --> stopping search - expected result(s) found"
                    result = True
                    break
                else:
                    print "    --> did not find expected result(s)"
    # check that icommand returned no result
    elif (outputtype == "EMPTY" or outputtype == ""):
        print "  Expecting EMPTY output"
        print "  stdout: [" + ",".join(output[0].splitlines()) + "]"
        print "  stderr: [" + output[1].strip() + "]"
        if output[0] == "":
            result = True
    # bad test formatting
    else:
        print "  WEIRD - SHOULD ALREADY HAVE BEEN CAUGHT ABOVE"
        print "  unknown outputtype requested: [" + outputtype + "]"
        assert False, "WEIRD - DUPLICATE BRANCH - hard fail, bad icommand format"
    # return error if stderr is populated unexpectedly
    if outputtype not in ['STDERR', 'STDERR_MULTILINE'] and output[1] != "":
        return False
    # return value
    return result


def assertiCmd(mysession, fullcmd, outputtype="", expectedresults="", use_regex=False):
    ''' Runs an icommand, detects output type, and searches for
    values in expected results list.

    Asserts that this result is correct.

    Returns elapsed runtime.
    '''
    begin = time.time()
    print "\n"
    print "ASSERTING PASS"
    check_icmd_outputtype(fullcmd, outputtype)
    assert getiCmdBoolean(mysession, fullcmd, outputtype, expectedresults, use_regex)
    elapsed = time.time() - begin
    return elapsed


def assertiCmdFail(mysession, fullcmd, outputtype="", expectedresults="", use_regex=False):
    ''' Runs an icommand, detects output type, and searches for
    values in expected results list.

    Asserts that this result is NOT correct.

    Returns elapsed runtime.
    '''
    begin = time.time()
    print "\n"
    print "ASSERTING FAIL"
    check_icmd_outputtype(fullcmd, outputtype)
    assert not getiCmdBoolean(mysession, fullcmd, outputtype, expectedresults, use_regex)
    elapsed = time.time() - begin
    return elapsed

def interruptiCmd(mysession, fullcmd, filename, filesize):
    ''' Runs an icommand, but does not let it complete.

    This function terminates the icommand once filename reaches (>=)
    filesize in bytes.

    Asserts that the icommand was successfully terminated early.

    Returns 0 or -1 or -2.
    '''
    parameters = shlex.split(fullcmd)  # preserves quoted substrings
    print "\n"
    print "INTERRUPTING iCMD"
    print "running icommand: " + mysession.get_username() + "[" + fullcmd + "]"
    print "  filename set to: [" + filename + "]"
    print "  filesize set to: [" + str(filesize) + "] bytes"
    resultcode = mysession.interruptCmd(parameters[0], parameters[1:], filename, filesize)
    if resultcode == 0:
        print "  resultcode: [0], interrupted successfully"
    elif resultcode == -1:
        print "  resultcode: [-1], icommand completed"
    else:
        print "  resultcode: [-2], icommand timeout"
    assert 0 == resultcode
    return resultcode

def interruptiCmdDelay(mysession, fullcmd, delay):
    ''' Runs an icommand, but does not let it complete.

    This function terminates the icommand after delay seconds.

    Asserts that the icommand was successfully terminated early.

    Returns 0 or -1.
    '''
    parameters = shlex.split(fullcmd)  # preserves quoted substrings
    print "\n"
    print "INTERRUPTING iCMD"
    print "running icommand: " + mysession.get_username() + "[" + fullcmd + "]"
    print "  timeout set to: [" + str(delay) + " seconds]"
    resultcode = mysession.interruptCmdDelay(parameters[0], parameters[1:], delay)
    if resultcode == 0:
        print "  resultcode: [0], interrupted successfully"
    else:
        print "  resultcode: [-1], icommand completed"
    assert 0 == resultcode
    return resultcode

def touch(fname, times=None):
    with file(fname, 'a'):
        os.utime(fname, times)

def cat(fname, string, times=None):
    with file(fname, 'a') as f:
        f.write(string)

def make_file(f_name, f_size, source='/dev/zero'):
    output = commands.getstatusoutput('dd if="' + source + '" of="' + f_name + '" count=1 bs=' + str(f_size))
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
                        session.get_username(),
                        session._session_id)

def make_large_local_tmp_dir(dir_name, file_count, file_size):
    os.mkdir(dir_name)
    for i in range(file_count):
        make_file(os.path.join(dir_name, "junk" + str(i).zfill(4)),
                  file_size)
    local_files = os.listdir(dir_name)
    assert len(local_files) == file_count, "dd loop did not make all " + str(file_count) + " files"
    return local_files

@contextlib.contextmanager
def core_re_backed_up():
    with tempfile.NamedTemporaryFile(prefix='core.re_backup') as f:
        shutil.copyfile('/etc/irods/core.re', f.name)
        yield
        shutil.copyfile(f.name, '/etc/irods/core.re')

def prepend_string_to_core_re(string):
    core_re_path = '/etc/irods/core.re'
    with open(core_re_path, 'r') as f: contents = f.read()
    with open(core_re_path, 'w') as f:
        f.write(string)
        f.write(contents)

def get_re_log_size():
    return os.stat(get_re_log_path()).st_size

def get_re_log_path():
    server_log_dir = os.path.join(get_irods_top_level_dir(), 'iRODS/server/log')
    command_str = 'ls -t ' + server_log_dir + '/reLog* | head -n1'
    proc = subprocess.Popen(command_str, stdout=subprocess.PIPE, shell=True)
    stdout, stderr = proc.communicate()
    if proc.returncode != 0 or stdout == '':
        raise subprocess.CalledProcessError(proc.returncode, command_str, 'stdout [{0}] stderr[{1}]'.format(stdout, stderr))
    log_file_path = stdout.rstrip()
    return log_file_path

def count_occurrences_of_string_in_re_log(string, start_index=0):
    with open(get_re_log_path()) as f:
        m = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
        n = 0
        i = m.find(string, start_index)
        while i != -1:
            n += 1
            i = m.find(string, i+1)
        return n

def run_command(args, check_rc=False, stdin_string='', use_unsafe_shell=False, environment=None):
    if not use_unsafe_shell and isinstance(args, str):
        args = shlex.split(args)
    p = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=environment, shell=use_unsafe_shell)
    stdout, stderr = p.communicate(input=stdin_string)
    rc = p.returncode
    if check_rc:
        if rc != 0:
            raise subprocess.CalledProcessError(rc, args, stdout + '\n\n' + stderr)
    return rc, stdout, stderr

def run_command_check_output(args, check_type='EMPTY', expected_results='', use_regex=False, **kwargs):
    assert check_type in ['EMPTY', 'STDOUT', 'STDERR', 'STDOUT_MULTILINE', 'STDERR_MULTILINE'], check_type

    rc, stdout, stderr = run_command(args, **kwargs)

    if isinstance(expected_results, str):
        expected_results = [expected_results]

    regex_msg = 'regex ' if use_regex else ''

    print 'Expecting {0}: {1}{2}'.format(check_type, regex_msg, expected_results)
    print '  stdout:'
    print '    | ' + '\n    | '.join(stdout.splitlines())
    print '  stderr:'
    print '    | ' + '\n    | '.join(stderr.splitlines())

    if check_type not in ['STDERR', 'STDERR_MULTILINE'] and stderr != '':
        print 'Unexpected output on stderr'
        return False

    if check_type in ['STDOUT', 'STDERR', 'STDOUT_MULTILINE', 'STDERR_MULTILINE']:
        lines = stdout.splitlines() if check_type in ['STDOUT', 'STDOUT_MULTILINE'] else stderr.splitlines()

        if check_type in ['STDOUT_MULTILINE', 'STDERR_MULTILINE']:
            for er in expected_results:
                regex_pattern = er if use_regex else re.escape(er)
                for line in lines:
                    if re.search(regex_pattern, line.rstrip('\n')):
                        break
                else:
                    print '    --> stopping search - expected result not found'
                    break
            else:
                return True
            return False
        else:
            for line in lines:
                found_count = 0
                for er in expected_results:
                    regex_pattern = er if use_regex else re.escape(er)
                    if re.search(regex_pattern, line.rstrip('\n')):
                        found_count += 1
                if found_count == len(expected_results):
                    return True
            return False
    elif check_type == 'EMPTY':
        if stdout == '':
            return True
        print 'Unexpected output on stdout'
        return False
    assert False

def assert_command(*args, **kwargs):
    assert run_command_check_output(*args, **kwargs)

def restart_irods_server():
    assert_command('{0} restart'.format(os.path.join(get_irods_top_level_dir(), 'iRODS/irodsctl')), 'STDOUT_MULTILINE', ['Stopping iRODS server', 'Starting iRODS server'])
