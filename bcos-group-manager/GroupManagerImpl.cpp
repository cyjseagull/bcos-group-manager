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
#include "controller/Common.h"

using namespace bcos;
using namespace bcos::group;

void GroupManagerImpl::generateNodeInfos(
    std::map<std::string, ChainNodeInfo::Ptr>& _nodeInfos, GroupInfo::Ptr _groupInfo)
{
    //_groupInfo
    for (auto const& it : _groupInfo->nodeInfos())
    {
        auto const& nodeInfo = it.second;
        auto serviceName =
            getApplicationName(_groupInfo->chainID(), _groupInfo->groupID(), nodeInfo->nodeName());
        _nodeInfos[serviceName] = nodeInfo;
    }
}

void GroupManagerImpl::asyncCreateGroup(
    GroupInfo::Ptr _groupInfo, std::function<void(Error::Ptr&&)> _callback)
{
    m_storage->asyncInsertGroupInfo(_groupInfo, [this, _callback, _groupInfo](Error::Ptr&& _error) {
        if (_error)
        {
            _callback(std::move(_error));
            return;
        }
        std::map<std::string, ChainNodeInfo::Ptr> nodeInfos;
        generateNodeInfos(nodeInfos, _groupInfo);
        m_nodeController->createNodes(
            nodeInfos, [this, _callback, _groupInfo](Error::Ptr&& _error) {
                // create the group successfully
                if (_error)
                {
                    GROUP_LOG(ERROR)
                        << LOG_DESC("asyncCreateGroup error") << LOG_KV("code", _error->errorCode())
                        << LOG_KV("msg", _error->errorMessage());
                    _callback(std::move(_error));
                    return;
                }
                // update the group status
                _groupInfo->setStatus((int32_t)GroupStatus::Created);
                m_storage->asyncUpdateGroupInfo(_groupInfo, _callback);
            });
    });
}

void GroupManagerImpl::asyncExpandGroupNode(std::string const& _chainID,
    std::string const& _groupID, ChainNodeInfo::Ptr _nodeInfo,
    std::function<void(Error::Ptr&&)> _callback)
{
    _nodeInfo->setStatus((int32_t)GroupStatus::Creating);
    m_storage->asyncGetGroupInfo(_chainID, _groupID,
        [this, _chainID, _groupID, _nodeInfo, _callback](Error::Ptr&&, GroupInfo::Ptr _groupInfo) {
            if (!_groupInfo)
            {
                std::stringstream errorMessage;
                errorMessage << LOG_DESC("asyncExpandGroupNode failed for the group doesn't exist")
                             << LOG_KV("chain", _chainID) << LOG_KV("group", _groupID);
                _callback(std::make_shared<Error>(
                    GroupMgrError::ExpandGroupNodeFailed, errorMessage.str()));
                return;
            }
            // insert the nodeInfo
            m_storage->asyncInsertNodeInfo(_chainID, _groupID, _nodeInfo,
                [this, _chainID, _groupID, _nodeInfo, _callback](Error::Ptr&& _error) {
                    if (_error)
                    {
                        GROUP_LOG(WARNING)
                            << LOG_DESC("asyncExpandGroupNode failed for insert node info failed")
                            << LOG_KV("code", _error->errorCode())
                            << LOG_KV("msg", _error->errorMessage()) << printNodeInfo(_nodeInfo);
                        _callback(std::move(_error));
                        return;
                    }
                    // create the node
                    std::map<std::string, ChainNodeInfo::Ptr> nodeInfos;
                    auto serviceName =
                        getApplicationName(_chainID, _groupID, _nodeInfo->nodeName());
                    nodeInfos[serviceName] = _nodeInfo;
                    m_nodeController->createNodes(nodeInfos, [this, _chainID, _groupID, _nodeInfo,
                                                                 _callback](Error::Ptr&& _error) {
                        if (_error)
                        {
                            GROUP_LOG(WARNING)
                                << LOG_DESC(
                                       "asyncExpandGroupNode failed for create the node failed")
                                << LOG_KV("code", _error->errorCode())
                                << LOG_KV("msg", _error->errorMessage());
                            _callback(std::move(_error));
                            return;
                        }
                        // update the node status
                        _nodeInfo->setStatus((int32_t)GroupStatus::Created);
                        m_storage->asyncUpdateNodeInfo(_chainID, _groupID, _nodeInfo, _callback);
                    });
                });
        });
}

void GroupManagerImpl::asyncRemoveGroup(std::string const& _chainID, std::string const& _groupID,
    std::function<void(Error::Ptr&&)> _callback)
{
    m_storage->asyncGetGroupInfo(_chainID, _groupID,
        [this, _chainID, _groupID, _callback](Error::Ptr&&, GroupInfo::Ptr _groupInfo) {
            if (!_groupInfo)
            {
                GROUP_LOG(WARNING) << LOG_DESC("asyncRemoveGroup failed for the group not exists")
                                   << LOG_KV("chain", _chainID) << LOG_KV("group", _groupID);
                _callback(std::make_shared<Error>(GroupMgrError::GroupNotExists, "GroupNotExists"));
                return;
            }
            // set the group status to Deleted
            _groupInfo->setStatus((int32_t)GroupStatus::Deleted);
            // update the group status
            m_storage->asyncUpdateGroupInfo(_groupInfo, [this, _callback, _groupInfo](
                                                            Error::Ptr&& _error) {
                // TODO: revert the status when remove group failed
                if (_error)
                {
                    GROUP_LOG(WARNING)
                        << LOG_DESC("asyncRemoveGroup failed for update the group status failed")
                        << LOG_KV("code", _error->errorCode())
                        << LOG_KV("msg", _error->errorMessage());
                    _callback(std::move(_error));
                    return;
                }
                // remove all the nodes of the group
                std::map<std::string, ChainNodeInfo::Ptr> nodeInfos;
                generateNodeInfos(nodeInfos, _groupInfo);
                m_nodeController->removeNodes(
                    nodeInfos, [_callback, _groupInfo](Error::Ptr&& _error) {
                        if (_error)
                        {
                            GROUP_LOG(ERROR)
                                << LOG_DESC("asyncRemoveGroup failed for uninstall nodes error")
                                << LOG_KV("code", _error->errorCode())
                                << LOG_KV("msg", _error->errorMessage());
                            _callback(std::move(_error));
                            return;
                        }
                        GROUP_LOG(INFO)
                            << LOG_DESC("asyncRemoveGroup success") << printGroupInfo(_groupInfo);
                        _callback(nullptr);
                    });
            });
        });
}

void GroupManagerImpl::asyncRemoveGroupNode(std::string const& _chainID,
    std::string const& _groupID, std::string const& _nodeName,
    std::function<void(Error::Ptr&&)> _callback)
{
    m_storage->asyncGetNodeInfo(_chainID, _groupID, _nodeName,
        [this, _chainID, _groupID, _nodeName, _callback](
            Error::Ptr&& _error, ChainNodeInfo::Ptr _nodeInfo) {
            if (!_nodeInfo)
            {
                GROUP_LOG(WARNING)
                    << LOG_DESC("asyncRemoveGroupNode error for the node doesn't exist")
                    << LOG_KV("chain", _chainID) << LOG_KV("group", _groupID)
                    << LOG_KV("node", _nodeName) << LOG_KV("code", _error ? _error->errorCode() : 0)
                    << LOG_KV("msg", _error ? _error->errorMessage() : "");
                _callback(std::make_shared<Error>(GroupMgrError::NodeNotExists, "NodeNotExists"));
                return;
            }
            // set the node status to be deleted
            _nodeInfo->setStatus((int32_t)GroupStatus::Deleted);
            // update the node status to the backend
            m_storage->asyncUpdateNodeInfo(_chainID, _groupID, _nodeInfo,
                [this, _callback, _chainID, _groupID, _nodeInfo](Error::Ptr&& _error) {
                    // TODO: revert the status when remove group node failed
                    if (_error)
                    {
                        GROUP_LOG(ERROR) << LOG_DESC("asyncRemoveGroupNode error")
                                         << LOG_KV("code", _error->errorCode())
                                         << LOG_KV("msg", _error->errorMessage())
                                         << LOG_KV("chain", _chainID) << LOG_KV("group", _groupID);
                        _callback(std::move(_error));
                        return;
                    }
                    // un-install the node
                    std::map<std::string, ChainNodeInfo::Ptr> nodeInfos;
                    auto serviceName =
                        getApplicationName(_chainID, _groupID, _nodeInfo->nodeName());
                    nodeInfos[serviceName] = _nodeInfo;
                    m_nodeController->removeNodes(nodeInfos, [_nodeInfo, _callback](
                                                                 Error::Ptr&& _error) {
                        // TODO: revert when uninstall failed
                        if (_error)
                        {
                            GROUP_LOG(WARNING)
                                << LOG_DESC("asyncRemoveGroupNode error for un-install node failed")
                                << LOG_KV("code", _error->errorCode())
                                << LOG_KV("msg", _error->errorMessage())
                                << printNodeInfo(_nodeInfo);
                            _callback(std::move(_error));
                            return;
                        }
                        GROUP_LOG(INFO)
                            << LOG_DESC("asyncRemoveGroupNode success") << printNodeInfo(_nodeInfo);
                        _callback(nullptr);
                    });
                });
        });
}

void GroupManagerImpl::asyncStartNode(std::string const& _chainID, std::string const& _groupID,
    std::string const& _nodeName, std::function<void(Error::Ptr&&)> _callback)
{
    m_storage->asyncGetNodeInfo(_chainID, _groupID, _nodeName,
        [this, _chainID, _groupID, _nodeName, _callback](
            Error::Ptr&& _error, ChainNodeInfo::Ptr _nodeInfo) {
            if (_error || !_nodeInfo)
            {
                GROUP_LOG(WARNING)
                    << LOG_DESC("asyncStartNode failed for the node doesn't exist")
                    << LOG_KV("code", _error->errorCode()) << LOG_KV("msg", _error->errorMessage())
                    << LOG_KV("chain", _chainID) << LOG_KV("group", _groupID)
                    << LOG_KV("node", _nodeName);
                _callback(std::make_shared<Error>(GroupMgrError::NodeNotExists, "NodeNotExists"));
                return;
            }
            if (_nodeInfo->status() != GroupStatus::Created ||
                _nodeInfo->status() != GroupStatus::Stopped)
            {
                GROUP_LOG(WARNING) << LOG_DESC("asyncStartNode failed for invalid status")
                                   << printNodeInfo(_nodeInfo);
                std::stringstream errorMsg;
                errorMsg << LOG_DESC("not allow to start the node when the node status is ")
                         << _nodeInfo->status();
                _callback(
                    std::make_shared<Error>(GroupMgrError::OperationNotAllowed, errorMsg.str()));
                return;
            }
            // set the node status to be starting
            _nodeInfo->setStatus((int32_t)GroupStatus::Starting);
            m_storage->asyncUpdateNodeInfo(_chainID, _groupID, _nodeInfo,
                [this, _chainID, _groupID, _nodeInfo, _callback](Error::Ptr&& _error) {
                    if (_error)
                    {
                        GROUP_LOG(ERROR)
                            << LOG_DESC("asyncStartNode failed for update the node status failed")
                            << LOG_KV("code", _error->errorCode())
                            << LOG_KV("msg", _error->errorMessage());
                        _callback(std::move(_error));
                        return;
                    }
                    // start the node
                    std::map<std::string, ChainNodeInfo::Ptr> nodeInfos;
                    auto serviceName =
                        getApplicationName(_chainID, _groupID, _nodeInfo->nodeName());
                    nodeInfos[serviceName] = _nodeInfo;
                    m_nodeController->startNodes(nodeInfos, [this, _chainID, _groupID, _nodeInfo,
                                                                _callback](Error::Ptr&& _error) {
                        if (_error)
                        {
                            GROUP_LOG(ERROR) << LOG_DESC("asyncStartNode the nodes failed")
                                             << LOG_KV("code", _error->errorCode())
                                             << LOG_KV("msg", _error->errorMessage());
                            _callback(std::move(_error));
                            return;
                        }
                        // set the node status to be started
                        _nodeInfo->setStatus((int32_t)GroupStatus::Started);
                        m_storage->asyncUpdateNodeInfo(_chainID, _groupID, _nodeInfo, _callback);
                    });
                });
        });
}

void GroupManagerImpl::asyncStopNode(std::string const& _chainID, std::string const& _groupID,
    std::string const& _nodeName, std::function<void(Error::Ptr&&)> _callback)
{
    m_storage->asyncGetNodeInfo(_chainID, _groupID, _nodeName,
        [this, _chainID, _groupID, _nodeName, _callback](
            Error::Ptr&& _error, ChainNodeInfo::Ptr _nodeInfo) {
            if (_error || !_nodeInfo)
            {
                GROUP_LOG(WARNING)
                    << LOG_DESC("asyncStopNode failed for the node doesn't exist")
                    << LOG_KV("code", _error->errorCode()) << LOG_KV("msg", _error->errorMessage())
                    << LOG_KV("chain", _chainID) << LOG_KV("group", _groupID)
                    << LOG_KV("node", _nodeName);
                _callback(std::make_shared<Error>(GroupMgrError::NodeNotExists, "NodeNotExists"));
                return;
            }
            if (_nodeInfo->status() != GroupStatus::Started)
            {
                GROUP_LOG(WARNING) << LOG_DESC("asyncStopNode failed for invalid status")
                                   << printNodeInfo(_nodeInfo);
                std::stringstream errorMsg;
                errorMsg << LOG_DESC("not allow to stop the node when the node status is ")
                         << _nodeInfo->status();
                _callback(
                    std::make_shared<Error>(GroupMgrError::OperationNotAllowed, errorMsg.str()));
                return;
            }
            // set the node status to be starting
            _nodeInfo->setStatus((int32_t)GroupStatus::Stopping);
            m_storage->asyncUpdateNodeInfo(_chainID, _groupID, _nodeInfo,
                [this, _chainID, _groupID, _nodeInfo, _callback](Error::Ptr&& _error) {
                    if (_error)
                    {
                        GROUP_LOG(ERROR)
                            << LOG_DESC("asyncStopNode failed for update the node status failed")
                            << LOG_KV("code", _error->errorCode())
                            << LOG_KV("msg", _error->errorMessage());
                        _callback(std::move(_error));
                        return;
                    }
                    // start the node
                    std::map<std::string, ChainNodeInfo::Ptr> nodeInfos;
                    auto serviceName =
                        getApplicationName(_chainID, _groupID, _nodeInfo->nodeName());
                    nodeInfos[serviceName] = _nodeInfo;
                    m_nodeController->stopNodes(nodeInfos, [this, _chainID, _groupID, _nodeInfo,
                                                               _callback](Error::Ptr&& _error) {
                        if (_error)
                        {
                            GROUP_LOG(ERROR) << LOG_DESC("asyncStopNode the nodes failed")
                                             << LOG_KV("code", _error->errorCode())
                                             << LOG_KV("msg", _error->errorMessage());
                            _callback(std::move(_error));
                            return;
                        }
                        // set the node status to be started
                        _nodeInfo->setStatus((int32_t)GroupStatus::Stopped);
                        m_storage->asyncUpdateNodeInfo(_chainID, _groupID, _nodeInfo, _callback);
                    });
                });
        });
}

void GroupManagerImpl::asyncGetChainList(
    std::function<void(Error::Ptr&&, std::set<std::string>&&)> _onGetChainList)
{
    auto chainList = m_storage->getChainList();
    if (_onGetChainList)
    {
        _onGetChainList(nullptr, std::move(chainList));
    }
}

void GroupManagerImpl::asyncGetGroupList(std::string _chainID,
    std::function<void(Error::Ptr&&, std::set<std::string>&&)> _onGetGroupList)
{
    auto chainInfo = m_storage->getChainInfo(_chainID);
    auto groupList = chainInfo->groupList();
    if (_onGetGroupList)
    {
        _onGetGroupList(nullptr, std::move(groupList));
    }
}

void GroupManagerImpl::asyncGetGroupInfo(std::string _chainID, std::string _groupID,
    std::function<void(Error::Ptr&&, GroupInfo::Ptr&&)> _onGetGroupInfo)
{
    m_storage->asyncGetGroupInfo(_chainID, _groupID, _onGetGroupInfo);
}

void GroupManagerImpl::asyncGetNodeInfo(std::string _chainID, std::string _groupID,
    std::string _nodeName, std::function<void(Error::Ptr&&, ChainNodeInfo::Ptr&&)> _onGetNodeInfo)
{
    return m_storage->asyncGetNodeInfo(_chainID, _groupID, _nodeName, _onGetNodeInfo);
}

void GroupManagerImpl::asyncRecoverGroup(std::string const& _chainID, std::string const& _groupID,
    std::function<void(Error::Ptr&&)> _callback)
{
    m_storage->asyncGetGroupInfo(_chainID, _groupID,
        [this, _chainID, _groupID, _callback](Error::Ptr&&, GroupInfo::Ptr _groupInfo) {
            if (!_groupInfo)
            {
                GROUP_LOG(WARNING)
                    << LOG_DESC("asyncRecoverGroup failed for the group doesn't exist")
                    << LOG_KV("chain", _chainID) << LOG_KV("group", _groupID);
                _callback(std::make_shared<Error>(GroupMgrError::GroupNotExists, "GroupNotExists"));
                return;
            }
            // check the group status
            if (_groupInfo->status() != GroupStatus::Deleted)
            {
                GROUP_LOG(WARNING) << LOG_DESC("asyncRecoverGroup: Not allowd to recover the group")
                                   << printGroupInfo(_groupInfo);
                std::stringstream errorMsg;
                errorMsg << "Not allowed recover the group when its status to be "
                         << _groupInfo->status();
                _callback(
                    std::make_shared<Error>(GroupMgrError::OperationNotAllowed, errorMsg.str()));
                return;
            }
            // set the group status to be recovering
            _groupInfo->setStatus((int32_t)GroupStatus::Recovering);
            m_storage->asyncUpdateGroupInfo(_groupInfo, [this, _groupInfo, _callback](
                                                            Error::Ptr&& _error) {
                if (_error)
                {
                    GROUP_LOG(ERROR)
                        << LOG_DESC("asyncRecoverGroup failed for update the group status failed")
                        << LOG_KV("code", _error->errorCode())
                        << LOG_KV("msg", _error->errorMessage());
                    _callback(std::move(_error));
                    return;
                }
                // create the node
                std::map<std::string, ChainNodeInfo::Ptr> nodeInfos;
                generateNodeInfos(nodeInfos, _groupInfo);
                m_nodeController->createNodes(
                    nodeInfos, [this, _callback, _groupInfo](Error::Ptr&& _error) {
                        // TODO: revert the status
                        if (_error)
                        {
                            GROUP_LOG(ERROR) << LOG_DESC("asyncRecoverGroup: createNodes failed")
                                             << LOG_KV("code", _error->errorCode())
                                             << LOG_KV("msg", _error->errorMessage())
                                             << printGroupInfo(_groupInfo);
                            _callback(std::move(_error));
                            return;
                        }
                        // update the group status
                        _groupInfo->setStatus((int32_t)GroupStatus::Created);
                        m_storage->asyncUpdateGroupInfo(_groupInfo, _callback);
                    });
            });
        });
}

void GroupManagerImpl::asyncRecoverGroupNode(std::string const& _chainID,
    std::string const& _groupID, std::string const& _nodeName,
    std::function<void(Error::Ptr&&)> _callback)
{
    m_storage->asyncGetGroupInfo(_chainID, _groupID,
        [this, _chainID, _groupID, _nodeName, _callback](Error::Ptr&&, GroupInfo::Ptr _groupInfo) {
            if (!_groupInfo)
            {
                GROUP_LOG(WARNING)
                    << LOG_DESC("asyncRecoverGroupNode failed for the group doesn't exist")
                    << LOG_KV("chain", _chainID) << LOG_KV("group", _groupID);
                _callback(std::make_shared<Error>(GroupMgrError::GroupNotExists, "GroupNotExists"));
                return;
            }
            // if the group status is Creating/Deleting/Deleted, can't recover the group node
            if (_groupInfo->status() != GroupStatus::Created)
            {
                GROUP_LOG(WARNING)
                    << LOG_DESC("asyncRecoverGroupNode: not allow to recover the group")
                    << printGroupInfo(_groupInfo);
                std::stringstream errorMsg;
                errorMsg << "Not allow to recover the group when the group status is "
                         << _groupInfo->status();
                _callback(
                    std::make_shared<Error>(GroupMgrError::OperationNotAllowed, errorMsg.str()));
                return;
            }
            // check the node status
            m_storage->asyncGetNodeInfo(_chainID, _groupID, _nodeName,
                [this, _chainID, _groupID, _nodeName, _callback](
                    Error::Ptr&&, ChainNodeInfo::Ptr _nodeInfo) {
                    if (!_nodeInfo)
                    {
                        GROUP_LOG(WARNING)
                            << LOG_DESC("asyncRecoverGroupNode failed for the node doesn't exist")
                            << LOG_KV("chain", _chainID) << LOG_KV("group", _groupID)
                            << LOG_KV("node", _nodeName);
                        _callback(
                            std::make_shared<Error>(GroupMgrError::NodeNotExists, "NodeNotExists"));
                        return;
                    }
                    // check the node status
                    if (_nodeInfo->status() != GroupStatus::Deleted)
                    {
                        std::stringstream errorMsg;
                        errorMsg << "Not allow to recover a node with status of "
                                 << _nodeInfo->status();
                        GROUP_LOG(WARNING)
                            << LOG_DESC("asyncRecoverGroupNode failed") << errorMsg.str();
                        _callback(std::make_shared<Error>(
                            GroupMgrError::OperationNotAllowed, errorMsg.str()));
                        return;
                    }
                    recoverNode(_chainID, _groupID, _nodeInfo, _callback);
                });
        });
}

void GroupManagerImpl::recoverNode(std::string const& _chainID, std::string const& _groupID,
    ChainNodeInfo::Ptr _nodeInfo, std::function<void(Error::Ptr&&)> _callback)
{
    // set the node status to recovering
    _nodeInfo->setStatus((int32_t)GroupStatus::Recovering);
    m_storage->asyncUpdateNodeInfo(_chainID, _groupID, _nodeInfo,
        [this, _chainID, _groupID, _nodeInfo, _callback](Error::Ptr&& _error) {
            if (_error)
            {
                GROUP_LOG(ERROR) << LOG_DESC("recoverNodes failed") << printNodeInfo(_nodeInfo)
                                 << LOG_KV("code", _error->errorCode())
                                 << LOG_KV("msg", _error->errorMessage());
                _callback(std::move(_error));
                return;
            }
            // create the node
            std::map<std::string, ChainNodeInfo::Ptr> nodeInfos;
            auto serviceName = getApplicationName(_chainID, _groupID, _nodeInfo->nodeName());
            nodeInfos[serviceName] = _nodeInfo;
            m_nodeController->createNodes(nodeInfos, [this, _chainID, _groupID, _nodeInfo,
                                                         _callback](Error::Ptr&& _error) {
                if (_error)
                {
                    GROUP_LOG(ERROR)
                        << LOG_DESC("recover node failed") << LOG_KV("code", _error->errorCode())
                        << LOG_KV("msg", _error->errorMessage()) << printNodeInfo(_nodeInfo);
                    _callback(std::move(_error));
                    return;
                }
                // update the status
                _nodeInfo->setStatus((int32_t)GroupStatus::Created);
                m_storage->asyncUpdateNodeInfo(_chainID, _groupID, _nodeInfo, _callback);
            });
        });
}