/**
 *  Copyright (C) 2021 FISCO BCOS.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @brief tars implementation for create/remove/stop/start the blockchain node
 * @file TarsNodeController.cpp
 * @author: yujiechen
 * @date 2021-09-27
 */
#include "TarsNodeController.h"

using namespace bcos;
using namespace bcos::group;

void TarsNodeController::generateTarsTask(tars::TaskReq& _tarsTask, std::string const& _command,
    std::map<std::string, ChainNodeInfo::Ptr> const& _nodeInfos,
    std::map<string, string> const& _parameters)
{
    // generate the tarsItemReq
    for (auto const& it : _nodeInfos)
    {
        auto appName = it.first;
        auto nodeInfo = it.second;
        auto const& deployInfo = nodeInfo->deployInfo();
        for (auto const& deployIt : deployInfo)
        {
            auto serviceName = deployIt.first;
            auto deployIp = deployIt.second;
            tars::TaskItemReq tarsItem;
            tarsItem.application = appName;
            tarsItem.serverName = serviceName;
            tarsItem.nodeName = deployIp;
            tarsItem.command = _command;
            tarsItem.parameters = _parameters;
            tarsItem.userName = m_config->userName();
            _tarsTask.taskItemReq.emplace_back(tarsItem);
        }
    }
    _tarsTask.serial = true;
    _tarsTask.userName = m_config->userName();
}

void TarsNodeController::addTaskReq(tars::EMTaskCommand const& _command,
    std::map<std::string, ChainNodeInfo::Ptr> const& _nodeInfos,
    std::function<void(Error::Ptr&&)> _callback,
    std::map<std::string, std::string> const& _parameters)
{
    tars::TaskReq req;
    generateTarsTask(req, etos(_command), _nodeInfos, _parameters);

    // maybe memory leak here?
    auto addTaskCallback = new AddTaskCallback(_callback);
    m_config->nodeManager()->async_addTaskReq(addTaskCallback, req);
}

void TarsNodeController::createNodes(std::map<std::string, ChainNodeInfo::Ptr> const& _nodeInfos,
    std::function<void(Error::Ptr&&)> _callback)
{
    tars::EMTaskCommand command = EM_CMD_PATCH;
    // TODO: set parameters
    addTaskReq(command, _nodeInfos, _callback);
}

void TarsNodeController::removeNodes(std::map<std::string, ChainNodeInfo::Ptr> const& _nodeInfos,
    std::function<void(Error::Ptr&&)> _callback)
{
    tars::EMTaskCommand command = EM_CMD_UNINSTALL;
    addTaskReq(command, _nodeInfos, _callback);
}

void TarsNodeController::startNodes(std::map<std::string, ChainNodeInfo::Ptr> const& _nodeInfos,
    std::function<void(Error::Ptr&&)> _callback)
{
    tars::EMTaskCommand command = EM_CMD_START;
    addTaskReq(command, _nodeInfos, _callback);
}

void TarsNodeController::stopNodes(std::map<std::string, ChainNodeInfo::Ptr> const& _nodeInfos,
    std::function<void(Error::Ptr&&)> _callback)
{
    tars::EMTaskCommand command = EM_CMD_STOP;
    addTaskReq(command, _nodeInfos, _callback);
}