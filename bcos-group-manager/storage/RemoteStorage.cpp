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
 * @file RemoteStorage.cpp
 * @author: yujiechen
 * @date 2021-09-16
 */
#include "RemoteStorage.h"
#include "Common.h"
#include <bcos-framework/interfaces/storage/Table.h>
#include <bcos-framework/libutilities/BatchStorageHelper.h>
#include <boost/archive/basic_archive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>

using namespace bcos;
using namespace bcos::group;
using namespace bcos::storage;

void RemoteStorage::asyncInsertGroupInfo(ChainInfo::Ptr _chainInfo, GroupInfo::Ptr _groupInfo,
    std::function<void(Error::Ptr&&)> _callback)
{
    GROUP_LOG(INFO) << LOG_DESC("asyncInsertGroupInfo") << printGroupInfo(_groupInfo);
    // update the chain info
    asyncSetChainInfo(_chainInfo, [this, _callback, _groupInfo](Error::Ptr&& _error) {
        if (_error)
        {
            GROUP_LOG(ERROR) << LOG_DESC(
                                    "asyncInsertGroupInfo error for update the chainInfo failed")
                             << LOG_KV("code", _error->errorCode())
                             << LOG_KV("msg", _error->errorMessage());
            _callback(std::move(_error));
            return;
        }
        // create the metaData
        asyncCreateGroupMetaInfo(_groupInfo, [this, _groupInfo, _callback](Error::Ptr&& _error) {
            if (_error)
            {
                GROUP_LOG(WARNING)
                    << LOG_DESC("asyncInsertGroupInfo error") << LOG_KV("code", _error->errorCode())
                    << LOG_KV("msg", _error->errorMessage());
                _callback(std::move(_error));
                return;
            }
            // create the groupInfo
            asyncCreateGroupInfo(_groupInfo, _callback);
        });
    });
}

void RemoteStorage::asyncCreateChainTable(
    ChainInfo::Ptr _chainInfo, std::function<void(Error::Ptr&&)> _callback)
{
    GROUP_LOG(INFO) << LOG_DESC("asyncCreateChainTable") << printChainInfo(_chainInfo);

    auto fields = CHAIN_ID + FIELD_SPLITTER + CHAIN_STATUS + FIELD_SPLITTER + CHAIN_INFOS;
    m_storage->asyncCreateTable(CHAIN_TABLE_NAME, fields,
        [this, _chainInfo, _callback](Error::UniquePtr&& _error, std::optional<Table>&&) {
            if (_error)
            {
                GROUP_LOG(WARNING)
                    << LOG_DESC("asyncCreateChainTable error")
                    << LOG_KV("code", _error->errorCode()) << LOG_KV("msg", _error->errorMessage())
                    << printChainInfo(_chainInfo);
                _callback(std::move(_error));
                return;
            }
            GROUP_LOG(INFO) << LOG_DESC("asyncCreateChainTable success")
                            << printChainInfo(_chainInfo);
            // set the chain info
            if (_chainInfo &&
                (_chainInfo->groupList().size() > 0 || _chainInfo->serviceList().size() > 0))
            {
                asyncSetChainInfo(_chainInfo, _callback);
                return;
            }
            _callback(nullptr);
        });
}

void RemoteStorage::asyncSetChainInfo(
    ChainInfo::Ptr _chainInfo, std::function<void(Error::Ptr&&)> _callback)
{
    Entry entry;
    // encode the chain infos
    auto chainInfoStr = encodeChainInfo(_chainInfo);
    entry.importFields({_chainInfo->chainID(),
        boost::lexical_cast<std::string>((int32_t)_chainInfo->status()), std::move(chainInfoStr)});
    // set the chain information
    m_storage->asyncSetRow(CHAIN_TABLE_NAME, _chainInfo->chainID(), std::move(entry),
        [_chainInfo, _callback](Error::UniquePtr&& _error) {
            if (_error)
            {
                GROUP_LOG(WARNING)
                    << LOG_DESC("asyncSetChainInfo error") << LOG_KV("code", _error->errorCode())
                    << LOG_KV("msg", _error->errorMessage()) << printChainInfo(_chainInfo);
                _callback(std::move(_error));
                return;
            }
            GROUP_LOG(INFO) << LOG_DESC("asyncSetChainInfo success") << printChainInfo(_chainInfo);
            _callback(nullptr);
        });
}

std::string RemoteStorage::encodeChainInfo(ChainInfo::Ptr _chainInfo)
{
    // encode the groupList and serviceList
    std::string chainInfoStr;
    boost::iostreams::stream<boost::iostreams::back_insert_device<std::string>> outputStream(
        chainInfoStr);
    boost::archive::binary_oarchive archive(outputStream,
        boost::archive::no_header | boost::archive::no_codecvt | boost::archive::no_tracking);
    archive << _chainInfo->groupList() << _chainInfo->serviceList();
    outputStream.flush();
    return chainInfoStr;
}

void RemoteStorage::asyncCreateGroupMetaInfo(
    GroupInfo::Ptr _groupInfo, std::function<void(Error::Ptr&&)> _callback)
{
    auto tableName = getGroupMetaTableName(_groupInfo->chainID(), _groupInfo->groupID());
    auto metaDataFields = GROUP_META_TABLE_KEY + FIELD_SPLITTER + GROUP_META_TABLE_VALUE;

    auto self = std::weak_ptr<RemoteStorage>(shared_from_this());
    m_storage->asyncCreateTable(tableName, metaDataFields,
        [self, _groupInfo, _callback](Error::UniquePtr&& _error, std::optional<Table>&&) {
            // create the group meta-table failed
            if (_error)
            {
                GROUP_LOG(WARNING)
                    << LOG_DESC("asyncCreateGroupMetaInfo error")
                    << LOG_KV("code", _error->errorCode()) << LOG_KV("msg", _error->errorMessage())
                    << printGroupInfo(_groupInfo);
                _callback(std::move(_error));
                return;
            }
            try
            {
                auto storage = self.lock();
                if (!storage)
                {
                    _callback(std::make_shared<Error>(
                        -1, "asyncInsertMetaInfo failed for get pointer failed"));
                    return;
                }
                storage->asyncSetGroupMetaData(_groupInfo, _callback);
            }
            catch (std::exception const& e)
            {
                GROUP_LOG(WARNING) << LOG_DESC("asyncInsertMetaInfo exception")
                                   << LOG_KV("error", boost::diagnostic_information(e));
                _callback(std::make_shared<Error>(-1, boost::diagnostic_information(e)));
            }
        });
}

void RemoteStorage::asyncUpdateGroupMetaData(std::string _chainID, std::string const& _groupID,
    std::string _key, std::string const& _value, std::function<void(Error::Ptr&& _error)> _callback)
{
    Entry entry;
    entry.importFields({_key, _value});
    auto tableName = getGroupMetaTableName(_chainID, _groupID);
    m_storage->asyncSetRow(tableName, _key, std::move(entry),
        [_chainID, _groupID, _key, _callback](Error::UniquePtr&& _error) {
            if (_error)
            {
                GROUP_LOG(WARNING)
                    << LOG_DESC("asyncUpdateGroupMetaData error")
                    << LOG_KV("code", _error->errorCode()) << LOG_KV("msg", _error->errorMessage())
                    << LOG_KV("chain", _chainID) << LOG_KV("group", _groupID)
                    << LOG_KV("key", _key);
                _callback(std::move(_error));
                return;
            }
            GROUP_LOG(DEBUG) << LOG_DESC("asyncUpdateGroupMetaData success")
                             << LOG_KV("chain", _chainID) << LOG_KV("group", _groupID)
                             << LOG_KV("key", _key);
            _callback(nullptr);
        });
}

void RemoteStorage::asyncSetGroupMetaData(
    GroupInfo::Ptr _groupInfo, std::function<void(Error::Ptr&&)> _callback)
{
    // set metadata
    std::vector<std::tuple<std::string_view, Entry>> entries;
    // set "ini" field
    Entry iniEntry;
    iniEntry.importFields({INI_CONFIG_KEY, _groupInfo->iniConfig()});
    entries.emplace_back(std::make_tuple(INI_CONFIG_KEY, std::move(iniEntry)));
    // set "genesis"
    Entry genesisEntry;
    genesisEntry.importFields({GENESIS_CONFIG_KEY, _groupInfo->genesisConfig()});
    entries.emplace_back(std::make_tuple(GENESIS_CONFIG_KEY, std::move(genesisEntry)));
    // set "status"
    Entry statusEntry;
    statusEntry.importFields(
        {STATUS_KEY, boost::lexical_cast<std::string>((int32_t)_groupInfo->status())});
    entries.emplace_back(std::make_tuple(STATUS_KEY, std::move(statusEntry)));

    auto tableName = getGroupMetaTableName(_groupInfo->chainID(), _groupInfo->groupID());
    asyncBatchSetRows(m_storage, tableName, entries,
        [_callback, _groupInfo](Error::UniquePtr&& _error, std::vector<bool>&&) {
            if (_error)
            {
                GROUP_LOG(WARNING)
                    << LOG_DESC("insertGroupMetaData error") << LOG_KV("code", _error->errorCode())
                    << LOG_KV("msg", _error->errorMessage()) << printGroupInfo(_groupInfo);
                _callback(std::move(_error));
                return;
            }
            GROUP_LOG(INFO) << LOG_DESC("insertGroupMetaData success")
                            << printGroupInfo(_groupInfo);
            _callback(nullptr);
        });
}

void RemoteStorage::asyncCreateGroupInfo(
    GroupInfo::Ptr _groupInfo, std::function<void(Error::Ptr&&)> _callback)
{
    auto tableName = getGroupTableName(_groupInfo->chainID(), _groupInfo->groupID());
    auto groupTableFields = GROUP_NODE_NAME + FIELD_SPLITTER + GROUP_NODE_TYPE + FIELD_SPLITTER +
                            GROUP_NODE_DEPLOY_INFO + FIELD_SPLITTER + GROUP_NODE_NODEID_INFO +
                            FIELD_SPLITTER + GROUP_NODE_INI_CONFIG + FIELD_SPLITTER +
                            GROUP_NODE_STATUS;
    m_storage->asyncCreateTable(tableName, groupTableFields,
        [this, _groupInfo, _callback](Error::UniquePtr&& _error, std::optional<Table>&&) {
            if (_error)
            {
                GROUP_LOG(WARNING)
                    << LOG_DESC("asyncCreateGroupTable error")
                    << LOG_KV("code", _error->errorCode()) << LOG_KV("msg", _error->errorMessage())
                    << printGroupInfo(_groupInfo);
                _callback(std::move(_error));
                return;
            }
            GROUP_LOG(INFO) << LOG_DESC(
                                   "asyncCreateGroupTable success, insert the group node infos now")
                            << printGroupInfo(_groupInfo);
            asyncSetGroupNodeInfos(_groupInfo, _callback);
        });
}

std::string RemoteStorage::encodeDeployInfo(ChainNodeInfo::ServiceToDeployIpMap const& _deployInfo)
{
    std::string value;
    boost::iostreams::stream<boost::iostreams::back_insert_device<std::string>> outputStream(value);
    boost::archive::binary_oarchive archive(outputStream,
        boost::archive::no_header | boost::archive::no_codecvt | boost::archive::no_tracking);
    archive << _deployInfo;
    outputStream.flush();
    return value;
}

void RemoteStorage::asyncSetGroupNodeInfos(
    GroupInfo::Ptr _groupInfo, std::function<void(Error::Ptr&&)> _callback)
{
    std::vector<std::tuple<std::string_view, Entry>> entries;
    auto const& nodeInfos = _groupInfo->nodeInfos();
    for (auto const& it : nodeInfos)
    {
        Entry entry;
        setNodeInfoEntry(entry, it.second);
        entries.emplace_back(it.second->nodeName(), std::move(entry));
    }
    auto tableName = getGroupTableName(_groupInfo->chainID(), _groupInfo->groupID());
    asyncBatchSetRows(m_storage, tableName, entries,
        [_groupInfo, _callback](Error::UniquePtr&& _error, std::vector<bool>&&) {
            if (_error)
            {
                GROUP_LOG(ERROR) << LOG_DESC("asyncSetGroupNodeInfos error")
                                 << LOG_KV("code", _error->errorCode())
                                 << LOG_KV("msg", _error->errorMessage())
                                 << printGroupInfo(_groupInfo);
                _callback(std::move(_error));
                return;
            }
            GROUP_LOG(INFO) << LOG_DESC("asyncSetGroupNodeInfos success")
                            << printGroupInfo(_groupInfo);
            _callback(nullptr);
        });
}

void RemoteStorage::asyncSetNodeInfo(std::string const& _chainID, std::string const& _groupID,
    ChainNodeInfo::Ptr _nodeInfo, std::function<void(Error::Ptr&&)> _callback)
{
    Entry entry;
    setNodeInfoEntry(entry, _nodeInfo);
    auto tableName = getGroupTableName(_groupID, _chainID);
    m_storage->asyncSetRow(tableName, _nodeInfo->nodeName(), std::move(entry),
        [_nodeInfo, _callback](Error::UniquePtr&& _error) {
            if (_error)
            {
                GROUP_LOG(WARNING)
                    << LOG_DESC("asyncSetNodeInfo error") << LOG_KV("code", _error->errorCode())
                    << LOG_KV("msg", _error->errorMessage()) << printNodeInfo(_nodeInfo);
                _callback(std::move(_error));
                return;
            }
            GROUP_LOG(INFO) << LOG_DESC("asyncSetNodeInfo success") << printNodeInfo(_nodeInfo);
            _callback(nullptr);
        });
}

void RemoteStorage::asyncGetChainList(
    std::function<void(Error::Ptr&&, std::shared_ptr<std::vector<std::string>>)> _callback)
{
    std::optional<Condition const> condition{};
    m_storage->asyncGetPrimaryKeys(CHAIN_TABLE_NAME, condition,
        [_callback](Error::UniquePtr&& _error, std::vector<std::string>&& _keys) {
            if (_error)
            {
                GROUP_LOG(WARNING)
                    << LOG_DESC("asyncGetChainList error") << LOG_KV("code", _error->errorCode())
                    << LOG_KV("msg", _error->errorMessage());
                _callback(std::move(_error), nullptr);
                return;
            }
            GROUP_LOG(INFO) << LOG_DESC("asyncGetChainList success")
                            << LOG_KV("chainSize", _keys.size());
            auto chainList = std::make_shared<std::vector<std::string>>();
            *chainList = std::move(_keys);
            _callback(std::move(_error), chainList);
        });
}

void RemoteStorage::asyncGetChainInfos(std::vector<std::string> const& _chainList,
    std::function<void(Error::Ptr&&, std::map<std::string, ChainInfo::Ptr>&&)> _callback)
{
    m_storage->asyncGetRows(CHAIN_TABLE_NAME, _chainList,
        [this, _chainList, _callback](
            Error::UniquePtr&& _error, std::vector<std::optional<Entry>>&& _entries) {
            std::map<std::string, ChainInfo::Ptr> chainInfoMap;
            if (_error)
            {
                GROUP_LOG(WARNING)
                    << LOG_DESC("asyncGetChainInfo error") << LOG_KV("code", _error->errorCode())
                    << LOG_KV("msg", _error->errorMessage())
                    << LOG_KV("chainSize", _chainList.size());
                _callback(std::move(_error), std::move(chainInfoMap));
                return;
            }
            // decode all chainInfos
            for (auto const& entry : _entries)
            {
                if (!entry.has_value())
                {
                    continue;
                }
                auto chainId = std::string(entry.value().getField(CHAIN_ID));
                chainInfoMap[chainId] = decodeChainInfo(chainId, entry.value());
            }
            _callback(nullptr, std::move(chainInfoMap));
        });
}

ChainInfo::Ptr RemoteStorage::decodeChainInfo(std::string const& _chainID, Entry const& _entry)
{
    auto chainInfo = m_config->chainInfoFactory()->createChainInfo(_chainID);
    // chainStatus
    // TODO: catch the exception
    auto status = boost::lexical_cast<int32_t>(_entry.getField(CHAIN_STATUS));
    chainInfo->setStatus((GroupStatus)status);
    // groupList and RPC/Gateway service list
    auto chainInfoData = _entry.getField(CHAIN_INFOS);
    boost::iostreams::stream<boost::iostreams::array_source> inputStream(
        chainInfoData.data(), chainInfoData.size());
    boost::archive::binary_iarchive archive(inputStream,
        boost::archive::no_header | boost::archive::no_codecvt | boost::archive::no_tracking);
    std::set<std::string> groupList;
    std::set<std::string> serviceList;
    archive >> groupList >> serviceList;
    chainInfo->setGroupList(std::move(groupList));
    chainInfo->setServiceList(std::move(serviceList));
    return chainInfo;
}

void RemoteStorage::asyncGetNodeInfos(std::string const& _chainID, std::string const& _groupID,
    std::vector<std::string> const& _nodeList,
    std::function<void(Error::Ptr&&, std::vector<ChainNodeInfo::Ptr>&&)> _callback)
{
    auto tableName = getGroupTableName(_chainID, _groupID);
    m_storage->asyncGetRows(tableName, _nodeList,
        [this, _callback](Error::UniquePtr&& _error, std::vector<std::optional<Entry>>&& _entries) {
            if (_error)
            {
                GROUP_LOG(WARNING)
                    << LOG_DESC("asyncGetNodeInfos error") << LOG_KV("code", _error->errorCode())
                    << LOG_KV("msg", _error->errorMessage());
                _callback(std::move(_error), std::vector<ChainNodeInfo::Ptr>());
                return;
            }
            _callback(nullptr, decodeNodeInfos(_entries));
        });
}

std::vector<ChainNodeInfo::Ptr> RemoteStorage::decodeNodeInfos(
    std::vector<std::optional<Entry>> const& _entries)
{
    std::vector<ChainNodeInfo::Ptr> nodeList;
    for (auto const& entry : _entries)
    {
        if (!entry.has_value())
        {
            continue;
        }
        auto nodeInfo = m_config->nodeInfoFactory()->createNodeInfo();
        // set nodeName
        nodeInfo->setNodeName(std::string(entry->getField(GROUP_NODE_NAME)));
        // set status
        // TODO: catch the exception
        auto status = boost::lexical_cast<int32_t>(entry->getField(GROUP_NODE_STATUS));
        nodeInfo->setStatus(status);
        // set nodeType
        NodeType type = (NodeType)boost::lexical_cast<int32_t>(entry->getField(GROUP_NODE_TYPE));
        nodeInfo->setNodeType(type);
        // set nodeID
        nodeInfo->setNodeID(std::string(entry->getField(GROUP_NODE_NODEID_INFO)));
        // set deployInfo
        auto deployInfoStr = entry->getField(GROUP_NODE_DEPLOY_INFO);
        boost::iostreams::stream<boost::iostreams::array_source> inputStream(
            deployInfoStr.data(), deployInfoStr.size());
        boost::archive::binary_iarchive archive(inputStream,
            boost::archive::no_header | boost::archive::no_codecvt | boost::archive::no_tracking);
        bcos::group::ChainNodeInfo::ServiceToDeployIpMap deployInfo;
        std::set<std::string> serviceList;
        archive >> deployInfo;
        nodeInfo->setDeployInfo(std::move(deployInfo));
        // set iniConfig
        nodeInfo->setIniConfig(std::string(entry->getField(GROUP_NODE_INI_CONFIG)));
        nodeList.emplace_back(nodeInfo);
    }
    return nodeList;
}

void RemoteStorage::asyncGetGroupNodeInfo(
    GroupInfo::Ptr _groupInfo, std::function<void(Error::Ptr&&, GroupInfo::Ptr)> _callback)
{
    auto tableName = getGroupTableName(_groupInfo->chainID(), _groupInfo->groupID());
    const std::optional<Condition const> condition{};
    m_storage->asyncGetPrimaryKeys(tableName, condition,
        [this, _groupInfo, _callback](Error::UniquePtr&& _error, std::vector<std::string>&& _keys) {
            if (_error)
            {
                GROUP_LOG(WARNING)
                    << LOG_DESC("asyncGetGroupNodeInfo error")
                    << LOG_KV("code", _error->errorCode()) << LOG_KV("msg", _error->errorMessage());
                _callback(std::move(_error), nullptr);
                return;
            }
            // get the node name list success, get the nodeInfos
            asyncGetNodeInfos(_groupInfo->chainID(), _groupInfo->groupID(), _keys,
                [_groupInfo, _callback](
                    Error::Ptr&& _error, std::vector<ChainNodeInfo::Ptr>&& _nodeList) {
                    if (_error)
                    {
                        GROUP_LOG(WARNING) << LOG_DESC("asyncGetGroupNodeInfo error")
                                           << LOG_KV("code", _error->errorCode())
                                           << LOG_KV("msg", _error->errorMessage())
                                           << LOG_KV("chain", _groupInfo->chainID())
                                           << LOG_KV("group", _groupInfo->groupID());
                        _callback(std::move(_error), nullptr);
                        return;
                    }
                    // set nodeInfo to the group
                    for (auto const& node : _nodeList)
                    {
                        _groupInfo->appendNodeInfo(node);
                    }
                    _callback(nullptr, _groupInfo);
                });
        });
}

void RemoteStorage::decodeGroupMetaInfo(
    GroupInfo::Ptr _groupInfo, std::vector<std::optional<Entry>> const& _entries)
{
    // for(auto const& entry)
    for (auto const& entry : _entries)
    {
        if (!entry.has_value())
        {
            continue;
        }
        auto const& entryData = entry.value();
        if (entryData.getField(GROUP_META_TABLE_KEY) == INI_CONFIG_KEY)
        {
            _groupInfo->setIniConfig(std::string(entryData.getField(GROUP_META_TABLE_VALUE)));
        }
        else if (entryData.getField(GROUP_META_TABLE_KEY) == GENESIS_CONFIG_KEY)
        {
            _groupInfo->setGenesisConfig(std::string(entryData.getField(GENESIS_CONFIG_KEY)));
        }
        else if (entryData.getField(GROUP_META_TABLE_KEY) == STATUS_KEY)
        {
            auto statusStr = entryData.getField(STATUS_KEY);
            // TODO: catch the exception
            _groupInfo->setStatus(boost::lexical_cast<int32_t>(statusStr));
        }
    }
}

void RemoteStorage::asyncGetGroupMetaInfo(
    GroupInfo::Ptr _groupInfo, std::function<void(Error::Ptr&&, GroupInfo::Ptr)> _callback)
{
    auto tableName = getGroupMetaTableName(_groupInfo->chainID(), _groupInfo->groupID());
    std::vector<std::string> keys{INI_CONFIG_KEY, GENESIS_CONFIG_KEY, STATUS_KEY};
    m_storage->asyncGetRows(tableName, keys,
        [this, _groupInfo, _callback](
            Error::UniquePtr&& _error, std::vector<std::optional<Entry>>&& _entries) {
            if (_error)
            {
                GROUP_LOG(WARNING)
                    << LOG_DESC("asyncGetGroupMetaInfo error")
                    << LOG_KV("code", _error->errorCode()) << LOG_KV("msg", _error->errorMessage());
                _callback(std::move(_error), nullptr);
                return;
            }
            decodeGroupMetaInfo(_groupInfo, _entries);
            _callback(nullptr, _groupInfo);
        });
}

void RemoteStorage::setNodeInfoEntry(bcos::storage::Entry& _entry, ChainNodeInfo::Ptr _nodeInfo)
{
    auto deployInfo = encodeDeployInfo(_nodeInfo->deployInfo());
    _entry.importFields(
        {_nodeInfo->nodeName(), boost::lexical_cast<std::string>((uint32_t)_nodeInfo->nodeType()),
            deployInfo, _nodeInfo->nodeID(), _nodeInfo->iniConfig(),
            boost::lexical_cast<std::string>((int32_t)_nodeInfo->status())});
}