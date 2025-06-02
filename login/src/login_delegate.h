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
#if 0
#ifndef wukong_login_delegate_h
#define wukong_login_delegate_h

#include <list>
#include <functional>
#include "corpc_redis.h"
#include "corpc_mysql.h"
#include "http_message.h"
#include "share/define.h"

using namespace corpc;

namespace wukong {
    typedef std::function<bool (std::shared_ptr<RequestMessage>&)> LoginCheckHandle;
    typedef std::function<bool (std::shared_ptr<RequestMessage>&, std::list<std::pair<std::string, std::string>>&)> CreateRoleHandle;
    typedef std::function<bool (RoleId, UserId&, ServerId&, std::list<std::pair<std::string, std::string>>&)> LoadProfileHandle;
    typedef std::function<void (const std::list<std::pair<std::string, std::string>>&, std::list<std::pair<std::string, std::string>>&)> MakeProfileHandle;

    struct LoginDelegate {
        LoginCheckHandle loginCheck;
        CreateRoleHandle createRole;
        LoadProfileHandle loadProfile;
        MakeProfileHandle makeProfile;
    };

}

#endif /* wukong_login_delegate_h */
#endif