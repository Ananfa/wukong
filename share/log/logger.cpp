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

#include "logger.h"

#include "corpc_routine_env.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

#include <cstdio>
#include <sys/time.h>

using namespace rapidjson;
using namespace wukong;

LogSink::LogSink(int id, const std::string &fileName, LogRotateType rotateType, bool addTimestamp):
    id_(id), fileName_(fileName), rotateType_(rotateType), addTimestamp_(addTimestamp),
    logFileTime_(0), logFile_(nullptr) {

}

LogSink::~LogSink() {
    if (logFile_) {
        fclose(logFile_);
        logFile_ = nullptr;
    }
}

void LogSink::writeLog(const std::string &message) {
    uint64_t logFileTime = 0;
    struct timeval tv;
    struct tm tm_now;

    if (addTimestamp_ || rotateType_ != ROTATE_TYPE_NONE) {
        gettimeofday(&tv, NULL);
        localtime_r(&tv.tv_sec, &tm_now);
    }

    switch (rotateType_) {
        case ROTATE_TYPE_HOURLY: {
            logFileTime = (tm_now.tm_year+1900)*1000000+(tm_now.tm_mon+1)*10000+tm_now.tm_mday*100+tm_now.tm_hour;
            break;    
        }
        case ROTATE_TYPE_DAILY: {
            logFileTime = (tm_now.tm_year+1900)*10000+(tm_now.tm_mon+1)*100+tm_now.tm_mday;
            break;    
        }
        case ROTATE_TYPE_MONTHLY: {
            logFileTime = (tm_now.tm_year+1900)*100+(tm_now.tm_mon+1);
            break;    
        }
        case ROTATE_TYPE_YEARLY: {
            logFileTime = tm_now.tm_year+1900;
            break;    
        }
        default:
        break;
    }

    if (logFileTime != logFileTime_) {
        if (logFile_) {
            fclose(logFile_);
            logFile_ = nullptr;
        }

        logFileTime_ = logFileTime;
    }

    if (logFile_ == nullptr) {
        char logFileName[200];
        snprintf(logFileName, 200, "%s_%d.log", fileName_.c_str(), logFileTime_);
        logFile_ = fopen(logFileName, "a");
    }

    if (addTimestamp_) {
        char buffer[26];
        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", &tm_now);
        fputs(buffer, logFile_);
        
        uint32_t mseconds = tv.tv_usec / 1000;
        snprintf(buffer, 26, ".%03u ", mseconds);
        fputs(buffer, logFile_);
    }

    fputs(message.c_str(), logFile_);

    fflush(logFile_);
}

bool Logger::init(const std::string &configPath) {
    // 读取日志配置并初始化日志
    FILE* fp = fopen(configPath.c_str(), "rb");
    
    char readBuffer[65536];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    
    Document doc;
    doc.ParseStream(is);
    
    if (!doc.IsArray()) {
        ERROR_LOG("log config error -- document not array\n");
        return false;
    }

    for (SizeType i = 0; i < doc.Size(); i++) {
        const Value& logConfig = doc[i];

        if (!logConfig.HasMember("id")) {
            ERROR_LOG("log config error -- doc[%d].id not define\n", i);
            return false;
        }
        int logId = logConfig["id"].GetUint();
        if (sinks_.find(logId) != sinks_.end()) {
            ERROR_LOG("log config error -- log id %d duplicate\n", logId);
            return false;
        }

        if (!logConfig.HasMember("fileName")) {
            ERROR_LOG("config error -- doc[%d].fileName not define\n", i);
            return false;
        }
        std::string fileName = logConfig["fileName"].GetString();
        
        if (!logConfig.HasMember("rotateType")) {
            ERROR_LOG("log config error -- doc[%d].rotateType not define\n", i);
            return false;
        }
        LogRotateType rotateType = (LogRotateType)logConfig["rotateType"].GetUint();

        bool addTimestamp = false;
        if (logConfig.HasMember("addTimestamp")) {
            addTimestamp = logConfig["addTimestamp"].GetBool();
        }

        sinks_[logId] = std::unique_ptr<LogSink>(new LogSink(logId, fileName, rotateType, addTimestamp));
    }

    return true;
}

void Logger::start() {
    // 启动日志线程
    t_ = std::thread(threadEntry, this);
}

void Logger::addLog(int type, const std::string &message) {
    LogMessage *logMsg = new LogMessage();
    logMsg->type = type;
    logMsg->message = message;

    queue_.push(logMsg);
}

void Logger::threadEntry( Logger *self ) {
    RoutineEnvironment::startCoroutine(logRoutine, self);
    RoutineEnvironment::runEventLoop();
}

void *Logger::logRoutine( void * arg ) {
    Logger *self = (Logger *)arg;

    LogMessageQueue& queue = self->queue_;

    // 初始化pipe readfd
    int readFd = queue.getReadFd();
    co_register_fd(readFd);
    co_set_timeout(readFd, -1, 1000);
    
    int ret;
    std::vector<char> buf(1024);
    while (true) {
        ret = (int)read(readFd, &buf[0], 1024);
        assert(ret != 0);
        if (ret < 0) {
            if (errno == EAGAIN) {
                continue;
            } else {
                // 管道出错
                ERROR_LOG("Logger::logRoutine read from pipe fd %d ret %d errno %d (%s)\n",
                       readFd, ret, errno, strerror(errno));
                
                // TODO: 如何处理？退出协程？
                // sleep 10 milisecond
                msleep(10);
            }
        }

        LogMessage *logMsg = queue.pop();
        while (logMsg) {
            self->writeLog(logMsg->type, logMsg->message);

            delete logMsg;
            logMsg = queue.pop();
        }
    }
}

void Logger::writeLog(int type, const std::string &message) {
    auto iter = sinks_.find(type);
    if (iter == sinks_.end()) {
        ERROR_LOG("Logger::writeLog log type %d invalid\n", type);
        return;
    }

    iter->second->writeLog(message);
}