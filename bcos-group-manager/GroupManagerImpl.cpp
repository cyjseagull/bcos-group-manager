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
 * @file GroupManagerImpl.cpp
 * @author: yujiechen
 * @date 2021-09-16
 */
#include "GroupManagerImpl.h"

using namespace bcos;
using namespace bcos::group;

void GroupManagerImpl::asyncCreateGroup(
    GroupInfo::Ptr _groupInfo, std::function<void(Error::Ptr&&)> _callback)
{}

void GroupManagerImpl::asyncExpandGroupNode(std::string const& _chainID,
    std::string const& _groupID, ChainNodeInfo::Ptr _nodeInfo,
    std::function<void(Error::Ptr&&)> _callback)
{}
void GroupManagerImpl::asyncRemoveGroup(std::string const& _chainID, std::string const& _groupID,
    std::function<void(Error::Ptr&&)> _callback)
{}
void GroupManagerImpl::asyncRemoveGroupNode(std::string const& _chainID,
    std::string const& _groupID, std::string const& _nodeName,
    std::function<void(Error::Ptr&&)> _callback)
{}
void GroupManagerImpl::asyncRecoverGroup(std::string const& _chainID, std::string const& _groupID,
    std::function<void(Error::Ptr&&)> _callback)
{}
void GroupManagerImpl::asyncRecoverGroupNode(std::string const& _chainID,
    std::string const& _groupID, std::string const& _nodeName,
    std::function<void(Error::Ptr&&)> _callback)
{}
void GroupManagerImpl::asyncStartNode(std::string const& _chainID, std::string const& _groupID,
    std::string const& _nodeName, std::function<void(Error::Ptr&&)> _callback)
{}
void GroupManagerImpl::asyncStopNode(std::string const& _chainID, std::string const& _groupID,
    std::string const& _nodeName, std::function<void(Error::Ptr&&)> _callback)
{}
void GroupManagerImpl::asyncGetChainList(
    std::function<void(Error::Ptr&&, std::vector<std::string>&&)> _onGetChainList)
{}
void GroupManagerImpl::asyncGetGroupList(std::string _chainID,
    std::function<void(Error::Ptr&&, std::vector<std::string>&&)> _onGetGroupList)
{}
void GroupManagerImpl::asyncGetGroupInfoList(std::string _chainID,
    std::function<void(Error::Ptr&&, std::vector<GroupInfo::Ptr>&&)> _onGetGroupListInfo)
{}

void GroupManagerImpl::asycnGetGroupInfo(std::string _chainID, std::string _groupID,
    std::function<void(Error::Ptr&&, GroupInfo::Ptr&&)> _onGetGroupInfo)
{}
void GroupManagerImpl::asycnGetNodeInfo(std::string _chainID, std::string _groupID,
    std::string _nodeName, std::function<void(Error::Ptr&&, ChainNodeInfo::Ptr&&)> _onGetNodeInfo)
{}