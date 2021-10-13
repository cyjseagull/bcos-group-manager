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
 * @brief interfaces for create/remove/stop/start the blockchain node
 * @file NodeControllerInterface.h
 * @author: yujiechen
 * @date 2021-09-27
 */
#pragma once
#include <bcos-framework/interfaces/multigroup/ChainNodeInfo.h>
#include <memory>
namespace bcos
{
namespace group
{
class NodeControllerInterface
{
public:
    using Ptr = std::shared_ptr<NodeControllerInterface>;
    NodeControllerInterface() = default;
    virtual ~NodeControllerInterface() {}

    virtual void createNodes(std::map<std::string, ChainNodeInfo::Ptr> const& _nodeInfos,
        std::function<void(Error::Ptr&&)> _callback) = 0;
    virtual void removeNodes(std::map<std::string, ChainNodeInfo::Ptr> const& _nodeInfos,
        std::function<void(Error::Ptr&&)> _callback) = 0;

    virtual void startNodes(std::map<std::string, ChainNodeInfo::Ptr> const& _nodeInfos,
        std::function<void(Error::Ptr&&)> _callback) = 0;
    virtual void stopNodes(std::map<std::string, ChainNodeInfo::Ptr> const& _nodeInfos,
        std::function<void(Error::Ptr&&)> _callback) = 0;

#if 0
    // upgrade the node info
    virtual void upgradeNode(std::map<std::string, ChainNodeInfo::Ptr> const& _nodeInfos,
        std::function<void(Error::Ptr&&)> _callback) = 0;
#endif
};
}  // namespace group
}  // namespace bcos