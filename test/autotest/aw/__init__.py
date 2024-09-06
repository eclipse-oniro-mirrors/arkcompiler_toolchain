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

Description: __init__.py of the aw package
"""
from aw.websocket import WebSocket
from aw.application import Application
from aw.utils import Utils
from aw.fport import Fport
from aw.taskpool import TaskPool
from aw.cdp import debugger
from aw.cdp import runtime
from aw.cdp import heap_profiler
from aw.cdp import cpu_profiler


communicate_with_debugger_server = Utils.communicate_with_debugger_server
async_wait_timeout = Utils.async_wait_timeout