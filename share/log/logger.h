/*
 * Created by Xianke Liu on 2023/12/10.
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

#ifndef wukong_logger_h
#define wukong_logger_h

#include <map>
#include <memory>
#include <string>
#include <thread>

#include "corpc_queue.h"

using namespace corpc;

// 日志模块设计思考：
// 1. 目前写文件操作不会进行协程切换，因此开协程进行日志落地无意义
// 2. 日志不是高频发生的操作，开单独线程负责写日志文件即可满足需求
namespace wukong {
    enum LogRotateType
    {
        ROTATE_TYPE_NONE,
        ROTATE_TYPE_HOURLY,
        ROTATE_TYPE_DAILY,
        ROTATE_TYPE_MONTHLY,
        ROTATE_TYPE_YEARLY
    };

    struct LogMessage {
        int type;
        std::string message;
    };

    class LogSink {
    public:
        LogSink(int id, const std::string &fileName, LogRotateType rotateType, bool addTimestamp);
        ~LogSink();

        void writeLog(const std::string &message);

    private:
        int id_;
        std::string fileName_;
        LogRotateType rotateType_;
        bool addTimestamp_;

        uint64_t logFileTime_;
        FILE *logFile_;
    };

    class Logger {
    public:
        typedef CoSyncQueue<LogMessage *> LogMessageQueue;
        typedef std::map<int, std::unique_ptr<LogSink>> LogSinkMap;

    public:
        static Logger& Instance() {
            static Logger instance;
            return instance;
        }

        bool init(const std::string &configPath);

        void start();

        void addLog(int type, const std::string &message);

    private:
        static void threadEntry( Logger *self );
        
        static void *logRoutine( void * arg );

        void writeLog(int type, const std::string &message);
    private:
        LogMessageQueue queue_;
        std::thread t_;

        LogSinkMap sinks_;

        // TODO: 日志文件句柄对象

    private:
        Logger() = default;                                // ctor hidden
        ~Logger() = default;                               // destruct hidden
        Logger(Logger const&) = delete;                    // copy ctor delete
        Logger(Logger &&) = delete;                        // move ctor delete
        Logger& operator=(Logger const&) = delete;         // assign op. delete
        Logger& operator=(Logger &&) = delete;             // move assign op. delete
    };
}

#define g_Logger wukong::Logger::Instance()

#endif /* wukong_logger_h */
