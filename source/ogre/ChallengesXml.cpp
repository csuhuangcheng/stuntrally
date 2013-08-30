#include "pch.h"
#include "common/Defines.h"
#include "ChallengesXml.h"
#include "common/TracksXml.h"
#include "tinyxml.h"
using namespace Ogre;


ChallTrack::ChallTrack()  //  defaults
	:name("J1-T"), laps(1), reversed(0)
	,passPoints(-1.f), timeNeeded(-1.f), passPos(-1.f)  // pass
{	}

Chall::Chall()  //  defaults
	:ver(1), diff(1), length(0.f), type(0), time(0.f)
	,name("none"), descr("none")
	,sim_mode("normal")
	,damage_type(2), boost_type(1), flip_type(2), rewind_type(1)
	,minimap(1), chk_arr(0), chk_beam(0), trk_ghost(1)
	// abs, tcs, autoshift, autorear
	// max dmg%, off road time-
	,avgPoints(-1.f), totalTime(-1.f), avgPos(-1.f)  // pass
	,carChng(0)
{	}


//  Load challenges
//-------------------------------------------------------------------------------------------------------------
bool ChallXml::LoadXml(std::string file, TracksXml* trks)
{
	TiXmlDocument doc;
	if (!doc.LoadFile(file.c_str()))  return false;
		
	TiXmlElement* root = doc.RootElement();
	if (!root)  return false;

	//  clear
	all.clear();

	///  challs
	const char* a;
	TiXmlElement* eCh = root->FirstChildElement("challenge");
	while (eCh)
	{
		Chall c;  // defaults in ctor
		a = eCh->Attribute("name");			if (a)  c.name = std::string(a);
		a = eCh->Attribute("descr");		if (a)  c.descr = std::string(a);
		a = eCh->Attribute("ver");			if (a)  c.ver = s2i(a);
		a = eCh->Attribute("difficulty");	if (a)  c.diff = s2i(a);
		a = eCh->Attribute("type");			if (a)  c.type = s2i(a);
		
		TiXmlElement* eSim = eCh->FirstChildElement("sim");
		if (eSim)
		{
			a = eSim->Attribute("mode");		if (a)  c.sim_mode = std::string(a);
			a = eSim->Attribute("damage");		if (a)  c.damage_type = s2i(a);  // range chk..
			a = eSim->Attribute("boost");		if (a)  c.boost_type = s2i(a);
			a = eSim->Attribute("flip");		if (a)  c.flip_type = s2i(a);
			a = eSim->Attribute("rewind");		if (a)  c.rewind_type = s2i(a);
			a = eSim->Attribute("carChng");		if (a)  c.carChng = s2i(a) > 0;
		}
		//  cars
		TiXmlElement* eCarT = eCh->FirstChildElement("cartype");
		if (eCarT)
		{
			a = eCarT->Attribute("names");
			if (a)
			{	String s = a;
				c.carTypes = StringUtil::split(s, "|");
			}
		}
		//<car names="ES|S1" />
		TiXmlElement* eCar = eCh->FirstChildElement("car");
		if (eCar)
		{
			a = eCar->Attribute("names");
			if (a)
			{	String s = a;
				c.cars = StringUtil::split(s, "|");
			}
		}
		
		TiXmlElement* eHud = eCh->FirstChildElement("hud");
		if (eHud)
		{
			a = eHud->Attribute("minimap");		if (a)  c.minimap = s2i(a) > 0;
			a = eHud->Attribute("chkArrow");	if (a)  c.chk_arr = s2i(a) > 0;
			a = eHud->Attribute("chkBeam");		if (a)  c.chk_beam = s2i(a) > 0;
			a = eHud->Attribute("trkGhost");	if (a)  c.trk_ghost = s2i(a) > 0;
		}
		TiXmlElement* ePass = eCh->FirstChildElement("pass");
		if (ePass)
		{
			a = ePass->Attribute("totalTime");	if (a)  c.totalTime = s2r(a);
			a = ePass->Attribute("avgPoints");	if (a)  c.avgPoints = s2r(a);
			a = ePass->Attribute("avgPos");		if (a)  c.avgPos = s2r(a);
		}

		//  tracks
		TiXmlElement* eTrks = eCh->FirstChildElement("tracks");
		if (eTrks)
		{
			TiXmlElement* eTr = eTrks->FirstChildElement("t");
			while (eTr)
			{
				ChallTrack t;
				a = eTr->Attribute("name");		if (a)  t.name = std::string(a);
				a = eTr->Attribute("reversed");	if (a)  t.reversed = s2i(a) > 0;
				a = eTr->Attribute("laps");		if (a)  t.laps = s2i(a);
				
				a = eTr->Attribute("passTime");   if (a)  t.timeNeeded = s2r(a);
				a = eTr->Attribute("passPoints"); if (a)  t.passPoints = s2r(a);
				a = eTr->Attribute("passPos");    if (a)  t.passPos = s2i(a);

				c.trks.push_back(t);
				eTr = eTr->NextSiblingElement("t");
			}
		}

		all.push_back(c);
		eCh = eCh->NextSiblingElement("challenge");
	}
	
	///  get champs total time (sum tracks times)
	if (trks)
	for (int c=0; c < all.size(); ++c)
	{
		Chall& l = all[c];
		float allTime = 0.f;
		for (int i=0; i < l.trks.size(); ++i)
		{
			const ChallTrack& trk = l.trks[i];

			float time = trks->times[trk.name] * trk.laps;
			allTime += time;  // sum trk time, total champ time
		}
		l.time = allTime;
	}
	return true;
}

//  progress
//-------------------------------------------------------------------------------------------------------------

ProgressTrackL::ProgressTrackL() :
	points(0.f), time(0.f), pos(0)
{	}

ProgressChall::ProgressChall() :
	curTrack(0), ver(0),
	avgPoints(0.f), totalTime(0.f), avgPos(0), fin(-1)
{	}


//  Load progress
//-------------------------------------------------------------------------------------------------------------
bool ProgressLXml::LoadXml(std::string file)
{
	TiXmlDocument doc;
	if (!doc.LoadFile(file.c_str()))  return false;
		
	TiXmlElement* root = doc.RootElement();
	if (!root)  return false;

	//  clear
	chs.clear();

	const char* a;
	TiXmlElement* eCh = root->FirstChildElement("chall");
	while (eCh)
	{
		ProgressChall pc;
		a = eCh->Attribute("name");	if (a)  pc.name = std::string(a);
		a = eCh->Attribute("ver");	if (a)  pc.ver = s2i(a);
		a = eCh->Attribute("cur");	if (a)  pc.curTrack = s2i(a);
		a = eCh->Attribute("car");	if (a)  pc.car = std::string(a);

		a = eCh->Attribute("p");	if (a)  pc.avgPoints = s2r(a);
		a = eCh->Attribute("t");	if (a)  pc.totalTime = s2r(a);
		a = eCh->Attribute("n");	if (a)  pc.avgPos = s2r(a);
		a = eCh->Attribute("z");	if (a)  pc.fin = s2i(a);
		
		//  tracks
		TiXmlElement* eTr = eCh->FirstChildElement("t");
		while (eTr)
		{
			ProgressTrackL pt;
			a = eTr->Attribute("p");	if (a)  pt.points = s2r(a);
			a = eTr->Attribute("t");	if (a)  pt.time = s2r(a);
			a = eTr->Attribute("n");	if (a)  pt.pos = s2i(a);
			
			pc.trks.push_back(pt);
			eTr = eTr->NextSiblingElement("t");
		}

		chs.push_back(pc);
		eCh = eCh->NextSiblingElement("chall");
	}
	return true;
}

//  Save progress
bool ProgressLXml::SaveXml(std::string file)
{
	TiXmlDocument xml;	TiXmlElement root("progress");

	for (int i=0; i < chs.size(); ++i)
	{
		const ProgressChall& pc = chs[i];
		TiXmlElement eCh("chall");
			eCh.SetAttribute("name",	pc.name.c_str() );
			eCh.SetAttribute("ver",		toStrC( pc.ver ));
			eCh.SetAttribute("cur",		toStrC( pc.curTrack ));
			eCh.SetAttribute("car",		pc.car.c_str() );

			eCh.SetAttribute("p",	fToStr( pc.avgPoints, 2).c_str());
			eCh.SetAttribute("t",	fToStr( pc.totalTime, 1).c_str());
			eCh.SetAttribute("n",	fToStr( pc.avgPos, 2).c_str());
			eCh.SetAttribute("z",	iToStr( pc.fin ).c_str());

			for (int i=0; i < pc.trks.size(); ++i)
			{
				const ProgressTrackL& pt = pc.trks[i];
				TiXmlElement eTr("t");

				eTr.SetAttribute("p",	fToStr( pt.points, 1).c_str());
				eTr.SetAttribute("t",	fToStr( pt.time, 1).c_str());
				eTr.SetAttribute("n",	iToStr( pt.pos ).c_str());
				eCh.InsertEndChild(eTr);
			}
		root.InsertEndChild(eCh);
	}
	xml.InsertEndChild(root);
	return xml.SaveFile(file.c_str());
}