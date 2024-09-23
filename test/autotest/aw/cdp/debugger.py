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

Description: Python CDP Debugger.
"""

from dataclasses import dataclass, field
from typing import Optional, List


@dataclass
class BreakLocationUrl:
    url: str
    line_number: int
    column_number: Optional[int] = 0

    def to_json(self):
        json = {'url': self.url,
                'lineNumber': self.line_number,
                'columnNumber': self.column_number}
        return json


@dataclass
class RemoveBreakpointsUrl:
    url: str = ""


@dataclass
class SetBreakpointsLocations:
    locations: list = field(default_factory=list)


def enable(max_scripts_cache_size: Optional[float] = None):
    command = {'method': 'Debugger.enable'}
    if max_scripts_cache_size:
        command['params'] = {'maxScriptsCacheSize': max_scripts_cache_size}
    return command


def resume():
    command = {'method': 'Debugger.resume'}
    return command


def remove_breakpoints_by_url(url: str):
    command = {'method': 'Debugger.removeBreakpointsByUrl',
               'params': {'url': url}}
    return command


def get_possible_and_set_breakpoint_by_url(locations: List[BreakLocationUrl]):
    params = {'locations': []}
    for location in locations:
        params['locations'].append(location.to_json())
    command = {'method': 'Debugger.getPossibleAndSetBreakpointByUrl',
               'params': params}
    return command


def step_over():
    command = {'method': 'Debugger.stepOver'}
    return command


def step_into():
    command = {'method': 'Debugger.stepInto'}
    return command


def step_out():
    command = {'method': 'Debugger.stepOut'}
    return command


def disable():
    command = {'method': 'Debugger.disable'}
    return command