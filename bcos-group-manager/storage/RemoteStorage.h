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
 * @file RemoteStorage.h
 * @author: yujiechen
 * @date 2021-09-16
 */

#pragma once
#include "Common.h"
#include "bcos-group-manager/config/GroupMgrConfig.h"
#include <bcos-framework/interfaces/storage/StorageInterface.h>
#include <bcos-framework/libutilities/Error.h>
namespace bcos
{
namespace group
{
class RemoteStorage : public std::enable_shared_from_this<RemoteStorage>
{
public:
    using Ptr = std::shared_ptr<RemoteStorage>;
    RemoteStorage(bcos::storage::StorageInterface::Ptr _storage, GroupMgrConfig::Ptr _config)
      : m_storage(_storage), m_config(_config)
    {}

    virtual ~RemoteStorage() {}
    virtual void asyncCreateChainTable(
        ChainInfo::Ptr _chainInfo, std::function<void(Error::Ptr&&)> _callback);
    virtual void asyncSetChainInfo(
        ChainInfo::Ptr _chainInfo, std::function<void(Error::Ptr&&)> _callback);

    virtual void asyncInsertGroupInfo(ChainInfo::Ptr _chainInfo, GroupInfo::Ptr _groupInfo,
        std::function<void(Error::Ptr&&)> _callback);

    // insert/update nodeInfo into the group table
    virtual void asyncSetNodeInfo(std::string const& _chainID, std::string const& _groupID,
        ChainNodeInfo::Ptr _nodeInfo, std::function<void(Error::Ptr&&)> _callback);

    virtual void asyncUpdateGroupMetaData(std::string _chainID, std::string const& _groupID,
        std::string _key, std::string const& _value, std::function<void(Error::Ptr&&)> _callback);

    virtual void asyncGetChainList(
        std::function<void(Error::Ptr&&, std::shared_ptr<std::vector<std::string>>)> _callback);
    virtual void asyncGetChainInfos(std::vector<std::string> const& _chainList,
        std::function<void(Error::Ptr&&, std::map<std::string, ChainInfo::Ptr>&&)> _callback);

    virtual void asyncGetGroupNodeInfo(
        GroupInfo::Ptr _groupInfo, std::function<void(Error::Ptr&&, GroupInfo::Ptr)> _callback);

    virtual void asyncGetGroupMetaInfo(
        GroupInfo::Ptr _groupInfo, std::function<void(Error::Ptr&&, GroupInfo::Ptr)> _callback);

    virtual void asyncGetNodeInfos(std::string const& _chainID, std::string const& _groupID,
        std::vector<std::string> const& _nodeList,
        std::function<void(Error::Ptr&&, std::vector<ChainNodeInfo::Ptr>&&)> _callback);

    // insert the meta data
    virtual void asyncSetGroupMetaData(
        GroupInfo::Ptr _groupInfo, std::function<void(Error::Ptr&&)> _callback);

    // insert/update the group table
    virtual void asyncSetGroupNodeInfos(
        GroupInfo::Ptr _groupInfo, std::function<void(Error::Ptr&&)> _callback);

protected:
    // create the metadata table
    virtual void asyncCreateGroupMetaInfo(
        GroupInfo::Ptr _groupInfo, std::function<void(Error::Ptr&&)> _callback);

    // create group table
    virtual void asyncCreateGroupInfo(
        GroupInfo::Ptr _groupInfo, std::function<void(Error::Ptr&&)> _callback);
    virtual std::string encodeDeployInfo(ChainNodeInfo::ServiceToDeployIpMap const& _deployInfo);
    virtual std::string encodeChainInfo(ChainInfo::Ptr _chainInfo);
    virtual ChainInfo::Ptr decodeChainInfo(
        std::string const& _chainID, bcos::storage::Entry const& _entry);
    virtual std::vector<ChainNodeInfo::Ptr> decodeNodeInfos(
        std::vector<std::optional<bcos::storage::Entry>> const& _entries);
    virtual void decodeGroupMetaInfo(GroupInfo::Ptr _groupInfo,
        std::vector<std::optional<bcos::storage::Entry>> const& _entries);

    virtual void setNodeInfoEntry(bcos::storage::Entry& _entry, ChainNodeInfo::Ptr _nodeInfo);

private:
    bcos::storage::StorageInterface::Ptr m_storage;
    GroupMgrConfig::Ptr m_config;
};
}  // namespace group
}  // namespace bcos