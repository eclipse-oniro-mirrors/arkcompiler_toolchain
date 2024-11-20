#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (c) 2024 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import doctest
import itertools
import os
import subprocess
import sys

# This script prints information about the build system, the operating
# system and the iOS or Mac SDK (depending on the platform "iphonesimulator",
# "iphoneos" or "macosx" generally).


def split_version(version):
    """Splits the Xcode version to 3 values.

    >>> list(SplitVersion('8.2.1.1'))
    ['8', '2', '1']
    >>> list(SplitVersion('9.3'))
    ['9', '3', '0']
    >>> list(SplitVersion('10.0'))
    ['10', '0', '0']
    """
    if isinstance(version, bytes):
        version = version.decode()
    version = version.split('.')
    return itertools.islice(itertools.chain(version, itertools.repeat('0')), 0, 3)


def format_version(version):
    """Converts Xcode version to a format required for DTXcode in Info.plist

    >>> FormatVersion('8.2.1')
    '0821'
    >>> FormatVersion('9.3')
    '0930'
    >>> FormatVersion('10.0')
    '1000'
    """
    major, minor, patch = split_version(version)
    return ('%2s%s%s' % (major, minor, patch)).replace(' ', '0')


def fill_xcode_version(settings_xcode):
    """Fills the Xcode version and build number into |settings_xcode|."""
    lines = subprocess.check_output(['xcodebuild', '-version']).splitlines()
    settings_xcode['xcode_version'] = format_version(lines[0].split()[-1])
    settings_xcode['xcode_version_int'] = int(settings_xcode['xcode_version'], 10)
    settings_xcode['xcode_build'] = lines[-1].split()[-1]


def fill_machine_os_build(settings_os):
    """Fills OS build number into |settings_os|."""
    settings_os['machine_os_build'] = subprocess.check_output(
        ['sw_vers', '-buildVersion']).strip()


def fill_sdk_path_and_version(settings_sdk, platform, xcode_version):
    """Fills the SDK path and version for |platform| into |settings|."""
    settings_sdk['sdk_path'] = subprocess.check_output([
        'xcrun', '-sdk', platform, '--show-sdk-path']).strip()
    settings_sdk['sdk_version'] = subprocess.check_output([
        'xcrun', '-sdk', platform, '--show-sdk-version']).strip()
    settings_sdk['sdk_platform_path'] = subprocess.check_output([
        'xcrun', '-sdk', platform, '--show-sdk-platform-path']).strip()
    if xcode_version >= '0720':
        settings_sdk['sdk_build'] = subprocess.check_output([
            'xcrun', '-sdk', platform, '--show-sdk-build-version']).strip()
    else:
        settings_sdk['sdk_build'] = settings_sdk['sdk_version']


if __name__ == '__main__':
    doctest.testmod()

    parser = argparse.ArgumentParser()
    parser.add_argument("--developer_dir", required=False)
    args, unknownargs = parser.parse_known_args()
    if args.developer_dir:
        os.environ['DEVELOPER_DIR'] = args.developer_dir

    if len(unknownargs) != 1:
        sys.stderr.write(
            'usage: %s [iphoneos|iphonesimulator|macosx]\n' %
            os.path.basename(sys.argv[0]))
        sys.exit(1)

    settings = {}
    fill_machine_os_build(settings)
    fill_xcode_version(settings)
    test_xcode_version = settings.get('xcode_version')
    if test_xcode_version is None:
        raise ValueError("Xcode version is not set or invalid.")
    fill_sdk_path_and_version(settings, unknownargs[0], test_xcode_version)

    for key in sorted(settings):
        value = settings[key]
        if isinstance(value, bytes):
            value = value.decode()
        if key != 'xcode_version_int':
            value = '"%s"' % value
            print('%s=%s' % (key, value))
        else:
            print('%s=%d' % (key, value))
