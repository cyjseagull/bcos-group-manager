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
 * @file GroupManagerFactory.h
 * @author: yujiechen
 * @date 2021-10-18
 */
#pragma once
#include "GroupManagerImpl.h"
#include "config/ConfigParser.h"
#include <bcos-framework/interfaces/multigroup/ChainInfoFactory.h>
#include <bcos-framework/interfaces/multigroup/ChainNodeInfoFactory.h>
#include <bcos-framework/interfaces/multigroup/GroupInfoFactory.h>
#include <bcos-framework/interfaces/storage/StorageInterface.h>
#include <tarscpp/framework/AdminReg.h>

namespace bcos
{
namespace group
{
class GroupManagerFactory
{
public:
    using Ptr = std::shared_ptr<GroupManagerFactory>;
    GroupManagerFactory() = default;
    virtual ~GroupManagerFactory() {}
    GroupManagerImpl::Ptr build(tars::TC_AutoPtr<AdminRegProxy> _nodeManager,
        ConfigParser::Ptr _parser, GroupInfoFactory::Ptr _groupInfoFactory,
        ChainNodeInfoFactory::Ptr _chainNodeInfoFactory,
        bcos::storage::StorageInterface::Ptr _storage);
};
}  // namespace group
}  // namespace bcos