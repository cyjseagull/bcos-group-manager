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
 * @brief configParser for the group manager
 * @file ConfigParser.cpp
 * @author: yujiechen
 * @date 2021-10-18
 */
#include "ConfigParser.h"
using namespace bcos;
using namespace bcos::group;

void ConfigParser::loadConfig(std::string const& _configPath)
{
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(_configPath, pt);

    // TODO: check the password
    m_userName = pt.get<std::string>("security.userName", "");

    loadServiceInfo(m_rpcInfos, pt, "RPC");
    loadServiceInfo(m_gatewayInfos, pt, "Gateway");

    m_storagePath = pt.get<std::string>("storage.storage_path", "data/");
}

void ConfigParser::loadServiceInfo(std::map<std::string, std::string>& _serviceInfo,
    boost::property_tree::ptree const& _pt, std::string const& _prefix)
{
    std::string sectionName = "service";
    if (!_pt.get_child_optional(sectionName))
    {
        GROUP_LOG(WARNING) << LOG_DESC("The service information has not been configurated");
        return;
    }
    for (auto const& it : _pt.get_child(sectionName))
    {
        if (!it.first.find(_prefix))
        {
            continue;
        }
        auto chainInfoData = it.first.data();
        std::vector<std::string> chainInfo;
        boost::split(chainInfo, chainInfoData, boost::is_any_of("."));
        if (chainInfo.size() <= 1 || chainInfo[0] != _prefix)
        {
            continue;
        }
        auto chainID = chainInfo[1];
        _serviceInfo[chainID] = it.second.data();
        GROUP_LOG(INFO) << LOG_DESC("loadServiceInfo") << LOG_KV("type", _prefix)
                        << LOG_KV("chainID", chainID) << LOG_KV("serviceName", it.second.data());
    }
}
