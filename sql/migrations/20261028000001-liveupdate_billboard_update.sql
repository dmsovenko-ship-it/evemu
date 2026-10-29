-- +migrate Up
-- Live-update #8: BillboardMgr.Update re-fetches the advert with a cache-buster
-- and re-picks the wanted portrait before re-applying the billboard textures.
--
-- BillboardMgr.Run() stores self.advertPath ONCE per session and
-- Billboard.UpdateBillboardContents() always re-applies that same path, so the
-- server's periodic billboard ball re-emit (Client::SendServiceCall +
-- SystemManager::ResendBillboards) had no visible effect.
--
-- photo.GetTextureFromURL() caches by URL in browserCache, so a stable advert URL
-- always returns the cached DDS (and trinity will not reload a texture whose path
-- did not change). A wallclock cache-buster makes the cache miss, produces a new
-- file name and forces the model to reload it.
INSERT INTO liveupdates
  (updateID, updateName, description, machoVersionMin, machoVersionMax,
   buildNumberMin, buildNumberMax, methodName, objectID, codeType, code)
VALUES
  (8, 'BillboardUpdateFix',
   'billboard advert cache-buster',
   0, 999999, 1, 500000, 'Update', 'svc.billboard::BillboardMgr', 'globalClassMethod',
   UNHEX('63020000000200000003000000430000007329000000790e007c00006a000083000001576e07000101016e0100587c00006a01007c0100830100016400005328010000004e2802000000740c000000557064617465426f756e7479741e0000005f42696c6c626f6172644d67725f5f55706461746542696c6c626f6172642802000000740400000073656c6674050000006d6f64656c280000000028000000007344000000433a5c6f70656e636f64652d70726f6a656374735c6576656d755c7574696c735c6c697665757064617465735c67656e5f62696c6c626f6172645f7570646174652e7079740600000055706461746512000000730a000000000103010e0103010401'))
ON DUPLICATE KEY UPDATE
  updateName=VALUES(updateName), description=VALUES(description),
  machoVersionMin=VALUES(machoVersionMin), machoVersionMax=VALUES(machoVersionMax),
  buildNumberMin=VALUES(buildNumberMin), buildNumberMax=VALUES(buildNumberMax),
  methodName=VALUES(methodName), objectID=VALUES(objectID),
  codeType=VALUES(codeType), code=VALUES(code);

-- +migrate Down
DELETE FROM liveupdates WHERE updateID=8;
