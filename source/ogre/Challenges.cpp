#include "pch.h"
#include "common/Defines.h"
#include "../vdrift/pathmanager.h"
#include "../vdrift/game.h"
#include "OgreGame.h"
#include "../road/Road.h"
#include "common/MultiList2.h"

using namespace std;
using namespace Ogre;
using namespace MyGUI;


///
void App::BackFromChs()
{
	pSet->gui.champ_num = -1;
	pSet->gui.chall_num = -1;
	//pChall = 0;
	//CarListUpd();  // off filtering
}

bool App::isChallGui()
{
	//return imgChall && imgChall->getVisible();
	return pSet->inMenu == MNU_Challenge;
}

void App::tabChallType(MyGUI::TabPtr wp, size_t id)
{
	pSet->chall_type = id;
	ChallsListUpdate();
}


///  Challenges list  fill
//----------------------------------------------------------------------------------------------------------------------
void App::ChallsListUpdate()
{
	const char clrCh[7][8] = {
	//  0 Rally  1 Scenery  2 Endurance  3 Chase  4 Stunts  5 Extreme  6 Test
		"#A0D0FF","#80FF80","#C0FF60","#FFC060","#FF8080","#C0A0E0","#909090" };

	liChalls->removeAllItems();  int n=1;  size_t sel = ITEM_NONE;
	int p = pSet->gui.champ_rev ? 1 : 0;
	for (int i=0; i < chall.all.size(); ++i,++n)
	{
		const Chall& chl = chall.all[i];
		if (chl.type == pSet->chall_type)
		{
			const ProgressChall& pc = progressL[p].chs[i];
			int ntrks = pc.trks.size(), ct = pc.curTrack;
			const String& clr = clrCh[chl.type];
			//String cars = carsXml.colormap[chl.ci->type];  if (cars.length() != 7)  clr = "#C0D0E0";
			
			liChalls->addItem(clr+ toStr(n/10)+toStr(n%10), 0);  int l = liChalls->getItemCount()-1;
			liChalls->setSubItemNameAt(1,l, clr+ chl.name.c_str());
			liChalls->setSubItemNameAt(2,l, clrsDiff[chl.diff]+ TR("#{Diff"+toStr(chl.diff)+"}"));
			liChalls->setSubItemNameAt(3,l, StrChallCars(chl));
			
			liChalls->setSubItemNameAt(4,l, clrsDiff[std::min(8,ntrks*2/3+1)]+ iToStr(ntrks,3));
			liChalls->setSubItemNameAt(5,l, clrsDiff[std::min(8,int(chl.time/3.f/60.f))]+ GetTimeShort(chl.time));
			liChalls->setSubItemNameAt(6,l, ct == 0 || ct == ntrks ? "" :
				clr+ fToStr(100.f * ct / ntrks,0,3)+" %");

			liChalls->setSubItemNameAt(7,l, " "+ StrPrize(pc.fin));
			liChalls->setSubItemNameAt(8,l, clr+ (pc.fin >= 0 ? fToStr(pc.avgPoints,1,5) : ""));
			if (n-1 == pSet->gui.chall_num)  sel = l;
	}	}
	liChalls->setIndexSelected(sel);
}


///  Challenges list  sel changed,  fill Stages list
//----------------------------------------------------------------------------------------------------------------------
void App::listChallChng(MyGUI::MultiList2* chlist, size_t id)
{
	if (id==ITEM_NONE || liChalls->getItemCount() == 0)  return;

	int nch = s2i(liChalls->getItemNameAt(id).substr(7))-1;
	if (nch < 0 || nch >= chall.all.size())  {  LogO("Error chall sel > size.");  return;  }

	CarListUpd();  // filter car list

	//  fill stages
	liStages->removeAllItems();

	int n = 1, p = pSet->gui.champ_rev ? 1 : 0;
	const Chall& ch = chall.all[nch];
	int ntrks = ch.trks.size();
	for (int i=0; i < ntrks; ++i,++n)
	{
		const ChallTrack& trk = ch.trks[i];
		StageListAdd(n, trk.name, trk.laps,
			"#E0F0FF"+fToStr(progressL[p].chs[nch].trks[i].points,1,3));
	}
	if (edChDesc)  edChDesc->setCaption(ch.descr);

	UpdChallDetail(nch);
}


//  list allowed cars types and cars
String App::StrChallCars(const Chall& ch)
{
	String str;
	int i,s;

	s = ch.carTypes.size();
	for (i=0; i < s; ++i)
	{
		const String& ct = ch.carTypes[i];
			str += carsXml.colormap[ct];  // car type color
		str += ct;
		if (i+1 < s)  str += ",";
	}
	//int id = carsXml.carmap[*i];
	//carsXml.cars[id-1];
	if (!str.empty())
		str += " ";
	
	s = ch.cars.size();
	for (i=0; i < s; ++i)
	{
		const String& c = ch.cars[i];
			int id = carsXml.carmap[c]-1;  // get car color from type
			if (id >= 0)  str += carsXml.colormap[ carsXml.cars[id].type ];
		str += c;
		if (i+1 < s)  str += ",";
	}
	return str;
}

//  test if car is in challenge allowed cars or types
bool App::IsChallCar(String name)
{
	if (!liChalls || liChalls->getIndexSelected()==ITEM_NONE)  return true;

	int chId = s2i(liChalls->getItemNameAt(liChalls->getIndexSelected()).substr(7))-1;
	const Chall& ch = chall.all[chId];

	int i,s;
	if (!ch.carTypes.empty())
	{	s = ch.carTypes.size();

		int id = carsXml.carmap[name]-1;
		if (id >= 0)
		{
			String type = carsXml.cars[id].type;

			for (i=0; i < s; ++i)
				if (type == ch.carTypes[i])  return true;
	}	}
	if (!ch.cars.empty())
	{	s = ch.cars.size();

		for (i=0; i < s; ++i)
			if (name == ch.cars[i])  return true;
	}
	return false;
}


///  chall start
//---------------------------------------------------------------------
void App::btnChallStart(WP)
{
	if (liChalls->getIndexSelected()==ITEM_NONE)  return;
	pSet->gui.chall_num = s2i(liChalls->getItemNameAt(liChalls->getIndexSelected()).substr(7))-1;

	//  if already finished, restart - will loose progress and scores ..
	int chId = pSet->gui.chall_num, p = pSet->game.champ_rev ? 1 : 0;
	LogO("|] Starting chall: "+toStr(chId)+(p?" rev":""));
	ProgressChall& pc = progressL[p].chs[chId];
	if (pc.curTrack == pc.trks.size())
	{
		LogO("|] Was at 100%, restarting progress.");
		pc.curTrack = 0;  //pc.score = 0.f;
	}
	// change btn caption to start/continue/restart ?..

	btnNewGame(0);
}

///  stage start / end
//----------------------------------------------------------------------------------------------------------------------
void App::btnChallStageStart(WP)
{
	//  check if chall ended
	int chId = pSet->game.chall_num, p = pSet->game.champ_rev ? 1 : 0;
	ProgressChall& pc = progressL[p].chs[chId];
	const Chall& ch = chall.all[chId];
	bool last = pc.curTrack == ch.trks.size();

	LogO("|] This was stage " + toStr(pc.curTrack) + "/" + toStr(ch.trks.size()) + " btn");
	if (last)
	{	//  show end window, todo: start particles..
		mWndChallStage->setVisible(false);
		// tutorial, tutorial hard, normal, hard, very hard, scenery, test
		const int ui[8] = {0,1,2,3,4,5,0,0};
		//if (imgChallEnd)
		//	imgChallEnd->setImageCoord(IntCoord(ui[std::min(7, std::max(0, ch.type))]*128,0,128,256));
		mWndChallEnd->setVisible(true);
		return;
	}

	bool finished = pGame->timer.GetLastLap(0) > 0.f;  //?-
	if (finished)
	{
		LogO("|] Loading next stage.");
		mWndChallStage->setVisible(false);
		btnNewGame(0);
	}else
	{
		LogO("|] Starting stage.");
		mWndChallStage->setVisible(false);
		pGame->pause = false;
		pGame->timer.waiting = false;
	}
}

//  stage back
void App::btnChallStageBack(WP)
{
	mWndChallStage->setVisible(false);
	isFocGui = true;  // show back gui
	toggleGui(false);
}

//  chall end
void App::btnChallEndClose(WP)
{
	mWndChallEnd->setVisible(false);
}


///  save progressL and update it on gui
void App::ProgressLSave(bool upgGui)
{
	progressL[0].SaveXml(PATHMANAGER::UserConfigDir() + "/progressL.xml");
	progressL[1].SaveXml(PATHMANAGER::UserConfigDir() + "/progressL_rev.xml");
	if (!upgGui)
		return;
	ChallsListUpdate();
	listChallChng(liChalls, liChalls->getIndexSelected());
}


///  challenge advance logic
//  caution: called from GAME, 2nd thread, no Ogre stuff here
//----------------------------------------------------------------------------------------------------------------------
void App::ChallengeAdvance(float timeCur/*total*/)
{
	int chId = pSet->game.chall_num, p = pSet->game.champ_rev ? 1 : 0;
	ProgressChall& pc = progressL[p].chs[chId];
	ProgressTrackL& pt = pc.trks[pc.curTrack];
	const Chall& ch = chall.all[chId];
	const ChallTrack& trk = ch.trks[pc.curTrack];
	LogO("|] --- Chall end: " + ch.name);

	///  compute track  poins  --------------
	float timeTrk = tracksXml.times[trk.name];
	if (timeTrk < 1.f)
	{	LogO("|] Error: Track has no best time !");  timeTrk = 10.f;	}
	timeTrk *= trk.laps;

	LogO("|] Track: " + trk.name);
	LogO("|] Your time: " + toStr(timeCur));
	LogO("|] Best time: " + toStr(timeTrk));

	float carMul = GetCarTimeMul(pSet->game.car[0], pSet->game.sim_mode);
	float points = 0.f;  int pos = 0;

	#if 1  // test score +- sec diff
	for (int i=-2; i <= 2; ++i)
	{
		pos = GetRacePos(timeCur + i*2.f, timeTrk, carMul, true, &points);
		LogO("|] var, add time: "+toStr(i*2)+" sec, points: "+fToStr(points,2));
	}
	#endif
	pos = GetRacePos(timeCur, timeTrk, carMul, true, &points);

	pt.time = timeCur;  pt.points = points;  pt.pos = pos;

	
	///  Pass Stage  --------------
	bool passed = true, pa;
	if (trk.timeNeeded > 0.f)
	{
		pa = pt.time <= trk.timeNeeded;
		LogO("]] TotTime: " + GetTimeString(pt.time) + "  Needed: " + GetTimeString(trk.timeNeeded) + "  Passed: " + (pa ? "yes":"no"));
		passed &= pa;
	}
	if (trk.passPoints > 0.f)
	{
		pa = pt.points >= trk.passPoints;
		LogO("]] Points: " + fToStr(pt.points,1) + "  Needed: " + fToStr(trk.passPoints,1) + "  Passed: " + (pa ? "yes":"no"));
		passed &= pa;
	}
	if (trk.passPos > 0)
	{
		pa = pt.pos <= trk.passPos;
		LogO("]] Pos: " + toStr(pt.pos) + "  Needed: " + toStr(trk.passPos) + "  Passed: " + (pa ? "yes":"no"));
		passed &= pa;
	}
	LogO(String("]] Passed total: ") + (passed ? "yes":"no"));

	
	//  --------------  advance  --------------
	bool last = pc.curTrack+1 == ch.trks.size();
	LogO("|] This was stage " + toStr(pc.curTrack+1) + "/" + toStr(ch.trks.size()));
	if (!last || (last && !passed))
	{
		//  show stage end [window]
		pGame->pause = true;
		pGame->timer.waiting = true;

		ChallFillStageInfo(true);  // cur track
		mWndChallStage->setVisible(true);
		
		if (pc.curTrack == 0)  // save picked car
			pc.car = pSet->game.car[0];
		if (passed)
			pc.curTrack++;  // next stage
		ProgressLSave();
	}else
	{
		//  chall ended
		pGame->pause = true;
		pGame->timer.waiting = true;

		ChallFillStageInfo(true);  // cur track
		mWndChallStage->setVisible(true);

		//  compute avg
		float avgPos = 0.f, avgPoints = 0.f, totalTime = 0.f;
		int ntrks = pc.trks.size();
		for (int t=0; t < ntrks; ++t)
		{
			const ProgressTrackL& pt = pc.trks[t];
			totalTime += pt.time;  avgPoints += pt.points;  avgPos += pt.pos;
		}
		avgPoints /= ntrks;  avgPos /= ntrks;

		//  save
		pc.curTrack++;
		pc.totalTime = totalTime;  pc.avgPoints = avgPoints;  pc.avgPos = avgPos;
		LogO("|] Chall finished");
		String s = 
			TR("#E0E0FF#{Challenge}") + ": " + ch.name +"\n\n"+
			TR("#A0D0F0#{TotalScore}") +"\n";
		#define sPass(pa)  (pa ? TR("  #00FF00#{Passed}") : TR("  #FF8000#{DidntPass}"))

		///  Pass Challenge  --------------
		String ss;
		bool passed = true, pa;
		if (ch.totalTime > 0.f)
		{
			pa = pc.totalTime <= ch.totalTime;
			LogO("]] TotalTime: " + GetTimeString(pc.totalTime) + "  Needed: " + GetTimeString(ch.totalTime) + "  Passed: " + (pa ? "yes":"no"));
			ss += TR("#D8C0FF#{TBTime}")     + ": " + GetTimeString(pc.totalTime)+ "  / " + GetTimeString(ch.totalTime) + sPass(pa) +"\n";
			passed &= pa;
		}
		if (ch.avgPoints > 0.f)
		{
			pa = pc.avgPoints >= ch.avgPoints;
			LogO("]] AvgPoints: " + fToStr(pc.avgPoints,1) + "  Needed: " + fToStr(ch.avgPoints,1) + "  Passed: " + (pa ? "yes":"no"));
			ss += TR("#D8C0FF#{TBPoints}")   + ": " + fToStr(pc.avgPoints,2,5) +   "  / " + fToStr(ch.avgPoints,2,5) + sPass(pa) +"\n";
			passed &= pa;
		}else  //if (passed)  // write points always on end
			ss += TR("#C0E0FF#{TBPoints}")   + ": " + fToStr(pc.avgPoints,2,5) /*+ sPass(pa)*/ +"\n";

		if (ch.avgPos > 0.f)
		{
			pa = pc.avgPos <= ch.avgPos;
			LogO("]] AvgPos: " + fToStr(pc.avgPos,1) + "  Needed: " + fToStr(ch.avgPos,1) + "  Passed: " + (pa ? "yes":"no"));
			ss += TR("#D8C0FF#{TBPosition}") + ": " + fToStr(pc.avgPos,2,5) +      "  / " + fToStr(ch.avgPos,2,5) + sPass(pa) +"\n";
			passed &= pa;
		}
		LogO(String("]] Passed total: ") + (passed ? "yes":"no"));
		
		pc.fin = passed ? 2 : -1;
		if (passed)  //todo: 0 bronze, 1 silver, 2 gold ..
			s += TR("\n#E0F0F8#{Prize}: ")+StrPrize(pc.fin)+"\n\n";
		s += ss;
		
		ProgressLSave();

		//  upd chall end [window]
		imgChallFail->setVisible(!passed);
		imgChallCup->setVisible( passed);  const int ui[8] = {2,3,4};
		imgChallCup->setImageCoord(IntCoord(ui[std::min(2, std::max(0, pc.fin))]*128,0,128,256));

		txChallEndC->setCaption(passed ? TR("#{ChampEndCongrats}") : "");
		txChallEndF->setCaption(passed ? TR("#{ChallEndFinished}") : TR("#{Finished}"));

		edChallEnd->setCaption(s);
		//mWndChallEnd->setVisible(true);  // show after stage end
		LogO("|]");
	}
}

String App::StrPrize(int i)  //-1..2
{
	const static String prAr[4] = {/*"#C0D0E0--"*/"","#D0B050#{Bronze}","#D8D8D8#{Silver}","#E8E050#{Gold}"};
	return TR(prAr[i+1]);
}


//  stage wnd text
//----------------------------------------------------------------------------------------------------------------------
void App::ChallFillStageInfo(bool finished)
{
	int chId = pSet->game.chall_num, p = pSet->game.champ_rev ? 1 : 0;
	const ProgressChall& pc = progressL[p].chs[chId];
	const ProgressTrackL& pt = pc.trks[pc.curTrack];
	const Chall& ch = chall.all[chId];
	const ChallTrack& trk = ch.trks[pc.curTrack];
	bool last = pc.curTrack+1 == ch.trks.size();

	String s;
	s = "#80FFE0"+ ch.name + "\n\n" +
		"#80FFC0"+ TR("#{Stage}") + ":  " + toStr(pc.curTrack+1) + " / " + toStr(ch.trks.size()) + "\n" +
		"#80FF80"+ TR("#{Track}") + ":  " + trk.name + "\n\n";

	if (!finished)  // track info at start
	{
		int id = tracksXml.trkmap[trk.name];
		if (id > 0)
		{
			const TrackInfo* ti = &tracksXml.trks[id-1];
			s += "#A0D0FF"+ TR("#{Difficulty}:  ") + clrsDiff[ti->diff] + TR("#{Diff"+toStr(ti->diff)+"}") + "\n";
			if (road)
			{	Real len = road->st.Length*0.001f * (pSet->show_mph ? 0.621371f : 1.f);
				s += "#A0D0FF"+ TR("#{Distance}:  ") + "#B0E0FF" + fToStr(len, 1,4) + (pSet->show_mph ? " mi" : " km") + "\n\n";
		}	}
	}

	if (finished)  // stage
	{
		s += "#80C0FF"+TR("#{Finished}.") + "\n\n";
		
		///  Pass Stage  --------------
		bool passed = true, pa;
		if (trk.timeNeeded > 0.f)
		{
			pa = pt.time <= trk.timeNeeded;
			s += TR("#D8C0FF#{TBTime}: ") + GetTimeString(pt.time) + "  / " + GetTimeString(trk.timeNeeded) + sPass(pa) +"\n";
			passed &= pa;
		}
		if (trk.passPoints > 0.f)
		{
			pa = pt.points >= trk.passPoints;
			s += TR("#D8C0FF#{TBPoints}: ") + fToStr(pt.points,1) + "  / " + fToStr(trk.passPoints,1) + sPass(pa) +"\n";
			passed &= pa;
		}
		if (trk.passPos > 0)
		{
			pa = pt.pos <= trk.passPos;
			s += TR("#D8C0FF#{TBPosition}: ") + toStr(pt.pos) + "  / " + toStr(trk.passPos) + sPass(pa) +"\n";
			passed &= pa;
		}
		
		if (passed)	s += "\n\n"+TR(last ? "#00FF00#{Continue}." : "#00FF00#{NextStage}.");
		else		s += "\n\n"+TR("#FF6000#{RepeatStage}.");
	}
	else
	{	///  Pass needed  --------------
		s += "#F0F060"+TR("#{Needed}") +"\n";
		if (trk.timeNeeded > 0.f)
			s += TR("  #D8C0FF#{TBTime}: ") + GetTimeString(trk.timeNeeded) +"\n";
		if (trk.passPoints > 0.f)
			s += TR("  #D8C0FF#{TBPoints}: ") + fToStr(trk.passPoints,1) +"\n";
		if (trk.passPos > 0)
			s += TR("  #D8C0FF#{TBPosition}: ") + toStr(trk.passPos) +"\n";
		if (road)
			s += "\n#A8B8C8"+ road->sTxtDesc;
	}

	edChallStage->setCaption(s);
	btChallStage->setCaption(finished ? TR("#{Continue}") : TR("#{Start}"));
	
	//  preview image
	if (!finished)  // only at chall start
	{
		ResourceGroupManager& resMgr = ResourceGroupManager::getSingleton();
		Ogre::TextureManager& texMgr = Ogre::TextureManager::getSingleton();

		String path = PathListTrkPrv(0, trk.name), sGrp = "TrkPrvCh";
		resMgr.addResourceLocation(path, "FileSystem", sGrp);  // add for this track
		resMgr.unloadResourceGroup(sGrp);
		resMgr.initialiseResourceGroup(sGrp);

		if (imgChallStage)
		{	try
			{	s = "view.jpg";
				texMgr.load(path+s, sGrp, TEX_TYPE_2D, MIP_UNLIMITED);  // need to load it first
				imgChallStage->setImageTexture(s);  // just for dim, doesnt set texture
				imgChallStage->_setTextureName(path+s);
			} catch(...) {  }
		}
		resMgr.removeResourceLocation(path, sGrp);
	}
}


//  chall details  (gui tab Stages)
//----------------------------------------------------------------------------------------------------------------------
void App::UpdChallDetail(int id)
{
	const Chall& ch = chall.all[id];
	int ntrks = ch.trks.size();
	int p = pSet->gui.champ_rev ? 1 : 0;
	
	String s1,s2,clr;
	//s1 += "\n";  s2 += "\n";

	clr = clrsDiff[ch.diff];  // track
	s1 += clr+ TR("#{Difficulty}\n");    s2 += clr+ TR("#{Diff"+toStr(ch.diff)+"}")+"\n";

	clr = clrsDiff[std::min(8,ntrks*2/3+1)];
	s1 += clr+ TR("#{Tracks}\n");        s2 += clr+ toStr(ntrks)+"\n";

	//s1 += "\n";  s2 += "\n";
	clr = clrsDiff[std::min(8,int(ch.time/3.f/60.f))];
	s1 += TR("#80F0E0#{Time} [m:s.]\n"); s2 += "#C0FFE0"+clr+ GetTimeShort(ch.time)+"\n";

	s1 += "\n";  s2 += "\n";  // cars
	s1 += TR("#F08080#{Cars}\n");        s2 += "#FFA0A0" + StrChallCars(ch)+"\n";
	if (ch.carChng)
	{	s1 += TR("#C0B0B0#{CarChange}\n");  s2 += TR("#A0B8A0#{Allowed}")+"\n";  }
	
	s1 += "\n";  s2 += "\n";  // game
	s1 += TR("#D090E0#{Game}")+"\n";     s2 += "\n";
	#define cmbs(cmb, i)  (i>=0 ? cmb->getItemNameAt(i) : TR("#{Any}"))
	s1 += TR("#A0B0C0  #{Simulation}\n");  s2 += "#B0C0D0"+ ch.sim_mode +"\n";
	s1 += TR("#A090E0  #{Damage}\n");      s2 += "#B090FF"+ cmbs(cmbDamage, ch.damage_type) +"\n";
	s1 += TR("#B080C0  #{InputMapRewind}\n"); s2 += "#C090D0"+ cmbs(cmbRewind, ch.rewind_type) +"\n";
	//s1 += "\n";  s2 += "\n";
	s1 += TR("#80C0FF  #{Boost}\n");       s2 += "#90D0FF"+ cmbs(cmbBoost, ch.boost_type) +"\n";
	s1 += TR("#6098A0  #{Flip}\n");        s2 += "#7098A0"+ cmbs(cmbFlip, ch.flip_type) +"\n";

	//s1 += "\n";  s2 += "\n";  //  hud
	//bool minimap, chk_arr, chk_beam, trk_ghost;  // deny using it if false

	s1 += "\n";  s2 += "\n";  // pass challenge
	if (ch.totalTime > 0.f || ch.avgPoints > 0.f || ch.avgPos > 0.f)
	{
		s1 += "\n";  s2 += "\n";
		s1 += TR("#F0F060#{Needed} - #{Challenge}")+"\n";   s2 += "\n";
		if (ch.totalTime > 0.f)  {
			s1 += TR("#D8C0FF  #{TBTime}\n");      s2 += "#F0D8FF"+GetTimeString(ch.totalTime)+"\n";  }
		if (ch.avgPoints > 0.f)  {
			s1 += TR("#D8C0FF  #{TBPoints}\n");    s2 += "#F0D8FF"+fToStr(ch.avgPoints,2,5)+"\n";  }
		if (ch.avgPos > 0.f)  {
			s1 += TR("#D8C0FF  #{TBPosition}\n");  s2 += "#F0D8FF"+fToStr(ch.avgPos,2,5)+"\n";  }
	}
	size_t t = liStages->getIndexSelected();
	if (t != ITEM_NONE)  // pass stage 
	{	const ChallTrack& trk = ch.trks[t];
		if (trk.timeNeeded > 0.f || trk.passPoints > 0.f || trk.passPos > 0)
		{	s1 += "\n";  s2 += "\n";
			s1 += TR("#FFC060#{Needed} - #{Stage}")+"\n";   s2 += "\n";
			if (trk.timeNeeded > 0.f)  {
				s1 += TR("#D8C0FF  #{TBTime}\n");      s2 += "#F0D8FF"+GetTimeString(trk.timeNeeded)+"\n";  }
			if (trk.passPoints > 0.f)  {
				s1 += TR("#D8C0FF  #{TBPoints}\n");    s2 += "#F0D8FF"+fToStr(trk.passPoints,2,5)+"\n";  }
			if (trk.passPos > 0.f)  {
				s1 += TR("#D8C0FF  #{TBPosition}\n");  s2 += "#F0D8FF"+fToStr(trk.passPos,2,5)+"\n";  }
	}	}
	s1 += "\n\n";  s2 += "\n\n";  // prog
	const ProgressChall& pc = progressL[p].chs[id];
	int cur = pc.curTrack, all = chall.all[id].trks.size();
	if (cur > 0)
	{
		s1 += TR("#B0FFFF#{Progress}\n");    s2 += "#D0FFFF"+fToStr(100.f * cur / all,1,5)+" %\n";
		s1 += TR("#E0F0FF#{Prize}\n");       s2 += StrPrize(pc.fin)+"\n";
		s1 += "\n";  s2 += "\n";
		s1 += TR("#D8E0FF  #{TBTime}\n");      s2 += "#F0F8FF"+GetTimeString(pc.totalTime)+"\n";
		s1 += TR("#D8E0FF  #{TBPoints}\n");    s2 += "#F0F8FF"+fToStr(pc.avgPoints,2,5)+"\n";
		s1 += TR("#D8E0FF  #{TBPosition}\n");  s2 += "#F0F8FF"+fToStr(pc.avgPos,2,5)+"\n";
	}
	txtCh->setCaption(s1);  valCh->setCaption(s2);

	//  btn start
	s1 = cur == all ? TR("#{Restart}") : (cur == 0 ? TR("#{Start}") : TR("#{Continue}"));
	btStChall->setCaption(s1);
	btChRestart->setVisible(cur > 0);
}