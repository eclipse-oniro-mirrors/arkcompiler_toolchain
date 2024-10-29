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

Description: Scenario test case.
"""

import logging
import os
import time

import pytest

from aw import Application
from aw import Utils


@pytest.mark.hot_reload
@pytest.mark.timeout(120)
class TestHotReload01:
    """
    测试用例：热重载基础功能, 重载文件impact HAR, 重载文件import HSP
    测试步骤：
        1、启动应用
        2、点击屏幕中央，调用foo函数，输出日志：
            Test Application HotReloadHarHsp01 => foo(): base function foo
            Test Application HotReloadHarHsp01 => calc(): 8
        3、触发热重载
        4、Hilog输出：Load patch success!
        5、点击屏幕中央，调用foo函数，输出日志：
            Test Application HotReloadHarHsp01 => foo(): patch function foo
            Test Application HotReloadHarHsp01 => calc(): 48
    测试应用代码示例：
        import {Sum} from '@ohos/HarLibrary'
        import {Multiply} from '@ohos/HspDiscovery'
        修改前：
            .onClick(() => {
              console.log('Test HotReload Application 01 => foo():', foo())
              console.log('Test HotReload Application 01 => calc():', calc(1, 2))
            })
            function foo() {
              return "base function foo"
            }
            function calc(x: number, y: number): number {
              return Multiply(Sum(x, x), Sum(y, y))
            }
        修改后：HotReloadHarHsp01.hap
            .onClick(() => {
              console.log('Test HotReload Application 01 => foo():', foo())
              console.log('Test HotReload Application 01 => calc():', calc(3, 4))
            })
            function foo() {
              return "patch function foo"
            }
            function calc(x: number, y: number): number {
              return Multiply(Sum(x, x), Sum(y, y))
            }
    """

    def setup_method(self):
        logging.info('Start running test_hot_reload_01: setup')

        self.log_path = rf'{os.path.dirname(__file__)}\..\log'
        self.hilog_file_name = 'test_hot_reload_01.hilog.txt'
        self.hilog_file_path = os.path.join(self.log_path, self.hilog_file_name)
        self.id_generator = Utils.message_id_generator()

        # receive the hilog before the test start
        Utils.clear_fault_log()
        self.hilog_process, self.write_thread = Utils.save_hilog(log_path=self.log_path,
                                                                 file_name=self.hilog_file_name,
                                                                 debug_on=False)

    def teardown_method(self):
        Application.uninstall(self.config['bundle_name'])

        # terminate the hilog receive process after the test done
        time.sleep(3)
        self.hilog_process.stdout.close()
        self.hilog_process.terminate()
        self.hilog_process.wait()
        self.write_thread.join()

        Utils.save_fault_log(log_path=self.log_path)
        logging.info('test_hot_reload_01 done')

    def test(self, test_suite_hotreload_har_hsp_01):
        logging.info('Start running test_hot_reload_01: test')
        self.config = test_suite_hotreload_har_hsp_01
        ################################################################################################################
        # trigger onclick event
        ################################################################################################################
        logging.info(f'{"=" * 30} Before Hot Reload {"=" * 30}')
        Application.click_on_middle()
        time.sleep(3)
        matched_log = Utils.search_hilog(self.hilog_file_path, key_world=b'base function foo')
        logging.info(matched_log)
        assert len(matched_log) == 1
        matched_log = Utils.search_hilog(self.hilog_file_path,
                                         key_world=b'Test Application HotReloadHarHsp01 => calc(): 8')
        logging.info(matched_log)
        assert len(matched_log) == 1
        ################################################################################################################
        # perform Hot Reload
        ################################################################################################################
        logging.info(f'{"=" * 30} Perform Hot Reload {"=" * 30}')
        Utils.hdc_file_send(source=self.config['local_hqf_entry_path'], sink=self.config['remote_hqf_entry_path'])
        Application.hot_reload(self.config['remote_hqf_entry_path'])
        time.sleep(3)
        matched_log = Utils.search_hilog(self.hilog_file_path, key_world=b'Load patch success!')
        logging.info(matched_log)
        assert len(matched_log) == 1
        ################################################################################################################
        # after hot reload
        ################################################################################################################
        logging.info(f'{"=" * 30} After Hot Reload {"=" * 30}')
        Application.click_on_middle()   # trigger onclick event
        time.sleep(3)
        matched_log = Utils.search_hilog(self.hilog_file_path, key_world=b'patch function foo')
        logging.info(matched_log)
        assert len(matched_log) == 1
        matched_log = Utils.search_hilog(self.hilog_file_path,
                                         key_world=b'Test Application HotReloadHarHsp01 => calc(): 48')
        logging.info(matched_log)
        assert len(matched_log) == 1
        ################################################################################################################
        # check if the application is running normally
        ################################################################################################################
        pid = Application.get_pid(self.config['bundle_name'])
        assert pid == self.config['pid'], logging.error(f'App is no longer running with pid: {pid}')
