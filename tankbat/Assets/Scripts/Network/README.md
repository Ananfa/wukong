# Tankbat online flow (wukong)

Matches `demo/client/src/main.cpp` + Lobby StartBattle, split across UI screens:

1. **Mode select** — 单机游戏 / 联网对战
2. **Login UI** (online only) — HTTP `/login` → `/createRole` → `/enterGame` → TCP Gateway Auth → `ENTERLOBBY`  
   (session kept; do once per play session)
3. **Main menu** — pick faction → **Start**
4. **Start game**
   - Offline: local `GameCore` + `LocalFrameDriver`
   - Online: TCP `START_BATTLE` → `BATTLE_ENTER` → KCP Auth → Snapshot / FrameSync
5. **Battle end / 返回菜单** — leave battle only; stay logged in → main menu
6. **Quit** — `Logout()` if online, then quit app

Login screen has **返回** → mode select.

## Inspector

Optional on `GameManager`: assign `modeSelectUI` / `loginUI` / buttons; if empty, panels are created at runtime (needs a Canvas).

On `TankBattleClient`:

- `loginBaseUrl` (default `http://127.0.0.1:11000`)
- `gameServerId` / `battleDefId` (must exist in BattleRoomTypes design table)
- `useDirectKcpBypass` only for debugging without Login/Lobby

## Generated protos

```powershell
cd F:\linux_dev\ananfa\wukong\proto
$out = F:\linux_dev\ananfa\wukong\tankbat\Assets\Scripts\Network\Generated
protoc -I. --csharp_out=$out battle_sync.proto
protoc -I. --csharp_out=$out game/game.proto
protoc -I. --csharp_out=$out common.proto
```
