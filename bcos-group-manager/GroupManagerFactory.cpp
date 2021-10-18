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
 * @brief group manager factory
 * @file GroupManagerFactory.cpp
 * @author: yujiechen
 * @date 2021-10-18
 */
#include "GroupManagerFactory.h"
#include "config/GroupMgrConfig.h"
#include "controller/TarsNodeController.h"
#include "storage/GroupInfoStorage.h"
using namespace bcos;
using namespace bcos::group;

GroupManagerImpl::Ptr GroupManagerFactory::build(tars::TC_AutoPtr<AdminRegProxy> _nodeManager,
    ConfigParser::Ptr _parser, GroupInfoFactory::Ptr _groupInfoFactory,
    ChainNodeInfoFactory::Ptr _chainNodeInfoFactory, bcos::storage::StorageInterface::Ptr _storage)
{
    auto chainInfoFactory = std::make_shared<ChainInfoFactory>();
    auto config = std::make_shared<GroupMgrConfig>(_nodeManager, chainInfoFactory,
        _groupInfoFactory, _chainNodeInfoFactory, _parser->userName());

    auto remoteStorage = std::make_shared<RemoteStorage>(_storage, config);

    auto groupInfoStorage = std::make_shared<GroupInfoStorage>(remoteStorage, config);
    groupInfoStorage->setRpcServiceInfo(_parser->rpcInfos());
    groupInfoStorage->setGatewayServiceInfo(_parser->gatewayInfos());

    // init the groupInfoStorage
    groupInfoStorage->init();

    auto nodeController = std::make_shared<TarsNodeController>(config);
    return std::make_shared<GroupManagerImpl>(nodeController, groupInfoStorage);
}