#pragma once

#include <env/const.hpp>


namespace VoxelWorld::Str
{
    ConstStr obstaclesMinNumPlatforms = "obstaclesMinNumPlatforms",
             obstaclesMaxNumPlatforms = "obstaclesMaxNumPlatforms",
             obstaclesMinGap = "obstaclesMinGap",
             obstaclesMaxGap = "obstaclesMaxGap";

    ConstStr teamSpirit = "teamSpirit",
             pickedUpObject = "pickedUpObject",
             visitedBuildingZoneWithObject = "visitedBuildingZoneWithObject";

    ConstStr collectSingle = "collectSingle",
             collectAll = "collectAll";

    ConstStr sokobanBoxOnTarget = "sokobanBoxOnTarget",
             sokobanBoxLeavesTarget = "sokobanBoxLeavesTarget",
             sokobanAllBoxesOnTarget = "sokobanAllBoxesOnTarget";

    ConstStr boxagoneTouchedFloor = "boxagoneTouchedFloor",
             boxagonePerStepReward = "boxagonePerStepReward",
             boxagoneLastManStanding = "boxagoneLastManStanding";
}
