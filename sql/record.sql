CREATE DATABASE /*!32312 IF NOT EXISTS*/ `record` /*!40100 DEFAULT CHARACTER SET utf8 */;

USE `record`;

CREATE TABLE `role` (
    `roleid` int(11) unsigned NOT NULL,
    `userid` int(11) unsigned NOT NULL,
    `serverid` int(10) unsigned NOT NULL,
    `data` mediumblob,
    `create_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `status` tinyint(1) unsigned NOT NULL DEFAULT 0, 
    PRIMARY KEY (`roleid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
