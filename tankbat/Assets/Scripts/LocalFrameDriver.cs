using System;

namespace TankBattle
{
    /// <summary>
    /// 本地权威帧驱动：30Hz 累加器 → SetFrameInputs + AdvanceSimulation。
    /// 在线模式由服务端下发的帧同步消息驱动，不使用本类。
    /// </summary>
    public class LocalFrameDriver
    {
        private readonly TankBattleNative.GameCore gameCore;
        private float accumulator;
        private readonly float fixedDeltaTime;
        private readonly int maxCatchUpSteps;

        public LocalFrameDriver(TankBattleNative.GameCore gameCore)
        {
            this.gameCore = gameCore ?? throw new ArgumentNullException(nameof(gameCore));
            fixedDeltaTime = LogicTiming.FixedDeltaTime;
            maxCatchUpSteps = LogicTiming.MaxCatchUpStepsPerFrame;
        }

        public uint CurrentFrame => gameCore.GetFrame();

        public void Reset()
        {
            accumulator = 0f;
        }

        /// <summary>推进固定逻辑帧；buildLocalInput 返回 null 表示本帧无玩家输入。</summary>
        /// <returns>本 Unity 帧内推进的逻辑帧数。</returns>
        public int Tick(float deltaTime, Func<PlayerInput?> buildLocalInput)
        {
            accumulator += deltaTime;
            int steps = 0;

            while (accumulator >= fixedDeltaTime && steps < maxCatchUpSteps)
            {
                uint nextFrame = gameCore.GetFrame() + 1;
                PlayerInput? localInput = buildLocalInput?.Invoke();

                if (localInput.HasValue)
                {
                    PlayerInput input = localInput.Value;
                    input.frame = nextFrame;
                    gameCore.SetFrameInputs(nextFrame, new[] { input });
                }
                else
                {
                    gameCore.SetFrameInputs(nextFrame, Array.Empty<PlayerInput>());
                }

                gameCore.AdvanceSimulation();
                accumulator -= fixedDeltaTime;
                steps++;
            }

            return steps;
        }
    }
}
