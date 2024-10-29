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
import re
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
        install_cmd = ['hdc', 'install', '-p', hap_path]
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
                return int(line.strip().split()[1])
        return 0

    @classmethod
    def launch_application(cls, bundle_name, hap_path, start_mode=None):
        cls.stop(bundle_name)
        cls.uninstall(bundle_name)
        cls.install(hap_path)
        cls.start(bundle_name, start_mode)
        cls.keep_awake()
        time.sleep(3)
        pid = cls.get_pid(bundle_name)
        return int(pid)

    @classmethod
    def attach(cls, bundle_name):
        attach_cmd = (['hdc', 'shell', 'aa', 'attach', '-b', bundle_name])
        logging.info('start application: ' + ' '.join(attach_cmd))
        attach_result = subprocess.run(attach_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        logging.info(attach_result.stdout)
        assert attach_result.stdout.decode('utf-8').strip() == 'attach app debug successfully.'

    @classmethod
    def click_on_middle(cls):
        """
        Simulate clicking the center of the screen
        """
        get_screen_info_cmd = ['hdc', 'shell', 'hidumper', '-s', 'RenderService', '-a', 'screen']
        screen_info = subprocess.run(get_screen_info_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        match = re.search(r'physical screen resolution: (\d+)x(\d+)', screen_info.stdout.decode('utf-8'))
        assert match is not None

        middle_x = int(match.group(1)) // 2
        middle_y = int(match.group(2)) // 2
        click_cmd = ['hdc', 'shell', 'uinput', '-T', '-c', str(middle_x), str(middle_y)]
        click_result = subprocess.run(click_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        click_result_out = click_result.stdout.decode('utf-8').strip()
        logging.info(click_result_out)
        assert "click coordinate" in click_result_out

    @classmethod
    def back(cls):
        """
        Simulate the back button to return to the previous step
        """
        cmd = ['hdc', 'shell', 'uitest', 'uiInput', 'keyEvent', 'Back']
        logging.info('click the back button: ' + ' '.join(cmd))
        result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        logging.info(result.stdout.strip())
        assert result.stdout.decode('utf-8').strip() == 'No Error'

    @classmethod
    def keep_awake(cls):
        keep_awake_cmd = ['hdc', 'shell', 'power-shell', 'setmode', '602']
        keep_awake_result = subprocess.run(keep_awake_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        keep_awake_result_out = keep_awake_result.stdout.decode('utf-8').strip()
        logging.info(keep_awake_result_out)
        assert "Set Mode Success!" in keep_awake_result_out

    @classmethod
    def hot_reload(cls, hqf_path):
        cmd = ['hdc', 'shell', 'bm', 'quickfix', '-a', '-f', hqf_path]
        logging.info('hot reload: ' + ' '.join(cmd))
        result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        logging.info(result.stdout.strip())
        assert result.stdout.decode('utf-8').strip() == 'apply quickfix succeed.'
