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
 * @file TarsNodeController.h
 * @author: yujiechen
 * @date 2021-09-27
 */
#pragma once
#include "Common.h"
#include "NodeControllerInterface.h"
#include "bcos-group-manager/config/GroupMgrConfig.h"
#include <bcos-tars-protocol/ErrorConverter.h>
#include <tarscpp/framework/AdminReg.h>
namespace bcos
{
namespace group
{
class AddTaskCallback : public tars::AdminRegPrxCallback
{
public:
    AddTaskCallback(std::function<void(Error::Ptr&&)> _callback) : m_callback(_callback) {}
    void callback_addTaskReq(tars::Int32 _ret) override { m_callback(bcostars::toBcosError(_ret)); }
    void callback_addTaskReq_exception(tars::Int32 _ret) override
    {
        m_callback(bcostars::toBcosError(_ret));
    }

private:
    std::function<void(Error::Ptr&&)> m_callback;
};

class TarsNodeController : public NodeControllerInterface
{
public:
    using Ptr = std::shared_ptr<TarsNodeController>;
    TarsNodeController(GroupMgrConfig::Ptr _config) : m_config(_config) {}
    ~TarsNodeController() override {}

    void createNodes(std::map<std::string, ChainNodeInfo::Ptr> const& _nodeInfos,
        std::function<void(Error::Ptr&&)> _callback) override;
    void removeNodes(std::map<std::string, ChainNodeInfo::Ptr> const& _nodeInfos,
        std::function<void(Error::Ptr&&)> _callback) override;

    void startNodes(std::map<std::string, ChainNodeInfo::Ptr> const& _nodeInfos,
        std::function<void(Error::Ptr&&)> _callback) override;
    void stopNodes(std::map<std::string, ChainNodeInfo::Ptr> const& _nodeInfos,
        std::function<void(Error::Ptr&&)> _callback) override;

#if 0
    // upgrade the node info
    void upgradeNode(std::map<std::string, ChainNodeInfo::Ptr> const& _nodeInfos,
        std::function<void(Error::Ptr&&)> _callback) override;
#endif

protected:
    virtual void generateTarsTask(tars::TaskReq& _tarsTask, std::string const& _command,
        std::map<std::string, ChainNodeInfo::Ptr> const& _nodeInfos,
        std::map<string, string> const& _parameters = std::map<string, string>());

    virtual void addTaskReq(tars::EMTaskCommand const& _command,
        std::map<std::string, ChainNodeInfo::Ptr> const& _nodeInfos,
        std::function<void(Error::Ptr&&)> _callback,
        std::map<std::string, std::string> const& _parameters = std::map<string, string>());

private:
    GroupMgrConfig::Ptr m_config;
};
}  // namespace group
}  // namespace bcos
