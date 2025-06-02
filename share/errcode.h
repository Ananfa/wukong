#ifndef wukong_errcode_h
#define wukong_errcode_h

#include "define.h"
#include <string>

namespace wukong {
    const int SUCCESS                           = 0; // 无错误
    const int STUB_NOT_AVALIABLE                = 1; // 服务器stub不可用

    const int LOAD_SCENE_FAILED                 = 100; // 加载场景失败
    const int SCENE_NOT_EXIST                   = 101; // 场景不存在
    const int SCENE_ALREADY_EXIST               = 102; // 场景已存在
    const int ALREADY_IN_SCENE                  = 103; // 已在场景中
}


#endif /* wukong_errcode_h */
