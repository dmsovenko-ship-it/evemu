-- +migrate Up
-- Penumbra (arc 6) mission content from the EVE University wiki.
INSERT INTO `missionObjectives` (`missionID`,`briefingID`,`objectiveType`,`itemTypeID`,`itemQty`,`npcCount`,`npcFactionID`,`structureTypeID`,`structureQty`,`objectiveText`,`briefingText`,`note`) VALUES
  (80120, 0, 1, 0, 0, 0, 0, 0, 0, 'Report to Katsen Isha', 'Well, that was half right. I''m just an intermediary; Katsen Isha is the man you need to report to. He asked me to arrange for some extra assistance with a few jobs he has in mind. 

I should warn you, though: sometimes business dealings with Isha can get a little involved. He''s the typical hard-working Deteis type and will expect the same of anyone on his payroll.', 'The_Intermediary'),
  (80121, 0, 2, 30967, 1, 0, 0, 0, 0, 'Deliver 1x Questionable Cargo (3.0 m3)', 'Welcome to Poinen, <Character>. I''ve been expecting you. In fact, I was quite pleased to have someone offer their services so soon after I put the word out. 

As I''m sure you know by now, my name is Katsen Isha. I work part-time as a lawyer and consultant for Nugoeihuvi Corporation here in the Onirvura Constellation, where much of our company''s most important business is handled. 

That brings me to the first task I have for someone, perhaps you. Some associates of mine have come across a fragile package that must be handled delicately. You needn''t take it far, and I''ve had your path cleared of any obstacles. Transport is simply a matter of discretion on your behalf. Could you deliver the cargo to our headquarters in Josameto?', 'Trust_And_Discretion'),
  (80122, 0, 3, 0, 0, 5, 500020, 30956, 1, 'Destroy the Hyasyoda Research Facility and recover 1x S.I Formula Sheet (0.1 m3).', 'Welcome back, <Character>. Now, before we get to anything else there''s the matter of money. If I''m going to be paying for your services, then securing some funds is our first priority. My knowledge plus your capabilities equals ISK. 

I want you to acquire some valuable formulas for me. They''re hidden away inside a deadspace research facility not far from here. I managed to come across the coordinates after a booster-addled Hyasyoda employee let slip about an arrangement to one of my associates. It seems her superiors are allowing criminal types to carry out illegal research in exchange for a cut of the profits…and clearly some of the merchandise too. 

As it turns out, our friends in Sukuuvestaa are the legal owners of the space where this little operation is underway, something that certainly helps our situation. They''ve agreed to have Peace and Order security forces busy themselves with other things while you get to work. 

Very foolish of Hyasyoda to mix themselves up with criminals in someone else''s backyard, I''m sure you''ll agree. I like to see it as an opportunity, however. Not only will we be securing State borders, but also teaching them a valuable lesson about owning one''s choices fully. What do you say?', 'Their_Loss_Our_Profit'),
  (80123, 0, 1, 0, 0, 0, 0, 0, 0, 'Choose next mission', 'Apparently Hyasyoda are more furious than I anticipated. I''m thinking we should probably move the formula sooner rather than later. I don''t exactly want their Corporate Police Force prying into our business. 

I''ve already lined up a buyer for us. All you need to do is move the goods. There''s a slight complication though. As I''m sure you can appreciate, my associates – that is, the ones with an interest in highly-illegal booster manufacture – they''re stationed at a somewhat remote NOH office outside of CONCORD jurisdiction, where the Chief Executive Panel finds it difficult to keep a close eye on everything. 

Ideally, I would like you to deliver the goods to our facility in KFR-ZE yourself, but I can make arrangements for the staff at KFR to retrieve it from a nearby low-security system if you prefer. That will, of course, reduce your cut of the profits. If you can fly a Covert Ops – or an Interceptor – anything like that, then I would recommend you make use of it now, but how you proceed in light of this situation is up to you. All I want is for the sale to be made; I care less about how. 

[[Two Steps Into Hell]]>><br>
I''m sure you''re aware how dangerous a place Syndicate can be. Serpentis are everywhere, and your fellow capsuleers have given the region a reputation for constant violence and war. I have mapped a route you could take through Genesis, via Faurulle, that will limit the amount of lawless space you need to traverse. That will make the trip longer, but safer. I guess that depends on how you feel about the short route and the jump into PF-346 it entails. That one system alone must have claimed millions of souls over the years; it is notoriously dangerous. 

Personally, I would advise discretion, and that you limit your risks. I suggest you take something evasive, be that through speed or cloaking, and avoid the busiest systems. I will have an agent on standby in KFR-ZE at a secure Nugoeihuvi facility near our Station offices. I suggest you avoid the station unless there is a need to dock there. Capsuleer pirates love to corner unaware travelers there; we have to deal with the combat on our doorstep all the time.

[[Playing it Safer]]>><br>
Our people in KFR-ZE come down to State space via Faurulle at times. This is the quickest route out of Syndicate and often the most desirable one for that very reason. We''ve lost a few convoys recently, trying to pass through PF-346 along the shortest route to State space. In case you don''t know it, <Character>, the PF-346 system has been made notorious by bloodthirsty capsuleers. I think it is perhaps best for all involved that we take no risks on this one and use the Faurulle drop-off. 

This is where you will be taking our formula. I will have my people on standby at a secure facility in-system. Please, though, do not linger there. There are no stations in system, so anyone noticing you will know that you''re out in space somewhere. The very last thing we need is some pirate scanning you down in one of our own facilities, and especially not in that one. 

[[An Honorable Betrayal]]>><br>
You want more time? Very well, <Character>. 

I''ll hold the job for you a little longer, but if I get the impression that you won''t be returning, then I''ll just hand it off to someone less busy. Business is business, my friend.', 'The_Paths_That_Are_Hidden'),
  (80124, 0, 1, 0, 0, 0, 0, 0, 0, 'Report to Arikio Kuretsu', '', 'An_Honorable_Betrayal'),
  (80128, 0, 3, 0, 0, 5, 0, 0, 0, '', '', 'Two_Steps_Into_Hell'),
  (80125, 0, 2, 23187, 1, 0, 0, 0, 0, 'Deliver S.I Formula Sheet', 'Hmm, that''s quite a story, <Character>. If you weren''t a known friend of the State, I''d be inclined to think you were attempting some kind of scheme. Regardless, I should still throw your ass out of here and have Hyasyoda set your status to kill on sight. That will not get us anywhere, however. 

My superiors have been informed of the situation and have asked me to request something from you, but before doing so, to also make something explicitly clear. 

The facility you were instructed to raid? It has not been under Hyasyoda control for almost a year now. I was told that if you do your research you should find that we also legally rented the area from Sukuuvestaa too, before Serpentis overran it and forced us out. Of course, SuVee could pay the House of Records and the Caldari Business Tribunal a few administrative fees to make that information disappear, if someone were to ever poke around. 

What I think you may also find interesting to know, though, is that around the time our lease was coming up for renewal, Serpentis began to appear in larger numbers and with better equipment. We always suspected someone local was funding them, a rogue Guristas outfit or even Gallentean extremists, but the idea that it might be one of our own corporations, at a time like this, it just never crossed our mind. 

Something definitely isn''t right here. I suspect Nugoeihuvi learned of the facility through Sukuuvestaa, who I can only guess, never wanted us to extend our lease for some reason. NOH, doing what it does best, brought in some thugs to clear the matter up. That''s until you arrived and, it would appear, saw that something wasn''t adding up. 

Now, the question is whether or not we can trust you. Obviously we are happy to share the information we have so far, but there is more to the story than we will say to an egger who has stolen from us. If you are serious about seeing this affair brought to a just end, then you will return the formula you took from us and prove your good intentions. 

Do anything less and you will still be considered an enemy of Hyasyoda Corporation. It''s as simple as that.', 'Proof_of_Intent'),
  (80129, 0, 2, 23187, 1, 0, 0, 0, 0, 'Deliver 1x S.I Formula Sheet (0.1 m3) to Nugoeihuvi Cargo Facility in Faurulle, Solitude.', 'Travel out to the lowsec system of Faurulle in Solitude to make the delivery. Once you have arrived in system, there will be a Nugoeihuvi facility with crew on standby to receive the goods. You will lose a percentage of the profit if you decide on this route, although the remaining risk and time involved will ensure it is still quite lucrative.', 'Playing_It_Safer'),
  (80130, 0, 1, 0, 0, 0, 0, 0, 0, 'Report to Arikio Kuretsu', 'Ah, you''re back, <Character>. Good. I hope your journey out toward Syndicate hasn''t left you too burned out; I''m going to need you to act quickly on something else. It turns out we have one more loose end to tie up. 

My people tell me that Hyasyoda have already launched an investigation into the blueprint theft. Worse yet, I am hearing talk about the sort of funding coming their way. The amounts are disconcerting — enough to pay for a capsuleer''s services in addition to a whole fleet of mercenaries and investigators. The situation is dangerous for us, <Character>. Our tracks are well covered, but, as they often say at Hyasyoda, only the paranoid survive. 

They''ve deployed their security wing, the Corporate Police Force, into “various locations of interest.” I''ve learned that Onirvura was listed as an area to investigate. They''re right here in the constellation, setting up a temporary base for interrogation and imprisonment. Fortunately for us, the CPF has hired a mercenary crew to handle facility defense while they set up. This gives us an opportunity to strike at them without it constituting a direct attack on Hyasyoda personnel. That is simply not an option. 

I cannot guarantee how long these mercenaries will be on duty, however, nor what kind of reaction your arrival on scene will provoke. What I do know is that if you can destroy the Logistics Center quickly, it will set them back weeks. The paths that lead back to us will undoubtedly have faded by the time they can resume their investigation.', 'Almost_Unmasked'),
  (80126, 0, 1, 0, 0, 0, 0, 0, 0, 'Report back to Katsen Isha', 'This Katsen Isha…he needs to be brought down, but we have to be exceptionally careful about how we do this. Dealing with Nugoeihuvi requires cunning and guile. You''ve shown sound judgment by coming to us early, <Character>, but it will only get worse if you leave things as they are now. We need answers to the questions you''ve brought to light. 

Can we count on you to earn Isha''s trust, so you can uncover whatever you need to help us? We need someone on the inside if we''re ever going to see this resolved; Nugoeihuvi are too well-trained at covering up their tracks. If you could be there, making the sale of the formulas yourself, we would undoubtedly learn a great deal. 

If you''re willing to take this on, then we will be there to assist you every step of the way. I''ve been authorized to grant you provisional support from one of our best counter-intelligence teams. They would observe your actions from a distance and monitor any relevant data from your fluid router cache. Anything Isha said to you over FTL, we would have. Anything your camera drones saw, we would see. 

You would have to keep your assignment strictly confidential, even from other Hyasyoda members. Nobody could know. Not your lover, not your mother, not even the Maker. If we need you again, we''ll find a way to get in touch, but until that time you would only have to concern yourself with earning Isha''s trust and acting as if this conversation never happened.

Do we have a deal?', 'Return_to_Isha'),
  (80127, 0, 1, 0, 0, 0, 0, 0, 0, 'Choose next mission', 'Welcome back, <Character>. I hope your time has been well spent considering our options. 

Hyasyoda seems to have calmed down a little for now, but it''s when they get quiet that I actually start to worry. They likely already have something in motion. I''m not interested in seeing if my instincts are right, though. We should shift these formulas as soon as possible. 

As I told you before, I''ve already lined up a buyer for us. All you need to do is move the goods. As I also explained, my associates – that is, the ones with an interest in highly illegal booster manufacture – they''re stationed at a somewhat remote NOH office outside of CONCORD jurisdiction, where the Chief Executive Panel finds it difficult to keep a close eye on everything. 

Ideally, I would like you to deliver the goods to our facility in KFR-ZE yourself, but I can make arrangements for the staff at KFR to retrieve it from a nearby low-security system if you prefer. That will, of course, reduce your cut of the profits. If you can fly a Covert Ops or an Interceptor, anything like that, then I would recommend you make use of it now. How you proceed in light of this situation is up to you though. All I want is for the sale to be made soon; I don''t care how. 

[[Two Steps into Hell]] >>
I''m sure you''re aware how dangerous a place Syndicate can be. Serpentis are everywhere, and your fellow capsuleers have earned the region a reputation for constant violence and war. I have mapped a route you could take through Genesis, via Faurulle, that will limit the amount of lawless space you need to traverse. That will make the trip longer, but safer too. I guess that depends on how you feel about the short route and the jump into PF-346 it entails. That one system alone must have claimed millions of souls over the years; it is notoriously dangerous. 

Personally, I would advise discretion, and that you limit your risks. I suggest you take something evasive, be that through speed or cloaking, and avoid the busiest systems. I will have an agent on standby in KFR-ZE at a secure Nugoeihuvi facility near our Station offices. I suggest you avoid the station unless there is a need to dock there. The more piratical among your own kind love to corner unaware ships there; we have to deal with the combat on our doorstep all the time.

[[Playing it Safer]] >>
Our people in KFR-ZE come down to State space via Faurulle at times. This is the quickest route out of Syndicate and often the most desirable one for that very reason. We''ve lost a few convoys recently, trying to pass through PF-346 along the shortest route to State space. In case you don''t know it, <Character>, the PF-346 system has been made notorious by your own kind as something of a deathtrap. I think it is perhaps best for all involved that we take no risks on this one and use the Faurulle drop-off. 

This is where you will be taking our formulas. I will have my people on standby at a secure facility in-system. Please, though, do not linger there. There are no stations in system, so anyone noticing you will know that you''re out in space and thus an easy target. The very last thing we need is some pirate capsuleer scanning you down in one of our own facilities, and especially not that one.', 'Re-examining_Options'),
  (80131, 0, 1, 0, 0, 0, 0, 0, 0, 'Choose next mission', '<Character>, good. I hope you''ve recovered from the Syndicate trip. Once again, good work out there. Unfortunately, it turns out we have one more loose end to tie up. 

My people tell me that Hyasyoda have already launched an investigation into the formula theft. I suppose this is a play to lend an air of legitimacy to the racket they''ve been running. The situation is an amusing one, yet also dangerous for us. Our tracks are well covered, but – as they often say at Hyasyoda itself – only the paranoid survive. 

They''ve deployed their security wing, the Corporate Police Force, into “various locations of interest.” I''ve learned that Onirvura was listed as an area to investigate. They''re right here in the constellation, setting up a temporary base for interrogation and imprisonment. Fortunately for us, the CPF has hired a mercenary crew to handle facility defense while they set up. This gives us an opportunity to strike at them without it constituting a direct attack on Hyasyoda personnel. That would muddy the waters a bit, as I''m sure you can appreciate. Mercs or not, though, they''re getting a little too close for comfort. 

I should mention also that we can still resolve this peacefully too, if you prefer. The intel that has brought them here can be manipulated, perhaps, and lead them elsewhere. I care very little about how you want us to proceed, as long as this situation is dealt with. I want to go to sleep tonight knowing our assets and our reputations are safe. 

[[The Crimson Decoy]] >><br>
It appears that the CPF investigation team is in Onirvura because of some intel – intel I believe we can manipulate. I have learned that they partially decrypted a communication between my people here and our blueprint buyers in KFR-ZE. Fortunately for us, they couldn''t fully decipher the location we were transmitting to, thank the Maker. My guys re-encrypt their comms every few seconds, so all they know is that this area is where the transmission originated and the destination system started with KF, and has a Z and E in its name. Still, too close for comfort. 

Ever heard of KFIE-Z, <Character>? It''s a dead-end system in Delve. The Headquarters of the Blood Raider Covenant, in fact. It''s the perfect decoy for us; we can make it look as if opportunistic Blood Raiders hit the Serpentis facility, and there was little more to it all than that. We just need to sell the lie. 

There is an old Blood Raider compound not far from here. Inside it, there should be a transmitter system we can use. All you need to do is secure the area and deliver my people safely to the Communications Tower so they can send out a fake transmission. The CPF should pick it up and follow the signal down to Delve… if they''re feeling suicidal, that is. 

Rumor has it the place has long since been abandoned, but I''ve come to distrust intel I don''t get first-hand. Besides, any time those Sani Sabik cultists are not making noise, you should be concerned. The quiet ones are the very worst. I suggest you go in expecting exactly that.

[[Pre-Emptive Opportunities]] >><br>
This Corporate Police Force investigation must be brought to an end before it can uncover anything that could lead back to us, unlikely as that may be. The forces inside the facility right now are not Hyasyoda or CPF personnel, just some hired mercs. This gives us an opportunity to strike preemptively at the compound without causing an incident and further complicating matters. 

I cannot guarantee how long they will be on duty, however, nor what kind of reaction your arrival on scene will provoke. What I do know is that if you can destroy the Logistics Center quickly, it will set them back weeks. The paths that lead back to us to will no doubt have faded into the background by the time they can resume their investigation.', 'Too_Close_for_Comfort'),
  (80132, 0, 3, 0, 0, 0, 500004, 0, 0, '', 'Ah, <Character>, welcome back. Word filtered back to me that the Hyasyoda investigation came to an abrupt and explosive halt. Good work. 

Now that we''re in the clear, we''re free to pursue the matter I mentioned earlier. There''s someone I wanted you to meet. I''m afraid I can''t do it myself; he and I must be careful not to be associated with one another. This is not for any good reason, <Character>, it is simply the way our judgmental world works. A Caldari Navy officer of his kind cannot afford the rumors that a friendship with me brings. You will learn though, in time, how little truth there is to them. You''ve already shown a great capacity to see through the surface of things, to the heart. You wouldn''t be here still if that wasn''t the case. 

My friend''s name is Sinas Egassuo. He is a man in a similar position to me, working quietly in the engine room, seeking results, not glory. He too, works as an Advisor, and almost every important operation undertaken by our Navy in the last decade bears his imprint. You will hear talk of Caldari patriots in your time in the State, and doubtless already have. This man is a living definition of one. 

You will find him somewhere around Liekuri, just look for the Caldari Navy compound in the system, and be careful. He may not be known and loved by the Caldari people, but the leadership of the Gallente Navy is well acquainted with him and his work, and has long wanted him dead. His outfit deals with assassins and bounty hunters on a daily basis. Expect nothing less when you find the compound. 

Oh, and if his surname seems familiar that''s because you''ve probably met his sister. She works as a capsuleer liaison for the Navy''s Command Division in Motsu. I wouldn''t mention anything about her though; sisters are a sore spot for him.', 'A_Generals_Best_Friend'),
  (80133, 0, 1, 0, 0, 0, 0, 0, 0, 'Report to Sinas Egassuo', 'It sounds like Sinas is on to something. I want you there to help him in any way you can. He''s headed back to his post in Astoh, so you''re off to Black Rise to meet him, <Character>. If you''ve never been there, then you should know that it can be a dangerous place; the leadership of the Federation''s capsuleer militia has set its sights on nothing less than total Gallentean occupation of the region. An absurd goal, but they''re welcome to try. 

From the sounds of it, Sinas will need you for something around there. My advice would be to start by flying an interceptor or frigate down that way. If on the off chance he needs you to do some heavy lifting, then you can go back for something larger. My impression is that he''ll want speed and discretion here though; if it was something a fleet of Ravens could hit he''d have handled it with his own crew already. 

Anyhow, fly out there, see what''s up, and keep in mind what I told you about him. If you can trust me, you should trust him.', 'Meet_Sinas'),
  (80134, 0, 2, 3812, 1, 0, 500004, 0, 0, 'Retrieve 1x FedNav F.O.F. Identifier Tag AC-106V:FNSBR', 'Welcome to Astoh, <Character>. I appreciate you coming by. Katsen just sent me the ISK, too. Thank you for being our middle man on that one. You may want to make yourself comfortable in that egg for a moment; I''ve a bit to cover here for your briefing. 

As you know, my name is Sinas Egassuo. As you probably do not know, my job is to coordinate search and rescue operations, as well as the liberation of Caldari Prisoners of War taken during battle in Black Rise. This duty I have fulfilled without hesitation, and I am proud to have led such operations. 

It is by carrying out this task, however, that I''ve come to notice something strange. Whenever we rescue people, there are often less Ishukone crew members and staff than the rest. It''s never enough to be conspicuous or noticeable, but I notice it, and I find it conspicuous. I can''t explain it though, and neither can the people we recover. 

My suspicion is that Ishukone personnel are being taken elsewhere, perhaps being treated more kindly due to their Liberal philosophies and good history with the Gallente. It wouldn''t surprise me to see the Federation trying to divide us over something like that, but we''d have to know about it first, and they don''t seem keen on that happening. 

I need you to investigate the matter for me and find out what''s going on. I can brief you on the details of the assignment once you accept, but until I have that commitment, I''m not willing to trust you with the information. Sorry <Character>, but Katsen''s recommendation only gets you so far. 

So, are you committed to the fight ahead of us or not?', 'Right_Tool_for_the_Job'),
  (80135, 0, 2, 30980, 1, 0, 500004, 0, 0, 'Retrieve 1x Caldari Prisoners Of War (0.5m3)', 't''s time for you to put that ID tag to use and see firsthand what it is we''re fighting against down here. 

Though you''ve no doubt heard of the fearless and capable pilots in the Caldari Militia, I wouldn''t make the mistake of thinking that the situation in Black Rise is a stable one. The region is a war-zone, and will probably remain that way for some time. Even though they''re capsuleers like you, our militia cannot be everywhere at once, and it is in their absence that the Federation often makes progress. 

We''ve been receiving reports of a new prison facility being erected somewhere in the Aokinen constellation. Your mission is to discover its location, bypass the security forces and slip inside. Once you''ve made your way inside, there will be people of ours to free. My suspicion though, is that you will not find any Ishukone crew to rescue. I suppose we''ll see what happens. 

If you''ve never done any exploratory scanning before, then now would be a good time to learn. The Federation is skilled at hiding their secrets, but I have a feeling that a facility this size will be found easily enough. If you are struggling to locate it, don''t lose hope - that''s exactly what the enemy wants us to do. 

You will need a Data Analyzer to complete this mission.

What do you say, <Character>?', 'The_Breakout'),
  (80136, 0, 2, 3812, 1, 0, 0, 0, 0, 'Retrieve Encrypted Data Fragment (0.1 m3)', 'It seems Ishukone are playing a dangerous game here, <Character>. Before we make any moves though, we have to be absolutely certain that things actually are what they look like. What does that mean? It means I need you for another infiltration mission. 

We''ve been monitoring the movement of selected Ishukone vessels over the last few days. Our focus is on the handful of diplomatic emissaries they''ve deployed inside Black Rise. Why does Ishukone have diplomats down here? We wondered the same thing, and that''s why we had them followed. 

While tracking one particular Ishukone vessel, our surveillance team lost them. They appear to have warped into a derelict research facility, conveniently located inside an area riddled with electromagnetic interference. Now, it''s quite possible that their actions were entirely legitimate. We just don''t know though, I ordered my men not to pursue. 

I''m betting that we can send you in there without any trouble, no matter what goes down. You capsuleers are invaluable for these sorts of missions where you have no idea how it will go. Would you be willing to scout the area for us and report back what you find?', 'Whisper_of_a_Conspiracy'),
  (80137, 0, 2, 32228, 1, 0, 0, 0, 0, 'Retrieve Vira Mikano (3.0 m3)', 'I''ve managed to get a message over to Katsen. He''s already begun to make some arrangements but I have to say <Character>, this is going to be a particularly dicey assignment, for all of us. 

Katsen is convinced he knows the man for this job. In fact, he claims that if anyone can decipher this data node, it will be this man. His name is Vira Mikano, and he''s a senior decryption analyst with Ishukone Watch. 

Yeah that''s right, I said Ishukone Watch. 

Katsen''s plan is to have some of his people “redirect” Mr. Mikano from his office in Oipo to your personal hangar on the station. From there, you will put him aboard your ship and safely escort him back here to Astoh. 

Let me stress something before you leap in, <Character>. Mikano is a family man, and a well-respected Caldari too. Hell, I once had drinks with him. If the lives of many more Caldari weren''t at stake here, I wouldn''t consider abducting him for even a moment, but desperate situations call for practical solutions. We need him to help us, and there is no chance in hell he''d do that without a little pressure, but he must not come to any harm whatsoever. 

Only accept this assignment if you''re absolutely ready for it. I''ll relay your acceptance to Katsen the moment you give me the nod. After that, there''s no turning back.', 'Practical_Solutions'),
  (80138, 0, 2, 3812, 1, 0, 0, 0, 0, 'Deliver Encoded Message', 'We''ve got them, <Character>. The decoded data from that node has given us undeniable proof. Ishukone has been engaged in the unauthorized exchange of Caldari prisoners of war, and worse yet, they''ve only been focusing on the rescue and recovery of their own people. The information you recovered reveals a number of staging points where they ferry their people back to the State.

In fact, Katsen suspects the only reason they evacuated others was to keep the number of prisoners relatively normal and inconspicuous. I don''t know about his claims, but I sure know what I''ve seen out there on the job. 

The next step from here is to let Ishukone know they''ve been compromised. The leverage that grants will allow us to return Mikano safely and without incident. More importantly though, it also means we can get them to stop this amateurish exchange they have going on, even if only momentarily. If they want to rescue our families, then they should be doing a proper damn job of it like the Navy, not this half-assed nonsense that leaves a paper trail extending across the cluster. This current setup will get everyone killed or blackmailed if it is allowed to continue. 

You''re off to another P.O.W holding ground, <Character>, same as before. This time though, you won''t be liberating anyone. In fact there is not even a Federation presence inside this facility. It is where the Ishukone prisoners have been held before being escorted back home. Your job is simply to deliver a document from Katsen. In it, he explains the extent to which we could ruin Ishukone with this information, and then requests a meeting. We''re pretty confident they''ll make time to schedule us in. 

Due to the extreme sensitivity of this information, we''d prefer by far if you could personally deliver the document. You''ve proven so dependable that we hoped a quick flyover wouldn''t be too much to ask?', 'Forewarning'),
  (80139, 0, 1, 0, 0, 0, 0, 0, 0, 'Choose your path', 'The Knowledge to Act

<Character>, good. I talked to that old friend of mine about a compromise between Ishukone and the other corporations, and we''ve managed to come up with a solution he believes they''ll agree to. 

We''re meeting shortly, but I wanted to update you on the progress. I believe we can solve this amicably, without pushing Ishukone into a corner. I can''t imagine a better result myself, given the current situation. 

It comes down to you, though. I''m not about to pretend that Katsen doesn''t still have other ideas about how to deal with this. My guess is that he wants you to keep digging, to properly back Ishukone into a corner. Well, you''re free to go off and start that war if you think it''s necessary, but I won''t be there alongside you. 

Sometimes conflict isn''t the solution. Take it from a man who has been fighting his whole life. There''s some things my guns could never change, <Character>. Either you understand that and you''re with me, or you can go the other way and learn the lesson yourself. 

I''ll push for a compromise either way, but it would help to have you in my corner on this one, more than you might appreciate. 

[[Home in Peace]] >>
If I''m honest, I wasn''t sure that you and I would ever see one another again. I know that Katsen probably offered you all kinds of things to continue pursuing Ishukone. You didn''t take it though, so good work. Maybe there were other temptations, perhaps? A few other bets you were hedging on the side? Hmm, yes, don''t worry, I won''t push the matter, but something tells me perhaps you''re starting to appreciate things many Caldari forget to. 

So then…let me catch you up. My friend and I believe a deal can be struck easily if we can put aside the question of moving recovered Ishukone personnel to their Headquarters in Malkalen. If you were to offer your help, it would lend a tremendous amount of credibility to the operation. Ishukone may not trust you, but they trust the Navy, and they will trust that you have the ability to do the job, if not the loyalty. Personally, I know you''ll do what''s right. You wouldn''t be here otherwise. 

There''s a huge risk here though, <Character>. You''ll be moving people from a low-security system in a warzone through to the heartlands of State space. Thousands of lives will depend on your ability to maintain a low profile.

[[Slipping Away]] >>
In any other situation, a request for more time would be fine, <Character>. But there are hundreds, thousands of people waiting on us. What am I supposed to tell my friend in Ishukone, too? That things have to wait because the capsuleer we''ve been depending on needs to deal with something else? Your request comes at a bad time, friend.', 'The_Knowledge_to_Act'),
  (80142, 0, 1, 0, 0, 0, 0, 0, 0, 'Choose your path', '<Character>, good. I talked to that old friend of mine about a compromise between Ishukone and the other corporations, and we''ve managed to come up with a solution he believes they''ll agree to. 

We''re meeting shortly, but I wanted to update you on the progress. I believe we can solve this amicably, without pushing Ishukone into a corner. I can''t imagine a better result myself, given the current situation. 

It comes down to you, though. I''m not about to pretend that Katsen doesn''t still have other ideas about how to deal with this. My guess is that he wants you to keep digging, to properly back Ishukone into a corner. Well, you''re free to go off and start that war if you think it''s necessary, but I won''t be there alongside you. 

Sometimes conflict isn''t the solution. Take it from a man who has been fighting his whole life. There''s some things my guns could never change, <Character>. Either you understand that and you''re with me, or you can go the other way and learn the lesson yourself. 

I''ll push for a compromise either way, but it would help to have you in my corner on this one, more than you might appreciate. 

[[Learning by Doing]] >><br>
From the looks of things, you''re not coming with me on this last leg, are you? I can tell you share Katsen''s conviction on this. You want Ishukone to pay for looking after their own people and turning their back on the rest. Well, I can understand it, <Character>. It''s admirable even, but not something I can support. You can''t teach a Caldari ethics by holding a gun to their head; things just don''t work that way, no matter how well-intentioned. 

Well, like I say, this is something for you to figure out yourself. Maybe someday you''ll recognize your error. Until then, safe flying my egger friend. Regardless of anything else, you''ve done more for me, and more for those people out there, than most others would do in a lifetime. I may disagree with how you''re handling the endgame, but what got you here was a good head and a good heart. 

Good luck with whatever you do, and try to remember what I said. 

[[Home in Peace]] >><br>
If I''m honest, I wasn''t sure that you and I would ever see one another again. I know that Katsen probably offered you all kinds of things to continue pursuing Ishukone. You didn''t take it though, so good work. Maybe there were other temptations, perhaps? A few other bets you were hedging on the side? Hmm, yes, don''t worry, I won''t push the matter, but something tells me perhaps you''re starting to appreciate things many Caldari forget to. 

So then…let me catch you up. My friend and I believe a deal can be struck easily if we can put aside the question of moving recovered Ishukone personnel to their Headquarters in Malkalen. If you were to offer your help, it would lend a tremendous amount of credibility to the operation. Ishukone may not trust you, but they trust the Navy, and they will trust that you have the ability to do the job, if not the loyalty. Personally, I know you''ll do what''s right. You wouldn''t be here otherwise. 

There''s a huge risk here though, <Character>. You''ll be moving people from a low-security system in a warzone through to the heartlands of State space. Thousands of lives will depend on your ability to maintain a low profile.', 'A_Difference_of_Opinion'),
  (80140, 0, 1, 0, 0, 0, 0, 0, 0, 'Report back to Arikio Kuretsu in Wuos', '', 'Slipping_Away'),
  (80143, 0, 1, 0, 0, 0, 0, 0, 0, 'Report to Katsen Isha at Poinen IV - Moon 13 - Nugoeihuvi Corporation Development Studio', '', 'Learning_by_Doing'),
  (80141, 0, 2, 32277, 1, 0, 500001, 0, 0, 'Retrieve Deteis Family (0.1 m3)', 'Welcome back to Wuos, <Character>, it''s reassuring to see you here, I have to admit. After all that time undercover, we were starting to worry that you''d forgotten your loyalties. Here you are though, and you''ve come bearing all kinds of vital information. Your loyalty and commitment is not characteristic of your kind, but then, everything about you has exceeded expectations. I have little doubt that on this last, most important mission, you will not fail me either. 

Katsen Isha is a man gifted with considerable political acumen, and his known associates make for a long list of people I don''t want to screw around with. We have learned a great deal from the information you''ve leaked us and uncovered much of the important details about this man. For the longest time, he seemed incorruptible and untouchable; a man with no bad habits, no sordid secrets and no exploitable desires, all surrounded by some serious corporate heavyweights. That impression of him has changed. 

Although you helped us find a number of interesting things, we recently followed up on a lead you provided us and stumbled upon something that utterly dwarfs anything else so far unearthed. We have identified the one thing that he actually cares about; his family. We also have the coordinates of their current location in our possession. 

It''s unfortunate, I''m not about to pretend otherwise, but if capturing his family is the only way to get a genuine assurance of silence out of this man, then it is the path we must tread. Isha is bordering damn close to treason already, he deserves far worse. We cannot let Ishukone''s operations be undone because of Nugoeihuvi''s lust for profit. The notion that he would try to blackmail us on such a thing speaks volumes about the values held dear by NOH. 

If you are willing to help us, then I can give you the coordinates of a central Nugoeihuvi Storage facility; a quiet but well protected little area that NOH staff use to store their ships and documents. Nobody would ever want to live there, and nobody actually living there would ever be that comfortable. It''s the perfect place for a man with many enemies to hide what he values most. 

Thousands of Caldari prisoners are depending on us to save them, and now it comes down to you, <Character>. Either our operation can continue under the radar, and thousands can be saved, or it can become a scandal that costs us ISK, our reputations, and the lives of countless Caldari. Isha must be stopped.', 'Across_the_Line'),
  (80144, 0, 2, 32267, 1, 0, 0, 0, 0, 'Retrieve 1x Ishukone Operational Reports', 'Welcome back to Poinen <Character>, it''s good to see you again, particularly under these circumstances. To be honest with you, I had some concerns that your time with Sinas changed the way you see things. He''s a good friend; a patriot, as I told you earlier, but this talk of coming to a compromise with Ishukone betrays the weaker side to him. There is no such thing as a fair deal from the likes of Ishukone – they won''t agree to help others unless they get to help themselves first. 

We Practicals know when to make and break deals, and we always remember the larger picture. It''s time to forget about Sinas and remember the fighting men and women abandoned by Ishukone rescue teams. That won''t stop until we make it stop, <Character>. 

You''re flying to Malkalen, to a major security headquarters for Ishukone Watch known as “Base 3”. This station acts as an archive for all Ishukone Watch operations in Black Rise. This is an internal archive, meaning it will house not only the original reports, but in their full, uncensored glory, too. As you might expect, that information is well guarded. They know they can''t hide the evidence from Nugoeihuvi, all they can do is put it behind a fleet. You are our best weapon now, <Character>. 

I won''t lie. I''m sending you into a fortress. I have no more doubts about you though. You have come this far and not once strayed from the path we have walked together. I know you will deliver.', 'The_Price_of_Silence'),
  (80145, 0, 2, 30980, 10, 0, 0, 0, 0, 'Retrieve Caldari POWs (5,000 m3)', 'If I''m honest, I wasn''t sure that you and I would ever see one another again. I know that Katsen probably offered you all kinds of things to continue pursuing Ishukone. You didn''t take it though, so good work. Maybe there were other temptations, perhaps? A few other bets you were hedging on the side? Hmm, yes, don''t worry, I won''t push the matter, but something tells me perhaps you''re starting to appreciate things many Caldari forget to.

So then…let me catch you up. My friend and I believe a deal can be struck easily if we can put aside the question of moving recovered Ishukone personnel to their Headquarters in Malkalen. If you were to offer your help, it would lend a tremendous amount of credibility to the operation. Ishukone may not trust you, but they trust the Navy, and they will trust that you have the ability to do the job, if not the loyalty. Personally, I know you''ll do what''s right. You wouldn''t be here otherwise.

There''s a huge risk here though, <Character>. You''ll be moving people from a low-security system in a warzone through to the heartlands of State space. Thousands of lives will depend on your ability to maintain a low profile.', 'Home_in_Peace')
ON DUPLICATE KEY UPDATE objectiveType=VALUES(objectiveType), itemTypeID=VALUES(itemTypeID), itemQty=VALUES(itemQty), npcCount=VALUES(npcCount), npcFactionID=VALUES(npcFactionID), structureTypeID=VALUES(structureTypeID), structureQty=VALUES(structureQty), objectiveText=VALUES(objectiveText), briefingText=VALUES(briefingText), note=VALUES(note);
-- +migrate Down
DELETE FROM `missionObjectives` WHERE `missionID` BETWEEN 80120 AND 80145;
