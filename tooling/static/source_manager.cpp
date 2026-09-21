/**
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "source_manager.h"

#include <string>
#include <string_view>

#include "types/numeric_id.h"

namespace ark::tooling::inspector {
std::pair<ScriptId, bool> SourceManager::GetScriptId(std::string_view fileName, std::string_view scriptIdentity) const
{
    os::memory::LockHolder lock(mutex_);

    auto p =
        fileNameToId_.emplace(ScriptKey {std::string(fileName), std::string(scriptIdentity)}, fileNameToId_.size());
    ScriptId id(p.first->second);
    bool isNewForThread = knownSources_.insert(id).second;

    if (p.second) {
        idToScript_.emplace(id, &p.first->first);
    }

    return {id, isNewForThread};
}

std::string_view SourceManager::GetSourceFileName(ScriptId id) const
{
    os::memory::LockHolder lock(mutex_);

    auto it = idToScript_.find(id);
    if (it != idToScript_.end()) {
        return it->second->fileName;
    }

    LOG(ERROR, DEBUGGER) << "No file with script id " << id;

    return {};
}

SourceManager::ScriptInfo SourceManager::GetScript(ScriptId id) const
{
    os::memory::LockHolder lock(mutex_);

    auto it = idToScript_.find(id);
    if (it == idToScript_.end()) {
        return {};
    }

    return {it->second->fileName, it->second->identity};
}

}  // namespace ark::tooling::inspector
