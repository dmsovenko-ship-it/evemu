-- Fix: typeID 23193 is "Takmahl Data Registry" (group 306), NOT a Standard Acceleration Gate.
-- Replace with 2902 (LCS Acceleration Gate, group 319) which exists in the SDE.
-- +migrate Up
UPDATE dunRoomObjects SET typeID = 2902 WHERE typeID = 23193;

-- +migrate Down
UPDATE dunRoomObjects SET typeID = 23193 WHERE typeID = 2902;
