/* ActionLog.cpp
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

#include "ActionLog.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "Government.h"
#include "GameData.h"
#include "ShipEvent.h"

#include <algorithm>
#include <cmath>
#include <set>

using namespace std;



// ActionRecord constructor with all fields.
ActionRecord::ActionRecord(const Date &date, int eventType, const Government *targetGov,
	const string &systemName, int crewKilled, int64_t valueDestroyed, bool wasWitnessed)
	: date(date), eventType(eventType), targetGov(targetGov), systemName(systemName),
	crewKilled(crewKilled), valueDestroyed(valueDestroyed), wasWitnessed(wasWitnessed)
{
}



ActionLog::ActionLog(size_t maxRecords)
	: maxRecords(maxRecords)
{
}



void ActionLog::Load(const DataNode &node)
{
	records.clear();
	cacheValid = false;

	for(const DataNode &child : node)
	{
		if(child.Token(0) == "max records" && child.Size() >= 2)
			maxRecords = static_cast<size_t>(child.Value(1));
		else if(child.Token(0) == "record")
		{
			ActionRecord record;

			for(const DataNode &grand : child)
			{
				const string &key = grand.Token(0);
				if(key == "date" && grand.Size() >= 4)
					record.date = Date(grand.Value(1), grand.Value(2), grand.Value(3));
				else if(key == "event" && grand.Size() >= 2)
					record.eventType = StringToEventType(grand.Token(1));
				else if(key == "target" && grand.Size() >= 2)
					record.targetGov = GameData::Governments().Get(grand.Token(1));
				else if(key == "system" && grand.Size() >= 2)
					record.systemName = grand.Token(1);
				else if(key == "crew killed" && grand.Size() >= 2)
					record.crewKilled = grand.Value(1);
				else if(key == "value destroyed" && grand.Size() >= 2)
					record.valueDestroyed = grand.Value(1);
				else if(key == "witnessed" && grand.Size() >= 2)
					record.wasWitnessed = (grand.Value(1) != 0);
			}

			records.push_back(record);
		}
	}

	TrimToMaxSize();
}



void ActionLog::Save(DataWriter &out) const
{
	out.Write("action log");
	out.BeginChild();
	{
		out.Write("max records", maxRecords);

		for(const ActionRecord &record : records)
		{
			out.Write("record");
			out.BeginChild();
			{
				out.Write("date", record.date.Day(), record.date.Month(), record.date.Year());
				out.Write("event", EventTypeToString(record.eventType));
				if(record.targetGov)
					out.Write("target", record.targetGov->TrueName());
				if(!record.systemName.empty())
					out.Write("system", record.systemName);
				if(record.crewKilled > 0)
					out.Write("crew killed", record.crewKilled);
				if(record.valueDestroyed > 0)
					out.Write("value destroyed", record.valueDestroyed);
				out.Write("witnessed", record.wasWitnessed ? 1 : 0);
			}
			out.EndChild();
		}
	}
	out.EndChild();
}



void ActionLog::Clear()
{
	records.clear();
	hostileActionCache.clear();
	cacheValid = false;
}



void ActionLog::Record(const ActionRecord &record)
{
	records.push_back(record);
	cacheValid = false;
	TrimToMaxSize();
}



void ActionLog::Record(const Date &date, int eventType, const Government *targetGov,
	const string &systemName, int crewKilled, int64_t valueDestroyed, bool wasWitnessed)
{
	Record(ActionRecord(date, eventType, targetGov, systemName, crewKilled,
		valueDestroyed, wasWitnessed));
}



size_t ActionLog::MaxRecords() const
{
	return maxRecords;
}



void ActionLog::SetMaxRecords(size_t max)
{
	maxRecords = max;
	TrimToMaxSize();
}



size_t ActionLog::Size() const
{
	return records.size();
}



bool ActionLog::IsEmpty() const
{
	return records.empty();
}



vector<const ActionRecord *> ActionLog::GetRecentActions(const Date &referenceDate, int days) const
{
	vector<const ActionRecord *> result;
	for(const ActionRecord &record : records)
		if(IsWithinRange(record, referenceDate, days))
			result.push_back(&record);
	return result;
}



vector<const ActionRecord *> ActionLog::GetActionsAgainst(const Government *gov,
	const Date &referenceDate, int days) const
{
	vector<const ActionRecord *> result;
	for(const ActionRecord &record : records)
		if(record.targetGov == gov && IsWithinRange(record, referenceDate, days))
			result.push_back(&record);
	return result;
}



int ActionLog::CountEventType(int eventType, const Date &referenceDate, int days) const
{
	int count = 0;
	for(const ActionRecord &record : records)
		if((record.eventType & eventType) && IsWithinRange(record, referenceDate, days))
			++count;
	return count;
}



int ActionLog::GetCrewKilledAgainst(const Government *gov, const Date &referenceDate, int days) const
{
	int total = 0;
	for(const ActionRecord &record : records)
		if(record.targetGov == gov && IsWithinRange(record, referenceDate, days))
			total += record.crewKilled;
	return total;
}



int64_t ActionLog::GetValueDestroyedAgainst(const Government *gov, const Date &referenceDate, int days) const
{
	int64_t total = 0;
	for(const ActionRecord &record : records)
		if(record.targetGov == gov && IsWithinRange(record, referenceDate, days))
			total += record.valueDestroyed;
	return total;
}



BehaviorPattern ActionLog::GetPatternScore(const Date &referenceDate, int days) const
{
	// Count actions by category.
	int pirateActions = 0;     // Attacks on merchants/civilians
	int bountyActions = 0;     // Attacks on pirates/enemies
	int protectorActions = 0;  // Assists
	int hostileActions = 0;    // All attacks

	set<const Government *> govTargets;

	for(const ActionRecord &record : records)
	{
		if(!IsWithinRange(record, referenceDate, days))
			continue;

		// Track distinct government targets.
		if(record.targetGov)
			govTargets.insert(record.targetGov);

		// Categorize the action.
		if(record.eventType & ShipEvent::ASSIST)
			++protectorActions;

		if(record.eventType & (ShipEvent::DESTROY | ShipEvent::DISABLE | ShipEvent::CAPTURE))
		{
			++hostileActions;

			// Determine if target was "pirate-like" or "civilian-like".
			if(record.targetGov)
			{
				// If we attacked a pirate government, that's bounty hunting.
				// If we attacked a legitimate government, that's piracy.
				// This is a simplification; could be enhanced with government attributes.
				if(record.targetGov->IsEnemy())
					++bountyActions;
				else
					++pirateActions;
			}
		}
	}

	// Determine dominant pattern based on action ratios.
	if(hostileActions == 0 && protectorActions == 0)
		return BehaviorPattern::TRADER;

	if(protectorActions > hostileActions * 2)
		return BehaviorPattern::PROTECTOR;

	// Check for warmonger (attacks many different governments).
	if(govTargets.size() >= 5)
		return BehaviorPattern::WARMONGER;

	// Check for saboteur (concentrated attacks on one government).
	if(govTargets.size() == 1 && hostileActions >= 10)
		return BehaviorPattern::SABOTEUR;

	if(bountyActions > pirateActions * 2)
		return BehaviorPattern::BOUNTY_HUNTER;

	if(pirateActions > bountyActions)
		return BehaviorPattern::PIRATE;

	return BehaviorPattern::UNKNOWN;
}



double ActionLog::GetHostilityScore(const Government *gov, const Date &referenceDate, int days) const
{
	if(!gov)
		return 0.0;

	double score = 0.0;
	const double BASE_HOSTILE_WEIGHT = 0.1;
	const double DESTROY_WEIGHT = 0.3;
	const double ATROCITY_WEIGHT = 1.0;
	const double CREW_KILL_WEIGHT = 0.02;
	const double VALUE_WEIGHT = 0.00001;

	for(const ActionRecord &record : records)
	{
		if(record.targetGov != gov || !IsWithinRange(record, referenceDate, days))
			continue;

		// Base score for any hostile action.
		if(record.eventType & (ShipEvent::PROVOKE | ShipEvent::DISABLE))
			score += BASE_HOSTILE_WEIGHT;

		// Higher weight for destruction.
		if(record.eventType & ShipEvent::DESTROY)
			score += DESTROY_WEIGHT;

		// Atrocities are weighted heavily.
		if(record.eventType & ShipEvent::ATROCITY)
			score += ATROCITY_WEIGHT;

		// Additional weight for casualties and damage.
		score += record.crewKilled * CREW_KILL_WEIGHT;
		score += static_cast<double>(record.valueDestroyed) * VALUE_WEIGHT;

		// Witnessed actions count more (they affect reputation directly).
		if(record.wasWitnessed)
			score *= 1.2;
	}

	return score;
}



bool ActionLog::HasEscalationPattern(const Government *gov, const Date &referenceDate, int days) const
{
	if(!gov)
		return false;

	// Divide the time period into thirds and compare action rates.
	int thirdDays = days / 3;
	if(thirdDays < 1)
		return false;

	// Count actions in each third.
	int earlyCount = 0;
	int middleCount = 0;
	int recentCount = 0;

	for(const ActionRecord &record : records)
	{
		if(record.targetGov != gov)
			continue;

		int daysAgo = referenceDate - record.date;
		if(daysAgo < 0 || daysAgo > days)
			continue;

		if(daysAgo <= thirdDays)
			++recentCount;
		else if(daysAgo <= thirdDays * 2)
			++middleCount;
		else
			++earlyCount;
	}

	// Escalation pattern: each period has more actions than the previous.
	return (recentCount > middleCount) && (middleCount > earlyCount) && (earlyCount > 0);
}



int ActionLog::GetDistinctTargets(const Date &referenceDate, int days) const
{
	set<const Government *> targets;
	for(const ActionRecord &record : records)
	{
		if(record.targetGov && IsWithinRange(record, referenceDate, days))
			targets.insert(record.targetGov);
	}
	return static_cast<int>(targets.size());
}



double ActionLog::GetWitnessedRatio(const Date &referenceDate, int days) const
{
	int witnessed = 0;
	int total = 0;

	for(const ActionRecord &record : records)
	{
		if(IsWithinRange(record, referenceDate, days))
		{
			++total;
			if(record.wasWitnessed)
				++witnessed;
		}
	}

	return total > 0 ? static_cast<double>(witnessed) / total : 0.0;
}



bool ActionLog::IsWithinRange(const ActionRecord &record, const Date &referenceDate, int days) const
{
	int daysAgo = referenceDate - record.date;
	return daysAgo >= 0 && daysAgo <= days;
}



void ActionLog::TrimToMaxSize()
{
	while(records.size() > maxRecords)
	{
		records.pop_front();
		cacheValid = false;
	}
}



string ActionLog::EventTypeToString(int eventType)
{
	string result;

	// Build a string representation of the event type bitmask.
	if(eventType & ShipEvent::ASSIST)
		result += "assist ";
	if(eventType & ShipEvent::DISABLE)
		result += "disable ";
	if(eventType & ShipEvent::BOARD)
		result += "board ";
	if(eventType & ShipEvent::CAPTURE)
		result += "capture ";
	if(eventType & ShipEvent::DESTROY)
		result += "destroy ";
	if(eventType & ShipEvent::SCAN_CARGO)
		result += "scan_cargo ";
	if(eventType & ShipEvent::SCAN_OUTFITS)
		result += "scan_outfits ";
	if(eventType & ShipEvent::PROVOKE)
		result += "provoke ";
	if(eventType & ShipEvent::ATROCITY)
		result += "atrocity ";

	// Remove trailing space.
	if(!result.empty() && result.back() == ' ')
		result.pop_back();

	return result.empty() ? "none" : result;
}



int ActionLog::StringToEventType(const string &str)
{
	int result = 0;

	// Parse space-separated event type names.
	size_t pos = 0;
	while(pos < str.size())
	{
		size_t end = str.find(' ', pos);
		if(end == string::npos)
			end = str.size();

		string token = str.substr(pos, end - pos);

		if(token == "assist")
			result |= ShipEvent::ASSIST;
		else if(token == "disable")
			result |= ShipEvent::DISABLE;
		else if(token == "board")
			result |= ShipEvent::BOARD;
		else if(token == "capture")
			result |= ShipEvent::CAPTURE;
		else if(token == "destroy")
			result |= ShipEvent::DESTROY;
		else if(token == "scan_cargo")
			result |= ShipEvent::SCAN_CARGO;
		else if(token == "scan_outfits")
			result |= ShipEvent::SCAN_OUTFITS;
		else if(token == "provoke")
			result |= ShipEvent::PROVOKE;
		else if(token == "atrocity")
			result |= ShipEvent::ATROCITY;

		pos = end + 1;
	}

	return result;
}
