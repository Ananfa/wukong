//
//  zk_client.h
//  ZkClient
//
//  Created by Shizan Lou on 2018/8/23.
//  Copyright © 2018年 Shizan Lou. All rights reserved.
//

#ifndef wukong_zk_client_h
#define wukong_zk_client_h

#include <string>
#include <functional>
#include <vector>
#include <map>
#include <zookeeper.h>

namespace wukong {
    class ZkRet {
        friend class ZkClient;
    public:
        bool ok() const { return code_ == ZOK; }
        bool code() const { return code_; }
        operator bool() const { return ok(); }
    protected:
        ZkRet() { code_ = ZOK; }
        ZkRet(int c) { code_ = c; }
    private:
        int code_;
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
        zhandle_t* getZkHandle() { return zkhandle_; };
        
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
            virtual ~Watch() {}
            virtual void set() const = 0;
            
            const std::string &path() const { return path_; }
            ZkClient* zkClient() const { return zkClient_; }
            bool isTemporary() const { return temporary_; }
            void setTemporary() { temporary_ = true; }
        protected:
            ZkClient *zkClient_;
            std::string path_;
            bool temporary_;
        };
        
        class ValidWatch: public Watch {
        public:
            typedef ValidCallback Callback;
            ValidWatch(ZkClient *zkClient, const std::string &path, const Callback &cb);
            virtual void set() const;
            void doCb(const ZkRet &value) const{ cb_(path_, value); };
        private:
            Callback cb_;
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
            void doCb(const std::string &value) const{ cb_(path_, value); };
        private:
            Callback cb_;
        };
        
        class ChildrenWatch: public Watch {
        public:
            typedef ChildrenCallback Callback;
            ChildrenWatch(ZkClient *zkClient, const std::string &path, const Callback &cb);
            virtual void set() const;
            void doCb(const std::vector<std::string> &value) const { cb_(path_, value); };
        private:
            Callback cb_;
        };
        
        class WatchPool {
        public:
            template<class T>
            std::shared_ptr<Watch> add(ZkClient *zkClient, const std::string &path, const typename T::Callback &cb) {
                std::string name = typeid(T).name() + path;
                auto it = watchMap_.find(name);
                if (it == watchMap_.end()) {
                    std::shared_ptr<Watch> wp = std::make_shared<T>(zkClient, path, cb);
                    watchMap_[name] = wp;
                    return wp;
                } else {
                    return it->second;
                }
            }
            
            template<class T>
            std::shared_ptr<Watch> get(const std::string &path) {
                std::string name = typeid(T).name() + path;
                auto it = watchMap_.find(name);
                if (it == watchMap_.end()) {
                    return std::shared_ptr<Watch>();
                } else {
                    return it->second;
                }
            }
            
            void setAll() const {
                for (auto &w : watchMap_) {
                    w.second->set();
                }
            }
            
        private:
            std::map<std::string, std::shared_ptr<Watch>> watchMap_;
        };
        
    private:
        // e.g. "127.0.0.1:2181,127.0.0.1:2182,127.0.0.1:2183"
        std::string host_;
        int timeout_;
        bool reconnecting_;
        zhandle_t *zkhandle_;
        WatchPool watchPool_;
        // Callback when connected
        std::function<void()> cb_;
        
    private:
        ZkClient();                                                  // ctor hidden
        ~ZkClient();                                                 // destruct hidden
        ZkClient(ZkClient const&) = delete;                   // copy ctor delete
        ZkClient(ZkClient &&) = delete;                       // move ctor delete
        ZkClient& operator=(ZkClient const&) = delete;        // assign op. delete
        ZkClient& operator=(ZkClient &&) = delete;            // move assign op. delete
    };

}

#define g_ZkClient wukong::ZkClient::Instance()

#endif /* wukong_zk_client_h */
