/*****************************************************************************
 * Copyright (c) 2014-2020 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "GameAction.h"

enum class TileModifyType : uint8_t
{
    AnyRemove,
    AnySwap,
    AnyInsertCorrupt,
    AnyRotate,
    AnyPaste,
    AnySort,
    AnyBaseHeightOffset,
    SurfaceShowParkFences,
    SurfaceToggleCorner,
    SurfaceToggleDiagonal,
    PathSetSlope,
    PathSetBroken,
    PathToggleEdge,
    EntranceMakeUsable,
    WallSetSlope,
    WallSetAnimationFrame,
    TrackBaseHeightOffset,
    TrackSetChain,
    TrackSetChainBlock,
    TrackSetBlockBrake,
    TrackSetIndestructible,
    ScenerySetQuarterLocation,
    ScenerySetQuarterCollision,
    BannerToggleBlockingEdge,
    CorruptClamp,
    Count,
};

DEFINE_GAME_ACTION(TileModifyAction, GAME_COMMAND_MODIFY_TILE, GameActions::Result)
{
private:
    CoordsXY _loc;
    TileModifyType _setting{};
    uint32_t _value1{};
    uint32_t _value2{};
    TileElement _pasteElement{};

public:
    TileModifyAction() = default;
    TileModifyAction(
        CoordsXY loc, TileModifyType setting, uint32_t value1 = 0, uint32_t value2 = 0, TileElement pasteElement = {});

    uint16_t GetActionFlags() const override;

    void Serialise(DataSerialiser & stream) override;
    GameActions::Result::Ptr Query() const override;
    GameActions::Result::Ptr Execute() const override;

private:
    GameActions::Result::Ptr QueryExecute(bool isExecuting) const;
};
