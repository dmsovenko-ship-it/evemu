-- +migrate Up
-- Syndication (arc 7) mission content from the EVE University wiki.
INSERT INTO `missionObjectives` (`missionID`,`briefingID`,`objectiveType`,`itemTypeID`,`itemQty`,`npcCount`,`npcFactionID`,`structureTypeID`,`structureQty`,`objectiveText`,`briefingText`,`note`) VALUES
  (80146, 0, 1, 0, 0, 0, 0, 0, 0, 'Report to Gian Parele', 'Impetus needs your help with our next holoreel. Can you go meet with one of our directors? If you do a good enough job, I''m sure we get you in the credits for that reel. You can find him over in Tolle. Look for the studio we have set up in that system. The director is probably in his pleasure yacht. You know, the holoreel lifestyle can be really difficult sometimes.', 'Impetus'),
  (80147, 0, 3, 0, 0, 5, 500022, 0, 0, 'Clear the area of any threats and ensure the location is safe for filming to begin', 'Ah, the famous <Character>. Welcome! I don''t get to mingle with an egger every day, so it''s an honor to finally meet you face to face -- or close enough to it anyways. 

I assume you''ve come to help with my next holoreel project here at Impetus? Yes, well, quite a few setbacks already, I''m afraid. As someone like you might appreciate, I didn''t fully understand what I was getting into when I decided to film everything in space. Things have become a little more difficult than I expected. We could really use an experienced capsuleer like you to keep everything on track. Frankly, I don''t know why we didn''t consider the option sooner. 

If you''re able to help out, then your first task would be to investigate a possible filming location. We''re not expecting any major problems, but after an accident last week scouting a similar location, I''d rather have these matters handled by someone who really knows what they''re doing. I don''t suppose you''d be free and willing to scout the area?', 'The_Tolle_Scar'),
  (80148, 0, 3, 0, 0, 4, 0, 0, 0, 'Escort Senator Ardanne''s son safely to the film set', 'The shining star in this holoreel project of mine is a fairly high profile individual, as you might expect. His name is Ralie Ardanne; the son of a low-profile Senator. He''s a real knockout at fourteen years old, and he already has a budding endorsements empire with all the major clothing labels. Now he''s looking to expand into holoreels, which is where Impetus comes in. His parents have prospered from senatorial life, and they spared no expense on Ralie''s cosmetic enhancements. It shows too: He really makes a mark in the 12-16 year old female demographic. Of course, a boy his age and with his connections shouldn''t be travelling the spacelanes unguarded. I was hoping you''d be interested in meeting him at his father''s senatorial residence? It wouldn''t be every day you get to see a place like that, <Character>, even as an egger. What do you say?', 'Priority_One'),
  (80149, 0, 3, 0, 0, 5, 0, 0, 0, 'Destroy hostile ships in the area', 'We''ve hit upon a lead for you to follow, <Character>. Our proximity to the Amarr border attracts just the sort of criminals who might have masterminded this terrible act. People smugglers, pimps, slaveholders: There are all sorts of dark corners in the Elalan constellation, with darker motives strewn throughout these recesses.

There''s a place not far from here in the Averon system known as “The Exchange.” It''s a hangout for lowlifes and criminals, a real nasty place. After talking with some friends in The Scope they pointed us in that direction. Seems this Minmatar crew -- the Pator 6 -- is known around these parts for raiding unwary travelers. 

Your assignment is a dangerous one, <Character>. You will need to fly into the trading hub and see if there are any further leads. Maybe even one of those Minmatar gang members is still flying around. Are you up to the task?', 'The_Averon_Exchange'),
  (80150, 0, 1, 0, 0, 0, 0, 0, 0, 'Report to Eron Viette', 'I''m glad to see that you''re willing to stick around with us on this, <Character>. The person I''ve been instructed to refer you to is our Regional Director. I don''t have his current location, but I can put you in touch with one of his secretaries. You can find her at Impetus offices in Stetille, Planet 2. 

I can tell them you''re on the way, if you think you have time to make the appointment?', 'A_Different_Kind_of_Director'),
  (80151, 0, 3, 0, 0, 5, 0, 0, 0, 'Destroy hostile ships', 'Welcome to Stetille, <Character>. My name is Eron Viette. Gian said you''d been invaluable to us recently, so if there is any way I can further assist you, please let me know. 

I understand that you''re seeking a meeting with our Sinq Laison Regional Director. That would be Mr. Roire, Audalle Roire. He spends most of his time at our space-bound office complex here in Stetille. I''m afraid he''s not very fond of the media, and since they have free docking access to any of our publicly listed stations, you can most often guess he won''t be in any of them. 

I''ll patch the coordinates to the offices if you like?', 'Assistance'),
  (80152, 0, 1, 0, 0, 0, 0, 0, 0, 'Choose your path', 'There are two ways we can go about this situation, <Character>. The Regional Director has instructed me to offer you these two options. He would like you to know that this is a very important decision; he would also like to wish you good luck in your choice. 

[[Into the Black]] >><br>
Due to the serious nature of this situation, our Regional Director greatly suggests contacting the authorities for further assistance. The FIO have a very strong presence in the area, and the Black Eagles have been extremely helpful to us in the past. I recommend talking to Mourmarie Mone, the Constellation Director for the Black Eagles. She is located in Noghere. Here are the coordinates of her office. Good luck, and thank you for your continued business with Impetus.

[[Outside the Scope]] >><br>
Although our Regional Director would like this matter taken to the authorities, there is a more discreet option for handing this delicate situation. As a universal leader in the entertainment industry, Impetus maintains many close ties with other media outlets. One of our many partners is the Scope, who has been very kind in reviewing our various blockbuster holoreels over the past few years. I can send you to Veine Coructie, a local editor for the Scope. He is very trustworthy, and he can provide a lot of information about suspicious activity in the area.

{{co|red|I must warn you that choosing this route will take you into low-security space. Though I do not recommend this choice, there may be more lucrative opportunities with the Scope than here with the FIO. Any news agency would pay dearly for the information you have, and the Scope is one of the biggest news agencies around. However, there are more risks involved with this option.', 'The_High_or_Low_Road'),
  (80153, 0, 1, 0, 0, 0, 0, 0, 0, 'Report to Veine Coructie in Vitrauze System', 'Although our Regional Director would like this matter taken to the authorities, there is a more discreet option for handing this delicate situation. As a universal leader in the entertainment industry, Impetus maintains many close ties with other media outlets. One of our many partners is the Scope, who has been very kind in reviewing our various blockbuster holoreels over the past few years. I can send you to Veine Coructie, a local editor for the Scope. He is very trustworthy, and he can provide a lot of information about suspicious activity in the area.

{{co|red|I must warn you that choosing this route will take you into low-security space. Though I do not recommend this choice, there may be more lucrative opportunities with the Scope than here with the FIO. Any news agency would pay dearly for the information you have, and the Scope is one of the biggest news agencies around. However, there are more risks involved with this option.', 'Outside_the_Scope'),
  (80154, 0, 2, 32200, 1, 0, 0, 30786, 1, 'Place 1x Covert Recording Device (1.0 m3) in Storage Warehouse at Pator 6''s hideout', 'Our sources have caught wind of the situation, <Character>, though I''m glad you contacted us. We won''t go public with this information yet, as that would be against our ethics as journalists. However, we will aid you in uncovering information on the missing child. Senator Ardanne has not always been kind to the media, but there''s no reason to hold a grudge against him in a time like this.

We''ve been keeping tabs on the Pator 6 for a while now. They''re mostly low-level criminals, but their name has been popping up a lot recently, especially in relation to kidnapping cases. I haven''t seen them do anything this high profile, though.

Our first course of action is to see if we can plant a bug in a known Pator 6 hangout. Hodts Ergre, one of our more experienced investigative reporters, managed to acquire an access code to this place. We''d like you to place a covert recording device in their hideout. Hodts will open the gate, and you''ll sneak in and plant the goods. This is a stealth operation, so it would probably be best if you fly something inconspicuous. I''m not saying you should go out there completely defenseless; as you know there are more pirates around here than just the Pator 6.

{{co|red|You can take any ship you want, but I highly recommend a frigate-sized vessel. A battleship will definitely tip them off that you‘re up to no good.', 'Hidden_Camera'),
  (80155, 0, 2, 32220, 1, 0, 0, 0, 0, 'Retrive 1x "Ralie Ardanne''s Belongings" (1.0 m3)', 'The bug appears to be working well. We''ve been picking up a lot of messages from the Pator 6. For such a relatively small-time gang, they have quite a large network established. I don''t know why we haven''t noticed them earlier. We''ll have to keep this bug going for a bit. There are a few breaking stories in there, or at least some info we could sell. 

While the Pator 6 has a rather large foothold throughout the region, we heard one system repeated over and over again: Palmon. I asked some of my sources in that system, and they sent me the coordinates of some sort of junkyard with a lot of traffic from unlicensed Minmatar pilots. I have a distinct feeling that the kid is there, or at least he was at one point... 

Hang on. We just intercepted a message from our bug. Looks like the Pator 6 is expecting company soon. Time to crash the party. See who the meeting is with, and try to bring back something we can pin on the gang. We need evidence that Ralie was there if we''re going to bust this case open. Gotta warn you, though: A battleship won''t be much good here. From what I''ve seen, they have a means of keeping battleships out of their hideouts. Thus, don''t take one on this mission.', 'Rendezvous'),
  (80156, 0, 2, 32223, 1, 0, 0, 0, 0, 'Deliver 1x Kidnapping Evidence (0.1 m3) to ''Scope'' satellite station', 'Truth is the fundamental goal of any good reporter, and we''ve uncovered enough of it to create a media storm. The power of the press is unprecedented in its ability to sway the general public, create massive sympathy for an unknown issue, and to create a massive backlash against nefarious groups. When we present this evidence on the holovids, there''s no hope for the Syndicate. Their reputation will be ruined if anything happens to the kid. This is the power of the media.

We have a satellite station that we like to use for these important broadcasts. It''s safely hidden in a secret location in case there''s any blowback from our special reports. Take this evidence and drop it off at the station. We''ll use that to raise awareness of this case, blow it right open, and turn the wrath of the unwashed masses on the Syndicate.', 'Handoff'),
  (80157, 0, 1, 0, 0, 0, 0, 0, 0, 'Report to Ascain Adeset in Mesybier System', 'That Black Eagle agent gave me the name of a TransStellar Shipping employee: Ascain Adeset. Looking in to our records, she''s had a pretty nasty past, especially in the human trafficking business. She had a rather spectacular arrest about twenty years ago. Looks like she''s been clean for a few years and is doing some legit business now. I''ll bet that she turned rat in prison. 

Ascain''s currently located in Mesybier. Best to follow the Feds and talk to her.', 'With_Authority'),
  (80158, 0, 1, 0, 0, 0, 0, 0, 0, 'Report to Mourmarie Mone', 'Due to the serious nature of this situation, our Regional Director greatly suggests contacting the authorities for further assistance. The FIO have a very strong presence in the area, and the Black Eagles have been extremely helpful to us in the past. I recommend talking to Mourmarie Mone, the Constellation Director for the Black Eagles. She is located in Noghere. Here are the coordinates of her office. Good luck, and thank you for your continued business with Impetus.', 'Into_the_Black'),
  (80159, 0, 1, 0, 0, 0, 500002, 0, 0, 'Enter the colony and track down any information you can about the Pator 6', 'No time for small talk. We have a very serious situation on our hands, and I need the utmost discretion from you. That kid may be a goner if we don''t act fast. Unfortunately, we don''t know much about this “Pator 6” gang except for a few rumblings here and there. A lot of pirates use this area as a haven. Most of them are small-time criminals, no more than thugs looking for an easy mark. The Serpentis have some hold in this area, though I doubt we''re looking at any sort of leech on the underbelly of the booster industry. No, this looks suspicious, and not like Serpentis at all. This sounds personal, or at least desperate. 

Alright, so it looks like we''ll be fishing for information. Since it was some Minmatar thugs that took the senator''s kid, I guess we''ll pound some Matari heads in first. This region is crawling with those people. I don''t know who is a bigger drain on the Federation, the criminals or the Minmatar. 

Anyway, there''s a colony of them in Charmerout. Go in there and start shooting up buildings. Hopefully that''ll scare up some leads for us. If not…well, we''ll have fewer Matari clogging up the Federation resources. Frankly, I''m fine with that.', 'Poor_Mans_Shakedown'),
  (80160, 0, 2, 32193, 1, 0, 500020, 0, 0, 'Retrieve the Ringmaster (1.0 m3) from her pleasure hub, and bring her back to Mourmarie Mone.', 'The Underground Circus represents everything wrong with our Federation, the exploitation of personal liberties for the sick desires of depraved “citizens.” True Gallente have respect for themselves and treat their freedom with dignity, respect, and class. I should have known that the Circus would have something to do with this kidnapping business: Children are commodities in a dump like that. 

Looks like it''s time for us to bring in the Ringmaster, the owner/operator of the Underground Circus. Don''t let her name fool you: She''s ruthless, cunning, and manipulative. I''ve worked with her before, back when she was a legit businesswoman. She''s degraded into a broker of sin and filth, a mockery of what she once was. Find her and bring her to me. With a bit of heavy questioning, we''ll get the right information from her.', 'Underground_Circus'),
  (80161, 0, 3, 0, 0, 5, 500009, 0, 0, 'Investigate the lead on the Pator 6. Destroy everything in sight.', 'After a bit of interrogation, the Ringmaster gave us the information we''re looking for. Funny, we barely put any effort into it: She succumbed pretty quickly. Oh well. Since we have her in custody, we''ll use her for more questioning later, just as soon as she''s rested up and ready for another round.

Anyway, based on her information, we''ve learned of a nearby Intaki settlement that has some loose association with the Pator 6. You know what the Intaki are like — one big mess of criminals. I don''t believe the lies they tell us in the media: All Intaki are a part of the Syndicate, directly or not. If it were up to me, I''d lock all the Intaki up for good, or maybe just send them all back to their cesspool of a home planet. 

Go to Pemene and find those Intaki. Keep an eye out for the Pator 6, and report back with any information you receive about the senator''s son. You may have to open fire on their settlement in order to get them to talk. If you run into any opposition, use force. You have the authorities on your side.', 'Intaki_Chase'),
  (80162, 0, 1, 0, 0, 0, 0, 0, 0, 'Report to Ascain Adeset', 'If we''re going to get to the Syndicate, then you''ll have to go undercover for us, <Character>. Otherwise, they would see us coming from an AU away.

Not to worry, though. I have a person in mind who can help: Ascain Adeset. We nabbed her a few years back for human trafficking. She was on some crazy booster as well, and took out four of our cruisers before we brought her down. We lost a lot of good officers in that raid. After serving a few years in prison, she saw the light and joined our side. Now she''s one of our best informants in the area. 

I planted her in TransStellar to ensure that she keeps that company legit. We have a very close eye on her in case she decides to change sides again. If she does, she''ll be lucky to make it back to prison at all. 

You can find her in Mesybier.', 'Rat_in_a_Corner'),
  (80163, 0, 1, 0, 0, 0, 0, 0, 0, 'Choose your path', 'Mourmarie sent you? How sweet. She hadn''t used me in a while, but this kidnapped kid is kinda big. That''s why the Black Eagles have pros like me on the, er, team. I''m in the freight business, and I''m completely legit now, I swear. My specialty used to be in sentient biomass freight, if you catch my drift, but now I focus on moving livestock. Not nearly as profitable, but it''s honest. 

Trafficking kids is a tricky business. Yeah, they''re small, but people have a soft spot for a sad child, so you gotta find places where nobody will notice them or care. I know a few of these places around these parts, but this is grasping at straws, really. The kid could be anywhere at this point. 

Check out one of these places, and I''ll send some of my friends to the others. Some eggers like you owe me favors. So take your pick. 

[[Octomet Plantation]] >><br>
What most people realize is that slavery exists everywhere around New Eden, not just in the Amarr Empire. Except around here we don''t call it slavery; we call it farming. Children are great for the big bio-farms out in the border regions and tucked away in deadspace pockets. They can work the fields at practically no cost, and you barely have to feed them. Since they''re kids, they don''t get no paycheck. Plus, you can raise them like cattle. It''s a great racket, if you''re into that type of thing. 

There''s a plantation nearby called the Octomet Plantation. From what I''ve heard, it''s connected to the Syndicate, but who knows? All I know is that if you want to hide a kid, there''s no better place than a child farm. To me, all kids look alike anyway. 

Take a gander around the plantation and see if Ralie is there. Seeing as he''s a high profile kid, odds are he''s in their mansion. I could be wrong about all this, of course. But either way, you''ll be taking out some bad guys, right?  You''ll probably have to fly to lowsec to get to this plantation. Gotta warn you beforehand so you don''t come blaming me if some pirate takes you out when you''re not looking. Better fit a battlecruiser or heavy frigate. You know, something sneaky. 

[[Little Fingers]] >><br>
Though I don''t think it''s the Syndicate''s style – too obvious, if you ask me – there is one place very near here that could be holding the kid: the Rattler Row.

Rattler Row is, for lack of a better word, a sweatshop. Child labor is more common than you think, and not just at child farms like the Octomet Plantation. Much of the manufacturing produced throughout New Eden is based on the exploitation of the young. Is it legal? Debatable. Is it profitable? Absolutely. It''s also the bread and butter of human smugglers, although sweatshop owners refer to them as “recruiters.”

The Rattler Row sweatshop focuses on manufacturing ammunition, though it''s not always the legit product. They''re really a front for counterfeit ammo, which is shoddily produced and sold to rubes all over the black market. Dangerous stuff, really. Kids seem pretty adept at making ammo, though, so they keep the product available at incredibly low costs.

Ralie could be there, working on the factory floor. Explore the Rattler Row. Look for any signs of Ralie. You''ll probably run into some of their guards, so take them out. It''s a long shot, but it might be worth it.

[[Oldest Profession]] >><br>
A strange habit found among body smugglers is to hide stuff at brothels. I don''t know exactly why they do this. Maybe the flesh trade, whatever they''re dealing with, just sort of attracts itself. I don''t know. 

Anyway, there''s a brothel that has been known to hide some kids. Not for working there, mind you: The flesh trade can be seedy, but they''re not that immoral. At least, not that ones I know of. The place is called The Spintrix. Real nasty little place. I might have frequented there in my younger days, but their standards have plummeted. The men working there are way below even my meager standards. 

Go to Archavoinet and visit The Spintrix. Poke around there and see if the kid is there. You may get some aggression from their clientele – they''re practically animals – so take them out. If a whore loses business, it''s no skin off my back. {{co|red|You will have to fly through lowsec to get to this destination. It isn''t far, and The Spintrix is actually in a highsec system. If you think you can run the gauntlet in your current ship, go ahead, but watch out for pirates. Don''t say I didn''t warn you.', 'Places_to_Hide'),
  (80164, 0, 1, 0, 0, 0, 0, 0, 0, 'Find the Carry On Hub and approach it.', 'Ok, ok. Don''t tell Mourmarie, but I''ve been holding out on her. She''s wrung me out of most of the info I have, especially some of the juicier leads from my former smuggling days. But this place is the mother lode, a veritable smorgasbord for the human traffickers in this corner of New Eden. You can''t tell anyone about this place, though: A rat is only good if nobody knows she exists, if you catch my meaning. 

The Carry On is a smuggler''s haven, a collection of speakeasies and underground associations devoted to human trafficking. It was a home away from home for me back in the day. I don''t think I can show my face there anymore, though. Too many enemies, too many friends. But you can.

Your best course of action is to find the Carry On hub, located at the heart of the complex. You''ll probably meet some of the guards there, or even some smugglers looking to protect their own. If they hit you, strike back. Smugglers are cowards at heart. Approach the hub and see what you can find there. Attack it if you must. This pains me to think about it, but it''s our only choice.', 'Carry_On'),
  (80165, 0, 3, 0, 0, 5, 500009, 0, 0, 'Destroy Studio I', 'Wow. I''ve never heard of this before. Studio I must be a recent development among traffickers. I really don''t know much about it, but something feels off about the situation. I mean, I''ve been a part of some really nasty enterprises in my time – trafficking for drugs, medical experiments, slavery, you name it – but something like Studio I? No way; nothing like that. 

This looks like a joint operation between Impetus and the Syndicate. I don''t know how close that relationship is, or how high up on the corporate chain it goes. But we know the kid is there. Get in there and get him. Destroy the Studio, too. Nothing good can come from it.', 'Studio_I'),
  (80166, 0, 2, 32292, 1, 0, 500002, 0, 0, 'Retrieve 1x Ralie Ardanne (1.0 m3)', 'You''ve really pissed off some nasty bad guys, my friend. Luckily, they''re starting to turn against each other as well. That studio was the lynchpin in this whole operation, it seems, and its destruction unraveled all their carefully laid plans. 

The Pator 6 is flying to Adacyne, and they''ve got Ralie with them. Time for a final showdown. Go knock them out and get the kid, once and for all. I''ll start working on my contacts in the Syndicate and see if there''s anything that can be done from their angle. Last thing we want is to have them go after me. Or us.', 'Showtime'),
  (80167, 0, 1, 0, 0, 0, 0, 0, 0, 'Choose your path', 'You''ve come a long way, <Character>. You found the kid, defeated the bad guys, and saved the day. Right out of a holoreel, right? Well, here''s where real life comes in, and it''s a complicated mess. They don''t show you this part in the ‘reels. 

While you were out taking care of the Pator 6, I contacted my Syndicate contacts. They''re pissed, but they are willing to forget everything and leave us alone. They can still keep their deal with Impetus, but they need the kid. Apparently, Ralie''s the crux of the deal. I have no idea why, nor do I want to know. 

Thus, you have two options. First, you can do the right thing and give Ralie back to the Feds. Be the hero, save the innocents, gratitude heaped upon you by the Federation, the works. I''m sure Mourmarie has something fun to reward you for your assistance. 

Or you can give Ralie back to the Syndicate. They''ve given me a very generous offer. Very generous. You''re essentially signing an innocent kid''s life away, but the money will be good. That''s all you eggers care about anyway, right?

All I can say is this: You can give the kid his freedom and be the hero. Or you can give him away, an innocent kid to ruthless and depraved criminals, for a lot of money. I won''t judge you on either choice, but I have to ask: Where do you draw the line? 

[[Safe Return]] >><br>
Mourmarie and Senator Ardanne have given me a rendezvous location for you. It''s somewhere in Enedore. When you return the kid, you''ll get your reward. I''ll contact the Syndicate and let them know that the deal is off. I wouldn''t expect them to take that lightly, though, so watch your back. 

Either way, glad you got the kid back. Been nice working with you. Good luck in the future, <Character>. 

[[Everybody Has a Price]] >><br>
The Syndicate wants to meet you at a specific location. You''ll drop the kid off, take the money – which I''ll get a cut of, of course – and leave quietly in the night. They have a special gift for you in addition to the money. Some sort of Syndicate-issue something or other. I dunno. 

Take the kid to the warehouse and drop him off. We''ll get paid and leave this whole mess behind us. It''ll be a happy ending for us, to be sure.

----

The Dark Frontier<br>
New Eden is a dark place, populated by killers, thieves, heroes, lowlifes, saints, sinners, and everything in between. Morality is what you make of it. But when push comes to shove, heroism and villainy are measured by one thing: ISK, the heart and soul of New Eden.', 'Wheres_the_Line'),
  (80168, 0, 2, 32292, 1, 0, 0, 0, 0, 'Take Ralie Ardanne (1 m3) to the warehouse and drop him off.', '', 'Everybody_Has_a_Price'),
  (80169, 0, 1, 0, 0, 0, 500009, 0, 0, 'You''re giving Ralie Ardanne back to the authorities.', 'Mourmarie and Senator Ardanne have given me a rendezvous location for you. It''s somewhere in Enedore. When you return the kid, you''ll get your reward. I''ll contact the Syndicate and let them know that the deal is off. I wouldn''t expect them to take that lightly, though, so watch your back. 

Either way, glad you got the kid back. Been nice working with you. Good luck in the future, <Character>.', 'Safe_Return')
ON DUPLICATE KEY UPDATE objectiveType=VALUES(objectiveType), itemTypeID=VALUES(itemTypeID), itemQty=VALUES(itemQty), npcCount=VALUES(npcCount), npcFactionID=VALUES(npcFactionID), structureTypeID=VALUES(structureTypeID), structureQty=VALUES(structureQty), objectiveText=VALUES(objectiveText), briefingText=VALUES(briefingText), note=VALUES(note);
-- +migrate Down
DELETE FROM `missionObjectives` WHERE `missionID` BETWEEN 80146 AND 80169;
