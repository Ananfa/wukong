#include "mysql_utils.h"

using namespace wukong;

bool MysqlUtils::LoadOrCreateUser(MYSQL *mysql, const std::string &account, UserId &userId, std::string &roleListStr) {
    MYSQL_STMT *stmt = mysql_stmt_init(mysql);
    if (!stmt) {
        ERROR_LOG("MysqlUtils::LoadOrCreateUser -- init mysql stmt failed\n");
        return false;
    }

    if (mysql_stmt_prepare(stmt, "CALL loadOrCreateUser(?)", 24)) {
        ERROR_LOG("MysqlUtils::LoadOrCreateUser -- prepare mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND ps_params[1];

    ps_params[0].buffer_type = MYSQL_TYPE_STRING;
    ps_params[0].buffer = (void *)account.c_str();
    ps_params[0].buffer_length = account.length();
    ps_params[0].length = 0;
    ps_params[0].is_null = 0;
    
    if (mysql_stmt_bind_param(stmt, ps_params)) {
        ERROR_LOG("MysqlUtils::LoadOrCreateUser -- bind param for mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt)) {
        ERROR_LOG("MysqlUtils::LoadOrCreateUser -- execute mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    userId = 0;
    std::string rolesStr(1023, char(0));
    uint64_t rolesStrLen = 0;
    
    MYSQL_BIND rs_bind[2];
    my_bool    is_null[2];
    memset(rs_bind, 0, sizeof(rs_bind));

    rs_bind[0].buffer_type = MYSQL_TYPE_LONG;
    rs_bind[0].is_null = &is_null[0];
    rs_bind[0].buffer = (void *)&userId;
    rs_bind[0].buffer_length = sizeof(userId);
    rs_bind[0].is_unsigned = true;

    rs_bind[1].buffer_type = MYSQL_TYPE_STRING;
    rs_bind[1].is_null = &is_null[1];
    rs_bind[1].buffer = (void *)rolesStr.c_str();
    rs_bind[1].buffer_length = rolesStr.length();
    rs_bind[1].length = &rolesStrLen;
    rs_bind[1].is_unsigned = true;

    if (mysql_stmt_bind_result(stmt, rs_bind)) {
        ERROR_LOG("DataManager::LoadOrCreateUser -- %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }
    
    if (mysql_stmt_fetch(stmt)) {
        ERROR_LOG("DataManager::LoadOrCreateUser -- %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }
    
    mysql_stmt_close(stmt);

    if (rolesStrLen > 0) {
        roleListStr = std::move(rolesStr);
    } else {
        roleListStr.clear();
    }

    return true;
}

bool MysqlUtils::LoadRoleIds(MYSQL *mysql, UserId userId, std::string &roleListStr) {
    MYSQL_STMT *stmt = mysql_stmt_init(mysql);
    if (!stmt) {
        ERROR_LOG("MysqlUtils::LoadRoleIds -- init mysql stmt failed\n");
        return false;
    }

    if (mysql_stmt_prepare(stmt, "SELECT roles FROM user WHERE userid=?", 37)) {
        ERROR_LOG("MysqlUtils::LoadRoleIds -- prepare mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND ps_params[1];

    ps_params[0].buffer_type = MYSQL_TYPE_LONG;
    ps_params[0].buffer = (void *)&userId;
    ps_params[0].length = 0;
    ps_params[0].is_null = 0;
    ps_params[0].is_unsigned = true;

    if (mysql_stmt_bind_param(stmt, ps_params)) {
        ERROR_LOG("MysqlUtils::LoadRoleIds -- bind param for mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt)) {
        ERROR_LOG("MysqlUtils::LoadRoleIds -- execute mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    std::string rolesStr(1023, char(0));
    uint64_t rolesStrLen = 0;

    MYSQL_BIND rs_bind[1];
    my_bool    is_null[1];
    memset(rs_bind, 0, sizeof(rs_bind));

    rs_bind[0].buffer_type = MYSQL_TYPE_STRING;
    rs_bind[0].is_null = &is_null[0];
    rs_bind[0].buffer = (void *)rolesStr.c_str();
    rs_bind[0].buffer_length = rolesStr.length();
    rs_bind[0].length = &rolesStrLen;
    rs_bind[0].is_unsigned = true;

    if (mysql_stmt_bind_result(stmt, rs_bind)) {
        ERROR_LOG("DataManager::LoadRoleIds -- %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }
    
    if (mysql_stmt_fetch(stmt)) {
        ERROR_LOG("DataManager::LoadRoleIds -- %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }
    
    mysql_stmt_close(stmt);

    if (rolesStrLen > 0) {
        roleListStr = std::move(rolesStr);
    } else {
        roleListStr.clear();
    }

    return true;
}

bool MysqlUtils::UpdateRoleIds(MYSQL *mysql, UserId userId, const std::string &roleListStr, uint32_t roleNum) {
    MYSQL_STMT *stmt = mysql_stmt_init(mysql);
    if (!stmt) {
        ERROR_LOG("MysqlUtils::UpdateRoleIds -- init mysql stmt failed\n");
        return false;
    }

    if (mysql_stmt_prepare(stmt, "UPDATE user SET roles=?, rolenum=? WHERE userid=? AND rolenum=?", 63)) {
        ERROR_LOG("MysqlUtils::UpdateRoleIds -- prepare mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    uint32_t lastNum = roleNum - 1;

    MYSQL_BIND ps_params[4];
    
    ps_params[0].buffer_type = MYSQL_TYPE_STRING;
    ps_params[0].buffer = (void *)roleListStr.c_str();
    ps_params[0].buffer_length = roleListStr.length();
    ps_params[0].length = 0;
    ps_params[0].is_null = 0;
    
    ps_params[1].buffer_type = MYSQL_TYPE_LONG;
    ps_params[1].buffer = (void *)&roleNum;
    ps_params[1].length = 0;
    ps_params[1].is_null = 0;
    ps_params[1].is_unsigned = true;

    ps_params[2].buffer_type = MYSQL_TYPE_LONG;
    ps_params[2].buffer = (void *)&userId;
    ps_params[2].length = 0;
    ps_params[2].is_null = 0;
    ps_params[2].is_unsigned = true;

    ps_params[3].buffer_type = MYSQL_TYPE_LONG;
    ps_params[3].buffer = (void *)&lastNum;
    ps_params[3].length = 0;
    ps_params[3].is_null = 0;
    ps_params[3].is_unsigned = true;

    if (mysql_stmt_bind_param(stmt, ps_params)) {
        ERROR_LOG("MysqlUtils::UpdateRoleIds -- bind param for mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt)) {
        ERROR_LOG("MysqlUtils::UpdateRoleIds -- execute mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    my_ulonglong res = mysql_affected_rows(mysql);
    if (res == (my_ulonglong)-1) {
        ERROR_LOG("MysqlUtils::UpdateRoleIds -- mysql error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    mysql_stmt_close(stmt);

    return true;
}

bool MysqlUtils::LoadRole(MYSQL *mysql, RoleId roleId, UserId &userId, ServerId &serverId, std::string &data) {
    MYSQL_STMT *stmt = mysql_stmt_init(mysql);
    if (!stmt) {
        ERROR_LOG("MysqlUtils::LoadRole -- init mysql stmt failed\n");
        return false;
    }

    if (mysql_stmt_prepare(stmt, "SELECT userid,serverid,data FROM role WHERE roleid=?", 52)) {
        ERROR_LOG("MysqlUtils::LoadRole -- prepare mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND ps_params[1];

    ps_params[0].buffer_type = MYSQL_TYPE_LONG;
    ps_params[0].buffer = (void *)&roleId;
    ps_params[0].length = 0;
    ps_params[0].is_null = 0;
    ps_params[0].is_unsigned = true;

    if (mysql_stmt_bind_param(stmt, ps_params)) {
        ERROR_LOG("MysqlUtils::LoadRole -- bind param for mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt)) {
        ERROR_LOG("MysqlUtils::LoadRole -- execute mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND rs_bind[3];
    my_bool    is_null[3];
    memset(rs_bind, 0, sizeof(rs_bind));

    userId = 0;
    /* set up and bind result set output buffers */
    rs_bind[0].buffer_type = MYSQL_TYPE_LONG;
    rs_bind[0].is_null = &is_null[0];
    rs_bind[0].buffer = (void *)&userId;
    rs_bind[0].buffer_length = sizeof(userId);
    rs_bind[0].is_unsigned = true;

    serverId = 0;
    rs_bind[1].buffer_type = MYSQL_TYPE_LONG;
    rs_bind[1].is_null = &is_null[1];
    rs_bind[1].buffer = (void *)&serverId;
    rs_bind[1].buffer_length = sizeof(serverId);
    rs_bind[1].is_unsigned = true;

    size_t data_length = 0;
    rs_bind[2].buffer_type = MYSQL_TYPE_MEDIUM_BLOB;
    rs_bind[2].is_null = &is_null[2];
    rs_bind[2].buffer = 0;
    rs_bind[2].buffer_length = 0;
    rs_bind[2].length = &data_length;

    if (mysql_stmt_bind_result(stmt, rs_bind)) {
        ERROR_LOG("MysqlUtils::LoadRole -- bind result for mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    int status = mysql_stmt_fetch(stmt);
    if (status && status != MYSQL_NO_DATA && status != MYSQL_DATA_TRUNCATED) {
        ERROR_LOG("MysqlUtils::LoadRole -- fetch mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    if (data_length > 0) {
        data.assign(data_length, 0);
        rs_bind[2].buffer = (uint8_t *)data.data();
        rs_bind[2].buffer_length = data_length;

        if (mysql_stmt_fetch_column(stmt, &rs_bind[2], 2, 0)) {
            ERROR_LOG("MysqlUtils::LoadRole -- fetch column for mysql stmt error: %s (errno: %d)\n",
                      mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
            mysql_stmt_close(stmt);
            return false;
        }
    } else {
        data.clear();
    }

    mysql_stmt_close(stmt);
    return true;
}

bool MysqlUtils::CreateRole(MYSQL *mysql, RoleId &roleId, UserId userId, ServerId serverId, const std::string &data) {
    MYSQL_STMT *stmt = mysql_stmt_init(mysql);
    if (!stmt) {
        ERROR_LOG("MysqlUtils::CreateRole -- init mysql stmt failed\n");
        return false;
    }

    if (mysql_stmt_prepare(stmt, "CALL createRole(?, ?, ?)", 24)) {
        ERROR_LOG("MysqlUtils::CreateRole -- prepare mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND ps_params[3];

    ps_params[0].buffer_type = MYSQL_TYPE_LONG;
    ps_params[0].buffer = (void *)&userId;
    ps_params[0].length = 0;
    ps_params[0].is_null = 0;
    ps_params[0].is_unsigned = true;

    ps_params[1].buffer_type = MYSQL_TYPE_SHORT;
    ps_params[1].buffer = (void *)&serverId;
    ps_params[1].length = 0;
    ps_params[1].is_null = 0;
    ps_params[1].is_unsigned = true;

    ps_params[2].buffer_type = MYSQL_TYPE_MEDIUM_BLOB;
    ps_params[2].buffer = (void *)data.c_str();
    ps_params[2].buffer_length = data.length();
    ps_params[2].length = 0;
    ps_params[2].is_null = 0;

    if (mysql_stmt_bind_param(stmt, ps_params)) {
        ERROR_LOG("MysqlUtils::CreateRole -- bind param for mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt)) {
        ERROR_LOG("MysqlUtils::CreateRole -- execute mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    roleId = 0;
    MYSQL_BIND rs_bind[1];
    my_bool    is_null[1];
    memset(rs_bind, 0, sizeof(rs_bind));

    rs_bind[0].buffer_type = MYSQL_TYPE_LONG;
    rs_bind[0].is_null = &is_null[0];
    rs_bind[0].buffer = (void *)&roleId;
    rs_bind[0].buffer_length = sizeof(roleId);
    rs_bind[0].is_unsigned = true;

    if (mysql_stmt_bind_result(stmt, rs_bind)) {
        ERROR_LOG("DataManager::LoadRoleIds -- %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }
    
    if (mysql_stmt_fetch(stmt)) {
        ERROR_LOG("DataManager::LoadRoleIds -- %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }
    
    mysql_stmt_close(stmt);

    return true;
}

bool MysqlUtils::UpdateRole(MYSQL *mysql, RoleId roleId, const std::string &data) {
    MYSQL_STMT *stmt = mysql_stmt_init(mysql);
    if (!stmt) {
        ERROR_LOG("MysqlUtils::UpdateRole -- init mysql stmt failed\n");
        return false;
    }

    if (mysql_stmt_prepare(stmt, "UPDATE role SET data=?, update_time=? WHERE roleid=?", 52)) {
        ERROR_LOG("MysqlUtils::UpdateRole -- prepare mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND ps_params[3];

    MYSQL_TIME ts;
    struct timeval now;
    struct tm tm_tmp;

    gettimeofday(&now, NULL);
    localtime_r(&now.tv_sec, &tm_tmp);

    ts.time_type = MYSQL_TIMESTAMP_TIME;
    ts.year = tm_tmp.tm_year+1900;
    ts.month = tm_tmp.tm_mon+1;
    ts.day = tm_tmp.tm_mday;
    ts.hour = tm_tmp.tm_hour;
    ts.minute = tm_tmp.tm_min;
    ts.second = tm_tmp.tm_sec;
    ts.second_part = now.tv_usec;

    ps_params[0].buffer_type = MYSQL_TYPE_MEDIUM_BLOB;
    ps_params[0].buffer = (void *)data.c_str();
    ps_params[0].buffer_length = data.length();
    ps_params[0].length = 0;
    ps_params[0].is_null = 0;

    ps_params[1].buffer_type = MYSQL_TYPE_TIMESTAMP;
    ps_params[1].buffer = (void *)&ts;
    ps_params[1].length = 0;
    ps_params[1].is_null = 0;

    ps_params[2].buffer_type = MYSQL_TYPE_LONG;
    ps_params[2].buffer = (void *)&roleId;
    ps_params[2].length = 0;
    ps_params[2].is_null = 0;
    ps_params[2].is_unsigned = true;

    if (mysql_stmt_bind_param(stmt, ps_params)) {
        ERROR_LOG("MysqlUtils::UpdateRole -- bind param for mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt)) {
        ERROR_LOG("MysqlUtils::UpdateRole -- execute mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    my_ulonglong res = mysql_affected_rows(mysql);
    if (res == (my_ulonglong)-1) {
        ERROR_LOG("MysqlUtils::UpdateRole -- mysql error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    mysql_stmt_close(stmt);

    return true;
}

bool MysqlUtils::CheckRole(MYSQL *mysql, RoleId roleId, UserId userId, ServerId serverId, bool &result) {
    MYSQL_STMT *stmt = mysql_stmt_init(mysql);
    if (!stmt) {
        ERROR_LOG("MysqlUtils::CheckRole -- init mysql stmt failed\n");
        return false;
    }

    if (mysql_stmt_prepare(stmt, "SELECT COUNT(*) FROM role WHERE roleid=? AND userid=? AND serverid=?", 68)) {
        ERROR_LOG("MysqlUtils::CheckRole -- prepare mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND ps_params[3];

    ps_params[0].buffer_type = MYSQL_TYPE_LONG;
    ps_params[0].buffer = (void *)&roleId;
    ps_params[0].length = 0;
    ps_params[0].is_null = 0;
    ps_params[0].is_unsigned = true;

    ps_params[1].buffer_type = MYSQL_TYPE_LONG;
    ps_params[1].buffer = (void *)&userId;
    ps_params[1].length = 0;
    ps_params[1].is_null = 0;
    ps_params[1].is_unsigned = true;

    ps_params[2].buffer_type = MYSQL_TYPE_SHORT;
    ps_params[2].buffer = (void *)&serverId;
    ps_params[2].length = 0;
    ps_params[2].is_null = 0;
    ps_params[2].is_unsigned = true;

    if (mysql_stmt_bind_param(stmt, ps_params)) {
        ERROR_LOG("MysqlUtils::CheckRole -- bind param for mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt)) {
        ERROR_LOG("MysqlUtils::CheckRole -- execute mysql stmt error: %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }

    uint32_t count = 0;
    MYSQL_BIND rs_bind[1];
    my_bool    is_null[1];
    memset(rs_bind, 0, sizeof(rs_bind));

    rs_bind[0].buffer_type = MYSQL_TYPE_LONG;
    rs_bind[0].is_null = &is_null[0];
    rs_bind[0].buffer = (void *)&count;
    rs_bind[0].buffer_length = sizeof(count);
    rs_bind[0].is_unsigned = true;

    if (mysql_stmt_bind_result(stmt, rs_bind)) {
        ERROR_LOG("DataManager::CheckRole -- %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }
    
    if (mysql_stmt_fetch(stmt)) {
        ERROR_LOG("DataManager::CheckRole -- %s (errno: %d)\n",
                  mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
        mysql_stmt_close(stmt);
        return false;
    }
    
    mysql_stmt_close(stmt);

    result = count > 0;
    return true;
}