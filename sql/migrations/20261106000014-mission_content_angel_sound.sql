-- +migrate Up
-- Mission content fields + Angel Sound (arc 3) content built from the EVE
-- University wiki pages (Objective + Mission briefing templates).
ALTER TABLE `missionObjectives`
  ADD COLUMN `npcFactionID` int(10) unsigned NOT NULL DEFAULT 0,
  ADD COLUMN `structureTypeID` int(10) unsigned NOT NULL DEFAULT 0,
  ADD COLUMN `structureQty` int(10) unsigned NOT NULL DEFAULT 0,
  ADD COLUMN `objectiveText` varchar(512) NOT NULL DEFAULT '',
  ADD COLUMN `briefingText` mediumtext NULL;

INSERT INTO `missionObjectives` (`missionID`,`briefingID`,`objectiveType`,`itemTypeID`,`itemQty`,`npcCount`,`npcFactionID`,`structureTypeID`,`structureQty`,`objectiveText`,`briefingText`,`note`) VALUES
  (80058, 0, 1, 0, 0, 0, 0, 0, 0, 'Report to Abdiel Verat at K-QWHE V - Moon 4 - Archangels Assembly Plant', 'Hey, man. 

If you''re out here in a shithole like Egbinger, then maybe there''s a chance you can help me out with something. We just had a bunch of Republic Fleet guys dock up and repair here. They''d been out in Curse dealing with some Angel Cartel raiders. 

They were describing how the Cartel is having trouble right now with some other pirate groups. Seems there''s a Guristas commander tearing them a new hole down there, as well as some Sansha meandering about causing problems. From the sounds of it, the Cartel''s taking some significant losses. 

Anyways, I was hoping you could fly down and check it out. While the Fleet was docked up, I had my boys go over their comms system. We''ve got ourselves the name of a Cartel agent involved in the invasion. Abdiel Verat. 

It may sound strange to hear it, <Character>, but if you can find some way to deal with those Guristas and Sansha, I''d be thankful. The Cartel may give us grief, but they''ve done good things for the Minmatar people too, even if nobody has a damn clue why, or what''s in it for them. 

All I know is that I''d prefer the Cartel to Guristas thugs and Sansha freaks. If you can, go and deal with their problems. Just don''t go helping them too much, got it?', 'The_Balance_of_Power'),
  (80059, 0, 1, 0, 0, 0, 0, 0, 0, 'Report to Abdiel Verat at K-QWHE V - Moon 4 - Archangels Assembly Plant', 'Oh, great, another one.

Look, I don''t know where you''re getting your intel, but I''ve got absolutely nothing to do with the Cartel. I''m not your “connection” in any sense of the word. Some scumbag was envious of my service record in Curse and decided to come up with that little story to explain away my success. You''ve been played, <Pilot>. 

I don''t know anything more about the Cartel than the next person. I have no idea about any pirate invasions that they could potentially need help with. And I certainly don''t know that Abdiel Verat would be the person you should talk to if you wanted to know more. 

As you can see, I can''t help you, and even if I did have valuable information, it would be reported immediately to my superiors. I have a reputation to uphold. 

Now, if you don''t mind, I have important Navy matters to attend to. I hope you understand.', 'Mistaken_Identity'),
  (80060, 0, 1, 0, 0, 0, 0, 0, 0, 'Report to Abdiel Verat at K-QWHE V - Moon 4 - Archangels Assembly Plant', 'Stin''s the name, or at least, it''s the one I''m using for this operation. I''m here to recruit some fresh blood for the battles in Curse. Seems you lot have started to wander off right when we need capsuleers the most. 

If you''re interested, I can hook you up with Abdiel Verat. She''s the primary point of contact for capsuleers in Curse.', 'Headhunted'),
  (80061, 0, 1, 0, 0, 0, 500019, 0, 0, '', 'Abdiel Verat, Angel Cartel. 

I look out for talented people such as you who are interested in work. And times have certainly made our work interesting.

Since you''re an egger, I''m guessing you know your way around wormholes, yes? While they''ve been a great aid for our scientists, they''ve also brought some problems to our doorstop, some of which is not good for business. 

Our explorers have discovered a wormhole in a nearby system, but we cannot spare the manpower to lock it down. If you help us out by scouting the area, then I may have other more lucrative tasks to follow.', 'New_Opportunities'),
  (80062, 0, 1, 0, 0, 0, 0, 0, 0, 'Choose your path', 'At the moment I have more work than I have people, so you need to choose how you want to proceed. 

We also have a shipment of important research material that needs to make it to Serpentis hands. The Inquest scientists in charge of the operation have been delayed, and moving the goods is a high-priority task we need someone to take over. We may also require an investigation into the reasons for the delay at Serpentis'' end. 

Both tasks need to be followed up. I don''t care what you choose; we''ll give one to you, and one to the next egger brave or stupid enough to show up down here. 

[[Natural Consequences]] >><br>
Obviously, the Sansha need to be cleared from our wormhole site. If you can prove yourself on this, chances are I''ll be able to request more assignments to give you. We have early reports of more problems abroad, so I''m sure we can keep you moving from target to target out there.


[[Serpentis Fill-In]] >><br>
Perhaps it''s not worth the trouble to take down the Sansha encampment. They might have already gone in and destabilized the wormhole. We play by risk and reward, and the reward here looks like it''ll be too low. 

There''s a high-priority shipment that has been sitting around waiting for someone capable enough to move it. It''s headed to the Serpentis research facilities in Fountain, but Sarpati''s crew is going to handle moving it from Curse over to their Headquarters. They were supposed to take it directly from this station but we''ve lost contact with the research team in charge of that operation. The job is yours if you want it. All you need to do is get it to nearby Serpentis Corp. transport hub, from there they''ll handle the rest. 

The cargo is small so you could easily fit it inside an interceptor or covert ops and I would suggest you do just that, <Pilot>. I will not accept any excuses for the cargo''s disappearance. 

Bedfellows<br>
The partnership between the Angel Cartel and the Serpentis is a perfect match. The Cartel has all the things the Serpentis lack: massive manpower, a gigantic network of dealers and thugs, a cold vicious enthusiasm for crime, and the organizational skills needed to help spread the Serpentis supply of boosters to new markets. Without the Cartel, the Serpentis would never have expanded to become as powerful and pervasive as it is today, and many say, the reverse is also true.

Despite all the success the partnership has brought the Serpentis, it may be the Cartel who got the better end of the bargain. Not only do the Serpentis supply the Cartel with the most-addictive and highest quality boosters in the cluster, they also provide unparalleled research. The Cartel has long had access to dozens of ancient Jovian ruins, but previously lacked the expertise to truly take advantage of them. The Serpentis changed all that, and many think the Cartel''s latest ship designs – and many other secret breakthroughs – come straight from Serpentis minds.', 'Fight_or_Flight'),
  (80063, 0, 2, 32393, 1, 0, 0, 0, 0, 'Deliver 1x Sealed Research Cache (2 m3)', 'Perhaps it''s not worth the trouble to take down the Sansha encampment. They might have already gone in and destabilized the wormhole. We play by risk and reward, and the reward here looks like it''ll be too low. 

There''s a high-priority shipment that has been sitting around waiting for someone capable enough to move it. It''s headed to the Serpentis research facilities in Fountain, but Sarpati''s crew is going to handle moving it from Curse over to their Headquarters. They were supposed to take it directly from this station but we''ve lost contact with the research team in charge of that operation. The job is yours if you want it. All you need to do is get it to nearby Serpentis Corp. transport hub, from there they''ll handle the rest. 

The cargo is small so you could easily fit it inside an interceptor or covert ops and I would suggest you do just that, <Character>. I will not accept any excuses for the cargo''s disappearance.', 'Serpentis_Fill-In'),
  (80067, 0, 3, 0, 0, 5, 500019, 0, 0, 'Eliminate the Sansha''s Nation blockade', 'Obviously, the Sansha need to be cleared from our wormhole site. If you can prove yourself on this, chances are I''ll be able to request more assignments to give you. We have early reports of more problems abroad, so I''m sure we can keep you moving from target to target out there.', 'Natural_Consequences'),
  (80068, 0, 3, 0, 0, 5, 500010, 0, 0, 'Drive off the Guristas raiders', 'Okay, the exploration team is away. If the Serpentis eggheads are right, they should arrive in Fountain securely and in record time.

Speak of the devil. I''m getting a report that one of our shipments coming back from Fountain just got hit by Guristas. It''s always something.

There''s a salvage crew nearby that can pick up the tech, but they aren''t equipped to handle Guristas. If you clear out the Guristas, I can send in the recovery team. As always you''ll be well paid.', 'Clear_the_Way'),
  (80064, 0, 3, 0, 0, 5, 500010, 0, 0, '', 'I just discovered why that research team never arrived to pick up the goods you ferried: their labs are under siege by Guristas forces. I don''t know what the hell the Guristas think they''re going to achieve down here, but if we don''t quickly shut them down we risk them gaining a foothold.

This attack was fierce and well-coordinated. They managed to overwhelm our defenses. All of our other forces are either too far away to assist or on other important missions they can''t be diverted from. You''re our best shot. I want you to save that research station from the Guristas assault.', 'Ride_to_the_Rescue'),
  (80065, 0, 3, 0, 0, 5, 0, 0, 0, 'Eliminate all hostile targets then approach Central Database Hub', 'Alright, you''re ready to take care of this Guristas commander? Good. 

Yukiro Demense is a capsuleer. With all those clones, he''ll be a tough one to break, but we have plenty of experience when it comes to dealing with this sort of problem. Our friends over at Serpentis Inquest were also more than willing to offer their expertise on this matter after the Guristas attacked their research facility. This Demense punk just made some very powerful enemies. 

You''ll be travelling to a local black market cloning facility, one that we''ve been happy to ignore until now. The facility administrator took a heavy bribe from Demense to allow one of his clones access, and so of course, they had their license and all good standings with us permanently revoked. Intelligence indicates that Demense is reliant on this facility to support his raids. With a clone backup available locally, he has been able to return to the fight without re-cloning in Venal up north.

Your first task is to storm this facility and make an example of the people there. Let nobody survive; their deaths must send a message across Curse. Once you have dealt with this indiscretion, you need to deliver a special data payload into their central database hub. As soon as you approach the hub, the data will auto-transmit a special virus engineered by our Inquest friends.

Do this and report back to me. I will have further instructions for you at that point.', 'The_Best_Kind_of_Revenge'),
  (80069, 0, 3, 0, 0, 5, 500010, 0, 0, 'Kill Gath Renton', 'Our biggest competition in the Tech III race is not the Guristas or the Sansha. Hell, it''s not even the empires. It''s your kind. Capsuleers. I''ll admit you guys have been giving us a run for our money. You''re uniquely suited to wormhole space. Thankfully a few of you have been less than careful in normal space. 

There''s a capsuleer named Renton that makes his home nearby. We know he''s been exploring wormhole space, and he''s likely been stockpiling Sleeper tech at this base. He''s only been a minor nuisance so far, but his tech cache could be useful. We would do the raid ourselves, but it''s more effective to have capsuleers take care of each other, much less bloodshed.

Find Renton''s base. Eliminate him. I''ll be able to send an another recovery team once you''ve wiped out all hostiles.', 'Salvage_Heist'),
  (80070, 0, 3, 0, 0, 5, 500019, 0, 0, 'Kill all', 'All that''s left to do is transport the tech you''ve helped us collect back up to Fountain. No, we''re not asking you to haul. We''ve got a crew for that. Your job is more exciting. 

We know there are Guristas still in the area, possibly with their Sansha pals. They want these parts badly enough to dare raiding in our territory. So, we decided to use their eagerness against them. We let the details leak about the timing of this shipment. They think they''ll be ambushing a defenseless industrial. What they''re going to find is you. 

Kill every vessel that shows up.', 'Rabbit_Trap'),
  (80066, 0, 3, 0, 0, 5, 500010, 0, 0, 'Kill Yukiro Demense', 'Demense is still in Curse. I''d say his superiors have become aware that the hammer of a thousand Angels is about to come crashing down on him and are keeping the fool out here on “deployment." 

Your task is to put the insolent little maggot down. Once he reanimates into that new clone, he''ll be a non-issue. As I said before, <Pilot>, the man made some powerful enemies. We have chosen our friends wisely, and if you only knew what they were capable of doing to your clone, you''d understand why we want them on our side. 

Go, kill him. If you succeed, perhaps I will share the true nature of Demense''s fate with you. 

Mission completed:<br>
Serpentis Inquest just confirmed the reanimation of his local clone. You''ve done well, <Pilot>. Loyalty and a capacity to deliver results, these are both admirable qualities that will get you far within our organization. 

You''ve caught the attention of my superiors, in fact. It seems they have a need for a capsuleer, and one who has already proven themselves trustworthy and capable. When you want the details for your new contact, I can pass them along. 

Demense''s fate? Let''s just say that he has contracted a disease which is going to follow him from clone to clone. Eternity is not all it''s cracked up to be.', 'Wrath_of_Angels'),
  (80071, 0, 1, 0, 0, 0, 0, 0, 0, 'Report to Kadeja Dulche in K-QWHE VII - Moon 8 - Dominations Assembly Plant', 'Ready to move on then, I take it? 

You''ll be working with Kadeja Dulche of the Dominations from here on out. Don''t ask me what he wants of you; it''s not my business to know these things.

You best get going. I''d thank you for the assistance you provided, but an invitation to a meeting with a Domination agent says more than any words of mine will. 

Declining a mission from a particular agent more than once every 4 hours may result in a loss of standing with that agent, except in the case of special missions that clearly state otherwise in their description.', 'Dominus'),
  (80072, 0, 1, 0, 0, 0, 0, 0, 0, 'Choose your path', 'You''ve done well for us dealing with the Guristas and Sansha. Clashes with other frontier organizations come with the territory, however, and are never that much of a problem. The Archangels are more than capable of handling this aspect of daily operations. It is when other entities begin to stray outside their usual boundaries and establish themselves in our space that people begin to become rightly concerned. In these situations, we are called in to deal with them.

One Minmatar corporation, Boundless Creations, has made the mistake of conducting research in our space. They have come here on the misunderstanding that, just because their research is illegal, we will turn a blind eye to their presence. We''ve not corrected them on this false assumption. Rather, we''ve let them move in all of their assets and staff and slowly build themselves up. The time has now come to reveal to them where they truly stand with the Cartel. 

Your mission is to destroy their research facility entirely, costing the interlopers billions. You can go about this whatever way you see fit. You can storm the facility head on, or weaken the defenses by knocking out their communications arrays first. 

Either way, everyone in that place needs to die. 

[[Situation Normal]] >><br>
Boundless Creations has a communications array keeping tabs on the research base. If you attack the base now, they''ll call in reinforcements and make your life difficult. 

The good news is that they''ve attempted to hide the array inside a gas cloud rather than protect it in the open. They expect it to be undetected, and to most people it would be. Our scouts have uncovered the area though, and have observed engineers living at the compound. They will be carrying a security code which you must recover. You will be able to use it to deactivate the array without alerting the main Boundless Creation fleet. 

[[Data Destruction]] >><br>
Going for the straight shot? You''re a pilot after my own heart.

The base is split into three sections. Each section contains a data center that houses all of Boundless Creations'' illegal research. You must destroy these. Each data center serves as a redundant backup for the others, so it needs to be all or nothing.

The Cartel is putting a lot of trust in you. Make it happen.', 'The_Lesser_of_Two'),
  (80075, 0, 3, 0, 0, 5, 500002, 32412, 1, 'Destroy the Boundless Creations Data Center Structure', 'The base is split into three sections. Each section contains a data center that houses all of Boundless Creations'' illegal research. You must destroy these. Each data center serves as a redundant backup for the others, so it needs to be all or nothing.
The Cartel is putting a lot of trust in you. If you''re going to storm the front, you better be ready to deliver.', 'Data_Destruction'),
  (80073, 0, 3, 0, 0, 5, 500002, 30965, 1, 'Disrupt Boundless Creations communications network', 'Boundless Creations has a communications array keeping tabs on the research base. If you attack the base now, they''ll call in reinforcements and make your life difficult.

The good news is that they''ve attempted to hide the array inside a gas cloud rather than protect it in the open. They expect it to be undetected, and to most people it would be. Our scouts have uncovered the area though, and have observed engineers living at the compound. They will be carrying a security code which you must recover. You will be able to use it to deactivate the array without alerting the main Boundless Creation fleet.', 'Situation_Normal'),
  (80074, 0, 3, 0, 0, 5, 500002, 32412, 2, 'Destroy the Boundless Creations Data Centers', 'You have little time to waste. The Boundless Creations research base is cut off from its support. It won''t be long before they figure out their communications aren''t getting through any more.

The base is split into three sections. Each section contains a data center that houses all of Boundless Creations'' illegal research. You must destroy these. Each data center serves as a redundant backup for the others, so it needs to be all or nothing.

The Cartel is putting a lot of trust in you. Make it happen.', 'Breaking_the_Lock'),
  (80076, 0, 3, 0, 0, 5, 500006, 0, 0, '', 'Aren''t you the lucky one? All the carnage you''ve inflicted on our enemies has caught the attention of SARO. They are the best that CONCORD has to offer, and they''re not to be balked at.

This particular SARO pilot''s name is Treonis, and she''s been asking for you personally. My guess? Taking you out is her bid at a promotion. 

Normally, we would launch a whole campaign to remove CONCORD scum from our space, but my superiors want to see you handle this situation. If you can destroy Treonis, I''m authorized to reward you with a single run blueprint for one of our rarer ships, the Cynabal.

Treonis is waiting for you by an abandoned colony in <System>. Destroy her, and the blueprint is yours. 

----

SARO<br>
Despite CONCORD''s strong association with empire space, they occasionally sponsor larger campaigns out in nullsec. These excursions are targeted at the "pirate" factions under the justification of returning the violence committed in high security space. CONCORD''s Special Affairs for Regulations & Order (SARO) is uniquely suited for these campaigns. Their training, hardware, and skill make them a threat not to be taken lightly.', 'Fear_of_Angels')
ON DUPLICATE KEY UPDATE objectiveType=VALUES(objectiveType), itemTypeID=VALUES(itemTypeID), itemQty=VALUES(itemQty), npcCount=VALUES(npcCount), npcFactionID=VALUES(npcFactionID), structureTypeID=VALUES(structureTypeID), structureQty=VALUES(structureQty), objectiveText=VALUES(objectiveText), briefingText=VALUES(briefingText), note=VALUES(note);
-- +migrate Down
DELETE FROM `missionObjectives` WHERE `missionID` BETWEEN 80058 AND 80076;
ALTER TABLE `missionObjectives`
  DROP COLUMN `npcFactionID`, DROP COLUMN `structureTypeID`, DROP COLUMN `structureQty`,
  DROP COLUMN `objectiveText`, DROP COLUMN `briefingText`;
