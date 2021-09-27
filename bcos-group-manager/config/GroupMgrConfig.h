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
 * @brief configurations for the group manager
 * @file GroupMgrConfig.h
 * @author: yujiechen
 * @date 2021-09-16
 */
#pragma once
#include <bcos-framework/interfaces/multigroup/ChainInfoFactory.h>
#include <bcos-framework/interfaces/multigroup/ChainNodeInfoFactory.h>
#include <bcos-framework/interfaces/multigroup/GroupInfoFactory.h>
#include <bcos-framework/libutilities/Common.h>
#include <bcos-framework/libutilities/Error.h>
#include <tarscpp/framework/AdminReg.h>
namespace bcos
{
namespace group
{
class GroupMgrConfig
{
public:
    using Ptr = std::shared_ptr<GroupMgrConfig>;
    GroupMgrConfig(std::shared_ptr<AdminRegProxy> _nodeManager,
        ChainInfoFactory::Ptr _chainInfoFactory, GroupInfoFactory::Ptr _groupInfoFactory,
        ChainNodeInfoFactory::Ptr _nodeInfoFactory)
      : m_nodeManager(_nodeManager),
        m_chainInfoFactory(_chainInfoFactory),
        m_groupInfoFactory(_groupInfoFactory),
        m_nodeInfoFactory(_nodeInfoFactory)
    {}

    std::shared_ptr<AdminRegProxy> nodeManager() { return m_nodeManager; }

    ChainInfoFactory::Ptr chainInfoFactory() { return m_chainInfoFactory; }
    GroupInfoFactory::Ptr groupInfoFactory() { return m_groupInfoFactory; }
    ChainNodeInfoFactory::Ptr nodeInfoFactory() { return m_nodeInfoFactory; }

private:
    // tars admin registry client, used to deploy/start/stop the node
    std::shared_ptr<tars::AdminRegProxy> m_nodeManager;

    ChainInfoFactory::Ptr m_chainInfoFactory;
    GroupInfoFactory::Ptr m_groupInfoFactory;
    ChainNodeInfoFactory::Ptr m_nodeInfoFactory;
};
}  // namespace group
}  // namespace bcos