#ifndef demo_errdef_h
#define demo_errdef_h

namespace demo {
    const uint16_t ERR_SERVER_ERROR              = 1;   // 服务器内部错误

    const uint16_t ERR_UNKNOWN_SCENE             = 100; // 未知场景
    const uint16_t ERR_FORBIT_SCENE              = 101; // 禁止进入场景
    const uint16_t ERR_ALREADY_IN_SCENE          = 102; // 已经在场景中
}

#endif /* demo_errdef_h */
