/* EconomicState.cpp
Copyright (c) 2025 by the Godel's Sky contributors

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "EconomicState.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "GameData.h"
#include "Government.h"
#include "Random.h"
#include "System.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <set>

using namespace std;



EconomicEvent::EconomicEvent(const Date &date, EconomicEventType type, int magnitude,
	const string &commodity, bool playerCaused)
	: date(date), type(type), magnitude(magnitude), commodity(commodity), playerCaused(playerCaused)
{
}



void EconomicConfig::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		if(key == "boom recovery days" && child.Size() >= 2)
			boomRecoveryDays = max(1, static_cast<int>(child.Value(1)));
		else if(key == "bust recovery days" && child.Size() >= 2)
			bustRecoveryDays = max(1, static_cast<int>(child.Value(1)));
		else if(key == "shortage recovery days" && child.Size() >= 2)
			shortageRecoveryDays = max(1, static_cast<int>(child.Value(1)));
		else if(key == "surplus recovery days" && child.Size() >= 2)
			surplusRecoveryDays = max(1, static_cast<int>(child.Value(1)));
		else if(key == "lockdown recovery days" && child.Size() >= 2)
			lockdownRecoveryDays = max(1, static_cast<int>(child.Value(1)));
		else if(key == "merchant loss threshold" && child.Size() >= 2)
			merchantLossThreshold = max(1, static_cast<int>(child.Value(1)));
		else if(key == "pirate loss threshold" && child.Size() >= 2)
			pirateLossThreshold = max(1, static_cast<int>(child.Value(1)));
		else if(key == "trade volume threshold" && child.Size() >= 2)
			tradeVolumeThreshold = max(1, static_cast<int>(child.Value(1)));
		else if(key == "smuggling threshold" && child.Size() >= 2)
			smugglingThreshold = max(1, static_cast<int>(child.Value(1)));
		else if(key == "bulk trade threshold" && child.Size() >= 2)
			bulkTradeThreshold = max(1, static_cast<int>(child.Value(1)));
		else if(key == "boom buy modifier" && child.Size() >= 2)
			boomBuyModifier = child.Value(1);
		else if(key == "boom sell modifier" && child.Size() >= 2)
			boomSellModifier = child.Value(1);
		else if(key == "bust buy modifier" && child.Size() >= 2)
			bustBuyModifier = child.Value(1);
		else if(key == "bust sell modifier" && child.Size() >= 2)
			bustSellModifier = child.Value(1);
		else if(key == "shortage modifier" && child.Size() >= 2)
			shortageModifier = child.Value(1);
		else if(key == "surplus modifier" && child.Size() >= 2)
			surplusModifier = child.Value(1);
		else if(key == "cascade radius" && child.Size() >= 2)
			cascadeRadius = max(0, static_cast<int>(child.Value(1)));
	}
}



void SystemEconomy::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		if(key == "state" && child.Size() >= 2)
		{
			const string &stateName = child.Token(1);
			if(stateName == "stable")
				state = EconomicStateType::STABLE;
			else if(stateName == "boom")
				state = EconomicStateType::BOOM;
			else if(stateName == "bust")
				state = EconomicStateType::BUST;
			else if(stateName == "shortage")
				state = EconomicStateType::SHORTAGE;
			else if(stateName == "surplus")
				state = EconomicStateType::SURPLUS;
			else if(stateName == "lockdown")
				state = EconomicStateType::LOCKDOWN;
		}
		else if(key == "affected commodity" && child.Size() >= 2)
			affectedCommodity = child.Token(1);
		else if(key == "state strength" && child.Size() >= 2)
			stateStrength = static_cast<int>(child.Value(1));
		else if(key == "state change date" && child.Size() >= 4)
			stateChangeDate = Date(child.Value(1), child.Value(2), child.Value(3));
		else if(key == "merchant losses" && child.Size() >= 2)
			merchantLosses = child.Value(1);
		else if(key == "pirate losses" && child.Size() >= 2)
			pirateLosses = child.Value(1);
		else if(key == "trade volume" && child.Size() >= 2)
			tradeVolume = child.Value(1);
		else if(key == "smuggling level" && child.Size() >= 2)
			smugglingLevel = child.Value(1);
		else if(key == "news headline" && child.Size() >= 2)
			newsHeadline = child.Token(1);
		else if(key == "news date" && child.Size() >= 4)
			newsDate = Date(child.Value(1), child.Value(2), child.Value(3));
		else if(key == "config")
			config.Load(child);
		else if(key == "event")
		{
			EconomicEvent event;
			for(const DataNode &grand : child)
			{
				const string &eventKey = grand.Token(0);
				if(eventKey == "date" && grand.Size() >= 4)
					event.date = Date(grand.Value(1), grand.Value(2), grand.Value(3));
				else if(eventKey == "type" && grand.Size() >= 2)
				{
					const string &typeName = grand.Token(1);
					if(typeName == "merchant destroyed")
						event.type = EconomicEventType::MERCHANT_DESTROYED;
					else if(typeName == "pirate destroyed")
						event.type = EconomicEventType::PIRATE_DESTROYED;
					else if(typeName == "trade completed")
						event.type = EconomicEventType::TRADE_COMPLETED;
					else if(typeName == "large purchase")
						event.type = EconomicEventType::LARGE_PURCHASE;
					else if(typeName == "large sale")
						event.type = EconomicEventType::LARGE_SALE;
					else if(typeName == "smuggling detected")
						event.type = EconomicEventType::SMUGGLING_DETECTED;
					else if(typeName == "convoy attacked")
						event.type = EconomicEventType::CONVOY_ATTACKED;
					else if(typeName == "blockade active")
						event.type = EconomicEventType::BLOCKADE_ACTIVE;
					else if(typeName == "relief delivered")
						event.type = EconomicEventType::RELIEF_DELIVERED;
					else if(typeName == "war started")
						event.type = EconomicEventType::WAR_STARTED;
					else if(typeName == "war ended")
						event.type = EconomicEventType::WAR_ENDED;
				}
				else if(eventKey == "magnitude" && grand.Size() >= 2)
					event.magnitude = static_cast<int>(grand.Value(1));
				else if(eventKey == "commodity" && grand.Size() >= 2)
					event.commodity = grand.Token(1);
				else if(eventKey == "description" && grand.Size() >= 2)
					event.description = grand.Token(1);
				else if(eventKey == "player caused")
					event.playerCaused = true;
			}
			recentEvents.push_back(event);
		}
	}
}



void SystemEconomy::Save(DataWriter &out) const
{
	out.Write("economy");
	out.BeginChild();
	{
		string stateName;
		switch(state)
		{
			case EconomicStateType::STABLE: stateName = "stable"; break;
			case EconomicStateType::BOOM: stateName = "boom"; break;
			case EconomicStateType::BUST: stateName = "bust"; break;
			case EconomicStateType::SHORTAGE: stateName = "shortage"; break;
			case EconomicStateType::SURPLUS: stateName = "surplus"; break;
			case EconomicStateType::LOCKDOWN: stateName = "lockdown"; break;
		}
		out.Write("state", stateName);

		if(!affectedCommodity.empty())
			out.Write("affected commodity", affectedCommodity);
		if(stateStrength > 0)
			out.Write("state strength", stateStrength);
		if(stateChangeDate)
			out.Write("state change date", stateChangeDate.Day(), stateChangeDate.Month(),
				stateChangeDate.Year());

		if(merchantLosses > 0.01)
			out.Write("merchant losses", merchantLosses);
		if(pirateLosses > 0.01)
			out.Write("pirate losses", pirateLosses);
		if(tradeVolume > 0.01)
			out.Write("trade volume", tradeVolume);
		if(smugglingLevel > 0.01)
			out.Write("smuggling level", smugglingLevel);

		if(!newsHeadline.empty())
		{
			out.Write("news headline", newsHeadline);
			if(newsDate)
				out.Write("news date", newsDate.Day(), newsDate.Month(), newsDate.Year());
		}

		for(const EconomicEvent &event : recentEvents)
		{
			out.Write("event");
			out.BeginChild();
			{
				if(event.date)
					out.Write("date", event.date.Day(), event.date.Month(), event.date.Year());

				string typeName;
				switch(event.type)
				{
					case EconomicEventType::MERCHANT_DESTROYED: typeName = "merchant destroyed"; break;
					case EconomicEventType::PIRATE_DESTROYED: typeName = "pirate destroyed"; break;
					case EconomicEventType::TRADE_COMPLETED: typeName = "trade completed"; break;
					case EconomicEventType::LARGE_PURCHASE: typeName = "large purchase"; break;
					case EconomicEventType::LARGE_SALE: typeName = "large sale"; break;
					case EconomicEventType::SMUGGLING_DETECTED: typeName = "smuggling detected"; break;
					case EconomicEventType::CONVOY_ATTACKED: typeName = "convoy attacked"; break;
					case EconomicEventType::BLOCKADE_ACTIVE: typeName = "blockade active"; break;
					case EconomicEventType::RELIEF_DELIVERED: typeName = "relief delivered"; break;
					case EconomicEventType::WAR_STARTED: typeName = "war started"; break;
					case EconomicEventType::WAR_ENDED: typeName = "war ended"; break;
				}
				out.Write("type", typeName);

				if(event.magnitude != 1)
					out.Write("magnitude", event.magnitude);
				if(!event.commodity.empty())
					out.Write("commodity", event.commodity);
				if(!event.description.empty())
					out.Write("description", event.description);
				if(event.playerCaused)
					out.Write("player caused");
			}
			out.EndChild();
		}
	}
	out.EndChild();
}



void SystemEconomy::Clear()
{
	state = EconomicStateType::STABLE;
	affectedCommodity.clear();
	stateStrength = 0;
	stateChangeDate = Date();
	merchantLosses = 0.0;
	pirateLosses = 0.0;
	tradeVolume = 0.0;
	smugglingLevel = 0.0;
	recentEvents.clear();
	newsHeadline.clear();
	newsDate = Date();
	significantChange = false;
}



EconomicStateType SystemEconomy::GetState() const
{
	return state;
}



const string &SystemEconomy::GetAffectedCommodity() const
{
	return affectedCommodity;
}



int SystemEconomy::DaysInCurrentState() const
{
	if(!stateChangeDate)
		return 0;
	return stateChangeDate.DaysSinceEpoch();
}



int SystemEconomy::GetStateStrength() const
{
	return stateStrength;
}



string SystemEconomy::GetStateName(EconomicStateType state)
{
	switch(state)
	{
		case EconomicStateType::STABLE: return "Stable";
		case EconomicStateType::BOOM: return "Booming";
		case EconomicStateType::BUST: return "Depressed";
		case EconomicStateType::SHORTAGE: return "Shortage";
		case EconomicStateType::SURPLUS: return "Surplus";
		case EconomicStateType::LOCKDOWN: return "Lockdown";
	}
	return "Unknown";
}



string SystemEconomy::GetStateDescription() const
{
	switch(state)
	{
		case EconomicStateType::STABLE:
			return "Economy is stable with normal trade activity.";
		case EconomicStateType::BOOM:
			return "Trade is flourishing. Better prices for sellers.";
		case EconomicStateType::BUST:
			return "Economic depression. Poor prices for all.";
		case EconomicStateType::SHORTAGE:
			return "Supply shortage of " + affectedCommodity + ". Prices elevated.";
		case EconomicStateType::SURPLUS:
			return "Oversupply of " + affectedCommodity + ". Prices depressed.";
		case EconomicStateType::LOCKDOWN:
			return "Trade suspended. Black market only.";
	}
	return "";
}



double SystemEconomy::GetPriceModifier(const string &commodity, bool buying) const
{
	double modifier = 1.0;

	switch(state)
	{
		case EconomicStateType::STABLE:
			break;
		case EconomicStateType::BOOM:
			modifier = buying ? config.boomBuyModifier : config.boomSellModifier;
			break;
		case EconomicStateType::BUST:
			modifier = buying ? config.bustBuyModifier : config.bustSellModifier;
			break;
		case EconomicStateType::SHORTAGE:
			if(commodity == affectedCommodity)
				modifier = config.shortageModifier;
			break;
		case EconomicStateType::SURPLUS:
			if(commodity == affectedCommodity)
				modifier = config.surplusModifier;
			break;
		case EconomicStateType::LOCKDOWN:
			break;
	}

	double strengthFactor = stateStrength / 100.0;
	return 1.0 + (modifier - 1.0) * strengthFactor;
}



double SystemEconomy::GetReputationModifier(double reputation, bool buying) const
{
	if(reputation >= 1000.0)
		return buying ? 0.85 : 1.15;
	else if(reputation >= 100.0)
		return buying ? 0.90 : 1.10;
	else if(reputation >= 10.0)
		return buying ? 0.95 : 1.05;
	else if(reputation <= -1000.0)
		return buying ? 1.20 : 0.80;
	else if(reputation <= -100.0)
		return buying ? 1.15 : 0.85;
	else if(reputation <= -10.0)
		return buying ? 1.05 : 0.95;

	return 1.0;
}



bool SystemEconomy::IsTradingAllowed() const
{
	return state != EconomicStateType::LOCKDOWN;
}



bool SystemEconomy::IsBlackMarketOnly() const
{
	return state == EconomicStateType::LOCKDOWN;
}



double SystemEconomy::GetBlackMarketModifier(bool buying) const
{
	return buying ? config.blackMarketBuyModifier : config.blackMarketSellModifier;
}



double SystemEconomy::GetBlackMarketDetectionChance() const
{
	return config.blackMarketDetectionChance;
}



void SystemEconomy::RecordEvent(const EconomicEvent &event)
{
	recentEvents.push_back(event);

	if(recentEvents.size() > MAX_EVENT_HISTORY)
		recentEvents.erase(recentEvents.begin());

	switch(event.type)
	{
		case EconomicEventType::MERCHANT_DESTROYED:
			merchantLosses += event.magnitude;
			break;
		case EconomicEventType::PIRATE_DESTROYED:
			pirateLosses += event.magnitude;
			break;
		case EconomicEventType::TRADE_COMPLETED:
		case EconomicEventType::LARGE_PURCHASE:
		case EconomicEventType::LARGE_SALE:
			tradeVolume += event.magnitude;
			break;
		case EconomicEventType::SMUGGLING_DETECTED:
			smugglingLevel += event.magnitude;
			break;
		default:
			break;
	}
}



void SystemEconomy::RecordEvent(EconomicEventType type, int magnitude,
	const string &commodity, bool playerCaused)
{
	EconomicEvent event;
	event.type = type;
	event.magnitude = magnitude;
	event.commodity = commodity;
	event.playerCaused = playerCaused;
	RecordEvent(event);
}



bool SystemEconomy::StepDaily(const Date &date, const System *system)
{
	merchantLosses *= DAILY_COUNTER_DECAY;
	pirateLosses *= DAILY_COUNTER_DECAY;
	tradeVolume *= DAILY_COUNTER_DECAY;
	smugglingLevel *= DAILY_COUNTER_DECAY;

	SimulateNPCActivity(date, system);

	bool stateChanged = EvaluateStateTransition(date, system);

	if(!stateChanged && state != EconomicStateType::STABLE)
		stateChanged = ApplyRecovery(date);

	significantChange = stateChanged;
	return stateChanged;
}



void SystemEconomy::SetState(EconomicStateType newState, const string &commodity, int strength)
{
	if(state != newState)
	{
		state = newState;
		stateStrength = strength;
		affectedCommodity = commodity;
		significantChange = true;
	}
	else
	{
		stateStrength = max(stateStrength, strength);
		if(!commodity.empty())
			affectedCommodity = commodity;
	}
}



int SystemEconomy::GetMerchantLosses() const
{
	return static_cast<int>(merchantLosses);
}



int SystemEconomy::GetPirateLosses() const
{
	return static_cast<int>(pirateLosses);
}



int SystemEconomy::GetTradeVolume() const
{
	return static_cast<int>(tradeVolume);
}



int SystemEconomy::GetSmugglingLevel() const
{
	return static_cast<int>(smugglingLevel);
}



const vector<EconomicEvent> &SystemEconomy::GetRecentEvents() const
{
	return recentEvents;
}



bool SystemEconomy::HasSignificantChange() const
{
	return significantChange;
}



const string &SystemEconomy::GetNewsHeadline() const
{
	return newsHeadline;
}



void SystemEconomy::SetNewsHeadline(const string &headline)
{
	newsHeadline = headline;
}



void SystemEconomy::ClearNewsHeadline()
{
	newsHeadline.clear();
}



bool SystemEconomy::EvaluateStateTransition(const Date &date, const System *system)
{
	EconomicStateType oldState = state;

	if(state == EconomicStateType::STABLE || state == EconomicStateType::BUST)
	{
		if(merchantLosses >= config.merchantLossThreshold)
		{
			state = EconomicStateType::BUST;
			stateStrength = min(100, static_cast<int>(merchantLosses * 10));
			stateChangeDate = date;
		}
	}

	if(state == EconomicStateType::STABLE || state == EconomicStateType::BOOM)
	{
		if(pirateLosses >= config.pirateLossThreshold)
		{
			state = EconomicStateType::BOOM;
			stateStrength = min(100, static_cast<int>(pirateLosses * 5));
			stateChangeDate = date;
		}
		else if(tradeVolume >= config.tradeVolumeThreshold)
		{
			state = EconomicStateType::BOOM;
			stateStrength = min(100, static_cast<int>(tradeVolume / 50));
			stateChangeDate = date;
		}
	}

	if(smugglingLevel >= config.smugglingThreshold)
	{
		state = EconomicStateType::LOCKDOWN;
		stateStrength = min(100, static_cast<int>(smugglingLevel * 2));
		stateChangeDate = date;
	}

	if(state != oldState)
	{
		GenerateNewsHeadline(oldState, state, system);
		return true;
	}

	return false;
}



bool SystemEconomy::ApplyRecovery(const Date &date)
{
	if(state == EconomicStateType::STABLE)
		return false;

	int recoveryDays = 14;
	switch(state)
	{
		case EconomicStateType::BOOM:
			recoveryDays = config.boomRecoveryDays;
			break;
		case EconomicStateType::BUST:
			recoveryDays = config.bustRecoveryDays;
			break;
		case EconomicStateType::SHORTAGE:
			recoveryDays = config.shortageRecoveryDays;
			break;
		case EconomicStateType::SURPLUS:
			recoveryDays = config.surplusRecoveryDays;
			break;
		case EconomicStateType::LOCKDOWN:
			recoveryDays = config.lockdownRecoveryDays;
			break;
		default:
			break;
	}

	int dailyRecovery = max(1, 100 / recoveryDays);
	stateStrength = max(0, stateStrength - dailyRecovery);

	if(stateStrength <= 0)
	{
		EconomicStateType oldState = state;
		state = EconomicStateType::STABLE;
		affectedCommodity.clear();
		stateChangeDate = date;
		GenerateNewsHeadline(oldState, state, nullptr);
		return true;
	}

	return false;
}



void SystemEconomy::PropagateEffects(const System *system, EconomicEventType type, int magnitude)
{
	if(!system || config.cascadeRadius <= 0 || magnitude <= 0)
		return;

	set<const System *> visited;
	queue<pair<const System *, int>> toVisit;

	visited.insert(system);
	toVisit.push({system, 0});

	while(!toVisit.empty())
	{
		auto [current, distance] = toVisit.front();
		toVisit.pop();

		if(distance >= config.cascadeRadius)
			continue;

		for(const System *neighbor : current->Links())
		{
			if(!neighbor || visited.count(neighbor))
				continue;

			visited.insert(neighbor);

			int propagatedMagnitude = magnitude / (1 << (distance + 1));
			if(propagatedMagnitude <= 0)
				continue;

			SystemEconomy &neighborEcon = GameData::GetEconomicManager().GetSystemEconomy(neighbor);
			neighborEcon.RecordEvent(type, propagatedMagnitude, "", false);

			toVisit.push({neighbor, distance + 1});
		}
	}
}



void SystemEconomy::SimulateNPCActivity(const Date &date, const System *system)
{
	if(!system)
		return;

	double danger = system->Danger();

	if(danger > 0 && Random::Real() < danger * 0.001)
	{
		int losses = 1 + static_cast<int>(Random::Real() * danger * 0.01);
		merchantLosses += losses;
	}

	if(danger > 0 && Random::Real() < 0.1)
	{
		int kills = static_cast<int>(Random::Real() * 2);
		if(kills > 0)
			pirateLosses += kills;
	}

	if(system->IsInhabited(nullptr))
	{
		double baseTraffic = 100.0 + Random::Normal() * 50.0;
		if(state == EconomicStateType::BOOM)
			baseTraffic *= 1.5;
		else if(state == EconomicStateType::BUST)
			baseTraffic *= 0.5;
		else if(state == EconomicStateType::LOCKDOWN)
			baseTraffic *= 0.1;

		tradeVolume += max(0.0, baseTraffic);
	}
}



void SystemEconomy::GenerateNewsHeadline(EconomicStateType oldState, EconomicStateType newState,
	const System *system)
{
	string systemName = system ? system->DisplayName() : "local system";

	switch(newState)
	{
		case EconomicStateType::STABLE:
			if(oldState == EconomicStateType::BOOM)
				newsHeadline = "Economic growth stabilizes in " + systemName + ".";
			else if(oldState == EconomicStateType::BUST)
				newsHeadline = "Economy recovers in " + systemName + " as trade resumes.";
			else if(oldState == EconomicStateType::LOCKDOWN)
				newsHeadline = "Trade restrictions lifted in " + systemName + ".";
			else
				newsHeadline = "Markets return to normal in " + systemName + ".";
			break;
		case EconomicStateType::BOOM:
			newsHeadline = "Trade flourishing in " + systemName + ". Merchants report record profits.";
			break;
		case EconomicStateType::BUST:
			newsHeadline = "Economic crisis in " + systemName + ". Merchants warn of convoy losses.";
			break;
		case EconomicStateType::SHORTAGE:
			newsHeadline = "Supply shortage reported in " + systemName + ". " +
				affectedCommodity + " prices soaring.";
			break;
		case EconomicStateType::SURPLUS:
			newsHeadline = "Market glut in " + systemName + ". " +
				affectedCommodity + " prices plummeting.";
			break;
		case EconomicStateType::LOCKDOWN:
			newsHeadline = "Authorities impose trade lockdown in " + systemName +
				". Only black market operating.";
			break;
	}
}



void EconomicManager::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);
		if(key == "system" && child.Size() >= 2)
		{
			const System *system = GameData::Systems().Get(child.Token(1));
			if(system)
			{
				for(const DataNode &grand : child)
				{
					if(grand.Token(0) == "economy")
						systemEconomies[system].Load(grand);
				}
			}
		}
		else if(key == "default config")
			defaultConfig.Load(child);
		else if(key == "black market modifier" && child.Size() >= 2)
			blackMarketModifier = child.Value(1);
		else if(key == "black market system" && child.Size() >= 2)
		{
			const System *system = GameData::Systems().Get(child.Token(1));
			if(system)
				blackMarketSystems.insert(system);
		}
	}
}



void EconomicManager::Save(DataWriter &out) const
{
	out.Write("economic state");
	out.BeginChild();
	{
		if(blackMarketModifier != 1.5)
			out.Write("black market modifier", blackMarketModifier);

		for(const System *system : blackMarketSystems)
			out.Write("black market system", system->TrueName());

		for(const auto &pair : systemEconomies)
		{
			if(pair.second.GetState() != EconomicStateType::STABLE ||
				pair.second.GetMerchantLosses() > 0 ||
				pair.second.GetPirateLosses() > 0 ||
				pair.second.GetTradeVolume() > 0)
			{
				out.Write("system", pair.first->TrueName());
				out.BeginChild();
				pair.second.Save(out);
				out.EndChild();
			}
		}
	}
	out.EndChild();
}



void EconomicManager::Clear()
{
	systemEconomies.clear();
	blackMarketSystems.clear();
}



vector<string> EconomicManager::StepDaily(const Date &date)
{
	vector<string> news;
	for(auto &pair : systemEconomies)
	{
		bool changed = pair.second.StepDaily(date, pair.first);
		if(changed)
		{
			const string &headline = pair.second.GetNewsHeadline();
			if(!headline.empty())
				news.push_back(headline);
		}
	}
	return news;
}



SystemEconomy &EconomicManager::GetSystemEconomy(const System *system)
{
	return systemEconomies[system];
}



const SystemEconomy *EconomicManager::GetSystemEconomy(const System *system) const
{
	auto it = systemEconomies.find(system);
	return (it != systemEconomies.end()) ? &it->second : nullptr;
}



void EconomicManager::RecordEvent(const System *system, EconomicEventType type,
	int magnitude, const string &commodity, bool playerCaused)
{
	if(!system)
		return;

	SystemEconomy &economy = systemEconomies[system];
	economy.RecordEvent(type, magnitude, commodity, playerCaused);

	bool shouldCascade = false;
	switch(type)
	{
		case EconomicEventType::MERCHANT_DESTROYED:
		case EconomicEventType::CONVOY_ATTACKED:
		case EconomicEventType::BLOCKADE_ACTIVE:
		case EconomicEventType::WAR_STARTED:
		case EconomicEventType::WAR_ENDED:
			shouldCascade = true;
			break;
		case EconomicEventType::LARGE_PURCHASE:
		case EconomicEventType::LARGE_SALE:
			shouldCascade = (magnitude >= 1000);
			break;
		case EconomicEventType::SMUGGLING_DETECTED:
			shouldCascade = (magnitude >= 100);
			break;
		default:
			break;
	}

	if(shouldCascade)
		economy.PropagateEffects(system, type, magnitude);
}



vector<const System *> EconomicManager::GetSystemsInState(EconomicStateType state) const
{
	vector<const System *> result;
	for(const auto &pair : systemEconomies)
		if(pair.second.GetState() == state)
			result.push_back(pair.first);
	return result;
}



vector<const System *> EconomicManager::GetActiveEconomies() const
{
	vector<const System *> result;
	for(const auto &pair : systemEconomies)
		if(pair.second.GetState() != EconomicStateType::STABLE)
			result.push_back(pair.first);
	return result;
}



vector<pair<const System *, string>> EconomicManager::GetRecentNews(int days) const
{
	vector<pair<const System *, string>> result;
	for(const auto &pair : systemEconomies)
	{
		const string &headline = pair.second.GetNewsHeadline();
		if(!headline.empty())
			result.emplace_back(pair.first, headline);
	}
	return result;
}



void EconomicManager::SetDefaultConfig(const EconomicConfig &config)
{
	defaultConfig = config;
}



const EconomicConfig &EconomicManager::GetDefaultConfig() const
{
	return defaultConfig;
}



bool EconomicManager::HasBlackMarket(const System *system) const
{
	if(blackMarketSystems.count(system))
		return true;

	const SystemEconomy *econ = GetSystemEconomy(system);
	if(econ && econ->IsBlackMarketOnly())
		return true;

	return false;
}



double EconomicManager::GetBlackMarketModifier() const
{
	return blackMarketModifier;
}
