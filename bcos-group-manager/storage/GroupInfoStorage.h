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
 * @file GroupInfoStorage.h
 * @author: yujiechen
 * @date 2021-09-16
 */
#pragma once
#include "RemoteStorage.h"
#include "bcos-group-manager/config/GroupMgrConfig.h"
#include <bcos-framework/interfaces/multigroup/ChainNodeInfo.h>
#include <bcos-framework/interfaces/multigroup/GroupInfo.h>
#include <bcos-framework/libutilities/Error.h>
#include <bcos-tars-protocol/GatewayService.h>
#include <bcos-tars-protocol/RpcService.h>
namespace bcos
{
namespace group
{
template <typename T>
class ServiceInfo
{
public:
    using Ptr = std::shared_ptr<ServiceInfo>;
    ServiceInfo(std::string const& _serviceName, T _serviceProxy)
      : m_serviceName(_serviceName), m_serviceProxy(_serviceProxy)
    {}
    virtual ~ServiceInfo() {}

    std::string const& serviceName() const { return m_serviceName; }
    T serviceProxy() const { return m_serviceProxy; }

private:
    std::string m_serviceName;
    T m_serviceProxy;
};

enum ServiceType : int32_t
{
    RPCSERVICE,
    GATEWAYSERVICE,
};

class GroupInfoStorage
{
public:
    using Ptr = std::shared_ptr<GroupInfoStorage>;
    GroupInfoStorage(RemoteStorage::Ptr _remoteStorage, GroupMgrConfig::Ptr _config)
      : m_remoteStorage(_remoteStorage), m_config(_config)
    {}

    virtual ~GroupInfoStorage() {}

    // init the storage
    virtual void init();
    virtual ChainInfo::Ptr getChainInfo(std::string const& _chainID) const;
    virtual std::set<std::string> getChainList() const;

    virtual void asyncGetGroupInfo(std::string const& _chainID, std::string const& _groupID,
        std::function<void(Error::Ptr&&, GroupInfo::Ptr)> _onGroupInfo);
    virtual void asyncGetNodeInfo(std::string const& _chainID, std::string const& _groupID,
        std::string const& _nodeName,
        std::function<void(Error::Ptr&&, ChainNodeInfo::Ptr)> _onNodeInfo);

    virtual void asyncInsertGroupInfo(
        GroupInfo::Ptr _groupInfo, std::function<void(Error::Ptr&&)> _onInsertGroup);

    virtual void asyncInsertNodeInfo(std::string const& _chainID, std::string const& _groupID,
        ChainNodeInfo::Ptr _nodeInfo, std::function<void(Error::Ptr&&)> _onInsertNode);
#if 0
    // TODO: for services manager
    virtual void asyncRemoveService(ServiceType _type, std::string const& _serviceName,
        std::function<void(Error::Ptr&&)> _onRemove);

    virtual void insertService(ServiceType _type, std::string const& _serviceName,
        std::function<void(Error::Ptr&&)> _onInsert);

    virtual void asyncGetServiceList(ServiceType _type, std::string const& _chainID,
        std::function<void(Error::Ptr&&)> _onServiceList);
#endif

protected:
    // try to create the chain table
    virtual void initChainTable();
    // fetch the chain informations
    virtual void initChainInfos();

    // update the groupCache and chainCache when create new group
    virtual bool updateGroupCache(GroupInfo::Ptr _groupInfo);
    virtual void updateChainGroupList(std::string const& _chainID, std::string const& _groupID);

    virtual void revertGroupCache(GroupInfo::Ptr _groupInfo);

    virtual GroupInfo::Ptr getGroupInfoFromCache(
        std::string const& _chainID, std::string const& _groupID);

    virtual GroupInfo::Ptr getGroupInfoFromCacheWithoutLock(
        std::string const& _chainID, std::string const& _groupID);

    virtual ChainInfo::Ptr getChainInfoFromCache(std::string const& _chainID) const;

private:
    // the mapping between chainID to groupID list, must be load from the storage when init
    std::map<std::string, ChainInfo::Ptr> m_chainInfos;
    mutable SharedMutex x_chainInfos;
    // the mapping between groupID to groupInfo
    std::map<std::string, std::map<std::string, GroupInfo::Ptr>> m_groupInfos;
    mutable SharedMutex x_groupInfos;

    // mappings between chainID to RPCService name
    std::map<std::string, ServiceInfo<bcostars::RpcServiceProxy>::Ptr> m_rpcServiceInfos;
    mutable SharedMutex x_rpcServiceInfos;

    // Note: the new created node must config the gatewayService name
    // mappings between chainDI to gatewayService name
    std::map<std::string, ServiceInfo<bcostars::GatewayServiceProxy>::Ptr> m_gatewayServiceInfos;
    mutable SharedMutex x_gatewayServiceInfos;

    RemoteStorage::Ptr m_remoteStorage;
    GroupMgrConfig::Ptr m_config;
};
}  // namespace group
}  // namespace bcos