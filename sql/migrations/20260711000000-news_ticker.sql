-- News ticker items for holoscreen (CQ news feed)
-- +migrate Up
CREATE TABLE IF NOT EXISTS `srvNewsItems` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `title` varchar(200) NOT NULL DEFAULT '',
  `description` text NOT NULL DEFAULT '',
  `date` varchar(30) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`),
  KEY `date` (`date`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
-- +migrate Down
DROP TABLE IF EXISTS `srvNewsItems`;
