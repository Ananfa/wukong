//
//  zk_client.h
//  ZkClient
//
//  Created by Shizan Lou on 2018/8/23.
//  Copyright © 2018年 Shizan Lou. All rights reserved.
//

#ifndef zk_client_h
#define zk_client_h

#include <string>
#include <functional>
#include <vector>
#include <map>
#include <zookeeper.h>

namespace wukong {
    class ZkRet {
        friend class ZkClient;
    public:
        bool ok() const { return _code == ZOK; }
        bool code() const { return _code; }
        operator bool() const { return ok(); }
    protected:
        ZkRet() { _code = ZOK; }
        ZkRet(int c) { _code = c; }
    private:
        int _code;
    };

    typedef std::function<void (const std::string &path, const ZkRet &value)> ValidCallback;
    typedef std::function<void (const std::string &path, const std::string &value)> DataCallback;
    typedef std::function<void (const std::string &path, const std::vector<std::string> &values)> ChildrenCallback;

    class ZkClient {
    public:
        static ZkClient& Instance() {
            static ZkClient instance;
            return instance;
        }

        ZkRet init(const std::string &host, int timeout, const std::function<void()> &cb);
        void setLogLevel(ZooLogLevel);
        zhandle_t* getZkHandle() { return _zkhandle; };
        
        ZkRet getData(const std::string &path, const DataCallback &cb);
        ZkRet setData(const std::string &path, const std::string &value, const ValidCallback &cb);
        ZkRet getChildren(const std::string &path, const ChildrenCallback &cb);
        ZkRet exists(const std::string &path, const ValidCallback &cb);
        ZkRet createNode(const std::string &path, const std::string &value, const ValidCallback &cb);
        // ephemeral node is a special node, its has the same lifetime as the session
        ZkRet createEphemeralNode(const std::string &path, const std::string &value, const ValidCallback &cb);
        // sequence node, the created node's name is not equal to the given path, it is like "path-xx", xx is an auto-increment number
        ZkRet createSequenceNode(const std::string &path, const std::string &value, const ValidCallback &cb);
        ZkRet createSequenceEphemeralNode(const std::string &path, const std::string &value, const ValidCallback &cb);
        ZkRet deleteNode(const std::string &path, const ValidCallback &cb);
        void watchData(const std::string &path, const DataCallback &cb);
        void watchDelete(const std::string &path, const ValidCallback &cb);
        void watchCreate(const std::string &path, const ValidCallback &cb);
        void watchChildren(const std::string &path, const ChildrenCallback &cb);
        
    private:
        ZkRet reconnect();
        ZkRet createNode(int flags, const std::string &path, const std::string &value, const ValidCallback &cb);
        
        static void dataCompletion(int rc, const char *value, int valueLen, const struct Stat *stat, const void *data);
        static void voidCompletion(int rc, const void *data);
        static void statCompletion(int rc, const struct Stat *stat, const void *data);
        static void stringCompletion(int rc, const char *name, const void *data);
        static void stringsCompletion(int rc, const struct String_vector *strings, const void *data);
        static void defaultWatcher(zhandle_t *zh, int type, int state, const char *path,void *watcherCtx);
        
        class Watch {
        public:
            Watch(ZkClient *zkClient, const std::string &path);
            virtual void set() const = 0;
            
            const std::string &path() const { return _path; }
            ZkClient* zkClient() const { return _zkClient; }
            bool isTemporary() const { return _temporary; }
            void setTemporary() { _temporary = true; }
        protected:
            ZkClient *_zkClient;
            std::string _path;
            bool _temporary;
        };
        
        class ValidWatch: public Watch {
        public:
            typedef ValidCallback Callback;
            ValidWatch(ZkClient *zkClient, const std::string &path, const Callback &cb);
            virtual void set() const;
            void doCb(const ZkRet &value) const{ _cb(_path, value); };
        private:
            Callback _cb;
        };
        
        class DeleteWatch: public ValidWatch {
        public:
            DeleteWatch(ZkClient *zkClient, const std::string &path, const Callback &cb);
            virtual void set() const;
        };
        
        class CreateWatch: public ValidWatch {
        public:
            CreateWatch(ZkClient *zkClient, const std::string &path, const Callback &cb);
            virtual void set() const;
        };
        
        class DataWatch: public Watch {
        public:
            typedef DataCallback Callback;
            DataWatch(ZkClient *zkClient, const std::string &path, const Callback &cb);
            virtual void set() const;
            void doCb(const std::string &value) const{ _cb(_path, value); };
        private:
            Callback _cb;
        };
        
        class ChildrenWatch: public Watch {
        public:
            typedef ChildrenCallback Callback;
            ChildrenWatch(ZkClient *zkClient, const std::string &path, const Callback &cb);
            virtual void set() const;
            void doCb(const std::vector<std::string> &value) const { _cb(_path, value); };
        private:
            Callback _cb;
        };
        
        class WatchPool {
        public:
            template<class T>
            std::shared_ptr<Watch> add(ZkClient *zkClient, const std::string &path, const typename T::Callback &cb) {
                std::string name = typeid(T).name() + path;
                auto it = _watchMap.find(name);
                if (it == _watchMap.end()) {
                    std::shared_ptr<Watch> wp = std::make_shared<T>(zkClient, path, cb);
                    _watchMap[name] = wp;
                    return wp;
                } else {
                    return it->second;
                }
            }
            
            template<class T>
            std::shared_ptr<Watch> get(const std::string &path) {
                std::string name = typeid(T).name() + path;
                auto it = _watchMap.find(name);
                if (it == _watchMap.end()) {
                    return std::shared_ptr<Watch>();
                } else {
                    return it->second;
                }
            }
            
            void setAll() const {
                for (auto it = _watchMap.begin(); it != _watchMap.end(); ++it) {
                    it->second->set();
                }
            }
            
        private:
            std::map<std::string, std::shared_ptr<Watch>> _watchMap;
        };
        
    private:
        // e.g. "127.0.0.1:2181,127.0.0.1:2182,127.0.0.1:2183"
        std::string _host;
        int _timeout;
        bool _reconnecting;
        zhandle_t *_zkhandle;
        WatchPool _watchPool;
        // Callback when connected
        std::function<void()> _cb;
        
    private:
        ZkClient();                                                  // ctor hidden
        ~ZkClient();                                                 // destruct hidden
        ZkClient(ZkClient const&) = delete;                   // copy ctor delete
        ZkClient(ZkClient &&) = delete;                       // move ctor delete
        ZkClient& operator=(ZkClient const&) = delete;        // assign op. delete
        ZkClient& operator=(ZkClient &&) = delete;            // move assign op. delete
    };

}

#define g_ZkClient ZkClient::Instance()

#endif /* zookeeper_client_h */
