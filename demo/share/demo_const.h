#ifndef demo_const_h
#define demo_const_h

namespace demo {
    const uint16_t C2S_MESSAGE_ID_ECHO             = 1000; // 回声测试请求消息
    const uint16_t C2S_MESSAGE_ID_ENTERSCENE       = 1100; // 进入场景请求消息

    const uint16_t S2C_MESSAGE_ID_ECHO             = 1000; // 回声测试返回消息
    const uint16_t S2C_MESSAGE_ID_ERROR            = 1001; // 错误信息
    const uint16_t S2C_MESSAGE_ID_ENTERSCENE       = 1100; // 进入场景返回消息
}

#endif /* demo_const_h */
