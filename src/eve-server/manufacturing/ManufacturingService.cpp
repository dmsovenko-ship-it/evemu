
#include "eve-server.h"

#include "inventory/InventoryItem.h"
#include "manufacturing/ManufacturingService.h"

ManufacturingService::ManufacturingService() :
    Service("manufacturing")
{
    this->Add("GetPathToItem", &ManufacturingService::GetPathToItem);
}

PyResult ManufacturingService::GetPathToItem(PyCallArgs& call, PyInt* itemID) {
    // installedItemLocationData = sm.GetService('manufacturing').GetPathToItem(quoteData.blueprint)
    _log(MANUF__MESSAGE, "ManufacturingService::GetPathToItem() size= %lli", call.tuple->size());
    call.Dump(MANUF__DUMP);

    InventoryItemRef item = sItemFactory.GetItemRef(itemID->value());
    if (item.get() == nullptr) {
        _log(MANUF__ERROR, "GetPathToItem: item %u not found", itemID->value());
        return PyStatic.NewNone();
    }

    // PathElement: [locationID, ownerID, flagID]
    PyList* path = new PyList();
    path->AddItem(new PyInt(item->locationID()));
    path->AddItem(new PyInt(item->ownerID()));
    path->AddItem(new PyInt(item->flag()));
    return path;
}
