#ifndef demo_const_h
#define demo_const_h

namespace demo {
	// 客户端发给服务器的消息号定义
    const uint16_t C2S_MESSAGE_ID_ECHO             = 1000; // 回声测试消息
    const uint16_t C2S_MESSAGE_ID_ENTERSCENE       = 1100; // 进入场景消息

    // 服务器发给客户端的消息号定义
    const uint16_t S2C_MESSAGE_ID_ECHO             = 1000; // 回声测试消息
    const uint16_t S2C_MESSAGE_ID_ERROR            = 1001; // 错误信息
    const uint16_t S2C_MESSAGE_ID_ENTERSCENE       = 1100; // 进入场景消息
    const uint16_t S2C_MESSAGE_ID_ENTERLOBBY       = 1101; // 进入大厅消息
}

#endif /* demo_const_h */
