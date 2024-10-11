#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Copyright (c) 2024 Huawei Device Co., Ltd.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Description: Action words of Application launch.
"""

import logging
import subprocess
import time


class Application(object):
    @classmethod
    def stop(cls, bundle_name):
        stop_cmd = ['hdc', 'shell', 'aa', 'force-stop', bundle_name]
        logging.info('force stop application: ' + ' '.join(stop_cmd))
        stop_result = subprocess.run(stop_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        logging.info(stop_result.stdout.strip())
        assert stop_result.returncode == 0

    @classmethod
    def uninstall(cls, bundle_name):
        uninstall_cmd = ['hdc', 'uninstall', bundle_name]
        logging.info('uninstall application: ' + ' '.join(uninstall_cmd))
        uninstall_result = subprocess.run(uninstall_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        logging.info(uninstall_result.stdout.strip())
        assert uninstall_result.returncode == 0

    @classmethod
    def install(cls, hap_path):
        install_cmd = ['hdc', 'install', '-r', hap_path]
        logging.info('install application: ' + ' '.join(install_cmd))
        install_result = subprocess.run(install_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        logging.info(install_result.stdout)
        assert 'successfully' in install_result.stdout.decode('utf-8')

    @classmethod
    def start(cls, bundle_name, start_mode=None):
        start_cmd = (['hdc', 'shell', 'aa', 'start', '-a', 'EntryAbility', '-b', bundle_name] +
                     ([start_mode] if start_mode else []))
        logging.info('start application: ' + ' '.join(start_cmd))
        start_result = subprocess.run(start_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        logging.info(start_result.stdout)
        assert start_result.stdout.decode('utf-8').strip() == 'start ability successfully.'

    @classmethod
    def get_pid(cls, bundle_name):
        ps_cmd = ['hdc', 'shell', 'ps', '-ef']
        ps_result = subprocess.run(ps_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        ps_result_out = ps_result.stdout.decode('utf-8')
        for line in ps_result_out.strip().split('\r\n'):
            if bundle_name in line:
                logging.info(f'pid of {bundle_name}: ' + line)
                return line.strip().split()[1]
        return 0

    @classmethod
    def launch_application(cls, bundle_name, hap_path, start_mode=None):
        cls.stop(bundle_name)
        cls.uninstall(bundle_name)
        cls.install(hap_path)
        cls.start(bundle_name, start_mode)
        time.sleep(3)
        pid = cls.get_pid(bundle_name)
        return int(pid)
