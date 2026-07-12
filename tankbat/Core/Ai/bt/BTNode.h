#pragma once
#include "../AiBlackboard.h"
#include "../AiIntent.h"
#include "../AiTypes.h"
#include "../../Common/GameRng.h"
#include "NavigationGrid.h"
#include "Tank.h"
#include <functional>
#include <memory>
#include <vector>

namespace TankBattle
{
    enum class BTStatus
    {
        Success = 0,
        Failure = 1,
        Running = 2
    };

    struct BTContext
    {
        Tank* self = nullptr;
        AiBlackboard* bb = nullptr;
        AiIntent* intent = nullptr;
        AiTankMemory* memory = nullptr;
        const std::vector<std::shared_ptr<Tank>>* allTanks = nullptr;
        const NavigationGrid* navGrid = nullptr;
        uint32_t frameIndex = 0;
        const GameRng* gameRng = nullptr;
    };

    class BTNode
    {
    public:
        virtual ~BTNode() = default;
        virtual BTStatus Tick(BTContext& ctx) = 0;
    };

    class BTSequence : public BTNode
    {
    public:
        void AddChild(std::unique_ptr<BTNode> child)
        {
            m_children.push_back(std::move(child));
        }

        BTStatus Tick(BTContext& ctx) override
        {
            for (size_t i = 0; i < m_children.size(); ++i)
            {
                BTStatus status = m_children[i]->Tick(ctx);
                if (status != BTStatus::Success)
                    return status;
            }
            return BTStatus::Success;
        }

    private:
        std::vector<std::unique_ptr<BTNode>> m_children;
    };

    class BTSelector : public BTNode
    {
    public:
        void AddChild(std::unique_ptr<BTNode> child)
        {
            m_children.push_back(std::move(child));
        }

        BTStatus Tick(BTContext& ctx) override
        {
            for (size_t i = 0; i < m_children.size(); ++i)
            {
                BTStatus status = m_children[i]->Tick(ctx);
                if (status != BTStatus::Failure)
                    return status;
            }
            return BTStatus::Failure;
        }

    private:
        std::vector<std::unique_ptr<BTNode>> m_children;
    };

    using BTActionFn = std::function<BTStatus(BTContext&)>;

    class BTAction : public BTNode
    {
    public:
        explicit BTAction(BTActionFn fn) : m_fn(std::move(fn)) {}

        BTStatus Tick(BTContext& ctx) override
        {
            return m_fn ? m_fn(ctx) : BTStatus::Failure;
        }

    private:
        BTActionFn m_fn;
    };

    class BTCondition : public BTNode
    {
    public:
        explicit BTCondition(BTActionFn fn) : m_fn(std::move(fn)) {}

        BTStatus Tick(BTContext& ctx) override
        {
            if (!m_fn) return BTStatus::Failure;
            return m_fn(ctx) == BTStatus::Success ? BTStatus::Success : BTStatus::Failure;
        }

    private:
        BTActionFn m_fn;
    };
}
