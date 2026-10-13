/*
    ------------------------------------------------------------------------------------
    LICENSE:
    ------------------------------------------------------------------------------------
    This file is part of EVEmu: EVE Online Server Emulator
    Copyright 2006 - 2021 The EVEmu Team
    For the latest information visit https://evemu.dev
    ------------------------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your option) any later
    version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along with
    this program; if not, write to the Free Software Foundation, Inc., 59 Temple
    Place - Suite 330, Boston, MA 02111-1307, USA, or go to
    http://www.gnu.org/copyleft/lesser.txt.
    ------------------------------------------------------------------------------------
    Author:        Allan
*/

#ifndef EVE_INVENTORY_ITEM_DB
#define EVE_INVENTORY_ITEM_DB

#include "ServiceDB.h"
#include "inventory/ItemType.h"


class ItemData;

class ItemDB
{
public:
    // get item data based on itemID
    static bool GetItemData(uint32 itemID, ItemData &into);   // called by RefPtr<_Ty> _Load() at InventoryItem.h:245
    static bool DeleteItem(uint32 itemID);

    // Batch preload for system boot (see ItemDB.cpp): fills an in-memory map of
    // ItemData for the system's celestials/stations/in-space entities in a few
    // batched SELECTs, so the per-item loaders run no per-item SELECT.
    // Consumed transparently by GetItemData(); release with ClearPreload().
    //
    // BuildSystemItemPreload() runs those SELECTs into a POD map and is
    // WORKER-THREAD SAFE (pure DB reads, no singletons). InstallPreload() swaps a
    // worker-built map into the boot cache on the game thread. PreloadSystemItems()
    // is the synchronous (game-thread) build+install fallback.
    static void BuildSystemItemPreload(uint32 systemID, std::map<uint32, ItemData>& out);
    static void InstallPreload(const std::map<uint32, ItemData>& in);
    static void PreloadSystemItems(uint32 systemID);
    static void ClearPreload();

    static void UpdateLocation(uint32 itemID, uint32 locationID, EVEItemFlags flag);

    static uint32 NewItem(const ItemData &data);

    static bool SaveItem(uint32 itemID, const ItemData &data);
    static void SaveItems(std::vector< Inv::SaveData > &data);
    static void SaveAttributes(bool isChar, std::vector< Inv::AttrData > &data);

    // only used in ConsoleCommands to test/process fx data
    static void GetItems(uint16 catID, std::map<uint16, std::string> &typeIDs);
};


#endif  // EVE_INVENTORY_ITEM_DB
