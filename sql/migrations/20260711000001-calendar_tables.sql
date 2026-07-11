-- Calendar system tables
-- These tables are required by CalendarDB.cpp but were never created.

CREATE TABLE IF NOT EXISTS `sysCalendarEvents` (
  `eventID` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `ownerID` int(10) NOT NULL DEFAULT 0,
  `creatorID` int(10) NOT NULL DEFAULT 0,
  `eventDateTime` bigint(20) NOT NULL DEFAULT 0,
  `dateModified` bigint(20) DEFAULT NULL,
  `eventDuration` int(10) DEFAULT NULL,
  `importance` tinyint(1) NOT NULL DEFAULT 0,
  `eventTitle` varchar(255) NOT NULL DEFAULT '',
  `eventText` text DEFAULT NULL,
  `flag` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `month` tinyint(3) unsigned NOT NULL DEFAULT 0,
  `year` int(10) unsigned NOT NULL DEFAULT 0,
  `autoEventType` tinyint(3) unsigned DEFAULT NULL,
  `isDeleted` tinyint(1) NOT NULL DEFAULT 0,
  PRIMARY KEY (`eventID`),
  KEY `ownerID` (`ownerID`,`month`,`year`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `sysCalendarInvitees` (
  `eventID` int(10) unsigned NOT NULL DEFAULT 0,
  `inviteeList` text DEFAULT NULL,
  PRIMARY KEY (`eventID`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `sysCalendarResponses` (
  `eventID` int(10) unsigned NOT NULL DEFAULT 0,
  `charID` int(10) NOT NULL DEFAULT 0,
  `response` tinyint(3) unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`eventID`,`charID`),
  KEY `charID` (`charID`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
