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
 * @brief GroupManager implementation
 * @file GroupManagerImpl.h
 * @author: yujiechen
 * @date 2021-09-16
 */
#pragma once
#include "bcos-group-manager/config/GroupMgrConfig.h"
#include "bcos-group-manager/controller/NodeControllerInterface.h"
#include "bcos-group-manager/storage/GroupInfoStorage.h"
#include <bcos-framework/interfaces/multigroup/GroupManagerInterface.h>
namespace bcos
{
namespace group
{
class GroupManagerImpl : public GroupManagerInterface
{
public:
    using Ptr = std::shared_ptr<GroupManagerImpl>;
    GroupManagerImpl(NodeControllerInterface::Ptr _nodeController, GroupInfoStorage::Ptr _storage)
      : m_nodeController(_nodeController), m_storage(_storage)
    {}
    ~GroupManagerImpl() override {}

    void asyncCreateGroup(
        GroupInfo::Ptr _groupInfo, std::function<void(Error::Ptr&&)> _callback) override;

    void asyncExpandGroupNode(std::string const& _chainID, std::string const& _groupID,
        ChainNodeInfo::Ptr _nodeInfo, std::function<void(Error::Ptr&&)> _callback) override;
    void asyncRemoveGroup(std::string const& _chainID, std::string const& _groupID,
        std::function<void(Error::Ptr&&)> _callback) override;
    void asyncRemoveGroupNode(std::string const& _chainID, std::string const& _groupID,
        std::string const& _nodeName, std::function<void(Error::Ptr&&)> _callback) override;
    void asyncRecoverGroup(std::string const& _chainID, std::string const& _groupID,
        std::function<void(Error::Ptr&&)> _callback) override;
    void asyncRecoverGroupNode(std::string const& _chainID, std::string const& _groupID,
        std::string const& _nodeName, std::function<void(Error::Ptr&&)> _callback) override;
    void asyncStartNode(std::string const& _chainID, std::string const& _groupID,
        std::string const& _nodeName, std::function<void(Error::Ptr&&)> _callback) override;
    void asyncStopNode(std::string const& _chainID, std::string const& _groupID,
        std::string const& _nodeName, std::function<void(Error::Ptr&&)> _callback) override;
    void asyncGetChainList(
        std::function<void(Error::Ptr&&, std::set<std::string>&&)> _onGetChainList) override;
    void asyncGetGroupList(std::string _chainID,
        std::function<void(Error::Ptr&&, std::set<std::string>&&)> _onGetGroupList) override;

    void asyncGetGroupInfo(std::string _chainID, std::string _groupID,
        std::function<void(Error::Ptr&&, GroupInfo::Ptr&&)> _onGetGroupInfo) override;
    void asyncGetNodeInfo(std::string _chainID, std::string _groupID, std::string _nodeName,
        std::function<void(Error::Ptr&&, ChainNodeInfo::Ptr&&)> _onGetNodeInfo) override;

protected:
    virtual void generateNodeInfos(
        std::map<std::string, ChainNodeInfo::Ptr>& _nodeInfos, GroupInfo::Ptr _groupInfo);

    virtual void recoverNode(std::string const& _chainID, std::string const& _groupID,
        ChainNodeInfo::Ptr _nodeInfo, std::function<void(Error::Ptr&&)> _callback);

private:
    NodeControllerInterface::Ptr m_nodeController;
    GroupInfoStorage::Ptr m_storage;
};
}  // namespace group
}  // namespace bcos
