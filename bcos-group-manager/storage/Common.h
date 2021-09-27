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
 * @brief Common files for the group manager
 * @file Common.h
 * @author: yujiechen
 * @date 2021-09-16
 */
#pragma once
#include <bcos-framework/libutilities/Error.h>
#include <bcos-framework/libutilities/Exceptions.h>
#include <memory.h>
#include <iostream>
#include <string>
namespace bcos
{
namespace group
{
DERIVE_BCOS_EXCEPTION(GroupManagerInitError);

enum GroupMgrError
{
    CreateGroupFailed = 4001,
};

const std::string FIELD_SPLITTER = ",";
const std::string CHAIN_TABLE_NAME = "t_chain";
// the chain table fields
const std::string CHAIN_ID = "id";
const std::string CHAIN_STATUS = "status";
// the chain informations, including the group list and the RPC/Gateway service name of the chain
const std::string CHAIN_INFOS = "infos";

// the group meta table fields
const std::string GROUP_META_TABLE_KEY = "key";
const std::string GROUP_META_TABLE_VALUE = "value";
// the group meta table keys
const std::string INI_CONFIG_KEY = "ini";
const std::string GENESIS_CONFIG_KEY = "genesis";
const std::string STATUS_KEY = "status";

// the group table fields
const std::string GROUP_NODE_NAME = "name";
const std::string GROUP_NODE_TYPE = "type";
const std::string GROUP_NODE_DEPLOY_INFO = "deploy_info";
const std::string GROUP_NODE_INI_CONFIG = "config";
// Note TODO: this maybe unsafe, try to make it safer
const std::string GROUP_NODE_PRIVATE_KEY_INFO = "priv";
const std::string GROUP_NODE_STATUS = "status";

inline std::string getGroupTableName(std::string const& _chainID, std::string const& _groupID)
{
    return "t_chain_" + _chainID + "_g_" + _groupID;
}
inline std::string getGroupMetaTableName(std::string const& _chainID, std::string const& _groupID)
{
    return "t_meta_chain_" + _chainID + "_g_" + _groupID;
}
}  // namespace group
}  // namespace bcos