/*
 * Battle 服 KCP 消息注册（客户端进房鉴权等）
 */
#ifndef wukong_battle_handler_h
#define wukong_battle_handler_h

namespace corpc {
class KcpMessageTerminal;
}

namespace wukong {

class BattleHandler {
public:
    static void registerMessages(corpc::KcpMessageTerminal *terminal);
};

}

#endif
