from __future__ import print_function

import re

from .. import lib
from .. import six

def check_command_output(command_arg, stdout, stderr, check_type='EMPTY', expected_results='', use_regex=False):
    assert check_type in ['EMPTY', 'STDOUT', 'STDERR', 'STDOUT_SINGLELINE',
                          'STDERR_SINGLELINE', 'STDOUT_MULTILINE', 'STDERR_MULTILINE', 'EMPTY_STDOUT'], check_type

    if isinstance(expected_results, six.string_types):
        expected_results = [expected_results]

    regex_msg = 'regex ' if use_regex else ''

    print('Expecting {0}: {1}{2}'.format(
        check_type, regex_msg, expected_results))
    print('  stdout:')
    print('    | ' + '\n    | '.join(stdout.splitlines()))
    print('  stderr:')
    print('    | ' + '\n    | '.join(stderr.splitlines()))

    if check_type not in ['STDERR', 'STDERR_SINGLELINE', 'STDERR_MULTILINE', 'EMPTY_STDOUT'] and stderr != '':
        print('Unexpected output on stderr\n')
        return False

    if check_type in ['STDOUT', 'STDERR']:
        output = stdout if check_type == 'STDOUT' else stderr
        for er in expected_results:
            regex_pattern = er if use_regex else re.escape(er)
            if not re.search(regex_pattern, output):
                print('Output not found\n')
                return False
        print('Output found\n')
        return True
    elif check_type in ['STDOUT_SINGLELINE', 'STDERR_SINGLELINE', 'STDOUT_MULTILINE', 'STDERR_MULTILINE']:
        lines = stdout.splitlines() if check_type in [
            'STDOUT_SINGLELINE', 'STDOUT_MULTILINE'] else stderr.splitlines()

        if check_type in ['STDOUT_MULTILINE', 'STDERR_MULTILINE']:
            for er in expected_results:
                regex_pattern = er if use_regex else re.escape(er)
                for line in lines:
                    if re.search(regex_pattern, line.rstrip('\n')):
                        break
                else:
                    print(
                        '    --> stopping search - expected result not found')
                    break
            else:
                print('Output found\n')
                return True
            print('Output not found\n')
            return False
        else:
            for line in lines:
                found_count = 0
                for er in expected_results:
                    regex_pattern = er if use_regex else re.escape(er)
                    if re.search(regex_pattern, line.rstrip('\n')):
                        found_count += 1
                if found_count == len(expected_results):
                    print('Output found\n')
                    return True
            print('Output not found\n')
            return False
    elif check_type in ['EMPTY', 'EMPTY_STDOUT']:
        if stdout == '':
            print('Output found\n')
            return True
        print('Unexpected output on stdout\n')
        return False
    assert False, check_type

def assert_command(*args, **kwargs):
    return _assert_helper(*args, should_fail=False, **kwargs)

def assert_command_fail(*args, **kwargs):
    return _assert_helper(*args, should_fail=True, **kwargs)

def _assert_helper(command_arg, check_type='EMPTY', expected_results='', use_regex=False, should_fail=False, desired_rc=None, **kwargs):

    out, err, rc = lib.execute_command_permissive(command_arg, **kwargs)

    fail_string = ' FAIL' if should_fail else ''
    if isinstance(command_arg, six.string_types):
        print('Assert{0} Command: {1}'.format(fail_string, command_arg))
    else:
        print('Assert{0} Command: {1}'.format(
            fail_string, ' '.join(command_arg)))

    result = should_fail != check_command_output(
        command_arg, out, err, check_type=check_type, expected_results=expected_results, use_regex=use_regex)

    if desired_rc is not None:
        print(
            'Checking return code: actual [{0}] desired [{1}]'.format(rc, desired_rc))
        if desired_rc != rc:
            print('RETURN CODE CHECK FAILED')
            result = False

    if not result:
        print('FAILED TESTING ASSERTION\n\n')
    assert result
    return rc, out, err


