#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Runs 'ld -shared' and generates a .TOC file that's untouched when unchanged.

This script exists to avoid using complex shell commands in
gcc_toolchain.gni's tool("solink"), in case the host running the compiler
does not have a POSIX-like shell (e.g. Windows).
"""

import argparse
import os
import subprocess
import sys
import shutil


def reformat_rsp_file(rspfile):
    """ Move all implibs from --whole-archive section"""
    with open(rspfile, "r") as fi:
        rspcontent = fi.read()
    result = []
    implibs = []
    naflag = False
    for arg in rspcontent.split(" "):
        if naflag and arg.endswith(".lib"):
            implibs.append(arg)
            continue
        result.append(arg)
        if arg == "-Wl,--whole-archive":
            naflag = True
            continue
        if arg == "-Wl,--no-whole-archive":
            naflag = False
            result.extend(implibs)

    with open(rspfile, "w") as fo:
        fo.write(" ".join(result))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--strip',
                        help='The strip binary to run',
                        metavar='PATH')
    parser.add_argument('--sofile',
                        required=True,
                        help='Shared object file produced by linking command',
                        metavar='FILE')
    parser.add_argument('--output',
                        required=True,
                        help='Final output shared object file',
                        metavar='FILE')
    parser.add_argument('--libfile', required=False, metavar='FILE')
    parser.add_argument('command', nargs='+', help='Linking command')
    args = parser.parse_args()

    if args.sofile.endswith(".dll"):
        rspfile = None
        for a in args.command:
            if a[0] == "@":
                rspfile = a[1:]
                break
        if rspfile:
            reformat_rsp_file(rspfile)
    # Work-around for gold being slow-by-default. http://crbug.com/632230
    fast_env = dict(os.environ)
    fast_env['LC_ALL'] = 'C'

    # First, run the actual link.
    result = subprocess.call(args.command, env=fast_env)
    if result != 0:
        return result

    # Finally, strip the linked shared object file (if desired).
    if args.strip:
        result = subprocess.call([args.strip, '-o', args.output, args.sofile])
    if args.libfile:
        libfile_name = os.path.basename(args.libfile)
        sofile_output_dir = os.path.dirname(args.sofile)
        unstripped_libfile = os.path.join(sofile_output_dir, libfile_name)
        shutil.copy2(unstripped_libfile, args.libfile)

    return result


if __name__ == "__main__":
    sys.exit(main())
