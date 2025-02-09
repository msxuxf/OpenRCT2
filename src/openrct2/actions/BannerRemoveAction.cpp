/*****************************************************************************
 * Copyright (c) 2014-2020 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "BannerRemoveAction.h"

#include "../management/Finance.h"
#include "../world/Banner.h"
#include "../world/MapAnimation.h"
#include "../world/Scenery.h"
#include "GameAction.h"

BannerRemoveAction::BannerRemoveAction(const CoordsXYZD& loc)
    : _loc(loc)
{
}

void BannerRemoveAction::AcceptParameters(GameActionParameterVisitor& visitor)
{
    visitor.Visit(_loc);
}

uint16_t BannerRemoveAction::GetActionFlags() const
{
    return GameAction::GetActionFlags();
}

void BannerRemoveAction::Serialise(DataSerialiser& stream)
{
    GameAction::Serialise(stream);

    stream << DS_TAG(_loc);
}

GameActions::Result::Ptr BannerRemoveAction::Query() const
{
    auto res = MakeResult();
    res->Expenditure = ExpenditureType::Landscaping;
    res->Position.x = _loc.x + 16;
    res->Position.y = _loc.y + 16;
    res->Position.z = _loc.z;
    res->ErrorTitle = STR_CANT_REMOVE_THIS;

    if (!LocationValid(_loc) || !map_can_build_at({ _loc.x, _loc.y, _loc.z - 16 }))
    {
        return MakeResult(GameActions::Status::NotOwned, STR_CANT_REMOVE_THIS, STR_LAND_NOT_OWNED_BY_PARK);
    }

    BannerElement* bannerElement = GetBannerElementAt();
    if (bannerElement == nullptr)
    {
        log_error("Invalid banner location, x = %d, y = %d, z = %d, direction = %d", _loc.x, _loc.y, _loc.z, _loc.direction);
        return MakeResult(GameActions::Status::InvalidParameters, STR_CANT_REMOVE_THIS);
    }

    if (bannerElement->GetIndex() >= MAX_BANNERS || bannerElement->GetIndex() == BANNER_INDEX_NULL)
    {
        log_error("Invalid banner index. index = ", bannerElement->GetIndex());
        return MakeResult(GameActions::Status::InvalidParameters, STR_CANT_REMOVE_THIS);
    }

    auto banner = bannerElement->GetBanner();
    if (banner == nullptr)
    {
        log_error("Invalid banner index. index = ", bannerElement->GetIndex());
        return MakeResult(GameActions::Status::InvalidParameters, STR_CANT_REMOVE_THIS);
    }

    rct_scenery_entry* bannerEntry = get_banner_entry(banner->type);
    if (bannerEntry != nullptr)
    {
        res->Cost = -((bannerEntry->banner.price * 3) / 4);
    }

    return res;
}

GameActions::Result::Ptr BannerRemoveAction::Execute() const
{
    auto res = MakeResult();
    res->Expenditure = ExpenditureType::Landscaping;
    res->Position.x = _loc.x + 16;
    res->Position.y = _loc.y + 16;
    res->Position.z = _loc.z;
    res->ErrorTitle = STR_CANT_REMOVE_THIS;

    BannerElement* bannerElement = GetBannerElementAt();
    if (bannerElement == nullptr)
    {
        log_error("Invalid banner location, x = %d, y = %d, z = %d, direction = %d", _loc.x, _loc.y, _loc.z, _loc.direction);
        return MakeResult(GameActions::Status::InvalidParameters, STR_CANT_REMOVE_THIS);
    }

    if (bannerElement->GetIndex() >= MAX_BANNERS || bannerElement->GetIndex() == BANNER_INDEX_NULL)
    {
        log_error("Invalid banner index. index = ", bannerElement->GetIndex());
        return MakeResult(GameActions::Status::InvalidParameters, STR_CANT_REMOVE_THIS);
    }

    auto banner = bannerElement->GetBanner();
    if (banner == nullptr)
    {
        log_error("Invalid banner index. index = ", bannerElement->GetIndex());
        return MakeResult(GameActions::Status::InvalidParameters, STR_CANT_REMOVE_THIS);
    }

    rct_scenery_entry* bannerEntry = get_banner_entry(banner->type);
    if (bannerEntry != nullptr)
    {
        res->Cost = -((bannerEntry->banner.price * 3) / 4);
    }

    tile_element_remove_banner_entry(reinterpret_cast<TileElement*>(bannerElement));
    map_invalidate_tile_zoom1({ _loc, _loc.z, _loc.z + 32 });
    bannerElement->Remove();

    return res;
}

BannerElement* BannerRemoveAction::GetBannerElementAt() const
{
    TileElement* tileElement = map_get_first_element_at(_loc);

    // Find the banner element at known z and position
    do
    {
        if (tileElement == nullptr)
            break;
        if (tileElement->GetType() != TILE_ELEMENT_TYPE_BANNER)
            continue;
        if (tileElement->GetBaseZ() != _loc.z)
            continue;
        if (tileElement->IsGhost() && !(GetFlags() & GAME_COMMAND_FLAG_GHOST))
            continue;
        if (tileElement->AsBanner()->GetPosition() != _loc.direction)
            continue;

        return tileElement->AsBanner();
    } while (!(tileElement++)->IsLastForTile());

    return nullptr;
}
