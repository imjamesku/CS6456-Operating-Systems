#!/usr/bin/env python3
from __future__ import print_function, unicode_literals

import errno
import logging
import os
import re
import resource
import subprocess
import sys

PROGRAM = ['./uvash']

# Notes on interpreting this test cases:
#
# The expected values for stdout and stderr are regular expressions, so
#   .* means any string (including empty strings)
#   .+ means any string (excluding empty strings)
# All pattern-matching is case-insensitive.
#
# If allow_extra_stdout or allow_extra_stderr is set in a test case, then
# extra lines of output can be given from stdout or stderr by the shell.
# Otherwise, no extra lines are allowed.
#
# Some test cases have an 'expect_output_files' argument, which is
# a list of files expected to be created by the test case and their contents.
#
# Some test cases have a 'prepare_function' argument, whcih is some python code
# to run before running the test case.
#
# Some test cases have a 'extra_popen' argument, which are extra flags to pass
# to subprocess.Popen

def after_every(l, to_insert):
    ret = []
    for i in l:
        ret.append(i)
        ret.append(to_insert)
    return ret

TESTS = [
    {
        'name': 'exit immediately',
        'input': ['exit'],
        'stdout': ['> '],
        'stderr': [],
    },
    {
        'name': 'empty command is invalid',
        'input': ['','exit'],
        'stdout': ['> > '],
        'stderr': ['.*invalid command.*'],
    },
    {
        'name': 'empty command is invalid, even with whitespace',
        'input': ['             \t     \v ','exit'],
        'stdout': ['> > '],
        'stderr': ['.*invalid command.*'],
    },
    {
        'name': 'trivial command',
        'input': ['/bin/true','exit'],
        'stdout': ['> > '],
        'stderr': [],
    },
    {
        'name': 'trivial command, exit status 0',
        'input': ['/bin/true','status','exit'],
        'stdout': ['> > .*Status: 0.*', '> '],
        'stderr': [],
    },
    {
        'name': 'trivial command with extra space, exit status 0',
        'input': [' /bin/true','status','exit'],
        'stdout': ['> > .*Status: 0.*', '> '],
        'stderr': [],
    },
    {
        'name': 'trivial command with extra tab, exit status 0',
        'input': ['\t/bin/true','status','exit'],
        'stdout': ['> > .*Status: 0.*', '> '],
        'stderr': [],
    },
    {
        'name': 'trivial command with extra vertical tab, exit status 0',
        'input': ['\v/bin/true','status','exit'],
        'stdout': ['> > .*Status: 0.*', '> '],
        'stderr': [],
    },
    {
        'name': 'trivial command, exit status 1',
        'input': ['/bin/false','status','exit'],
        'stdout': ['> > .*Status: 1.*', '> '],
        'stderr': [],
    },
    {
        'name': 'only redirections is invalid',
        'input': ['> foo.txt < bar.txt','exit'],
        'stdout': ['> > '],
        'stderr': ['.*invalid command.*'],
    },
    {
        'name': 'redirection to nothing is invalid',
        'input': ['/bin/true > ','exit'],
        'stdout': ['> > '],
        'stderr': ['.*invalid command.*'],
    },
    {
        'name': 'redirection from nothing is invalid',
        'input': ['/bin/true < ','exit'],
        'stdout': ['> > '],
        'stderr': ['.*invalid command.*'],
    },
    {
        'name': 'pass arguments',
        'input': ['test/argument_test.sh first second_with_underscore third', 'print', 'status', 'exit'],
        'stdout': [
            '> > ',
            'number of arguments: 3',
            'argument 1: first',
            'argument 2: second_with_underscore',
            'argument 3: third',
            'argument 4: ',
            '.*Status: 0.*',
            '> ',
        ],
        'stderr': [],
    },
    {
        'name': '" is not quote',
        'input': ['test/argument_test.sh "not quoted"', 'print', 'status', 'exit'],
        'stdout': [
            '> > ',
            'number of arguments: 2',
            'argument 1: "not',
            'argument 2: quoted"',
            'argument 3: ',
            'argument 4: ',
            '.*Status: 0.*',
            '> ',
        ],
        'stderr': [],
    },
    {
        'name': 'varying argument counts and lengths',
        'input': ['test/argument_test.sh aX bX cX dX eX', 'status', 'test/argument_test.sh f g hZZ i', 'print', 'test/argument_test.sh j k l', 'status', 'print', 'exit'],
        'stdout': [
            '> > .*Status: 0.*',
            '> > ',
            'number of arguments: 4',
            'argument 1: f',
            'argument 2: g',
            'argument 3: hZZ',
            'argument 4: i',
            '> > .*Status: 0.*',
            '> ',
            'number of arguments: 3',
            'argument 1: j',
            'argument 2: k',
            'argument 3: l',
            'argument 4: ',
            '> ',
        ],
        'stderr': [],
    },
    {
        'name': 'varing command lengths (1)',
        'input': ['./test/argument_test.sh a b c d e', 'print', 'status', '/bin/echo f g h i', 'print', 'status', 'test/argument_test.sh j k l', 'print', 'status', 'exit'],
        'stdout': [
            '> > ',
            'number of arguments: 5',
            'argument 1: a',
            'argument 2: b',
            'argument 3: c',
            'argument 4: d',
            '> .*Status: 0.*',
            '> > f g h i',
            '.*Status: 0.*',
            '> > ',
            'number of arguments: 3',
            'argument 1: j',
            'argument 2: k',
            'argument 3: l',
            'argument 4: ',
            '> .*Status: 0.*',
            '> ',
        ],
        'stderr': [],
    },
    {
        'name': 'varing command lengths (2)',
        'input': ['/bin/echo f g h i', 'print', './test/argument_test.sh aXX bXX cXX dXX eXX', 'print', 'test/argument_test.sh j k l', 'print', 'exit'],
        'stdout': [
            '> > f g h i',
            '> > ',
            'number of arguments: 5',
            'argument 1: aXX',
            'argument 2: bXX',
            'argument 3: cXX',
            'argument 4: dXX',
            '> > ',
            'number of arguments: 3',
            'argument 1: j',
            'argument 2: k',
            'argument 3: l',
            'argument 4: ',
            '> ',
        ],
        'stderr': [],
    },
    {
        'name': 'very long argument',
        'input': ['/bin/echo short', 'print', '/bin/echo ' + ('Q' * 80), 'print', 'status', 'exit'],
        'stdout': [
            '> > short',
            '> > ' + ('Q' * 80),
            '> .*Status: 0.*',
            '> ',
        ],
        'stderr': [],
    },
    {
        'name': 'lots of arguments',
        'input': ['/bin/echo short', 'print', 'test/argument_test.sh ' + ' '.join(map(lambda i: chr(ord('A') + i), range(20))), 'print', 'exit'],
        'stdout': [
            '> > short',
            '> > ',
            'number of arguments: 20',
            'argument 1: A',
            'argument 2: B',
            'argument 3: C',
            'argument 4: D',
            '> ',
        ],
        'stderr': [],
    },
    {
        'name': 'extra whitespace without redirects',
        'input': ['   \t\t /bin/echo\ttesting    one   two \vthree ', 'print', 'status', 'exit'],
        'stdout': ['> > testing one two three', '> .*Status: 0.*', '> '],
        'stderr': [],
    },
    {
        'name': 'redirections require whitespace around >',
        'input': ['/bin/echo  this is a >test', 'print', 'status', 'exit'],
        'stdout': ['> > this is a >test', '> .*Status: 0.*', '> '],
        'stderr': [],
    },
    {
        'name': 'redirections require whitespace around <',
        'input': ['/bin/echo  this is a <test', 'print', 'status', 'exit'],
        'stdout': ['> > this is a <test', '> .*Status: 0.*', '> '],
        'stderr': [],
    },
    {
        'name': '>> is not a redirection operator',
        'input': ['/bin/echo  this is a >> test', 'print', 'status', 'exit'],
        'stdout': ['> > this is a >> test', '> .*Status: 0.*', '> '],
        'stderr': [],
    },
    {
        'name': 'redirect stdin inode',
        'input': ['/usr/bin/stat -L -c %i/%d /proc/self/fd/0 < test/input.txt', 'print', 'exit'],
        'stdout': [
            '> > {}/{}'.format(
                os.stat('test/input.txt').st_ino,
                os.stat('test/input.txt').st_dev,
            ),
            '> ',
        ],
        'stderr': [],
    },
    {
        'name': 'redirect stdin contents',
        'input': ['/bin/cat < test/input.txt', 'print', 'status', 'exit'],
        'stdout': [
            '> > This is an example input file.',
            'Which has multiple lines.',
            '> .*Status: 0.*',
            '> ',
        ],
        'stderr': [],
    },
    {
        'name': 'simple pipe output',
        'input': ['/bin/echo testing  one two three | /bin/sed -e s/one/XXX/', 'print', 'exit'],
        'stdout': [
            '.*testing XXX two three',
        ],
        'allow_extra_stdout': True,
        'stderr': []
    },
    {
        'name': 'simple pipe exit status',
        'input': ['/bin/echo testing one two three | /bin/sed -e s/one/XXX/', 'status', 'exit'],
        'stdout': [
            '> .*Status: 0.*',
        ],
        'allow_extra_stdout': True,
        'stderr': []
    },
    {
        'name': 'longer pipeline output',
        'input': ['/bin/echo testing one two three | /bin/sed -e s/one/XXX/ | /bin/sed -e s/two/YYY/', 'print', 'exit'],
        'stdout': [
            '.*testing XXX YYY three',
        ],
        'allow_extra_stdout': True,
        'stderr': []
    },
    {
        'name': 'longer pipeline exit status (all 0s)',
        'input': ['/bin/echo testing one two three | /bin/sed -e s/one/XXX/ | /bin/sed -e s/two/YYY/', 'status', 'exit'],
        'stdout': [
            '.*Status: 0.*',
        ],
        'allow_extra_stdout': True,
        'stderr': []
    },
    {
        'name': 'pipeline with two exit status 1s and one 0 has 1s',
        'input': ['/bin/true ignored 1 | /bin/false ignored 2 | /bin/false ignored 3', 'status', 'exit'],
        'stdout': [
            '> > .*Status: 1.*',
        ],
        'allow_extra_stdout': True,
        'stderr': []
    },
    {
        'name': 'pipeline with two exit status 1s and one 0 has 0',
        'input': ['/bin/true some ignored arguments | /bin/false ignored argument | /bin/false more ignored argument', 'status', 'exit'],
        'stdout': [
            '> > .*Status: 1.*',
        ],
        'allow_extra_stdout': True,
        'stderr': []
    },
    {
        'name': 'fork fails',
        'input': ['/bin/echo testing one two three', 'exit'],
        'stdout': ['> > '],
        'stderr': ['.+'], # some non-empty error message
        'allow_extra_stderr': True,
        'extra_popen': {
            'preexec_fn': lambda: resource.setrlimit(resource.RLIMIT_NPROC, (0,0)),
        },
    },
    {
        'name': 'fork fails in a pipeline ',
        'input': ['/bin/cat | /bin/cat | /bin/cat | /bin/cat', 'exit'],
        'stdout': ['> > '],
        'stderr': ['.+'], # some non-empty error message
        'allow_extra_stderr': True,
        'extra_popen': {
            'preexec_fn': lambda: resource.setrlimit(resource.RLIMIT_NPROC, (0,0)),
        },
    },
    {
        'name': '|s without spaces is not a pipeline',
        'input': ['/bin/echo this|argument|has|pipes', 'print', 'status', 'exit'],
        'stdout': [r'> > this\|argument\|has\|pipes', '> .*Status: 0.*', '> '],
        'stderr': [],
    },
    {
        'name': '|s without spaces mixed with | with spaces (output)',
        'input': ['/bin/echo this|argument|has|pipes | /bin/sed -e s/argument/XXX/', 'print', 'exit'],
        'stdout': [r'.*this\|XXX\|has\|pipes', '> '],
        'allow_extra_stdout': True,
        'stderr': [],
    },
    {
        'name': '|s without spaces mixed with | with spaces (exit status)',
        'input': ['/bin/echo this|argument|has|pipes | /bin/sed -e s/argument/XXX/', 'status', 'exit'],
        'stdout': ['.*Status: 0.*'],
        'allow_extra_stdout': True,
        'stderr': [],
    },
    {
        'name': 'exec fails',
        'input': ['test/invalid-exec', 'exit'],
        'stdout': [],
        'allow_extra_stdout': True,
        'stderr': ['.+'], # some non-empty error message
        'allow_extra_stderr': True,
    },
    {
        'name': 'redirect stdout',
        'input': ['/bin/echo testing one two three > test/redirect-stdout-output.txt', 'exit'],
        'expect_output_files': {
            'test/redirect-stdout-output.txt' : ['testing one two three']
        },
        'stdout': ['> > '],
        'stderr': [],
    },
    {
        'name': 'redirect stdout no print',
        'input': ['/bin/echo testing one two three > test/redirect-stdout-output.txt', 'print', 'exit'],
        'expect_output_files': {
            'test/redirect-stdout-output.txt' : ['testing one two three']
        },
        'stdout': ['> > > '],
        'stderr': [],
    },
    {
        'name': 'redirect stdout does not redirect stderr',
        'input': ['test/sample_outputs.sh > test/redirect-stdout-output.txt', 'status', 'exit'],
        'expect_output_files': {
            'test/redirect-stdout-output.txt' : ['This is the contents of stdout.']
        },
        'stdout': ['.*Status: 0.*', '> '],
        'stderr': ['This is the contents of stderr.'],
    },
    {
        'name': 'redirect in middle of command',
        'input': ['/bin/echo testing one two > test/redirect-stdout-output.txt three ', 'status', 'exit'],
        'expect_output_files': {
            'test/redirect-stdout-output.txt' : ['testing one two three']
        },
        'stdout': ['.*Status: 0.*', '> '],
        'stderr': [],
    },
    {
        'name': 'redirect at beginning of command',
        'input': ['> test/redirect-stdout-output.txt /bin/echo testing one two three ', 'status', 'exit'],
        'expect_output_files': {
            'test/redirect-stdout-output.txt' : ['testing one two three']
        },
        'stdout': ['.*Status: 0.*', '> '],
        'stderr': [],
    },
    {
        'name': 'extra whitespace in redirect at beginning',
        'input': ['  >    \ttest/redirect-stdout-output.txt\t  /bin/echo\ttesting    one   two \vthree ', 'status', 'exit'],
        'expect_output_files': {
            'test/redirect-stdout-output.txt' : ['testing one two three']
        },
        'stdout': ['.*Status: 0.*', '> '],
        'stderr': [],
    },
    {
        'name': 'redirect output then use normal output',
        'input': ['/bin/echo foo > /dev/null', 'status', '/bin/echo bar', 'print', 'exit'],
        'stdout': ['> > .*Status: 0.*', '> > bar', '> '],
        'stderr': [],
    },
    {
        'name': 'custom use normal output',
        'input': ['/bin/echo bar', 'print', 'exit'],
        'stdout': ['> > bar', '> '],
        'stderr': [],
    },
    {
        'name': 'redirect input then use normal input',
        'input': ['/usr/bin/stat -L -c %F /proc/self/fd/0', 'print', 'status', '/bin/cat < test/input.txt', 'print', 'status', '/usr/bin/stat -L -c %F /proc/self/fd/0', 'print', 'status', 'exit'],
        'stdout': [
            '> > fifo', '> .*Status: 0.*',
            '> > This is an example input file.', 'Which has multiple lines.', '> .*Status: 0.*',
            '> > fifo', '> .*Status: 0.*',
            '> '
        ],
        'stderr': [],
    },
    {
        'name': 'redirect output truncates file',
        'input': ['/bin/echo testing one two three > test/redirect-stdout-output.txt', 'status', 'exit'],
        'expect_output_files': {
            'test/redirect-stdout-output.txt' : ['testing one two three'],
        },
        'stdout': ['.*Status: 0.*', '> '],
        'stderr': [],
        'prepare_function': lambda: create_file('test/redirect-stdout-output.txt',
            'This is a long string meant to ensure that echo\n'
            'will not overwrite it if the shell does not open\n'
            'the file with O_TRUNC.\n'
            'This is a long string meant to ensure that echo\n'
            'will not overwrite it if the shell does not open\n'
            'the file with O_TRUNC.\n'
        ),
    },
    {
        'name': 'echo 100 times output',
        'input': after_every(list(map(lambda i: '/bin/echo %s' % (i), range(100))), 'print') + ['exit'],
        'stdout': list(map(lambda i: '.*%s' % (i), range(100))), # .* for possible prefix of prompt
        'stderr': [],
        'allow_extra_stdout': True,
    },
    {
        'name': 'echo 100 times exit status',
        'input': after_every(list(map(lambda i: '/bin/echo %s' % (i), range(100))), 'status') + ['exit'],
        'stdout': list(map(lambda i: '.*Status: 0.*', range(100))),
        'stderr': [],
        'allow_extra_stdout': True,
    },
    {
        'name': '100 output redirections (with limit of 50 open files)',
        'input': after_every(list(map(lambda i: '/bin/echo valuefrom%s > test/redirect-output-%s' % (i, i), range(100))), 'status') + ['exit'],
        'stdout': list(map(lambda i: '.*Status: 0.*', range(100))),
        'stderr': [],
        'expect_output_files': dict(list(map(
            lambda i: ('test/redirect-output-%s' % (i), ['valuefrom%s' % (i)]),
            range(100)
        ))),
        'allow_extra_stdout': True,
        'extra_popen': {
            'preexec_fn': lambda: resource.setrlimit(resource.RLIMIT_NOFILE, (50,50)),
        },
    },
    {
        'name': '100 input redirections (with limit of 50 open files)',
        'input': after_every(list(map(lambda i: '/bin/cat < test/input.txt', range(100))), 'print') + ['exit'],
        'stdout': list(map(lambda i: '.*This is an example input file.', range(100))),
        'stderr': [],
        'allow_extra_stdout': True,
        'extra_popen': {
            'preexec_fn': lambda: resource.setrlimit(resource.RLIMIT_NOFILE, (50,50)),
        },
    },
    {
        'name': '100 pipelines (with limit of 50 open files)',
        'input': after_every(list(map(lambda i: '/bin/echo a test | /bin/sed -e s/test/xxx/', range(100))), 'print') + ['exit'],
        'stdout': list(map(lambda i: '.*a xxx', range(100))),
        'stderr': [],
        'allow_extra_stdout': True,
        'extra_popen': {
            'preexec_fn': lambda: resource.setrlimit(resource.RLIMIT_NOFILE, (50,50)),
        },
    },
    {
        'name': 'redirect to operator is invalid',
        'input': ['/bin/false > > ', 'exit'],
        'stdout': ['> > '],
        'stderr': ['.*invalid command.*'],
    },
    {
        'name': 'redirect from operator is invalid',
        'input': ['/bin/false < | ', 'exit'],
        'stdout': ['> > '],
        'stderr': ['.*invalid command.*'],
    },
    # some possible missing tests:
        # effects of multiple redirection in one command
        # verify that PATH is not being used
]

def create_file(filename, contents):
    with open(filename, 'w') as fh:
        fh.write(contents)

def to_bytes(s):
    return bytes(s, 'UTF-8')

def compare_lines(
        label,
        expected_lines,
        actual_lines,
        allow_extra_lines,
):
    errors = []
    actual_index = 0
    if len(actual_lines) > 0 and actual_lines[-1] == b'':
        actual_lines = actual_lines[:-1]
    for expected_line in map(to_bytes, expected_lines):
        pattern = re.compile(expected_line, re.IGNORECASE)
        prev_index = actual_index
        found_match = False
        while actual_index < len(actual_lines):
            actual_index += 1
            m = pattern.fullmatch(actual_lines[actual_index-1])
            if m != None:
                found_match = True
                break
            if not allow_extra_lines:
                errors.append('in {}: could not find a match for pattern [{}] in line [{}]'.format(
                    label,
                    expected_line.decode('UTF-8', errors='replace'),
                    actual_lines[actual_index-1].decode('UTF-8', errors='replace')
                ))
        if not found_match and actual_index >= len(actual_lines):
            errors.append('in {}: could not find match for pattern [{}] in {}'.format(
                label,
                expected_line.decode('UTF-8', errors='replace'),
                list(map(lambda x: x.decode('UTF-8', errors='replace'), actual_lines[prev_index:actual_index]))
            ))
            break
    if not allow_extra_lines and actual_index < len(actual_lines):
        errors.append('in {}: unexpected extra output [{}]'.format(label, list(map(lambda x: x.decode('UTF-8', errors='replace'), actual_lines[actual_index:]))))
    return errors

def run_test(
        input,
        stdout,
        stderr,
        allow_extra_stdout=False,
        allow_extra_stderr=False,
        timeout=5,
        name=None,
        extra_popen={},
        expect_output_files={},
        prepare_function=None,
):
    for filename in expect_output_files.keys():
        if not filename.startswith('test/'):
            raise Exception("invalid test case: generated file not starting with test/")
        try:
            os.unlink(filename)
        except OSError:
            pass
    if prepare_function != None:
        prepare_function()
    errors = []
    input = b'\n'.join(map(to_bytes, input)) + b'\n'
    process = subprocess.Popen(
        PROGRAM,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        **extra_popen
    )
    try:
        out_data, err_data = process.communicate(input, timeout=timeout)
    except subprocess.TimeoutExpired as to:
        out_data = to.output
        if out_data == None:
            out_data = b''
        if sys.version_info >= (3, 5):
            err_data = to.stderr
        else:
            err_data = b'<error output not available>'
        if err_data == None:
            err_data = b'<error output not available>'
        errors += [ 'timed out after {} seconds'.format(timeout) ]
    errors += compare_lines(
        'stdout',
        stdout,
        out_data.split(b'\n'),
        allow_extra_lines=allow_extra_stdout,
    )
    errors += compare_lines(
        'stderr',
        stderr,
        err_data.split(b'\n'),
        allow_extra_lines=allow_extra_stderr,
    )
    for filename, expected_contents in sorted(expect_output_files.items()):
        try:
            with open(filename, 'rb') as fh:
                lines = list(map(lambda x: x[:-1] if x.endswith(b'\n') else x, fh.readlines()))
                errors += compare_lines(
                    'created file {}'.format(filename),
                    expected_contents,
                    lines,
                    allow_extra_lines=False
                )
            os.unlink(filename)
        except OSError as e:
            if e.errno == errno.ENOENT:
                errors += [ 'file {} was not created'.format(filename) ]
            else:
                errors += [ 'error {} while reading {}'.format(e, filename) ]
    return errors

if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    passed = 0
    failed_any = False
    for test in TESTS:
        errors = run_test(**test)
        if len(errors) == 0:
            print("Passed test", test['name'])
            passed += 1
        else:
            failed_any = True
            print("---")
            print("Failed test", test['name'])
            if len(test['input']) < 5:
                print("Test input:")
                for line in test['input']:
                    print("  ", line)
            if len(test['stdout']) < 5:
                print("Expected stdout regular expression pattern: ",
                      "(extra lines allowed)" if test.get('allow_extra_stdout', False) else "")
                for line in test['stdout']:
                    print("  ", line)
            if len(test['stderr']) < 5:
                print("Expected stderr regular expression pattern: ",
                      "(extra lines allowed)" if test.get('allow_extra_stderr', False) else "")
                for line in test['stderr']:
                    print("  ", line)
            if 'extra_popen' in test or 'prepare_function' in test:
                print("(This test also has some setup code.)")
            print("Errors:")
            print("\n".join(map(lambda x: "  " + x, errors)))
            print("---")
    if failed_any:
        print("""---
Note on interpreting test output patterns:
All expected values matched against a "regular expression" where:
    .* means any string (including empty strings)
    .+ means any string (excluding empty strings)
    everything is matched case-insensitively
""")

