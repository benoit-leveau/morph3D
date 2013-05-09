/**********************************************************************
 *<
	PROJECT: Morph3D (Morphing object plugin for 3DSMax)

	FILE: Distance.cpp

	DESCRIPTION: Implementation of the Maths Distance Functions

	CREATED BY: Benoît Leveau

	HISTORY: 16/09/06

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#include "StdAfx.h"
#include "GlobalDefines.h"
#include "Stats.h"
#include <ostream>

#ifdef DO_STATS

std::string GetStdString(float value){char szTmp[20];sprintf_s(szTmp, 20, "%.3f", value);return std::string(szTmp);}
std::string GetStdString(int value)  {char szTmp[20];sprintf_s(szTmp, 20, "%d", value);return std::string(szTmp);}

extern std::ostream &operator<<(std::ostream &o, const GlobalTimer& t)
{
	std::string strAllTimers(t.strAllTimers);
	std::map<std::string, TimerStat>::const_iterator it;
	for (it = t.mapTimers.begin(); 
		 it!=t.mapTimers.end(); 
		 ++it){
		if (it->second.bDoAverage){
			strAllTimers += (it->first + std::string(" Average Timer: ") + GetStdString(it->second.fTotalTime/it->second.nb_calls) + "\n");
		}
		else if (it->second.bDoCount){
			strAllTimers += (it->first + std::string(" Nb of calls: ") + GetStdString(it->second.nb_calls) + "\n");
		}
	}
	o<<strAllTimers;
	return o;
}

#endif // DO_STATS