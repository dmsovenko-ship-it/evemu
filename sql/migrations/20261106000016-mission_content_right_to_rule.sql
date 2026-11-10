-- +migrate Up
-- Right to Rule (arc 5) mission content from the EVE University wiki.
INSERT INTO `missionObjectives` (`missionID`,`briefingID`,`objectiveType`,`itemTypeID`,`itemQty`,`npcCount`,`npcFactionID`,`structureTypeID`,`structureQty`,`objectiveText`,`briefingText`,`note`) VALUES
  (80096, 0, 1, 0, 0, 0, 0, 0, 0, 'Report to Kandus Sandar', 'For you, my friend, there''s always something.

Greetings, pilot. Her Majesty''s Ministry of Internal Order wants your aid.

My fellow agent, Kandus Sandar, is currently investigating Sansha activity in Kor-Azor, more specifically in the Miyan territories. Are you willing to lend help to his investigation?', 'Aiding_an_Investigator'),
  (80097, 0, 3, 0, 0, 5, 500019, 0, 0, 'Destroy all enemy ships and retrieve 1 Reports (0.1 m3)', 'Ah, a capsuleer, are you? Fortunate. 

I am Kandus Sandar, the Ministry of Internal Order officer in charge of Pirate Affairs in this area. Under normal circumstances, I would send you to one of our many agents for proper work, but I have a special task for you . 

I am waiting for one of my investigators. He is now several hours overdue in delivering his reports. Fly out to his last known location. Despite whatever you may find there, I need those reports.', 'Late_Reports'),
  (80098, 0, 3, 0, 0, 5, 500019, 0, 0, 'Destroy all enemy ships', 'These reports suggest that a group of Sansha''s Nation ships were seen heading for a listening outpost owned by one Lord Touraj Miyan, a local Holder. The Sansha must have killed my agent to prevent this information from getting out. Head over to the outpost and give the Sansha a surprise.', 'The_Outclassed_Outpost'),
  (80099, 0, 3, 0, 0, 5, 500019, 0, 0, 'Destroy enemy ships', 'This needs to be taken care of quickly, <Character>. Another group of Sansha''s Nation ships has launched an attack. This time, they targeted a slave convoy belonging to - take a guess - Lord Touraj Miyan. 

However, we''ve got word that an unusual ship joined the Sansha in the attack. The transmission cut off before we could get concrete details on who or what it was, but I''m willing to bet it''s the commander of the bunch. The fleet is large, but should be a non-issue for a capsuleer. 

Stop the Sansha and their commander.', 'Raging_Sansha'),
  (80100, 0, 3, 0, 0, 5, 500019, 0, 0, 'Destroy enemy ships', 'Okay, we have his location. Follow the bookmark and take him down.', 'Cowardly_Commander'),
  (80101, 0, 1, 0, 0, 0, 0, 0, 0, '', '', 'arc5'),
  (80102, 0, 3, 0, 0, 5, 500019, 0, 0, 'Destroy enemy ships', 'Sansha''s Nation is targeting Touraj Miyan. Why? Miyan is a fool, yes, but the Sansha are unlikely to care about an ineffectual noble. The other Holders, however, are sharks. They can smell weakness. Miyan already lost about half of his domains back in YC 110. The Holders are just circling, waiting for their chance to take another bite. 

If it wasn''t for the efforts of Lord Miyan''s staff, he''d be dead already. His current security chief, Riff Hebian, is a smart but skeptical man, and difficult to sway. Head over to Miyan''s manor and convince Hebian that the Sansha are targeting Miyan directly. His help and trust would be invaluable.', 'Background_Check'),
  (80103, 0, 2, 19730, 1, 0, 0, 0, 0, 'Retrieve the Communications Logs (0.1 m3)', 'Touraj Miyan has a lot of people who hate him. They want him dead, want his land, or just want to embarrass him. Some of these people are more powerful than others. It will take a while to investigate them all.

First up is Lady Temari Jariza. Among Holders, she''s a nobody. She doesn''t even technically own any land holdings, just a small, floating citadel in Amdonen granted to her five-times-great grandfather for exceptional service in putting down a rebellion.

Unlike her ancestors, Lady Jariza actually has ambitions. Unfortunately for her, she''s not taken very seriously. She''s attempted to improve her standing through most of the common channels, ranging from patronage to marriage. She''s even tried to court Lord Miyan numerous times, but she has thus far been unsuccessful. It''s for this reason that she''s on the suspect list. She has motive, but I need to know if she''s had the means and opportunity. 

I need you to hack the encrypted communications array in her territory. This should provide you with logs of all the communications and transactions in recent weeks. Bring these logs back to me.', 'Longing_Leman'),
  (80104, 0, 1, 0, 0, 0, 0, 0, 0, '', 'I have a few contacts in Lord Darabi''s territory. Maybe something will turn up with this Jariza connection, but I doubt it.

I''ve learned of a more pressing lead for you. Remember those Amarr ships you ran into? It seems that one Lord Manik has been shopping around for mercenaries.

I''m giving you the meeting location I found while innocently perusing Manik''s records. There should be a mercenary contact. You''re going to find out who they are, and then we''re going to wipe them out. I''ll have more instructions when you reach the site.', 'Languishing_Lord'),
  (80105, 0, 3, 0, 0, 5, 0, 3056, 1, 'Destroy all enemy ships and The Outpost', 'I''m sending you the mercenary''s coordinates. This should be the base they''ve been operating out of. Wipe them out. Leave no opportunity to rebuild.', 'Razing_the_Outpost'),
  (80106, 0, 3, 0, 0, 5, 0, 0, 0, 'Destroy enemy ships, then retrieve Mina Darabi near the Amarr Chapel.', 'With the destruction of the mercenary camp, Lord Manik is denying he ever had any contact with them. I have a copy of your ship''s logs from their meeting, but it may prove difficult to pin him with any conspiracy charges. 

Good thing I''ve got a new lead. Remember Lord Darabi, Miyan''s rival and the new target of Jariza''s wooing? His daughter, Mina, has been one of my contacts. Mina says she has key information, but she will only tell me in person. 

This is where the fun part comes in. Daddy Darabi has kept Mina under tight surveillance because of a scandal that happened a few years ago. Thus, we have to stage a kidnapping. This is going to get bloody, but I trust Mina when she says her information is worth it. 

Fly to the Darabi estate where Mina is being kept and "kidnap" Mina Darabi. Any guards are expendable.', 'Ascending_Nobles'),
  (80107, 0, 2, 32097, 1, 0, 500019, 0, 0, 'Retrieve 1 x Rahsa, Sansha Commander (1.0 m3)', 'Mina has quite a story to tell. Jariza has been courting Lord Darabi for some time. Jariza''s been so good at winning Darabi''s trust that Mina became suspicious. Mina proved an amazing spy: She''s not only traced funds flowing from Darabi to Jariza, but she also found an interesting communication from one Rahsa Teff.

Rahsa Teff is a known felon and former Blood Raider. The MIO lost track of him a year ago. Rumor was another pirate group poached him. It sounds as if the rumor is true. Not only is Rahsa Teff with the Sansha, but you met him already. He''s the Sansha commander who kept eluding us. 

I''ve traced the source of Rahsa''s message. We know where he and the Sansha are located. Let''s end this. Eliminate the Sansha base, and capture Rahsa Teff.', 'Hunting_the_Hunter'),
  (80108, 0, 1, 0, 0, 0, 0, 0, 0, 'Choose your path', 'So this man, Rahsa Teff, was behind it all. Rahsa Teff was a native to this region and wanted Miyan out. He knew his Sansha buddies were attracting attention, so he used Jariza to pull funds from Darabi. Rahsa then gave the money to Manik to fund his little mercenary gang. Manik''s build up was a distraction the whole time, as were the Amarr ships you encountered when chasing Rahsa down. 

Now that we have Rahsa Teff in my custody, the question is what to do with him. We could keep him around for more questioning, but I''d just as soon blow him out the airlock. What do you think? Is his life worth it? 

Interrogation: Catching the Scent >><br>
So Rahsa has something more to say? Give my people a few minutes alone with him.... 

Silence Rahsa: The Nation''s Path >><br>
You''ve proven yourself to me time and again. Rahsa will be sent for sentencing immediately. I love it when the system works quickly.

By the way, I got a message from a Krethar Mann. Seems your efforts have attracted her attention. Can''t say I blame her. You did well. You''ve again proven yourself a stalwart friend of the Empire. Krethar Mann is currently in her ship in Oguser. You may want to try communicating with her remotely.', 'Fate_of_a_Madman'),
  (80115, 0, 1, 0, 0, 0, 0, 0, 0, 'Report to Krethar Mann in Oguser', 'You''ve proven yourself to me time and again. Rahsa will be sent for sentencing immediately. I love it when the system works quickly.

By the way, I got a message from a Krethar Mann. Seems your efforts have attracted her attention. Can''t say I blame her. You did well. You''ve again proven yourself a stalwart friend of the Empire. Krethar Mann is currently in her ship in Oguser. You may want to try communicating with her remotely.', 'Silence_Rahsa'),
  (80109, 0, 2, 32106, 1, 0, 500019, 0, 0, 'Retrieve 1x Sansha Command Signal Receiver  (1.0 m3)', 'So Rahsa has something more to say? Give my people a few minutes alone with him.... ah, there''s a message coming in now. 

Hm. I''ll be damned. You were right. Someone else was working with him. He managed to activate some sort of suicide short in his socket before we got a name out of him. Too bad for him he''s still useful to us.', 'Catching_the_Scent'),
  (80110, 0, 3, 0, 0, 5, 0, 32127, 1, 'Place the Homemade Sansha Beacon (1 m3) in the Linked Broadcast Array Hub, and destroy the ships that spawn.', 'Hang on just a second. They must have changed socket sizes….

There. I present you one decoy Rahsa. Not a bad piece of work, if I do say so myself. Whoever Rahsa''s friends were, they''re probably looking for our departed Sansha right now. This device will mimic Rahsa''s personal frequency: Just plug it in to a broadcast tower and you''ve got yourself an escapee Rahsa. Or the next best thing. 

There''s the perfect broadcast tower in Shaha. Place the homemade Sansha beacon in the tower, and then wait to see if "rescue" arrives. Then blow them out of the sky.', 'Falling_into_Place'),
  (80116, 0, 2, 32272, 1, 0, 500003, 0, 0, 'Retrieve Priomodial Biomass', 'Hello, Capsuleer. Rahsa spoke highly of your tracking skills. You should feel proud; his praise never came easily. And do not worry about Aralin Jick. He has not betrayed his blessed ideals. The Nation has had no influence over him for many years.

Myself? I am not Sansha, though for the purposes of our business I speak for their interest. And do keep in mind it would do no good to run back to the authorities to turn me in. You will find we are quite untouchable.

On to business. My associates are engaged in a certain project that you can contribute greatly to. What we require first is a supply of biomass. We cannot use standard clone-grade biomass; any common thug could acquire vats of that from the local medical station. We are looking for a variation known as “primordial biomass.” It is currently only used in controlled science labs for extremeophile research. I have the location of one such base.

Head to this location. One of my ships will meet you at the acceleration gate. Head to the science facility and retrieve the primordial biomass. You may have to assault the facility directly. Once you have the biomass, return it to our ship at the entrance. When it has been transferred to the ship''s cargo, I''ll contact you with the next mission.', 'A_Human_Body'),
  (80111, 0, 1, 0, 0, 0, 0, 0, 0, '', 'Harkan must be dealt with immediately. His knowledge of Kor-Azor security and security codes make him a serious threat. I''ll contact Kandus Sandar to get the MIO moving. Right now you need to arrest Harkan. He was granted a small fief upon his retirement. You have a good chance of catching him there.', 'Making_an_Arrest'),
  (80117, 0, 2, 32106, 1, 0, 500003, 0, 0, 'Retrieve command signal receiver', 'Well, well, our friend at the MIO has been busy. We had assumed the authorities would have lost interest with the death of Rahsa, but they have proven persistent in assaulting the remaining Sansha forces.

A very important flotilla was recently destroyed by the Amarr. This particular group was carrying a very important component to our plan: a command signal receiver. This is a very specialized and controlled piece of Nation hardware. Only a rare few Sansha are implanted with one, and upon a Slave''s death the device self-destructs. A number of very taught strings had to be pulled to procure this device, and we do do not want the Amarr to study the device in any detail.

This is the battle site. You will find the Sansha ships already destroyed, but the MIO ships are still surveying the wreckage. Eliminate the Amarr ships and find the command signal receiver. When the Amarr ships have been cleared, I will send one of my ships to pick up the device, same as before.', 'A_Metal_Mind'),
  (80112, 0, 1, 0, 0, 0, 0, 0, 0, 'Report to Riff Hebian', 'You must inform Riff Hebian of Harkan''s demise. He was very fond of the man, so he should be informed personally.', 'An_Unfortunate_End'),
  (80118, 0, 3, 0, 0, 5, 500003, 3491, 1, 'Hack communications array', 'There is one more required piece before we make our final play. As you have likely guessed we are building a clone, a doppelganger of Lord Miyan that we control. This is an extremely complex creation, as it must be both technologically sophisticated and detection-proof. To make the illusion truly convincing, we need one final thing: a mind scan.

There is one particular mind scan that is perfect for our use. The man is long dead, executed, but the government considered his unique viewpoint still worthy of study. He was killed by the same mind scan process used by you capsuleers. This mind scan is kept on a planet side facility for psychiatric research. It has one vulnerable point, accessible by hacking the system''s comm relay. 

Find the comm relay at this location. Hack into the relay, and we can pull the mind scan directly to our server.', 'A_Digital_Soul'),
  (80113, 0, 3, 0, 0, 5, 0, 0, 0, 'Destroy all Sansha ships', 'I have to apologize to you for my skepticism of your aims earlier. We''ve been increasing our security since the attack on Miyan''s manor. But we never imagined one of our own was a threat. You have to understand that many of us have nothing but reverence for the Commodore. To find out he was a traitor...

But he''s gone. The task of judgment is out of our hands now. 

Only one task remains. With the destruction of their base and the deaths of Rahsa and now Harkan, the remaining Sansha are panicking. They''re throwing a last ditch assault at one of Miyan''s facilities. Lord Miyan himself is perfectly safe. Per our heightened security measures, I''ve sent him to a hidden safe house far from there. 

The Sansha attacked my Lord and corrupted my predecessor. Annihilate them.', 'Panic_Response'),
  (80119, 0, 2, 3779, 1, 0, 500003, 0, 0, 'Deliver the fake body', 'Now for the last step. The remaining Sansha ships are assaulting a Miyan station even as we speak. Riff Hebian believes this is the final battle against the Sansha, so he has committed the majority of his forces. While they are away, you will attack our true target: Lord Miyan''s safe house.

Meet my ship at the entrance to the compound. It will provide you with Lord Miyan''s "replacement." Fight your way through the levels of the compound until you find his safe house. Destroy the structure and any ships present, and in the wreckage plant the replacement. The Amarr forces will find him in the rubble and believe that Miyan survived. Then your work will be complete.', 'Regal_Replacement'),
  (80114, 0, 3, 0, 0, 3, 500019, 0, 0, 'Destroy Harkan''s Behemoth Battleship', 'Harkan''s not dead! He''s just launched an attack on Lord Miyan''s safe house! Lord Miyan is in danger!

He''s already broken past the main acceleration gate. Harkan knows our systems inside out. Our automated defenses are useless. Worse, I''ve reports that several of our own security forces are helping him!

The safe house is in Choga. I need your help to rescue Miyan from Harkan''s forces. I can''t rely on my own security forces, because I don''t know how many more will turn.', 'The_Right_to_Rule')
ON DUPLICATE KEY UPDATE objectiveType=VALUES(objectiveType), itemTypeID=VALUES(itemTypeID), itemQty=VALUES(itemQty), npcCount=VALUES(npcCount), npcFactionID=VALUES(npcFactionID), structureTypeID=VALUES(structureTypeID), structureQty=VALUES(structureQty), objectiveText=VALUES(objectiveText), briefingText=VALUES(briefingText), note=VALUES(note);
-- +migrate Down
DELETE FROM `missionObjectives` WHERE `missionID` BETWEEN 80096 AND 80119;
