-- Tutorial page actions — maps actionID → (actionTypeID, actionData)
-- Client expects columns: actionID, actionTypeID, actionData
-- actionTypeID maps to const.actionTypes in client (e.g. 1=OpenCareerFunnel, 2=SetRookieState, etc.)
-- +migrate Up

CREATE TABLE IF NOT EXISTS `tutorial_actions` (
  `actionID` int(11) NOT NULL DEFAULT 0,
  `actionTypeID` int(11) NOT NULL DEFAULT 0,
  `actionData` text DEFAULT NULL,
  PRIMARY KEY (`actionID`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Data to be populated per-server based on available localization messageIDs
-- See tutorial_pages.pageActionID for which actions each page triggers

-- +migrate Down
DROP TABLE IF EXISTS `tutorial_actions`;
