#include "mysql_utils.h"

using namespace wukong;

bool MysqlUtils::LoadRoleData(MYSQL *mysql, RoleId roleId, ServerId &serverId, std::string &data) {
    MYSQL_STMT *stmt = mysql_stmt_init(mysql);
    if (!stmt) {
        ERROR_LOG("MysqlUtils::LoadRole -- init mysql stmt failed\n");
        return false;
    }

    if (mysql_stmt_prepare(stmt, "SELECT serverid,data FROM role WHERE roleid=?", 45)) {
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

    MYSQL_BIND rs_bind[13];
    my_bool    is_null[2];
    memset(rs_bind, 0, sizeof(rs_bind));

    /* set up and bind result set output buffers */
    rs_bind[0].buffer_type = MYSQL_TYPE_LONG;
    rs_bind[0].is_null = &is_null[0];
    rs_bind[0].buffer = (void *)&serverId;
    rs_bind[0].buffer_length = sizeof(serverId);
    rs_bind[0].is_unsigned = true;

    size_t data_length = 0;
    rs_bind[1].buffer_type = MYSQL_TYPE_MEDIUM_BLOB;
    rs_bind[1].is_null = &is_null[1];
    rs_bind[1].buffer = 0;
    rs_bind[1].buffer_length = 0;
    rs_bind[1].length = &data_length;

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
        rs_bind[1].buffer = (uint8_t *)data.data();
        rs_bind[1].buffer_length = data_length;

        if (mysql_stmt_fetch_column(stmt, &rs_bind[1], 1, 0)) {
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