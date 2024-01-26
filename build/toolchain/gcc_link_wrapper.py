#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Runs a linking command and optionally a strip command.

This script exists to avoid using complex shell commands in
gcc_toolchain.gni's tool("link"), in case the host running the compiler
does not have a POSIX-like shell (e.g. Windows).
"""

import argparse
import os
import subprocess
import sys


def is_static_link(command):
    if "-static" in command:
        return True
    else:
        return False


""" since static link and dynamic link have different CRT files on ohos,
and we use dynamic link CRT files as default, so when link statically,
we need change the CRT files
"""


def update_crt(command):
    for item in command:
        if str(item).find("crtbegin_dynamic.o") >= 0:
            index = command.index(item)
            new_crtbegin = str(item).replace("crtbegin_dynamic.o",
                                             "crtbegin_static.o")
            command[index] = new_crtbegin
    return command


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--strip',
                        help='The strip binary to run',
                        metavar='PATH')
    parser.add_argument('--unstripped-file',
                        help='Executable file produced by linking command',
                        metavar='FILE')
    parser.add_argument('--output',
                        required=True,
                        help='Final output executable file',
                        metavar='FILE')
    parser.add_argument('--clang_rt_dso_path',
                        help=('Clang asan runtime shared library'))
    parser.add_argument('command', nargs='+', help='Linking command')
    args = parser.parse_args()

    # Work-around for gold being slow-by-default. http://crbug.com/632230
    fast_env = dict(os.environ)
    fast_env['LC_ALL'] = 'C'
    if is_static_link(args.command):
        command = update_crt(args.command)
        if args.clang_rt_dso_path is not None:
            return 0
    else:
        command = args.command
    result = subprocess.call(command, env=fast_env)
    if result != 0:
        return result

    # Finally, strip the linked executable (if desired).
    if args.strip:
        result = subprocess.call([args.strip, '-o', args.output, args.unstripped_file])

    return result


if __name__ == "__main__":
    sys.exit(main())
