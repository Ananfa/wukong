#ifndef wukong_errcode_h
#define wukong_errcode_h

#include "define.h"
#include <string>

namespace wukong {
    const int SUCCESS                           = 0; // 无错误

    const int LOAD_SCENE_FAILED                 = 100; // 加载场景失败
    const int SCENE_ALREADY_EXIST               = 101; // 场景已存在
}


#endif /* wukong_errcode_h */
