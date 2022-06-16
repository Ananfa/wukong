#include "corpc_routine_env.h"
#include "corpc_utils.h"
#include "lobby_game_object.h"
#include "demo_const.h"
#include "common.pb.h"

using namespace corpc;
using namespace demo;
   
extern "C"  
{  
    #include "lua.h"  
    #include "lauxlib.h"  
    #include "lualib.h"

    int luaopen_libfixecho(lua_State *L);
}

static int lua_echo(lua_State *L)  
{
    DEBUG_LOG("lua_echo -- lua stack size: %d\n", lua_gettop(L));
    // 注意：必须先将lua栈中的参数取出，否则协程切换后lua栈会被另作他用
    long handle1 = lua_tonumber(L, 1);
    long handle2 = lua_tonumber(L, 2);
    uint16_t tag = lua_tonumber(L, 3);

    LobbyGameObject *gobj = (LobbyGameObject*)handle1;
    wukong::pb::StringValue *msg = (wukong::pb::StringValue*)handle2;

    gobj->setExp(gobj->getExp()+1);

    gobj->send(S2C_MESSAGE_ID_ECHO, tag, *msg);

    return 1;  
}

static const struct luaL_Reg myLib[] =   
{  
    {"lua_echo", lua_echo}, 
    {NULL, NULL}       //数组中最后一对必须是{NULL, NULL}，用来表示结束      
};  

extern int luaopen_libfixecho(lua_State *L)  
{  
    luaL_newlib(L, myLib);
    return 1;       // 把myLib表压入了栈中，所以就需要返回1  
}
