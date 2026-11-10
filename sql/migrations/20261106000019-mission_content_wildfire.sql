-- +migrate Up
-- Wildfire (arc 8) mission content from the EVE University wiki.
INSERT INTO `missionObjectives` (`missionID`,`briefingID`,`objectiveType`,`itemTypeID`,`itemQty`,`npcCount`,`npcFactionID`,`structureTypeID`,`structureQty`,`objectiveText`,`briefingText`,`note`) VALUES
  (80170, 0, 2, 32099, 1, 0, 500011, 0, 0, 'Find and retrieve 1x Olfei Medallion (0.1 m3)', 'A visitor, and a capsuleer no less. I can tell my day is about to get more interesting. 

Hello <Character>. I''m guessing you flew out here because you heard I was handing out work, right? Well, I do have something for you. Nothing amazingly difficult and nothing someone without a pod could handle easily, either. If you can pull this off, we can see about maybe getting you in on some other things. Maybe. 

We Matari are known for our ingenuity. Civilization owes us a great debt, and the Amarr owe it twice over. Before they came, we were by far the most advanced nation; our technological achievements were regarded with envy by the rest of the cluster. You eggers know a lot, but did you know that? 

Well, regardless of history, and regardless of your race, you will need to demonstrate an understanding of this if you want to work with us. Prove your resourcefulness by bringing me an Olfei Medallion. You will have to explore as only a capsuleer can and find one somewhere in the Sveipar constellation, near our homeworld Pator. Of course, some other egger can milk your wallet in exchange for doing the thinking and grunt work for you. That works too. Whether you scan it down inside an Angel Cartel fortress, or buy it for 2 ISK on the market - I don''t really care. Results are what matter most.

You''ll go far with me if you keep that in mind.', 'WF_A_Demonstration'),
  (80171, 0, 2, 32202, 1, 0, 0, 0, 0, 'Retrieve 1x Hauteker Memoirs (0.1 m3)', 'Now that we''re past the small talk perhaps you''re ready for some proper work. I guess we''ll see. 

There''s this Gallentean guy, a historian named Aillon Boufin…yeah, I know. The story is that he''s been studying Minmatar history for years now, and as you''d expect, he''s come across a whole shipload of information and documents in that time. Apparently he has one of the largest unofficial collections of documents and artifacts from our earlier days. We''re talking real old here, <Character>, some of it - going back to when it was just us, alone against the Amarr. 

Now, I''m told that normally whenever we want to look at something of his, there aren''t too many problems. Recently however, when some of our academics from the Republic University asked to see a particular document known as the Hauteker Memoirs, they were denied. Since that time, they have bent over backwards for this guy. They have even tried to buy it back - this document that Matari people wrote, this document that belongs to us. They have tried that repeatedly, raising and raising the price and yet each of these times more refusals. 

This is when things got ugly. Boufin recently made a statement to the University, saying he is only trying to “preserve Minmatar heritage”. After that insult, they called us in. The implication that RU staff would somehow endanger our own history by simply studying a document...it''s almost like he was trying to piss us all off. 

Well, the insults stop here. The Brutor tribe has arrived, and with a capsuleer to back their word on the matter no less. It has been decided by the tribe that the return of this document to Matari hands represents a higher priority than staying on good terms with some overprotective historian, Gallente or not. You will see to this. And before you go jumping in, there is a 5m ISK collateral to cover associated risks. The tribe is not willing to risk our own property falling into foreign hands again. Perhaps you can see why we''re looking to hire an egger? With that kind of money it''s not really an option.', 'WF_The_Cost_of_Preservation'),
  (80172, 0, 2, 32204, 1, 0, 500011, 0, 0, 'Retrieve 1x Wildfire Khumaak (0.3 m3)', 'Good thing you''re still about, we''ll need you again for this one. 

The Republic University guys had a look over the document and it has what they''re after: directions. There''s a mention in one of the sections of an old Nefantar bloodline, and where they buried their dead. I''m told they''re important because of some old folktale about how they buried a special type of Khumaak with one of their leaders. Pretty odd stuff, but also kinda interesting. 

They''re on a myth-busting mission, though, that''s the thing. There is no actual Khumaak, just a bunch of old Ammatar bones, probably. Still, you go there, check it out, and then they can say in their report that the area was inspected by a capsuleer and turned up nothing. They put that together with the proof in this old book that you inspected the right area and, I dunno, write an essay about how clever they are. The important thing for me was just getting our own history back, and that''s what you should remember too. 

So anyways, you feel like digging up some graves? The coordinates will be 2.5m in collateral, to cover associated risks yet again. If for some crazy reason there is actually a unique Khumaak out there, it belongs in a museum, not your cargo bay. 2.5m ISK would buy a lot of museums, you see. Whatever way it goes, our history won''t lose out again.', 'WF_Written_By_The_Victors'),
  (80173, 0, 2, 32204, 1, 0, 0, 32206, 1, 'Deliver 1x Wildfire Khumaak (0.3 m3) to Defiants Base.', 'Ever heard of the Defiants, <Character>? Well, either way, you''re about to meet them – if you think you''re ready for it. They''re one of the few groups the Brutor tribe can trust these days. I once had the honor of meeting their leader, Karishal Muritor, before our own Fleet had him put down like a dog for daring to fight the Amarr. A shameful day for us all, it was. 

Still, it showed us who will fight, and who can be trusted. There is no group of people I personally would want more than them to handle this. We don''t know how significant this Khumaak is, but let me tell you friend, some of our people are very excited about its discovery. They think it may shed new light on our past. Me, I''m excited too I guess, but more concerned than anything. I''ll relax once this is in the Defiants'' hands, not any earlier. 

So, can we count on you to make the delivery? I think you and I are past collateral now, <Character>. I''m happy to trust you with the current location of their camp, if only because you have no chance of finding them after this meeting. The Defiants are only discovered when they want to be.', 'WF_Glowing_Embers'),
  (80174, 0, 3, 0, 0, 5, 500011, 0, 0, 'Destroy Angel ships and defend the Defiants base.', '<Character>, you''re not gonna like this. Only moments ago the Angel Cartel launched an attack on the Defiant base to recover what was taken from them. Seems they only realized now that they were sitting on something of value. Hell, we barely even know what we''re sitting on right now. 

The Defiants have fled the camp, which has now been converted into a base of operations for the Angel''s invasion fleet. The Defiants can easily evade a bunch of Angel thugs, but they have asked that we put this chase to an end quickly and violently. Your task will be to smash the Angel''s operational base. That will sever the attacking forces still chasing our brothers from their reinforcements. As soon as that happens, the Defiants can stop running and turn to meet their attackers in a fair fight. They''ve never lost one, <Character>. Those Angels will drop like sacks of fedo crap once they have their support pulled out from under them. 

You have your orders, <Character>, will you accept them?', 'WF_From_Way_Above'),
  (80175, 0, 1, 0, 0, 0, 0, 32256, 1, 'Investigate the RSS Facility', 'Alright, we''ve talked it out and I was right. You''re going in there. The RSS almost threatened to have you killed for attempting it, but with the accusations being leveled at them right now they''re just gonna have to get over it. We needed someone independent, not tied to any one faction, and who we knew we could trust. You were the obvious answer; I didn''t even have to suggest it. 

The RSS has given us the location of the compound where this agent was doing his work. Your task will be to fly to Alakgur and investigate it. We''re looking for anything that would give us further insight into why he was visiting the Defiants. All they could tell us was that he expressed an interest in examining the Khumaak and then suddenly showed up. The Angels followed shortly after. 

It''s a politically delicate assignment <Character>, but a pretty easy one from your end. Just fly in there, have a poke around, and bring back anything you think may be relevant to our investigations. I''ll be plugged into your camera drones for this one, along with the rest of the RSS, heh. We''ll tell you what to look for too, if we see anything in there. Sorry about the invasive measures, but it was either that or a 500m ISK collateral to the RSS. 

You down for this?', 'WF_Friends_In_High_Places'),
  (80176, 0, 1, 0, 0, 0, 0, 0, 0, 'Report to Nilf Abruskur (Aldrat)', 'Excellent, you''re back. Firstly, I have an update on the documents you recovered. The Thukker and Krusual have flown in analysts to go over the copies. It may take some time to sift through the data and find the fresh leads, but I''m confident they''ll find something useful in there. 

In the meantime I have something else for you to do. I''ve made a few arrangements with the RSS and convinced them to let you continue to work on this Wildfire Khumaak business. Officially, you will be acting as the RSS liaison for the Brutor tribe, but in reality, you''ll be on the front line working for the both of us to figure out what''s going on. You''re our go-to guy for this one and you got that job based on my recommendation. Don''t make me look bad, alright?

I''ve been asked to point you towards Corporal Nilf Abruskur, who you can find in the system of Aldrat, Metropolis. I don''t know the guy, but I do know the RSS, and my advice is not to trust them too much. In fact, part of the reason I recommended you for this job was because I know I can rely on your judgment. That, and you know how to deliver results.

I want you to keep an eye on this RSS operation. My gut still tells me that they''re keeping something from us. I suppose we''ll see, right?', 'WF_My_Little_Eye'),
  (80177, 0, 2, 32229, 1, 0, 500002, 0, 0, 'Intercept the counteragent. Retrieve 1x Singed Datapad (0.1 m3)', 'Greetings <Character>, I''m Corporal Nilf Abruskur, I''m glad to have you working with us. Now, we have much to discuss and little time in which to discuss it, so if you don''t mind I''ll get straight to the point. 

You conducted yourself with skill and cunning for Agent Takalo. Due to this, and due to the fact that you''re already involved in this delicate matter we have decided to enlist your services, if you will provide them. 

As you''ve no doubt learned by now, it appears we have an intelligence asset in the Ammatar Consulate, and their handler – our agent – has disappeared under mysterious circumstances as well. It is a messy situation at the moment, but I have little doubt that we''ll clean it up one way or another. 

Our Ammatar asset is the priority for now; if we can find her, we will be able to make sense of the Wildfire Khumaak you have discovered. The tribes have their best people working on those documents you recovered as well, trying to glean her identity from something in there, but even though we don''t know her identity, they have shown us who one of her enemies is. 

The data you have recovered has strongly supported existing evidence that we have an Ammatar counteragent in our midst, and now we have their name. We believe this person has gathered information that may compromise our own source in the Ammatar government. This traitor is currently residing at one of our residential quarters with his family, completely unaware that the world is about to come crashing down on him. 

We''ve dissolved his ship''s FTL link without his knowledge, and for the past twelve hours we''ve run a dead-end intercept on every transmission he''s tried to make out of the area. We know he''s about to make an escape with his family sometime soon. 

Your task is to ensure that ship does not escape the area intact. Local agents will stand down, they know the score.', 'WF_Dead_End_Intercept'),
  (80178, 0, 2, 32234, 1, 0, 0, 0, 0, 'Retrieve 1x Encrypted Transmission (0.1 m3)', 'We''ve had analysts from the Krusual and Thukker tribes going over the documents you recovered from our agent''s private compound. They''ve made one more promising discovery so far which we want you to follow up. 

They found a recent communication from our source inside the Ammatar Consulate. It seems she sent a package of information on the Wildfire Khumaak to the system of Jark for pickup not long ago. Our guess is that due to the sensitivity of the information and the amount of attention it''s attracting, our asset in the Consulate could only get it to the border, not across. This could play in our favor though. Due to its location in the San Matar constellation – the heartlands of Ammatar space - there''s a good chance our agent didn''t make it out there yet. We have to be careful where our operatives are seen sometimes. 

The drop-off location inside Jark is a complex known as Tili''s Red Light Palace. Initial intelligence is that it''s just another merc RRdive, nothing special. Once you have arrived to the area we''ll forward the exact location of the cargo. 

Speaking of which, I trust I needn''t remind you how important those documents could be, or indeed, how sensitive. Proceed with caution, do not take any risks.', 'WF_Surfacing'),
  (80179, 0, 3, 0, 0, 5, 500011, 0, 0, 'Protect outpost', 'We''ve gone over the essential details from the information you recovered, <Character>, good work on getting this back to us. Here''s the decoded message from our Ammatar source: 

"All right, my sources have traced the origins of that Khumaak and we''ve been able to get a little bit of information, but we''re still digging. There''s remarkably little mention of this stuff anywhere in official records. 

"It''s called a Wildfire scepter. Apparently, the Wildfires were a particular kind of Khumaak-like scepter that existed only in certain conclaves of Starkman Prime in the years while the planet was under Amarrian occupation. The physical details all match up; the holes in the shaft, the blood obsidian sun center, the slightly smaller sun spirals. Only one other has been found, ever, and has since gone missing.

“According to its previous owner, the Wildfires were all destroyed some years before the Starkmanir rebellion in 22947, and it''s not even known for what purpose they were initially created or why, indeed, they were destroyed. We''ll have our sources keep investigating.

“Lastly, I''ve cleared you for entrance to the ruins of St Arzad. I''ve attached the relevant security access codes. I give you my word that the people there will provide their full cooperation and discretion. I''m told that one of the historians even has a text that mentions this Khumaak of yours.” 

There is nothing to suggest that our own agent ever made it out to those ruins. He did not pick up the package you recovered, and he has still not returned to base. As our temporary representative, you will be standing in for him. 

Your task is to travel to an orbital forward point for an archaeological dig being conducted on a nearby planet. There''s a team of historians and archaeologists there, a couple of which are supposed to have texts that reference the Wildfire Scepters. Make your way over there and await further instructions. We''ll get to the bottom of this mystery yet.', 'WF_Who_Art_in_Heaven'),
  (80180, 0, 2, 32241, 1, 0, 500011, 0, 0, 'Hack and recover 1x Drive Cluster EDF-285 (0.1 m3)', 'We dropped the ball on this one <Character>, and those putrescent, motherless Angel freaks made sure to capitalize on it. Our only consolation is that it cost them, too. We''ve managed to root the Angel spy within our ranks – the one who tipped the Cartel off about your visit to the historians. They''re in a world of trouble right now; you can trust me on that. 

From our initial interrogation of this captured Angel agent, we have learned that just before the Cartel blew up the outpost, they managed to find and make away with the data we needed on the Wildfire Khumaak. The historian that was supposed to have a text for you relating to it was tortured into handing over his research and was then killed. 

They''re offering us a deal. We give them back their agent, and they give us the information we want. They tried to make a bid for the Wildfire, but there wasn''t a chance in hell we going to entertain that idea, especially once we discovered who their agent was. We negotiated it down to the agent, but we have no intention of making a fair deal with them, simply because when it comes to the Angels, there is no such thing. They have no desire for the safe return of this traitorous scum; in fact if they could kill him right now to shut him up, they wouldn''t hesitate. 

In addition to providing us intel that the historical texts on the Khumaak have been taken, our captured agent has also revealed something else: it''s location. His testimony and a good deal of other intelligence we''re receiving is pointing to one of the Angels'' main staging outposts in Metropolis. 

The defense forces posted at this place are impressive, <Character>, beyond even your capabilities. We''ve come up with a plan to lure some out and reduce their numbers to something you''ll be more easily able to manage. We forced subversion on our Angel prisoner, made him tell his superiors that we''re sending a huge ambush fleet to the meeting point. This should goad them into bringing along a bigger reserve of manpower, most of which will be drawn from that base. 

While we''re having the meet, you sneak in, bypass the remaining resistance, and grab the texts that were taken. Underneath the central command center, we''re told there will be several data banks. The documents we''re looking for will be in one of these. You''ll need to deactivate the security firewalls, which means some hacking. You''re looking for a drive cluster called "EDF-285." Grab it and get the hell out of there before the Angels figure out the game.', 'WF_Playing_All_Their_Cards'),
  (80181, 0, 1, 0, 0, 0, 0, 0, 0, 'Report to Hiva Shesha in Tanoo', 'Our historians have pored over the information found on the drive cluster EDF-285. There''s a lot on there, but not all of it is pertinent to the Wildfire Khumaak. I''ve also been in contact with a Krusual historian who has spent some time researching the Khumaak. The historians cross-referenced each other''s work and stumbled upon a lead. I guess those nerds are good for something, eh? 

I''m sending you to Hiva Shesha, the Krusual historian. She''s taken her poor interns with her to the San Matar constellation – right in the heart of Ammatar space. I don''t know what she''s discovered, but I''m sure it''s important. Best you get over there and talk with her. Let us know what you''ve uncovered, and keep your eyes peeled for our missing agent. I have a feeling that he went to seek out his Ammatar contact. There''s no telling what he''s up to, either.

Anyway, good luck.', 'WF_History_In_The_Making'),
  (80182, 0, 2, 32280, 1, 0, 500007, 0, 0, 'Retrieve 1x Blood Obsidian Orb (0.1 m3)', 'Do you have any idea how tough this relic is to crack? You''d think something this simple would come with a manual. We''ve used the Khumaak for a long time as a symbol of our defiance. But the presence of one little rock adds a whole new level of meaning. 

Oh yeah. Welcome, <Character>! I take it that Nilf sent you over? You''ve earned a lot of trust with the Republic, and I hope we can continue that trend. 

I was one of the Republic University scholars to take a look at the Wildfire Khumaak for the Brutors. I was also sent to look over the documents you recovered from the RSS. So, essentially, I''ve been with you from the beginning, lurking in the shadows. That''s what we historians do: Skulk about in dusty corners looking for information. It''s a dirty job, but somebody has to do it.

The Ammatar “sister” that was the source of all the information was based in a nearby cathedral called the Church of the Obsidian. I''ve contacted the church, and she''s no longer there. In fact, I have no clue where she went. She did make reference to some artifact located in the church. They''ve denied me access to the church grounds – those bloody lapdogs – but I think you might be able to persuade them. Get inside that church and get something called the “Blood Obsidian Orb.” I don''t know what to do with it after that, but I think that''s our first step.

Take this analyzer with you, and use it on the church. It should help you unearth the Blood Obsidian Orb.', 'WF_Church_Of_The_Obsidian'),
  (80183, 0, 2, 32283, 1, 0, 500007, 0, 0, 'Retrieve 1x Engraved Blood Obsidian Tablet (0.1 m3)', 'I''m still going to need some more time with the Blood Obsidian Orb, but I have managed to glean some information from it. My theory is that the orb acts as a protective case for something hidden inside of it. I''d hate to break open a priceless relic of the past, and I''m trying to avoid that situation as much as possible.

Something was written on the orb as well, and I want you to go find what it is. There''s a tablet that goes along with the orb and supposedly tells more about its purpose. The Church of the Obsidian did not have this tablet, as far as I can tell, but I was able to decipher a code on the orb that told of the tablet''s location: the ruins of an old Ammatar church.

The church is an ancient place, and a prime spot for archaeology. Use that analyzer I gave you for the Church of the Obsidian. I expect there to be a lot of stuff to gather among those ruins. Bring back whatever you can, but I''m especially looking for the Blood Obsidian Tablet. When you come back, I expect that I can crack this thing wide open…figuratively speaking, of course.', 'WF_Heresiology'),
  (80184, 0, 2, 3812, 1, 0, 500011, 16730, 1, 'Insert 1x Obsidian Datacore (0.1 m3) into the Chapel Container', 'I''ve been a scholar and a historian for many years now, so I know how to handle ancient relics. I will confess, though, that I was about to cut right into the middle of the Blood Obsidian Orb to see what was inside. Yes, I was getting rather desperate for answers. Luckily, the tablet you uncovered was more than just mystical mumbo-jumbo and flowery metaphors. Rather, it was practically an instruction manual to open the orb. Spirits be praised!

I found this datacore inside the orb. According to the tablet, there''s a church in this area called the Wildfire Life Chapel. Inside the chapel is something called the Book of St. Arzad. I think that was what you came across in those ruins: parts of this lost book. My, from the looks of it, this is heavy stuff. I haven''t had the chance to piece it all together yet, but I have a feeling that this book goes beyond a simple relic like the Wildfire Khumaak and into much bigger territory. 

This book must be recovered. I''m almost certain that it''s in the chapel. Put the datacore in that chapel and see what comes of it. It''s our only hope. See what happens.', 'WF_Wildfire_mission'),
  (80185, 0, 2, 32294, 1, 0, 500011, 0, 0, 'Retrieve 1x Book Of St. Arzad (0.1 m3)', 'We need that book, and that RSS agent has it. This is my worst fear realized: Karkoti Rend was trying to get his hands on the book. The RSS claims that he was working alone, but I don''t know how true that really is.

Either way, I''ve tracked down his location to a nearby Angel Cartel base. This compounds the urgency of this mission. I barely trust the RSS as it is, but the Angels are outright scum. Go get the book from there, but make sure it''s safe. The book is of the utmost priority!', 'WF_Stillwater'),
  (80186, 0, 1, 0, 0, 0, 0, 0, 0, 'Choose your path', 'I have devoted much of my life to history of all sorts, from its unflappable truths to its infallible lies to the nuanced information systems found throughout the chronicles, artifacts, disputed ruins, and subjective quagmires. Despite what people think, history is a shapeless void of nonsense, akin to space''s vacuum or the cold, thoughtless smattering of stars that we call home. There is no meaning besides what we make of it; we do not know truth outside of the lenses we place over this shapeless void, the teeming nothingness, the endless morass of information.

This book troubles me. I do not know what to make of it, nor do I know how to proceed. If what this book says is true, and the chances are good that at least some of it is, then the Starkmanir did not begin their rebellion out of a thirst for independence, but rather out of loyalty to their Holder, a man known to them as St. Arzad. The Wildfire Khumaak, a symbol he stole from them, is not an enduring relic of independence, nor is it a glowing tribute to the spirit of the Matari people. Rather, it is an artifact devoted to the memory of Starkmanir captivity, an heirloom of oppression, but also an endearing homage to a fallen captor. 

In other words, the Wildfire Khumaak subverts the defiance of my people. 

That is, of course, if this is true. All we have is the relic and this book. The story of St. Arzad is practically hidden or buried in the Amarr history: I can''t find a trace of it. For all we know, this is apocryphal heresy, a rash account of history, a fever dream from some demented Starkmanir, or an outright hoax.

I don''t know. I honestly don''t know. I''m torn on what to do, and thus I will put the choice in your hands. I don''t want the responsibility for what happens to this text. There are two places we can take it: Return it to the Defiants and reunite it with the Wildfire Khumaak, or donate it to a Gallente historian I know. The former option gives the book back its people, though I don''t know how they''ll approach the text (or even if they''ll release it to the general public, which to me seems a shame). The latter will keep the book safe and make it available for the greater good. Even though it''s a troubling account, it''s still history, and it still needs to be known.

The choice is in your hands. 

[[Retraction]] >>
As I said, I leave the decision in your hands. This book is a remnant of the Minmatar people, and thus it probably belongs with my brethren. If you feel the same way, then take the Book of St. Arzad back to Oggur Marendei, the Defiant member you spoke with earlier. They have the Wildfire Khumaak, and they should have this book as well. 

Take this book to Oggur Marendei at the Defiant base. Let them deal with it. It''s out of my hands. 

[[Revelation (mission)|Revelation]] >>
Posmon Aubenard is a friend of mine, a Gallente historian working with the University of Caille. He''s a trusted academic, an astute scholar, and a respected member of the university''s staff. If you take this book to him, I know that he will preserve it and keep it safe, possibly even put the book in the proper historical light with a best-selling book of his own in a few years. That might not sit well with my Minmatar brethren, but at least we won''t lose the truth, or whatever part of the truth this book represents. The Defiants can keep the Wildfire Khumaak – although I''m sure Posmon would love to get his hands on it – but it''s just an empty symbol without the book. Go and give the Book of St. Arzad to him, and let him keep it safe. It''s the right thing to do. Isn''t it?', 'WF_With_Great_Power'),
  (80187, 0, 2, 32294, 1, 0, 0, 0, 0, 'Deliver 1x Book Of St. Arzad (0.1 m3)', 'Posmon Aubenard is a friend of mine, a Gallente historian working with the University of Caille. He''s a trusted academic, an astute scholar, and a respected member of the university''s staff. If you take this book to him, I know that he will preserve it and keep it safe, possibly even put the book in the proper historical light with a best-selling book of his own in a few years. That might not sit well with my Minmatar brethren, but at least we won''t lose the truth, or whatever part of the truth this book represents. The Defiants can keep the Wildfire Khumaak – although I''m sure Posmon would love to get his hands on it – but it''s just an empty symbol without the book. Go and give the Book of St. Arzad to him, and let him keep it safe. It''s the right thing to do. Isn''t it?', 'WF_Revelation_mission'),
  (80188, 0, 2, 32294, 1, 0, 0, 0, 0, 'Deliver 1x Book Of St. Arzad (0.1 m3)', 'As I said, I leave the decision in your hands. This book is a remnant of the Minmatar people, and thus it probably belongs with my brethren. If you feel the same way, then take the Book of St. Arzad back to Oggur Marendei, the Defiant member you spoke with earlier. They have the Wildfire Khumaak, and they should have this book as well.

Take this book to Oggur Marendei at the Defiant base. Let them deal with it. It''s out of my hands.', 'WF_Retraction')
ON DUPLICATE KEY UPDATE objectiveType=VALUES(objectiveType), itemTypeID=VALUES(itemTypeID), itemQty=VALUES(itemQty), npcCount=VALUES(npcCount), npcFactionID=VALUES(npcFactionID), structureTypeID=VALUES(structureTypeID), structureQty=VALUES(structureQty), objectiveText=VALUES(objectiveText), briefingText=VALUES(briefingText), note=VALUES(note);
-- +migrate Down
DELETE FROM `missionObjectives` WHERE `missionID` BETWEEN 80170 AND 80188;
