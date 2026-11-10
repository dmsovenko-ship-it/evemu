-- Point the epic arcs at the real EVE agent rows (the 3019502.. wrappers
-- were synthetic and nameless; the real agents already exist with names).
-- +migrate Up
UPDATE epicArc SET startingAgentID=3019356 WHERE arcID=1;
UPDATE epicArc SET startingAgentID=3019465 WHERE arcID=3;
UPDATE epicArc SET startingAgentID=3019463 WHERE arcID=4;
UPDATE epicArc SET startingAgentID=3019415 WHERE arcID=5;
UPDATE epicArc SET startingAgentID=3019396 WHERE arcID=6;
UPDATE epicArc SET startingAgentID=3019412 WHERE arcID=7;
UPDATE epicArc SET startingAgentID=3019411 WHERE arcID=8;
-- +migrate Down
-- (arcs stay on the real agents)
