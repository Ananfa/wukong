//
//  zk_client.cpp
//  ZkClient
//
//  Created by Shizan Lou on 2018/8/23.
//  Copyright © 2018年 Shizan Lou. All rights reserved.
//
#if 0
#include "corpc_routine_env.h"
#include "zk_client.h"

#include <poll.h>

using namespace wukong;

const char* zoo_error_str(int code) {
    switch(code) {
        case ZOK:
            return "Everything is OK";
        case ZSYSTEMERROR:
            return "System error";
        case ZRUNTIMEINCONSISTENCY:
            return "A runtime inconsistency was found";
        case ZDATAINCONSISTENCY:
            return "A data inconsistency was found";
        case ZCONNECTIONLOSS:
            return "Connection to the server has been lost";
        case ZMARSHALLINGERROR:
            return "Error while marshalling or unmarshalling data";
        case ZUNIMPLEMENTED:
            return "Operation is unimplemented";
        case ZOPERATIONTIMEOUT:
            return "Operation timeout";
        case ZBADARGUMENTS:
            return "Invalid arguments";
        case ZINVALIDSTATE:
            return "Invalid zhandle state";
        case ZAPIERROR:
            return "Api error";
        case ZNONODE:
            return "Node does not exist";
        case ZNOAUTH:
            return "Not authenticated";
        case ZBADVERSION:
            return "Version conflict";
        case ZNOCHILDRENFOREPHEMERALS:
            return "Ephemeral nodes may not have children";
        case ZNODEEXISTS:
            return "The node already exists";
        case ZNOTEMPTY:
            return "The node has children";
        case ZSESSIONEXPIRED:
            return "The session has been expired by the server";
        case ZINVALIDCALLBACK:
            return "Invalid callback specified";
        case ZINVALIDACL:
            return "Invalid ACL specified";
        case ZAUTHFAILED:
            return "Client authentication failed";
        case ZCLOSING:
            return "ZooKeeper is closing";
        case ZNOTHING:
            return "(not error) no server responses to process";
        case ZSESSIONMOVED:
            return "Session moved to another server, so operation is ignored";
        default:
            return "Unknown error";
    }
}

const char* zoo_event_str(int event) {
    if (ZOO_CREATED_EVENT == event)
        return "ZOO_CREATED_EVENT";
    else if (ZOO_DELETED_EVENT == event)
        return "ZOO_DELETED_EVENT";
    else if (ZOO_CHANGED_EVENT == event)
        return "ZOO_CHANGED_EVENT";
    else if (ZOO_CHILD_EVENT == event)
        return "ZOO_CHILD_EVENT";
    else if (ZOO_SESSION_EVENT == event)
        return "ZOO_SESSION_EVENT";
    else if (ZOO_NOTWATCHING_EVENT == event)
        return "ZOO_NOTWATCHING_EVENT";
    else
        return "UNKNOWN_EVENT";
}

const char* zoo_state_str(int state) {
    if (ZOO_EXPIRED_SESSION_STATE == state)
        return "ZOO_EXPIRED_SESSION_STATE";
    else if (ZOO_AUTH_FAILED_STATE == state)
        return "ZOO_AUTH_FAILED_STATE";
    else if (ZOO_CONNECTING_STATE == state)
        return "ZOO_CONNECTING_STATE";
    else if (ZOO_ASSOCIATING_STATE == state)
        return "ZOO_ASSOCIATING_STATE";
    else if (ZOO_CONNECTED_STATE == state)
        return "ZOO_CONNECTED_STATE";
    else
        return "UNKNOWN_STATE";
}

static void *processRoutine(void *arg) {
    co_enable_hook_sys();
    
    int maxfd = 1;
    struct pollfd fds[maxfd];
    
    while (true) {
        int fd, interest;
        int timeout;
        struct timeval tv;
        
        zookeeper_interest(g_ZkClient.getZkHandle(), &fd, &interest, &tv);
        
        if (fd != -1) {
            fds[0].fd = fd;
            fds[0].events = (interest & ZOOKEEPER_READ) ? POLLIN : 0;
            fds[0].events |= (interest & ZOOKEEPER_WRITE) ? POLLOUT : 0;
        }
        timeout = (uint32_t)tv.tv_sec * 1000 + (tv.tv_usec / 1000);
        
        poll(fds, maxfd, timeout);
        
        if (fd != -1) {
            interest = (fds[0].revents & POLLIN) ? ZOOKEEPER_READ : 0;
            interest |= ((fds[0].revents & POLLOUT || fds[0].revents & POLLHUP)) ? ZOOKEEPER_WRITE : 0;
            zookeeper_process(g_ZkClient.getZkHandle(), interest);
        }
    }
    
    return nullptr;
}

ZkClient::ZkClient()
    : host_(""), timeout_(0)
    , zkhandle_(nullptr)
    , reconnecting_(false) {}

ZkClient::~ZkClient() {
    if (zkhandle_) {
        zookeeper_close(zkhandle_);
        zkhandle_ = nullptr;
    }
}

void ZkClient::defaultWatcher(zhandle_t* zh, int type, int state, const char *path, void *watcherCtx) {
    DEBUG_LOG("[%s %d] type: %s, state: %s, path: %s, watcherCtx: %s\n", __FUNCTION__, __LINE__, zoo_event_str(type), zoo_state_str(state), path, (char *)watcherCtx);
    
    if (type == ZOO_SESSION_EVENT) {
        ZkClient *zkClient = static_cast<ZkClient*>(watcherCtx);
        if (state == ZOO_EXPIRED_SESSION_STATE) {
            LOG("[%s %d] zookeeper session expired\n", __FUNCTION__, __LINE__);
            zkClient->reconnect();
        } else if (state == ZOO_CONNECTED_STATE) {
            const clientid_t *clientid = zoo_client_id(zh);
            LOG("[%s %d] connected to zookeeper server with clientid = %lld\n", __FUNCTION__, __LINE__, clientid->client_id);
            
            zkClient->cb_();
        }
    } else if (type == ZOO_CREATED_EVENT) {
        ZkClient *zkClient = static_cast<ZkClient*>(watcherCtx);
        zkClient->watchPool_.get<CreateWatch>(path)->set();
    } else if (type == ZOO_DELETED_EVENT) {
        ZkClient *zkClient = static_cast<ZkClient*>(watcherCtx);
        zkClient->watchPool_.get<DeleteWatch>(path)->set();
    } else if (type == ZOO_CHANGED_EVENT) {
        ZkClient *zkClient = static_cast<ZkClient*>(watcherCtx);
        zkClient->watchPool_.get<DataWatch>(path)->set();
    } else if (type == ZOO_CHILD_EVENT) {
        ZkClient *zkClient = static_cast<ZkClient*>(watcherCtx);
        zkClient->watchPool_.get<ChildrenWatch>(path)->set();
    } else if (type == ZOO_NOTWATCHING_EVENT) {
        
    } else {
        ERROR_LOG("[%s %d] unkown zoo event type\n", __FUNCTION__, __LINE__);
    }
}

ZkRet ZkClient::init(const std::string &host, int timeout, const std::function<void()> &cb) {
    host_ = host;
    timeout_ = timeout;
    cb_ = cb;
    
    setLogLevel(ZOO_LOG_LEVEL_WARN);
    zkhandle_ = zookeeper_init(host_.c_str(), &ZkClient::defaultWatcher, timeout_, 0, this, 0);
    if (zkhandle_ == nullptr) {
        ERROR_LOG("init error when connecting to zookeeper servers...\n");
        return ZkRet(ZSYSTEMERROR);
    }
    
    corpc::RoutineEnvironment::startCoroutine(processRoutine, nullptr);
    return ZkRet(ZOK);
}

ZkRet ZkClient::reconnect() {
    if (zkhandle_) {
        zookeeper_close(zkhandle_);
        zkhandle_ = nullptr;
    }

    zkhandle_ = zookeeper_init(host_.c_str(), &ZkClient::defaultWatcher, timeout_, 0, this, 0);
    if (zkhandle_ == nullptr) {
        ERROR_LOG("reconnect error when connecting to zookeeper servers...\n");
        return ZkRet(ZSYSTEMERROR);
    }
    
    return ZkRet(ZOK);
}

void ZkClient::setLogLevel(ZooLogLevel loglevel) {
    zoo_set_debug_level(loglevel);
}

ZkRet ZkClient::getData(const std::string &path, const DataCallback &cb) {
    DataWatch *watch = new DataWatch(this, path, cb);
    watch->setTemporary();
    int ret = zoo_aget(zkhandle_, path.c_str(), false, &ZkClient::dataCompletion, watch);
    return ZkRet(ret);
}

ZkRet ZkClient::setData(const std::string &path, const std::string &value, const ValidCallback &cb) {
    ValidWatch *watch = new ValidWatch(this, path, cb);
    watch->setTemporary();
    int ret = zoo_aset(zkhandle_, path.c_str(), value.c_str(), (int)value.length(), -1, &ZkClient::statCompletion, watch);
    return ZkRet(ret);
}

ZkRet ZkClient::getChildren(const std::string &path, const ChildrenCallback &cb) {
    ChildrenWatch *watch = new ChildrenWatch(this, path, cb);
    watch->setTemporary();
    int ret = zoo_awget_children(zkhandle_, path.c_str(), nullptr, nullptr, &ZkClient::stringsCompletion, watch);
    return ZkRet(ret);
}

ZkRet ZkClient::exists(const std::string &path, const ValidCallback &cb) {
    ValidWatch *watch = new ValidWatch(this, path, cb);
    watch->setTemporary();
    int ret = zoo_aexists(zkhandle_, path.c_str(), false, &ZkClient::statCompletion, watch);
    return ZkRet(ret);
}

ZkRet ZkClient::createNode(const std::string &path, const std::string &value, const ValidCallback &cb) {
    return createNode(0, path, value, cb);
}

ZkRet ZkClient::createEphemeralNode(const std::string &path, const std::string &value, const ValidCallback &cb) {
    return createNode(ZOO_EPHEMERAL, path, value, cb);
}

ZkRet ZkClient::createSequenceNode(const std::string &path, const std::string &value, const ValidCallback &cb) {
    return createNode(ZOO_SEQUENCE, path, value, cb);
}

ZkRet ZkClient::createSequenceEphemeralNode(const std::string &path, const std::string &value, const ValidCallback &cb) {
    return createNode(ZOO_SEQUENCE|ZOO_EPHEMERAL, path, value, cb);
}

ZkRet ZkClient::deleteNode(const std::string &path, const ValidCallback &cb) {
    ValidWatch *watch = new ValidWatch(this, path, cb);
    watch->setTemporary();
    int ret = zoo_adelete(zkhandle_, path.c_str(), -1, &ZkClient::voidCompletion, watch);
    return ZkRet(ret);
}

void ZkClient::watchData(const std::string &path, const DataCallback &cb) {
    std::shared_ptr<Watch> wp = watchPool_.add<DataWatch>(this, path, cb);
    wp->set();
}

void ZkClient::watchDelete(const std::string &path, const ValidCallback &cb) {
    std::shared_ptr<Watch> wp = watchPool_.add<DeleteWatch>(this, path, cb);
    wp->set();
}

void ZkClient::watchCreate(const std::string &path, const ValidCallback &cb) {
    std::shared_ptr<Watch> wp = watchPool_.add<CreateWatch>(this, path, cb);
    wp->set();
}

void ZkClient::watchChildren(const std::string &path, const ChildrenCallback &cb) {
    std::shared_ptr<Watch> wp = watchPool_.add<ChildrenWatch>(this, path, cb);
    wp->set();
}

ZkRet ZkClient::createNode(int flags, const std::string &path, const std::string &value, const ValidCallback &cb) {
    ValidWatch *watch = new ValidWatch(this, path, cb);
    watch->setTemporary();
    int ret = zoo_acreate(zkhandle_, path.c_str(), value.c_str(), (int)value.length(), &ZOO_OPEN_ACL_UNSAFE, flags, &ZkClient::stringCompletion, watch);
    return ZkRet(ret);
}

void ZkClient::dataCompletion(int rc, const char *value, int valueLen, const struct Stat *stat, const void *data) {
    const DataWatch *watch = dynamic_cast<const DataWatch*>(static_cast<const Watch*>(data));
    
    if (rc == ZOK) {
        watch->doCb(std::string(value, valueLen));
    } else {
        // ZCONNECTIONLOSS
        // ZOPERATIONTIMEOUT
        // ZNONODE
        // ZNOAUTH
        ERROR_LOG("data completion error, ret = %s, path = %s\n", zoo_error_str(rc), watch->path().c_str());
    }
    
    if (watch->isTemporary()) {
        delete watch;
    }
}

void ZkClient::voidCompletion(int rc, const void *data) {
    const ValidWatch *watch = dynamic_cast<const ValidWatch*>(static_cast<const Watch*>(data));
    ZkRet ret = ZkRet(rc);
    watch->doCb(ret);
    
    if (watch->isTemporary()) {
        delete watch;
    }
}

void ZkClient::statCompletion(int rc, const struct Stat *stat, const void *data) {
    const ValidWatch *watch = dynamic_cast<const ValidWatch*>(static_cast<const Watch*>(data));
    ZkRet ret = ZkRet(rc);
    watch->doCb(ret);
    
    if (watch->isTemporary()) {
        delete watch;
    }
}

void ZkClient::stringCompletion(int rc, const char *value, const void *data) {
    const ValidWatch *watch = dynamic_cast<const ValidWatch*>(static_cast<const Watch*>(data));
    ZkRet ret = ZkRet(rc);
    watch->doCb(ret);
    
    if (!ret) {
        ERROR_LOG("string completion error, ret = %s, path = %s\n", zoo_error_str(rc), watch->path().c_str());
    }
    
    if (watch->isTemporary()) {
        delete watch;
    }
}

void ZkClient::stringsCompletion(int rc, const struct String_vector *strings, const void *data) {
    const ChildrenWatch *watch = dynamic_cast<const ChildrenWatch*>(static_cast<const Watch*>(data));
    
    if(rc == ZOK) {
        std::vector<std::string> vec;
        for(int i = 0; i < strings->count; ++i) {
            vec.push_back(strings->data[i]);
        }
        watch->doCb(vec);
    } else {
        // ZCONNECTIONLOSS
        // ZOPERATIONTIMEOUT
        // ZNONODE
        // ZNOAUTH
        ERROR_LOG("strings completion error, ret = %s, path = %s\n", zoo_error_str(rc), watch->path().c_str());
    }
    
    if (watch->isTemporary()) {
        delete watch;
    }
}

ZkClient::Watch::Watch(ZkClient *zkClient, const std::string &path)
    : zkClient_(zkClient)
    , path_(path)
    , temporary_(false) {}

ZkClient::ValidWatch::ValidWatch(ZkClient *zkClient, const std::string &path, const ValidCallback &cb)
    : Watch (zkClient, path)
    , cb_(cb) {}

void ZkClient::ValidWatch::set() const {
    int ret = zoo_aexists(zkClient_->zkhandle_, path_.c_str(), false, &ZkClient::statCompletion, this);
    if (ret != ZOK) {
        // ZBADARGUMENTS
        // ZINVALIDSTATE
        // ZMARSHALLINGERROR
        ERROR_LOG("ValidWatch aexists failed, ret = %s, path = %s\n", zoo_error_str(ret), path_.c_str());
    }
}

ZkClient::DeleteWatch::DeleteWatch(ZkClient *zkClient, const std::string &path, const ValidCallback &cb)
    : ValidWatch (zkClient, path, cb) {}

void ZkClient::DeleteWatch::set() const {
    int ret = zoo_aexists(zkClient_->zkhandle_, path_.c_str(), true, &ZkClient::statCompletion, this);
    if (ret != ZOK) {
        // ZBADARGUMENTS
        // ZINVALIDSTATE
        // ZMARSHALLINGERROR
        ERROR_LOG("DeleteWatch aexists failed, ret = %s, path = %s\n", zoo_error_str(ret), path_.c_str());
    }
}

ZkClient::CreateWatch::CreateWatch(ZkClient *zkClient, const std::string &path, const ValidCallback &cb)
    : ValidWatch (zkClient, path, cb) {}

void ZkClient::CreateWatch::set() const {
    int ret = zoo_aexists(zkClient_->zkhandle_, path_.c_str(), true, &ZkClient::statCompletion, this);
    if (ret != ZOK) {
        // ZBADARGUMENTS
        // ZINVALIDSTATE
        // ZMARSHALLINGERROR
        ERROR_LOG("CreateWatch aexists failed, ret = %s, path = %s\n", zoo_error_str(ret), path_.c_str());
    }
}

ZkClient::DataWatch::DataWatch(ZkClient *zkClient, const std::string &path, const DataCallback &cb)
    : Watch (zkClient, path)
    , cb_(cb) {}

void ZkClient::DataWatch::set() const {
    int ret = zoo_aget(zkClient_->zkhandle_, path_.c_str(), true, &ZkClient::dataCompletion, this);
    if (ret != ZOK) {
        // ZBADARGUMENTS
        // ZINVALIDSTATE
        // ZMARSHALLINGERROR
        ERROR_LOG("DataWatch aget failed, ret = %s, path = %s\n", zoo_error_str(ret), path_.c_str());
    }
}

ZkClient::ChildrenWatch::ChildrenWatch(ZkClient *zkClient, const std::string &path, const ChildrenCallback &cb)
    : Watch (zkClient, path)
    , cb_(cb) {}

void ZkClient::ChildrenWatch::set() const {
    int ret = zoo_aget_children(zkClient_->zkhandle_, path_.c_str(), true, &ZkClient::stringsCompletion, this);
    if (ret != ZOK) {
        // ZBADARGUMENTS
        // ZINVALIDSTATE
        // ZMARSHALLINGERROR
        ERROR_LOG("ChildrenWatch aget_children failed, ret = %s, path = %s\n", zoo_error_str(ret), path_.c_str());
    }
}
#endif