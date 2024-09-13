#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

import os
import sys
import argparse
import subprocess
import tarfile
import zipfile
import ssl
import shutil
from multiprocessing import cpu_count
from concurrent.futures import ThreadPoolExecutor, as_completed
from functools import partial
from urllib.request import urlopen
import urllib.error
from rich.progress import (
    BarColumn,
    DownloadColumn,
    Progress,
    TaskID,
    TextColumn,
    TimeRemainingColumn,
    TransferSpeedColumn,
)
from util import read_json_file

progress = Progress(
    TextColumn("[bold blue]{task.fields[filename]}", justify="right"),
    BarColumn(bar_width=None),
    "[progress.percentage]{task.percentage:>3.1f}%",
    "•",
    DownloadColumn(),
    "•",
    TransferSpeedColumn(),
    "•",
    TimeRemainingColumn(),
)

def _run_cmd(cmd):
    res = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE)
    sout, serr = res.communicate()
    return sout.rstrip().decode('utf-8'), serr, res.returncode

def _check_sha256(check_url, local_file):
    check_sha256_cmd = ''.join(['curl -s -k ', check_url, '.sha256'])
    local_sha256_cmd = ''.join(['sha256sum ', local_file, "|cut -d ' ' -f1"])
    check_sha256, err, returncode = _run_cmd(check_sha256_cmd)
    local_sha256, err, returncode = _run_cmd(local_sha256_cmd)
    return check_sha256 == local_sha256

def _check_sha256_by_mark(args, check_url, code_dir, unzip_dir, unzip_filename):
    check_sha256_cmd = ''.join(['curl -s -k ', check_url, '.sha256'])
    check_sha256, err, returncode = _run_cmd(check_sha256_cmd)
    mark_file_dir = os.path.join(code_dir, unzip_dir)
    mark_file_name = ''.join([check_sha256, '.', unzip_filename, '.mark'])
    mark_file_path = os.path.join(mark_file_dir, mark_file_name)
    args.mark_file_path = mark_file_path
    return os.path.exists(mark_file_path)

def _config_parse(config, tool_repo):
    unzip_dir = config.get('unzip_dir')
    huaweicloud_url = ''.join([tool_repo, config.get('file_path')])
    unzip_filename = config.get('unzip_filename')
    md5_huaweicloud_url_cmd = ''.join(['echo ', huaweicloud_url, "|md5sum|cut -d ' ' -f1"])
    md5_huaweicloud_url, err, returncode = _run_cmd(md5_huaweicloud_url_cmd)
    bin_file = os.path.basename(huaweicloud_url)
    return unzip_dir, huaweicloud_url, unzip_filename, md5_huaweicloud_url, bin_file

def _uncompress(args, src_file, code_dir, unzip_dir, unzip_filename, mark_file_path):
    dest_dir = os.path.join(code_dir, unzip_dir)
    if src_file[-3:] == 'zip':
        cmd = 'unzip -o {} -d {};echo 0 > {}'.format(src_file, dest_dir, mark_file_path)
    elif src_file[-6:] == 'tar.gz':
        cmd = 'tar -xvzf {} -C {};echo 0 > {}'.format(src_file, dest_dir, mark_file_path)
    else:
        cmd = 'tar -xvf {} -C {};echo 0 > {}'.format(src_file, dest_dir, mark_file_path)
    _, _, returncode = _run_cmd(cmd)
    return returncode

def _copy_url(args, task_id, url, local_file, code_dir, unzip_dir, unzip_filename, mark_file_path):
    # download files
    download_buffer_size = 32768
    progress.console.log('Requesting {}'.format(url))
    flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL
    modes = 0o777
    try:
        response = urlopen(url)
    except urllib.error.HTTPError as e:
        progress.console.log("Failed to open {}, HTTPError: {}".format(url, e.code), style='red')
        return 1
    progress.update(task_id, total=int(response.info()["Content-length"]))
    with os.fdopen(os.open(local_file, flags, modes), 'wb') as dest_file:
        progress.start_task(task_id)
        for data in iter(partial(response.read, download_buffer_size), b""):
            dest_file.write(data)
            progress.update(task_id, advance=len(data))
    progress.console.log("Downloaded {}".format(local_file))
    if not _check_sha256(url, local_file):
        progress.console.log('{}, Sha256 check download FAILED.'.format(local_file), style='red')
        return 1

    # decompressing files
    progress.console.log("Decompressing {}".format(local_file))
    returncode = _uncompress(args, local_file, code_dir, unzip_dir, unzip_filename, mark_file_path)
    progress.console.log("Decompressed {}".format(local_file))
    return returncode


def _hwcloud_download_wrapper(args, config, bin_dir, code_dir, retries):
    attempt = 0
    success = False
    with progress:
        while not success and attempt < retries:
            success = _hwcloud_download(args, config, bin_dir, code_dir, retries)
            attempt += 1
    return success


def _hwcloud_download(args, config, bin_dir, code_dir, retries):
    try:
        cnt = cpu_count()
    except:
        cnt = 1

    success = False
    with ThreadPoolExecutor(max_workers=cnt) as pool:
        tasks = dict()
        for config_info in config:
            unzip_dir, huaweicloud_url, unzip_filename, md5_huaweicloud_url, bin_file = _config_parse(config_info,
                args.tool_repo)
            abs_unzip_dir = os.path.join(code_dir, unzip_dir)
            if not os.path.exists(abs_unzip_dir):
                os.makedirs(abs_unzip_dir)
            if _check_sha256_by_mark(args, huaweicloud_url, code_dir, unzip_dir, unzip_filename):
                progress.console.log('{}, Sha256 markword check OK.'.format(huaweicloud_url), style='green')
                continue

            _run_cmd(''.join(['rm -rf ', code_dir, '/', unzip_dir, '/*.', unzip_filename, '.mark']))
            _run_cmd(''.join(['rm -rf ', code_dir, '/', unzip_dir, '/', unzip_filename]))
            local_file = os.path.join(bin_dir, ''.join([md5_huaweicloud_url, '.', bin_file]))

            if os.path.exists(local_file) and not _check_sha256(huaweicloud_url, local_file):
                os.remove(local_file)

            if not os.path.exists(local_file):
                filename = huaweicloud_url.split("/")[-1]
                task_id = progress.add_task("download", filename=filename, start=False)
                task = pool.submit(_copy_url, args, task_id, huaweicloud_url, local_file, code_dir, unzip_dir,
                    unzip_filename, args.mark_file_path)
                tasks[task] = os.path.basename(huaweicloud_url)
                continue

            if _check_sha256(huaweicloud_url, local_file):
                progress.console.log('{}, Sha256 check download OK.'.format(local_file), style='green')
                task = pool.submit(_uncompress, args, local_file, code_dir, unzip_dir, unzip_filename,
                    args.mark_file_path)
                tasks[task] = os.path.basename(huaweicloud_url)
            else:
                os.remove(local_file)
        returncode = 0
        for task in as_completed(tasks):
            if task.result():
                returncode += task.result()
            progress.console.log('{}, download and decompress completed, exit code: {}'
                                     .format(tasks.get(task), task.result()), style='green')
        success = returncode == 0
    return success


def _file_handle(config, code_dir):
    for config_info in config:
        src_dir = ''.join([code_dir, config_info.get('src')])
        dest_dir = ''.join([code_dir, config_info.get('dest')])
        tmp_dir = config_info.get('tmp')
        symlink_src = config_info.get('symlink_src')
        symlink_dest = config_info.get('symlink_dest')
        if os.path.exists(src_dir):
            if tmp_dir:
                tmp_dir = ''.join([code_dir, tmp_dir])
                shutil.move(src_dir, tmp_dir)
                cmd = 'mv {}/*.mark {}'.format(dest_dir, tmp_dir)
                _run_cmd(cmd)
                if os.path.exists(dest_dir):
                    shutil.rmtree(dest_dir)
                shutil.move(tmp_dir, dest_dir)
            elif symlink_src and symlink_dest:
                if os.path.exists(dest_dir) and dest_dir != src_dir:
                    shutil.rmtree(dest_dir)
                shutil.move(src_dir, dest_dir)
                os.symlink(''.join([dest_dir, symlink_src]), ''.join([dest_dir, symlink_dest]))
            else:
                _run_cmd('chmod 755 {} -R'.format(dest_dir))

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--skip-ssl', action='store_true', help='skip ssl authentication')
    parser.add_argument('--tool-repo', default='https://repo.huaweicloud.com', help='prebuilt file download source')
    parser.add_argument('--host-cpu', help='host cpu', required=True)
    parser.add_argument('--host-platform', help='host platform', required=True)
    args = parser.parse_args()
    args.code_dir = os.path.abspath(os.path.join(os.getcwd()))
    if args.skip_ssl:
        ssl._create_default_https_context = ssl._create_unverified_context

    host_platform = args.host_platform
    host_cpu = args.host_cpu
    tool_repo = args.tool_repo
    config_file = os.path.join(args.code_dir,
        'arkcompiler/toolchain/build/prebuilts_download/prebuilts_download_config.json')
    config_info = read_json_file(config_file)
    file_handle_config = config_info.get('file_handle_config')

    args.bin_dir = os.path.join(args.code_dir, config_info.get('prebuilts_download_dir'))
    if not os.path.exists(args.bin_dir):
        os.makedirs(args.bin_dir)
    copy_config = config_info.get(host_platform).get(host_cpu).get('copy_config')
    if host_platform == 'linux':
        linux_copy_config = config_info.get(host_platform).get(host_cpu).get('linux_copy_config')
        copy_config.extend(linux_copy_config)
    elif host_platform == 'darwin':
        darwin_copy_config = config_info.get(host_platform).get(host_cpu).get('darwin_copy_config')
        copy_config.extend(darwin_copy_config)
    retries = config_info.get('retries')
    args.retries = 1 if retries is None else retries
    if not _hwcloud_download_wrapper(args, copy_config, args.bin_dir, args.code_dir, args.retries):
        return 1
    _file_handle(file_handle_config, args.code_dir)
    return 0


if __name__ == '__main__':
    sys.exit(main())
