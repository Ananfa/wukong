/*
 * Created by Xianke Liu on 2026/1/4.
 */

#include "battle_service.h"
#include "battle_room_manager.h"

#include "corpc_utils.h"

using namespace wukong;

// corpc 约定：
// - not_care_response：ctl、response 可为 NULL，仅对 ctl 判空后再 SetFailed。
// - delete_in_done：仅当 proto 为该选项时 done 才有效且必须在实现中 Run；否则不必判断或调用 done。
void BattleServiceImpl::requestBattleAssignment(::google::protobuf::RpcController* controller,
                       const ::wukong::pb::RequestBattleAssignmentRequest* request,
                       ::wukong::pb::RequestBattleAssignmentResponse* response,
                       ::google::protobuf::Closure* done) {
    (void)done;
    corpc::Controller *ctl = static_cast<corpc::Controller *>(controller);
    if (!g_BattleRoomManager.assignFromRequest(*request, *response)) {
        if (ctl) {
            ctl->SetFailed("requestBattleAssignment failed");
        }
    }
}

void BattleServiceImpl::removeBattlePlayer(::google::protobuf::RpcController* controller,
                       const ::wukong::pb::RemoveBattlePlayerRequest* request,
                       ::corpc::Void* response,
                       ::google::protobuf::Closure* done) {
    (void)controller;
    (void)response;
    (void)done;
    g_BattleRoomManager.removePlayer(request->room_id(), request->role_id());
}
