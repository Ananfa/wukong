# ThirdParty notes

## Corpc / Kcp

Copied from `libcorpc/clients/unity` for Battle KCP FrameSync.

Uses official `Google.Protobuf` from `Assets/Plugins/Google.Protobuf`.

## System.Buffers

`Kcp` requires `System.Buffers` / `Memory<T>`. Prefer Unity 2021+ (.NET Standard 2.1).
If missing, add Unity package or copy `System.Memory.dll` / `System.Buffers.dll` into `Assets/Plugins`.
