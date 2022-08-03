CREATE DATABASE /*!32312 IF NOT EXISTS*/ `record` /*!40100 DEFAULT CHARACTER SET utf8 */;

USE `record`;
# 注意：user和role可进行分片设计，约定每个分片表最多存99999999个数据，需要设置每个分片表的AUTO_INCREMENT初始值
CREATE TABLE `user` (
    `userid` int(11) unsigned NOT NULL AUTO_INCREMENT,
    `account` varchar(40) NOT NULL,
    `roles` varchar(1023) NOT NULL DEFAULT '',
    `rolenum` int(5) NOT NULL DEFAULT 0,
    PRIMARY KEY (`userid`),
    UNIQUE KEY `account` (`account`)
) ENGINE=InnoDB AUTO_INCREMENT=100000000 DEFAULT CHARSET=utf8;

DELIMITER //
CREATE PROCEDURE loadOrCreateUser(IN p_account varchar(40))
BEGIN
    INSERT IGNORE INTO user (account) values (p_account);
    SELECT userid, roles FROM user WHERE account = p_account;
END //
DELIMITER ;

CREATE TABLE `role` (
    `roleid` int(11) unsigned NOT NULL AUTO_INCREMENT,
    `userid` int(11) unsigned NOT NULL,
    `serverid` int(10) unsigned NOT NULL,
    `data` mediumblob,
    `create_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `status` tinyint(1) unsigned NOT NULL DEFAULT 0, 
    PRIMARY KEY (`roleid`)
) ENGINE=InnoDB AUTO_INCREMENT=100000000 DEFAULT CHARSET=utf8;

DELIMITER //
CREATE PROCEDURE createRole(IN p_userid int(11), IN p_serverid int(10), IN p_data mediumblob)
BEGIN
    INSERT INTO role (userid, serverid, data) values (p_userid, p_serverid, p_data);
    SELECT LAST_INSERT_ID();
END //
DELIMITER ;
