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
 * @brief Storage for the groupInfo
 * @file GroupInfoStorage.cpp
 * @author: yujiechen
 * @date 2021-09-16
 */
#include "GroupInfoStorage.h"
#include "Common.h"
#include <bcos-framework/interfaces/rpc/RPCInterface.h>
#include <bcos-framework/interfaces/storage/Common.h>
#include <bcos-tars-protocol/client/GatewayServiceClient.h>
#include <bcos-tars-protocol/client/RpcServiceClient.h>
#include <boost/lexical_cast.hpp>
#include <chrono>
#include <future>

using namespace bcos;
using namespace bcos::group;
using namespace bcos::protocol;
using namespace bcos::rpc;

// init the storage
void GroupInfoStorage::init()
{
    // init the chain table, try to create the chain table
    initChainTable();
    // load information for all chain
    initChainInfos();
}

void GroupInfoStorage::initChainTable()
{
    GROUP_LOG(INFO) << LOG_DESC("GroupInfoStorage: initChainTable");
    auto initRet = std::make_shared<std::promise<Error::Ptr>>();
    auto future = initRet->get_future();
    // try to create empty chainInfo
    m_remoteStorage->asyncCreateChainTable(nullptr, [initRet](Error::Ptr&& _error) {
        if (_error && _error->errorCode() != bcos::storage::StorageError::TableExists)
        {
            GROUP_LOG(ERROR) << LOG_DESC("initChainTable error for create chain table failed")
                             << LOG_KV("code", _error->errorCode())
                             << LOG_KV("msg", _error->errorMessage());
            initRet->set_value(_error);
            return;
        }
        initRet->set_value(nullptr);
        GROUP_LOG(INFO) << LOG_DESC("GroupInfoStorage: initChainTable success");
    });
    auto error = future.get();
    if (error)
    {
        BOOST_THROW_EXCEPTION(
            GroupManagerInitError() << errinfo_comment(
                "GroupManagerInitError for init the chain table failed, code: " +
                std::to_string(error->errorCode()) + ", message:" + error->errorMessage()));
    }
}

void GroupInfoStorage::initChainInfos()
{
    GROUP_LOG(INFO) << LOG_DESC("GroupInfoStorage: initChainInfos");
    auto initResult = std::make_shared<std::promise<Error::Ptr>>();
    auto future = initResult->get_future();
    // load all chain list from the storage
    m_remoteStorage->asyncGetChainList([this, initResult](Error::Ptr&& _error,
                                           std::shared_ptr<std::vector<std::string>> _chainList) {
        if (_error)
        {
            GROUP_LOG(ERROR) << LOG_DESC("GroupInfoStorage init error when get chain list")
                             << LOG_KV("code", _error->errorCode())
                             << LOG_KV("msg", _error->errorMessage());
            initResult->set_value(_error);
            return;
        }
        GROUP_LOG(INFO) << LOG_DESC(
            "GroupInfoStorage init: load chain list success, load the chain informations then...");
        m_remoteStorage->asyncGetChainInfos(
            *_chainList, [this, initResult](Error::Ptr&& _error,
                             std::map<std::string, ChainInfo::Ptr>&& _chainInfos) {
                if (_error)
                {
                    GROUP_LOG(ERROR) << LOG_DESC("GroupInfoStorage init error when get chain infos")
                                     << LOG_KV("code", _error->errorCode())
                                     << LOG_KV("msg", _error->errorMessage());
                    initResult->set_value(_error);
                    return;
                }
                GROUP_LOG(INFO) << LOG_DESC(
                    "GroupInfoStorage init: load the chain informations success");
                // print the chain information for debug
                for (auto const& it : _chainInfos)
                {
                    GROUP_LOG(INFO) << LOG_DESC("chain info:") << printChainInfo(it.second);
                }
                WriteGuard l(x_chainInfos);
                m_chainInfos = std::move(_chainInfos);
                initResult->set_value(nullptr);
            });
    });
    // init failed
    auto error = future.get();
    if (error)
    {
        BOOST_THROW_EXCEPTION(
            GroupManagerInitError() << errinfo_comment(
                "GroupManagerInitError for get chain informations failed, code: " +
                std::to_string(error->errorCode()) + ", message:" + error->errorMessage()));
    }
    GROUP_LOG(INFO) << LOG_DESC("GroupInfoStorage: initChainInfos success");
}

ChainInfo::Ptr GroupInfoStorage::getChainInfo(std::string const& _chainID) const
{
    ReadGuard l(x_chainInfos);
    if (m_chainInfos.count(_chainID))
    {
        return m_chainInfos.at(_chainID);
    }
    return nullptr;
}

// Note: all the chain informations cached in the memory
std::set<std::string> GroupInfoStorage::getChainList() const
{
    std::set<std::string> chainIDList;
    ReadGuard l(x_chainInfos);
    for (auto const& it : m_chainInfos)
    {
        chainIDList.insert(it.first);
    }
    return chainIDList;
}

GroupInfo::Ptr GroupInfoStorage::getGroupInfoFromCache(
    std::string const& _chainID, std::string const& _groupID)
{
    // find the information from the local cache firstly
    ReadGuard l(x_groupInfos);
    return getGroupInfoFromCacheWithoutLock(_chainID, _groupID);
}

GroupInfo::Ptr GroupInfoStorage::getGroupInfoFromCacheWithoutLock(
    std::string const& _chainID, std::string const& _groupID)
{
    if (m_groupInfos.count(_chainID) && m_groupInfos[_chainID].count(_groupID))
    {
        return m_groupInfos[_chainID][_groupID];
    }
    return nullptr;
}

void GroupInfoStorage::asyncGetGroupInfo(std::string const& _chainID, std::string const& _groupID,
    std::function<void(Error::Ptr&&, GroupInfo::Ptr)> _onGroupInfo)
{
    auto groupInfo = getGroupInfoFromCache(_chainID, _groupID);
    if (groupInfo)
    {
        _onGroupInfo(nullptr, groupInfo);
    }
    // miss the cache, fetch from the remoteStorage
    groupInfo = m_config->groupInfoFactory()->createGroupInfo(_chainID, _groupID);
    // fetch the group metaData
    m_remoteStorage->asyncGetGroupMetaInfo(
        groupInfo, [this, groupInfo, _onGroupInfo](Error::Ptr&& _error, GroupInfo::Ptr _groupInfo) {
            if (_error)
            {
                GROUP_LOG(WARNING)
                    << LOG_DESC("asyncGetGroupInfo error for get meta info failed")
                    << LOG_KV("chain", groupInfo->chainID())
                    << LOG_KV("group", groupInfo->groupID()) << LOG_KV("code", _error->errorCode())
                    << LOG_KV("msg", _error->errorMessage());
                _onGroupInfo(std::move(_error), nullptr);
                return;
            }
            // get groupNodeInfos
            m_remoteStorage->asyncGetGroupNodeInfo(
                _groupInfo, [this, _onGroupInfo, _groupInfo](
                                Error::Ptr&& _error, GroupInfo::Ptr _groupInfoWithNodeList) {
                    if (_error)
                    {
                        GROUP_LOG(WARNING)
                            << LOG_DESC("asyncGetGroupInfo error for get node list info failed")
                            << LOG_KV("chain", _groupInfo->chainID())
                            << LOG_KV("group", _groupInfo->groupID())
                            << LOG_KV("code", _error->errorCode())
                            << LOG_KV("msg", _error->errorMessage());
                        _onGroupInfo(std::move(_error), nullptr);
                        return;
                    }
                    updateGroupCache(_groupInfoWithNodeList);
                    _onGroupInfo(nullptr, _groupInfoWithNodeList);
                });
        });
}

bool GroupInfoStorage::updateGroupCache(GroupInfo::Ptr _groupInfo, bool _enforce)
{
    UpgradableGuard l(x_groupInfos);
    if (!_enforce)
    {
        auto groupInfo =
            getGroupInfoFromCacheWithoutLock(_groupInfo->chainID(), _groupInfo->groupID());
        if (groupInfo)
        {
            return false;
        }
    }
    UpgradeGuard ul(l);
    m_groupInfos[_groupInfo->chainID()][_groupInfo->groupID()] = _groupInfo;
    updateChainGroupList(_groupInfo->chainID(), _groupInfo->groupID());
    return true;
}

void GroupInfoStorage::revertGroupCache(GroupInfo::Ptr _groupInfo)
{
    // remove the groupInfo
    {
        UpgradableGuard l(x_groupInfos);
        if (m_groupInfos.count(_groupInfo->chainID()) &&
            m_groupInfos[_groupInfo->chainID()].count(_groupInfo->groupID()))
        {
            UpgradeGuard ul(l);
            m_groupInfos[_groupInfo->chainID()].erase(_groupInfo->groupID());
        }
    }
    // remove the group from the chainInfos
    {
        auto chainInfo = getChainInfoFromCache(_groupInfo->chainID());
        if (chainInfo)
        {
            chainInfo->removeGroup(_groupInfo->groupID());
        }
    }
}

void GroupInfoStorage::revertGroupNodeCache(
    std::string const& _chainID, std::string const& _groupID, ChainNodeInfo::Ptr _nodeInfo)
{
    auto groupInfo = getGroupInfoFromCache(_chainID, _groupID);
    groupInfo->removeNodeInfo(_nodeInfo);
}

void GroupInfoStorage::updateChainGroupList(
    std::string const& _chainID, std::string const& _groupID)
{
    UpgradableGuard l(x_chainInfos);
    ChainInfo::Ptr chainInfo;
    if (m_chainInfos.count(_chainID))
    {
        chainInfo = m_chainInfos[_chainID];
    }
    else
    {
        // a new chain
        chainInfo = m_config->chainInfoFactory()->createChainInfo(_chainID);
        UpgradeGuard ul(l);
        m_chainInfos[_chainID] = chainInfo;
    }
    chainInfo->appendGroup(_groupID);
}

ChainInfo::Ptr GroupInfoStorage::getChainInfoFromCache(std::string const& _chainID) const
{
    ReadGuard l(x_chainInfos);
    if (m_chainInfos.count(_chainID))
    {
        return m_chainInfos.at(_chainID);
    }
    return nullptr;
}

void GroupInfoStorage::asyncGetNodeInfo(std::string const& _chainID, std::string const& _groupID,
    std::string const& _nodeName, std::function<void(Error::Ptr&&, ChainNodeInfo::Ptr)> _onNodeInfo)
{
    asyncGetGroupInfo(_chainID, _groupID,
        [_onNodeInfo, _chainID, _groupID, _nodeName](
            Error::Ptr&& _error, GroupInfo::Ptr&& _groupInfo) {
            if (_error)
            {
                GROUP_LOG(WARNING) << LOG_DESC("asyncGetNodeInfo error for get group info failed")
                                   << LOG_KV("chain", _chainID) << LOG_KV("group", _groupID)
                                   << LOG_KV("node", _nodeName);
                _onNodeInfo(std::move(_error), nullptr);
                return;
            }
            _onNodeInfo(
                nullptr, std::const_pointer_cast<ChainNodeInfo>(_groupInfo->nodeInfo(_nodeName)));
        });
}

void GroupInfoStorage::asyncInsertGroupInfo(
    GroupInfo::Ptr _groupInfo, std::function<void(Error::Ptr&&)> _onInsertGroup)
{
    _groupInfo->setStatus((int32_t)GroupStatus::Creating);
    asyncGetGroupInfo(_groupInfo->chainID(), _groupInfo->groupID(),
        [this, _groupInfo, _onInsertGroup](Error::Ptr&&, GroupInfo::Ptr&& _groupInfoRet) {
            Error::Ptr error = nullptr;
            std::stringstream errorInfo;
            // the group already exists
            if (_groupInfoRet)
            {
                errorInfo << LOG_DESC("group already exists, existed group info: ")
                          << printGroupInfo(_groupInfoRet);
                error = std::make_shared<Error>(GroupMgrError::CreateGroupFailed, errorInfo.str());
                _onInsertGroup(std::move(error));
                return;
            }
            // the group doesn't exists, update the cache
            auto ret = updateGroupCache(_groupInfo);
            // insert the group info failed
            if (!ret)
            {
                auto groupInfo =
                    getGroupInfoFromCache(_groupInfo->chainID(), _groupInfo->groupID());
                errorInfo << LOG_DESC("group already exists, existed group info: ")
                          << printGroupInfo(groupInfo);
                error = std::make_shared<Error>(GroupMgrError::CreateGroupFailed, errorInfo.str());
                _onInsertGroup(std::move(error));
                return;
            }
            // Note: the chainInfo must be exists when updateGroupCache success
            auto chainInfo = getChainInfoFromCache(_groupInfo->chainID());
            m_remoteStorage->asyncInsertGroupInfo(
                chainInfo, _groupInfo, [this, _groupInfo, _onInsertGroup](Error::Ptr&& _error) {
                    if (_error)
                    {
                        std::stringstream errorInfo;
                        errorInfo << LOG_DESC("asyncInsertGroupInfo into backend storage failed")
                                  << LOG_KV("code", _error->errorCode())
                                  << LOG_KV("msg", _error->errorMessage());
                        GROUP_LOG(ERROR) << errorInfo.str();
                        revertGroupCache(_groupInfo);
                        _onInsertGroup(std::make_shared<Error>(
                            GroupMgrError::CreateGroupFailed, errorInfo.str()));
                        return;
                    }
                    // TODO: asyncNotifyGroupInfo with callback
                    asyncNotifyGroupInfo(_groupInfo, nullptr);
                    _onInsertGroup(nullptr);
                });
        });
}

void GroupInfoStorage::asyncUpdateGroupInfo(
    GroupInfo::Ptr _groupInfo, std::function<void(Error::Ptr&&)> _onUpdate)
{
    asyncGetGroupInfo(_groupInfo->chainID(), _groupInfo->groupID(),
        [this, _groupInfo, _onUpdate](Error::Ptr&& _error, GroupInfo::Ptr&& _groupInfoRet) {
            // the group doesn't exist
            if (!_groupInfoRet)
            {
                GROUP_LOG(ERROR) << LOG_DESC(
                                        "asyncUpdateGroupInfo error for the group doesn't exists")
                                 << LOG_KV("code", _error ? _error->errorCode() : 0)
                                 << LOG_KV("msg", _error ? _error->errorMessage() : "");
                _onUpdate(std::make_shared<Error>(GroupMgrError::GroupNotExists, "GroupNotExists"));
                return;
            }
            // the group exists
            m_remoteStorage->asyncSetGroupMetaData(_groupInfo, [this, _onUpdate, _groupInfo](
                                                                   Error::Ptr&& _error) {
                if (_error)
                {
                    GROUP_LOG(ERROR)
                        << LOG_DESC("asyncUpdateGroupInfo error for update groupMetaData failed")
                        << LOG_KV("code", _error->errorCode())
                        << LOG_KV("msg", _error->errorMessage()) << printGroupInfo(_groupInfo);
                    _onUpdate(std::move(_error));
                    return;
                }
                // set the group metadata success, set the nodeInfos then
                m_remoteStorage->asyncSetGroupNodeInfos(_groupInfo, [this, _groupInfo, _onUpdate](
                                                                        Error::Ptr&& _error) {
                    if (_error)
                    {
                        GROUP_LOG(ERROR)
                            << LOG_DESC("asyncUpdateGroupInfo error for update node infos failed")
                            << LOG_KV("code", _error->errorCode())
                            << LOG_KV("msg", _error->errorMessage());
                        _onUpdate(std::move(_error));
                        return;
                    }
                    // update the group cache
                    updateGroupCache(_groupInfo, true);
                    // TODO: asyncNotifyGroupInfo with callback
                    asyncNotifyGroupInfo(_groupInfo, nullptr);
                    _onUpdate(nullptr);
                });
            });
        });
}

void GroupInfoStorage::asyncInsertNodeInfo(std::string const& _chainID, std::string const& _groupID,
    ChainNodeInfo::Ptr _nodeInfo, std::function<void(Error::Ptr&&)> _onInsertNode)
{
    asyncGetNodeInfo(_chainID, _groupID, _nodeInfo->nodeName(),
        [this, _chainID, _groupID, _nodeInfo, _onInsertNode](
            Error::Ptr&&, ChainNodeInfo::Ptr _nodeInfoRet) {
            // already exist the conflicted node
            Error::Ptr error = nullptr;
            std::stringstream errorInfo;
            if (_nodeInfoRet)
            {
                errorInfo << LOG_DESC("node already exists, existed node info: ")
                          << printNodeInfo(_nodeInfoRet);
                error = std::make_shared<Error>(GroupMgrError::CreateGroupFailed, errorInfo.str());
                _onInsertNode(std::move(error));
                return;
            }
            // try to update the nodeInfo
            auto groupInfo = getGroupInfoFromCache(_chainID, _groupID);
            if (!groupInfo)
            {
                errorInfo << LOG_DESC("The group of the node belongs to doesn't exist");
                error = std::make_shared<Error>(GroupMgrError::CreateGroupFailed, errorInfo.str());
                _onInsertNode(std::move(error));
                return;
            }
            // add the nodeInfo to the cache firstly
            auto ret = groupInfo->appendNodeInfo(_nodeInfo);
            if (!ret)
            {
                errorInfo << LOG_DESC("node already exists");
                error = std::make_shared<Error>(GroupMgrError::CreateGroupFailed, errorInfo.str());
                _onInsertNode(std::move(error));
                return;
            }
            // insert the nodeInfo to the backend
            m_remoteStorage->asyncSetNodeInfo(_chainID, _groupID, _nodeInfo,
                [this, groupInfo, _nodeInfo, _onInsertNode](Error::Ptr&& _error) {
                    Error::Ptr error = nullptr;
                    std::stringstream errorInfo;
                    if (_error)
                    {
                        GROUP_LOG(WARNING)
                            << LOG_DESC("asyncInsertNodeInfo failed")
                            << LOG_KV("code", _error->errorCode())
                            << LOG_KV("msg", _error->errorMessage()) << printNodeInfo(_nodeInfo);
                        errorInfo << LOG_DESC("update the node info to the storage error")
                                  << LOG_KV("code", _error->errorCode())
                                  << LOG_KV("msg", _error->errorMessage());
                        error = std::make_shared<Error>(
                            GroupMgrError::CreateGroupFailed, errorInfo.str());
                        _onInsertNode(std::move(error));
                        revertGroupNodeCache(groupInfo->chainID(), groupInfo->groupID(), _nodeInfo);
                        return;
                    }
                    // TODO: asyncNotifyGroupInfo with callback
                    asyncNotifyGroupInfo(groupInfo, nullptr);
                    _onInsertNode(nullptr);
                });
        });
}

void GroupInfoStorage::asyncUpdateNodeInfo(std::string const& _chainID, std::string const& _groupID,
    ChainNodeInfo::Ptr _nodeInfo, std::function<void(Error::Ptr&&)> _onUpdate)
{
    asyncGetNodeInfo(_chainID, _groupID, _nodeInfo->nodeName(),
        [this, _chainID, _groupID, _nodeInfo, _onUpdate](
            Error::Ptr&& _error, ChainNodeInfo::Ptr _nodeInfoRet) {
            if (!_nodeInfoRet)
            {
                GROUP_LOG(WARNING)
                    << LOG_DESC("asyncUpdateNodeInfo failed for the node doesn't exist")
                    << LOG_KV("code", _error ? _error->errorCode() : 0)
                    << LOG_KV("msg", _error ? _error->errorMessage() : "");
                _onUpdate(std::make_shared<Error>(GroupMgrError::NodeNotExists, "NodeNotExists"));
                return;
            }
            m_remoteStorage->asyncSetNodeInfo(_chainID, _groupID, _nodeInfo,
                [this, _chainID, _groupID, _nodeInfo, _onUpdate](Error::Ptr&& _error) {
                    if (_error)
                    {
                        GROUP_LOG(ERROR)
                            << LOG_DESC(
                                   "asyncUpdateNodeInfo failed for update the node info failed")
                            << LOG_KV("code", _error->errorCode())
                            << LOG_KV("msg", _error->errorMessage()) << printNodeInfo(_nodeInfo);
                        _onUpdate(std::move(_error));
                        return;
                    }
                    // update the cached nodeInfo
                    auto groupInfo = getGroupInfoFromCache(_chainID, _groupID);
                    groupInfo->updateNodeInfo(_nodeInfo);
                    // TODO: asyncNotifyGroupInfo with callback
                    asyncNotifyGroupInfo(groupInfo, nullptr);
                    _onUpdate(nullptr);
                });
        });
}

void GroupInfoStorage::asyncNotifyGroupInfo(
    GroupInfo::Ptr _groupInfo, std::function<void(Error::Ptr&&)> _onNotify)
{
    asyncNotifyGroupInfo<bcostars::RpcServicePrx, bcostars::RpcServiceClient>(
        _groupInfo, c_RPCSerivceObjName, x_rpcServiceInfos, m_rpcServiceInfos);
    asyncNotifyGroupInfo<bcostars::GatewayServicePrx, bcostars::GatewayServiceClient>(
        _groupInfo, c_GatewayServiceObjName, x_gatewayServiceInfos, m_gatewayServiceInfos);
    // TODO: notify failed case
    if (_onNotify)
    {
        _onNotify(nullptr);
    }
}

std::string GroupInfoStorage::endPointToString(
    std::string const& _objName, TC_Endpoint const& _endPoint)
{
    return _objName + "@tcp -h " + _endPoint.getHost() + " -p " +
           boost::lexical_cast<std::string>(_endPoint.getPort());
}
