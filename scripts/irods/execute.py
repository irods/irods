from __future__ import print_function
import copy
import errno
import logging
import pprint
import subprocess
import shlex
import sys
import time

from . import six

from .exceptions import IrodsError, IrodsWarning

def indent(*text, **kwargs):
    if 'indentation' in kwargs:
        indentation = kwargs['indentation']
    else:
        indentation = '  '
    return '\n'.join([''.join([indentation, '\n{0}'.format(indentation).join(lines.splitlines())]) for lines in text])

def safe_shlex_split_for_2_6(args):
    if not isinstance(args, str) and isinstance(args, six.text_type):
        args = args.encode('ascii')
    return shlex.split(args)

def communicate_and_log(p, args, input=None):
    l = logging.getLogger(__name__)
    out, err = [(None if t is None else t.decode('utf_8')) for t in p.communicate(input=(None if input is None else input.encode('ascii')))]
    message = ['Command %s returned with code %s.' % (args, p.returncode)]
    if input:
        message.append('stdin:\n%s' % indent(input))
    if out:
        message.append('stdout:\n%s' % indent(out))
    if err:
        message.append('stderr:\n%s' % indent(err))
    l.debug('\n'.join(message))
    return (out, err)

def execute_command_nonblocking(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, use_unsafe_shell=False, **kwargs):
    l = logging.getLogger(__name__)
    if not use_unsafe_shell and isinstance(args, six.string_types):
        args = safe_shlex_split_for_2_6(args)
    kwargs['stdout'] = stdout
    kwargs['stderr'] = stderr
    kwargs['shell'] = use_unsafe_shell
    if 'env' in kwargs:
        kwargs_without_env = copy.copy(kwargs)
        kwargs_without_env['env'] = 'HIDDEN'
    else:
        kwargs_without_env = kwargs
    l.debug('Calling %s with options:\n%s', args, pprint.pformat(kwargs_without_env))
    try:
        return subprocess.Popen(args, **kwargs)
    except OSError as e:
        six.reraise(IrodsError,
            IrodsError('\n'.join([
                'Call to open process with {0} failed:'.format(
                    args),
                indent(
                    'Could not find the requested executable \'{0}\'; '
                    'please ensure \'{0}\' is installed and in the path.'.format(
                        args[0]))])),
            sys.exc_info()[2])

def execute_command_timeout(args, timeout=10, **kwargs):
    p = execute_command_nonblocking(args, **kwargs)
    start_time = time.time()
    while time.time() < start_time + timeout:
        if p.poll() is not None:
            out, err = communicate_and_log(p, args)
            check_command_return(args, out, err, p.returncode, **kwargs)
            break
        time.sleep(0.3)
    else:
        try:
            if p.poll() is None:
                p.kill()
        except OSError:
            pass
        raise IrodsError('The call {0} did not complete within {1} seconds.'.format(args, timeout))

def execute_command_permissive(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, input=None, **kwargs):
    if input is not None:
        if 'stdin' in kwargs and kwargs['stdin'] != subprocess.PIPE:
            raise IrodsError('\'input\' option is mutually exclusive with a \'stdin\' '
                    'option that is not equal to \'subprocess.PIPE\'.')
        kwargs['stdin'] = subprocess.PIPE
    p = execute_command_nonblocking(args, stdout=stdout, stderr=stderr, **kwargs)
    out, err = communicate_and_log(p, args, input)
    return (out, err, p.returncode)

def check_command_return(args, out, err, returncode, input=None, **kwargs):
    if returncode is not None and returncode != 0:
        if 'env' in kwargs:
            kwargs_without_env = copy.copy(kwargs)
            kwargs_without_env['env'] = 'HIDDEN'
        else:
            kwargs_without_env = kwargs
        deets = [
            'Options passed to Popen:',
            indent(*['{0}: {1}'.format(k, v) for k, v in kwargs_without_env.items()]),
            'Return code: {0}'.format(returncode)]
        if input:
            deets.extend(['Standard input:', indent(input)])
        if out:
            deets.extend(['Standard output:', indent(out)])
        if err:
            deets.extend(['Error output:', indent(err)])
        raise IrodsError('\n'.join([
            'Call to open process with {0} returned an error:'.format(
                args),
            indent(*deets)]))

def execute_command(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, input=None, **kwargs):
    out, err, returncode = execute_command_permissive(args, stdout=stdout, stderr=stderr, input=input, **kwargs)
    check_command_return(args, out, err, returncode, input=input, **kwargs)

    return (out, err)
