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
 * @file ConfigParser.h
 * @author: yujiechen
 * @date 2021-10-18
 */
#pragma once
#include <bcos-framework/interfaces/multigroup/GroupTypeDef.h>
#include <bcos-framework/libutilities/Common.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <string>
namespace bcos
{
namespace group
{
class ConfigParser
{
public:
    using Ptr = std::shared_ptr<ConfigParser>;
    ConfigParser(std::string const& _configPath) { loadConfig(_configPath); }

    virtual ~ConfigParser() {}

    std::string const& userName() const { return m_userName; }
    std::map<std::string, std::string> const& rpcInfos() const { return m_rpcInfos; }
    std::map<std::string, std::string> const& gatewayInfos() const { return m_gatewayInfos; }
    std::string const& storagePath() const { return m_storagePath; }

protected:
    virtual void loadConfig(std::string const& _configPath);
    virtual void loadServiceInfo(std::map<std::string, std::string>& _serviceInfo,
        boost::property_tree::ptree const& _pt, std::string const& _prefix);

private:
    std::string m_userName;
    // TODO: support update rpcInfos and gatewayInfos dynamic
    std::map<std::string, std::string> m_rpcInfos;
    std::map<std::string, std::string> m_gatewayInfos;
    std::string m_storagePath;
};
}  // namespace group
}  // namespace bcos