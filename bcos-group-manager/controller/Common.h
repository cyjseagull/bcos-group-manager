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
 * @brief common utilities for TarsNodeController
 * @file Common.cpp
 * @author: yujiechen
 * @date 2021-09-27
 */
#pragma once
#include <bcos-framework/libutilities/Error.h>
#include <tarscpp/framework/AdminReg.h>
#include <tarscpp/protocol/BaseF.h>
namespace bcos
{
namespace group
{
inline std::ostream& operator<<(std::ostream& _out, tars::Int32 const& _ret)
{
    switch (_ret)
    {
    case tars::TARSSERVERSUCCESS:
        _out << "tars server success";
        break;
    case tars::TARSSERVERDECODEERR:
        _out << "tars server decode error";
        break;
    case tars::TARSSERVERENCODEERR:
        _out << "tars server encode error";
        break;
    case tars::TARSSERVERNOFUNCERR:
        _out << "tars server no function error ";
        break;
    case tars::TARSSERVERNOSERVANTERR:
        _out << "tars server no servant error";
        break;
    case tars::TARSSERVERRESETGRID:
        _out << "tars server reset grid";
        break;
    case tars::TARSSERVERQUEUETIMEOUT:
        _out << "tars server queueue timeout";
        break;
    case tars::TARSINVOKETIMEOUT:
        _out << "tars invoke timeout";
        break;
    case tars::TARSPROXYCONNECTERR:
        _out << "tars proxy connect error";
        break;
    case tars::TARSSERVEROVERLOAD:
        _out << "tars server overload";
        break;
    case tars::TARSADAPTERNULL:
        _out << "tars adapter null";
        break;
    case tars::TARSINVOKEBYINVALIDESET:
        _out << "tars invoke by invalid set";
        break;
    case tars::TARSCLIENTDECODEERR:
        _out << "tars client decode error";
        break;
    case tars::TARSSENDREQUESTERR:
        _out << "tars send request error";
        break;
    case tars::TARSSERVERUNKNOWNERR:
        _out << "tars server unknown error";
        break;
    default:
        _out << "Unknown";
        break;
    }
    return _out;
}

// get the applicationName
inline std::string getApplicationName(
    std::string const& _chainID, std::string const& _groupID, std::string const& _nodeName)
{
    return (_chainID + _groupID + _nodeName);
}
}  // namespace group
}  // namespace bcos