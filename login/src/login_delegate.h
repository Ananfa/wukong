/*
 * Created by Xianke Liu on 2021/6/7.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef login_delegate_h
#define login_delegate_h

#include <list>
#include <functional>
#include "corpc_redis.h"
#include "corpc_mysql.h"
#include "http_message.h"
#include "share/define.h"

using namespace corpc;

namespace wukong {
    typedef std::function<bool (std::shared_ptr<RequestMessage>&)> LoginCheckHandler;
    typedef std::function<bool (std::shared_ptr<RequestMessage>&, std::list<std::pair<std::string, std::string>>&)> CreateRoleHandler;
    typedef std::function<bool (RedisConnectPool*, MysqlConnectPool*, const std::string&, const std::string&, RoleId roleId, ServerId&, std::list<std::pair<std::string, std::string>>&)> LoadProfileHandler;
    typedef std::function<void (const std::list<std::pair<std::string, std::string>>&, std::list<std::pair<std::string, std::string>>&)> MakeProfileHandler;

    struct LoginDelegate {
        LoginCheckHandler loginCheck;
        CreateRoleHandler createRole;
        LoadProfileHandler loadProfile;
        MakeProfileHandler makeProfile;
    };

}

#endif /* login_delegate_h */