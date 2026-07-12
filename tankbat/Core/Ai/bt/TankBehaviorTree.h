#pragma once
#include "BTNode.h"
#include <memory>

namespace TankBattle
{
    std::unique_ptr<BTNode> BuildDefaultTankBehaviorTree();
}
