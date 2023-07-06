/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <functional>
#include "cli_command.h"
#include "log_wrapper.h"

namespace OHOS::ArkCompiler::Toolchain{
const std::string HELP_MSG = "usage: <command> <options>\n"
    " These are common commands list:\n"
    "  allocationtrack(at)               allocation-track-start with options\n"
    "  break(b)                          break with options\n"
    "  backtrack(bt)                     backtrace\n"
    "  continue(c)                       continue\n"
    "  cpuprofile(cp)                    cpu-profile-start with options\n"
    "  delete(d)                         delete with options\n"
    "  disable                           disable\n"
    "  display                           display\n"
    "  enable                            enable\n"
    "  finish(fin)                       finish\n"
    "  frame(f)                          frame\n"
    "  heapdump(hd)                      heapdump with options\n"
    "  help(h)                           list available commands\n"
    "  ignore(ig)                        ignore\n"
    "  infobreakpoints(infob)            info-breakpoints\n"
    "  infosource(infos)                 info-source\n"
    "  jump(j)                           jump\n"
    "  list(l)                           list\n"
    "  next(n)                           next\n"
    "  print(p)                          print with options\n"
    "  ptype                             ptype\n"
    "  quit(q)                           quit\n"
    "  run(r)                            run\n"
    "  setvar(sv)                        set value with options\n"
    "  step(s)                           step\n"
    "  undisplay                         undisplay\n"
    "  watch                             watch\n";

const std::vector<std::string> cmdList = {
    "allocationtrack",
    "break",
    "backtrack",
    "continue",
    "cpuprofile",
    "delete",
    "disable",
    "display",
    "enable",
    "finish",
    "frame",
    "heapdump",
    "help",
    "ignore",
    "infobreakpoints",
    "infosource",
    "jump",
    "list",
    "next",
    "print",
    "ptype",
    "run",
    "setvar",
    "step",
    "undisplay",
    "watch"
};
std::string CliCommand::ExecCommand()
{

    int result = CreateCommandMap();
    if (result != ERR_OK) {
        LOGE("failed to create command map.");
    }

    result = OnCommand();
    if (result != ERR_OK) {
        LOGE("failed to execute your command.");
        resultReceiver_ = "error: failed to execute your command.\n";
    }
    return resultReceiver_;
}

ErrCode CliCommand::CreateCommandMap()
{
    commandMap_ = {
        {std::make_pair("allocationtrack","at"), std::bind(&CliCommand::ExecAllocationTrackCommand, this)},
        {std::make_pair("break","b"), std::bind(&CliCommand::ExecBreakCommand, this)},
        {std::make_pair("backtrack","bt"), std::bind(&CliCommand::ExecBackTrackCommand, this)},
        {std::make_pair("continue","c"), std::bind(&CliCommand::ExecContinueCommand, this)},
        {std::make_pair("cpuprofile","cp"), std::bind(&CliCommand::ExecCpuProfileCommand, this)},
        {std::make_pair("delete","d"), std::bind(&CliCommand::ExecDeleteCommand, this)},
        {std::make_pair("disable","disable"), std::bind(&CliCommand::ExecDisableCommand, this)},
        {std::make_pair("display","display"), std::bind(&CliCommand::ExecDisplayCommand, this)},
        {std::make_pair("enable","enable"), std::bind(&CliCommand::ExecEnableCommand, this)},
        {std::make_pair("finish","fin"), std::bind(&CliCommand::ExecFinishCommand, this)},
        {std::make_pair("frame","f"), std::bind(&CliCommand::ExecFrameCommand, this)},
        {std::make_pair("heapdump","hd"), std::bind(&CliCommand::ExecHeapDumpCommand, this)},
        {std::make_pair("help","h"), std::bind(&CliCommand::ExecHelpCommand, this)},
        {std::make_pair("ignore","ig"), std::bind(&CliCommand::ExecIgnoreCommand, this)},
        {std::make_pair("infobreakpoints","infob"), std::bind(&CliCommand::ExecInfoBCommand, this)},
        {std::make_pair("infosource","infos"), std::bind(&CliCommand::ExecInfoSCommand, this)},
        {std::make_pair("jump","j"), std::bind(&CliCommand::ExecJumpCommand, this)},
        {std::make_pair("list","l"), std::bind(&CliCommand::ExecListCommand, this)},
        {std::make_pair("next","n"), std::bind(&CliCommand::ExecNextCommand, this)},
        {std::make_pair("print","p"), std::bind(&CliCommand::ExecPrintCommand, this)},
        {std::make_pair("ptype","ptype"), std::bind(&CliCommand::ExecPtypeCommand, this)},
        {std::make_pair("run","r"), std::bind(&CliCommand::ExecRunCommand, this)},
        {std::make_pair("setvar","sv"), std::bind(&CliCommand::ExecSetValueCommand, this)},
        {std::make_pair("step","s"), std::bind(&CliCommand::ExecStepCommand, this)},
        {std::make_pair("undisplay","undisplay"), std::bind(&CliCommand::ExecUndisplayCommand, this)},
        {std::make_pair("watch","wa"), std::bind(&CliCommand::ExecWatchCommand, this)},
    };

    return ERR_OK;
}

ErrCode CliCommand::ExecAllocationTrackCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecBreakCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecBackTrackCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecContinueCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecCpuProfileCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecDeleteCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecDisableCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecDisplayCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecEnableCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecFinishCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecFrameCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecHeapDumpCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecHelpCommand()
{
    std::cout << HELP_MSG;
    return ERR_OK;
}

ErrCode CliCommand::ExecIgnoreCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecInfoBCommand()
{
    return ERR_OK;
}
ErrCode CliCommand::ExecInfoSCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecJumpCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecListCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecNextCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecPrintCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecPtypeCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecRunCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecSetValueCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecStepCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecUndisplayCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::ExecWatchCommand()
{
    return ERR_OK;
}

ErrCode CliCommand::OnCommand()
{
    std::map<StrPair, std::function<int()>>::iterator it;
    StrPair cmdPair;
    bool haveCmdFlag = false;

    for (it = commandMap_.begin(); it != commandMap_.end(); it++) {
        cmdPair = it->first;
        if (!strcmp(cmdPair.first.c_str(), cmd_.c_str())
            ||!strcmp(cmdPair.second.c_str(), cmd_.c_str())) {
            auto respond = it->second;
            respond();
            std::cout << "exe success, cmd is " << cmd_ << std::endl;
            return ERR_OK;
        }
    }

    for(unsigned int i = 0; i < cmdList.size(); i++) {
        if (!strncmp(cmdList[i].c_str(), cmd_.c_str(), std::strlen(cmd_.c_str()))) {
            haveCmdFlag = true;
            std::cout << cmdList[i] << " ";
        }
    }

    if (haveCmdFlag) {
        std::cout << std::endl;
    } else {
        ExecHelpCommand();
    }
    
    return ERR_OK;
}

} // namespace OHOS::ArkCompiler::Toolchain
