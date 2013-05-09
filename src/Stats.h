/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing object plugin for 3DSMax)

	FILE: Stats.h

	DESCRIPTION: Place to set all the stats related functions

	CREATED BY: Benoît Leveau

	HISTORY: 16/09/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#pragma once


#ifndef DO_STATS

#define STATS(x)
#define USE_TIMER
#define TIMER_DESC(x, y)
#define TIMER(x)
#define OUTPUT_STATS(x)

#else // DO_STATS

#include <string>
#include <ostream>
#include <windows.h>
#include <stdio.h>
#include <map>

#define TM_COUNTS		1<<0
#define TM_RECURSIVE	1<<1
#define TM_TOTAL		1<<2
#define TM_AVERAGE		1<<4

#define STAT_PATH "F:\\Visual Studio Projects\\3DSMax\\Projects\\stats\\"

extern std::string GetStdString(float value);
extern std::string GetStdString(int value);

struct TimerStat{
	int rec_level;
	int nb_calls;
	int max_rec_level;
	float fTotalTime;
	bool bDoAverage;
	bool bDoCount;
	TimerStat():rec_level(0),nb_calls(0),max_rec_level(0),fTotalTime(0.f),bDoAverage(false),bDoCount(false){}
};

class GlobalTimer{
	friend std::ostream &operator<<(std::ostream &o, const GlobalTimer&);
	std::string strAllTimers;
	std::map<std::string, TimerStat> mapTimers;
public:
	GlobalTimer(){};
	~GlobalTimer(){};
	void Add(std::string strTimer, float fTimeElapsed){
		strAllTimers += strTimer + " " + GetStdString(fTimeElapsed) + "\n";
	}
	TimerStat *GetStats(std::string &str){
		//if (mapTimers.find(str) == mapTimers.end())
		//	mapTimers[str] = TimerStat();
		return &mapTimers[str];
	}
};

extern std::ostream &operator<<(std::ostream &o, const GlobalTimer&);

class LocalTimer{
	int flag;
	int nTimer;
	float fTimeElapsed;
	std::string strDescription;
	GlobalTimer &_myTimer;
public:
	LocalTimer(int flag, GlobalTimer &_myTimer, std::string strFunction, int nLine, char *szAddDescription=NULL): flag(flag), _myTimer(_myTimer){
		strDescription = strFunction + " (" + GetStdString(nLine) + ")";
		if (szAddDescription){
			strDescription += " - ";
			strDescription += szAddDescription;
		}
		TimerStat *stat = _myTimer.GetStats(strDescription);
		if (flag & TM_COUNTS){
			++stat->nb_calls;
			stat->bDoCount = true;
		}
		if (flag & TM_AVERAGE)
			stat->bDoAverage = true;
		if (TM_RECURSIVE){
			++stat->rec_level;
			if (stat->rec_level>stat->max_rec_level) stat->max_rec_level = stat->rec_level;
		}
		nTimer = GetTickCount();
	}
	~LocalTimer(){
		int nTime = GetTickCount();
		TimerStat *stat = _myTimer.GetStats(strDescription);
		if (flag & TM_RECURSIVE)
			--stat->rec_level;
		if (stat->rec_level==0 || !(flag & TM_RECURSIVE)){
			fTimeElapsed = ((float)(nTime-nTimer))/1000.0f;
			stat->fTotalTime += fTimeElapsed;
			if (flag & TM_RECURSIVE)
				strDescription+=" Max Recursion: " + GetStdString(stat->max_rec_level) + "\n";
			if (flag & TM_TOTAL)
				strDescription+=" Time elapsed: " + GetStdString(fTimeElapsed) + "\n";
			if (flag & TM_RECURSIVE || flag & TM_TOTAL)
				_myTimer.Add(strDescription, fTimeElapsed);
		}
	}
};

#define STATS(x) x
#define USE_TIMER GlobalTimer _myTimer;
#define	TIMER(x) LocalTimer _localTimer(x, _myTimer, __FUNCTION__, __LINE__)
#define	TIMER_DESC(x, y) LocalTimer _localTimer__LINE__(x, _myTimer, __FUNCTION__, __LINE__, y)
#define OUTPUT_STATS(x) {std::ofstream statStream((std::string(STAT_PATH)+("stats_")+std::string(x)+(".txt")).c_str());statStream<<*this;statStream<<_myTimer;statStream.close();}

#endif // DO_STATS