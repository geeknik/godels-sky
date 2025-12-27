/* EncounterRecord.cpp
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

#include "EncounterRecord.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "ShipEvent.h"

#include <algorithm>
#include <map>

using namespace std;



EncounterRecord::EncounterRecord(const Date &date, const string &system, const string &uuid)
	: firstMet(date), lastSeen(date), lastSystem(system), timesEncountered(1), npcUuid(uuid)
{
}



void EncounterRecord::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);

		if(key == "first met" && child.Size() >= 4)
			firstMet = Date(child.Value(1), child.Value(2), child.Value(3));
		else if(key == "last seen" && child.Size() >= 4)
			lastSeen = Date(child.Value(1), child.Value(2), child.Value(3));
		else if(key == "last system" && child.Size() >= 2)
			lastSystem = child.Token(1);
		else if(key == "times encountered" && child.Size() >= 2)
			timesEncountered = child.Value(1);
		else if(key == "events" && child.Size() >= 2)
			eventsBitmask = child.Value(1);
		else if(key == "person" && child.Size() >= 2)
			personName = child.Token(1);
		else if(key == "uuid" && child.Size() >= 2)
			npcUuid = child.Token(1);
		else if(key == "times assisted" && child.Size() >= 2)
			timesAssisted = child.Value(1);
		else if(key == "times attacked" && child.Size() >= 2)
			timesAttacked = child.Value(1);
		else if(key == "times scanned" && child.Size() >= 2)
			timesScanned = child.Value(1);
		else if(key == "was disabled" && child.Size() >= 2)
			wasDisabled = (child.Value(1) != 0);
		else if(key == "was boarded" && child.Size() >= 2)
			wasBoarded = (child.Value(1) != 0);
		else if(key == "player was assisted" && child.Size() >= 2)
			playerWasAssisted = (child.Value(1) != 0);
		else if(key == "total trade value" && child.Size() >= 2)
			totalTradeValue = child.Value(1);
	}
}



void EncounterRecord::Save(DataWriter &out) const
{
	out.Write("encounter");
	out.BeginChild();
	{
		if(!npcUuid.empty())
			out.Write("uuid", npcUuid);
		if(!personName.empty())
			out.Write("person", personName);

		out.Write("first met", firstMet.Day(), firstMet.Month(), firstMet.Year());
		out.Write("last seen", lastSeen.Day(), lastSeen.Month(), lastSeen.Year());

		if(!lastSystem.empty())
			out.Write("last system", lastSystem);

		out.Write("times encountered", timesEncountered);

		if(eventsBitmask)
			out.Write("events", eventsBitmask);

		if(timesAssisted > 0)
			out.Write("times assisted", timesAssisted);
		if(timesAttacked > 0)
			out.Write("times attacked", timesAttacked);
		if(timesScanned > 0)
			out.Write("times scanned", timesScanned);
		if(wasDisabled)
			out.Write("was disabled", 1);
		if(wasBoarded)
			out.Write("was boarded", 1);
		if(playerWasAssisted)
			out.Write("player was assisted", 1);
		if(totalTradeValue > 0)
			out.Write("total trade value", totalTradeValue);
	}
	out.EndChild();
}



void EncounterRecord::RecordEncounter(const Date &date, const string &system)
{
	++timesEncountered;
	lastSeen = date;
	if(!system.empty())
		lastSystem = system;
}



void EncounterRecord::RecordEvent(int eventType)
{
	eventsBitmask |= eventType;

	if(eventType & ShipEvent::ASSIST)
		++timesAssisted;
	if(eventType & (ShipEvent::DISABLE | ShipEvent::DESTROY | ShipEvent::PROVOKE))
		++timesAttacked;
	if(eventType & (ShipEvent::SCAN_CARGO | ShipEvent::SCAN_OUTFITS))
		++timesScanned;
	if(eventType & ShipEvent::DISABLE)
		wasDisabled = true;
	if(eventType & ShipEvent::BOARD)
		wasBoarded = true;
}



void EncounterRecord::RecordTrade(int64_t value)
{
	totalTradeValue += value;
}



NPCDisposition EncounterRecord::GetDisposition() const
{
	// Calculate a disposition score based on interaction history.
	int positiveScore = timesAssisted * 2 + (playerWasAssisted ? 3 : 0);
	int negativeScore = timesAttacked * 2 + (wasDisabled ? 5 : 0) + (wasBoarded ? 3 : 0);

	// Add score for trade relationships.
	if(totalTradeValue > 100000)
		positiveScore += 2;
	else if(totalTradeValue > 10000)
		positiveScore += 1;

	// Determine disposition based on score difference.
	int netScore = positiveScore - negativeScore;

	if(negativeScore >= 10)
		return NPCDisposition::NEMESIS;
	if(negativeScore >= 4)
		return NPCDisposition::HOSTILE;
	if(negativeScore >= 2)
		return NPCDisposition::WARY;

	if(positiveScore >= 6)
		return NPCDisposition::INDEBTED;
	if(positiveScore >= 3)
		return NPCDisposition::GRATEFUL;
	if(positiveScore >= 1)
		return NPCDisposition::FRIENDLY;

	if(netScore < 0)
		return NPCDisposition::WARY;

	if(timesEncountered > 0)
		return NPCDisposition::NEUTRAL;

	return NPCDisposition::UNKNOWN;
}



string EncounterRecord::DispositionName(NPCDisposition disposition)
{
	switch(disposition)
	{
		case NPCDisposition::UNKNOWN:
			return "unknown";
		case NPCDisposition::FRIENDLY:
			return "friendly";
		case NPCDisposition::NEUTRAL:
			return "neutral";
		case NPCDisposition::WARY:
			return "wary";
		case NPCDisposition::HOSTILE:
			return "hostile";
		case NPCDisposition::NEMESIS:
			return "nemesis";
		case NPCDisposition::GRATEFUL:
			return "grateful";
		case NPCDisposition::INDEBTED:
			return "indebted";
	}
	return "unknown";
}



bool EncounterRecord::WouldRecognizePlayer() const
{
	// Recognition threshold: at least 3 encounters or a significant event.
	return timesEncountered >= 3 || wasDisabled || wasBoarded ||
		timesAssisted >= 2 || timesAttacked >= 2;
}



bool EncounterRecord::HasNegativeHistory() const
{
	return timesAttacked > 0 || wasDisabled || wasBoarded;
}



bool EncounterRecord::HasPositiveHistory() const
{
	return timesAssisted > 0 || playerWasAssisted || totalTradeValue > 0;
}



double EncounterRecord::GetPerceivedThreat() const
{
	// Base threat from attacks.
	double threat = timesAttacked * 0.15;

	// Significant events add more threat.
	if(wasDisabled)
		threat += 0.3;
	if(wasBoarded)
		threat += 0.2;

	// Cap at 1.0 for normal threat, can exceed for extreme cases.
	return min(threat, 1.5);
}



double EncounterRecord::GetFriendshipLevel() const
{
	// Base friendship from assists.
	double friendship = timesAssisted * 0.1;

	if(playerWasAssisted)
		friendship += 0.2;

	// Trade relationships build trust.
	if(totalTradeValue > 100000)
		friendship += 0.2;
	else if(totalTradeValue > 10000)
		friendship += 0.1;

	// Negative history reduces friendship.
	if(HasNegativeHistory())
		friendship *= 0.5;

	return min(friendship, 1.0);
}



// EncounterLog implementation.

EncounterLog::EncounterLog(size_t maxRecords)
	: maxRecords(maxRecords)
{
}



void EncounterLog::Load(const DataNode &node)
{
	records.clear();

	for(const DataNode &child : node)
	{
		if(child.Token(0) == "max records" && child.Size() >= 2)
			maxRecords = static_cast<size_t>(child.Value(1));
		else if(child.Token(0) == "encounter")
		{
			EncounterRecord record;
			record.Load(child);

			if(!record.npcUuid.empty())
				records[record.npcUuid] = std::move(record);
		}
	}
}



void EncounterLog::Save(DataWriter &out) const
{
	out.Write("encounter log");
	out.BeginChild();
	{
		out.Write("max records", maxRecords);

		for(const auto &pair : records)
			pair.second.Save(out);
	}
	out.EndChild();
}



void EncounterLog::Clear()
{
	records.clear();
}



EncounterRecord &EncounterLog::GetOrCreate(const string &uuid, const Date &date,
	const string &system)
{
	auto it = records.find(uuid);
	if(it != records.end())
	{
		it->second.RecordEncounter(date, system);
		return it->second;
	}

	// Create new record.
	EncounterRecord record(date, system, uuid);
	auto result = records.emplace(uuid, std::move(record));

	TrimToMaxSize();

	return result.first->second;
}



const EncounterRecord *EncounterLog::Get(const string &uuid) const
{
	auto it = records.find(uuid);
	return (it != records.end()) ? &it->second : nullptr;
}



bool EncounterLog::HasRecord(const string &uuid) const
{
	return records.count(uuid) > 0;
}



void EncounterLog::Remove(const string &uuid)
{
	records.erase(uuid);
}



vector<const EncounterRecord *> EncounterLog::GetByDisposition(NPCDisposition disposition) const
{
	vector<const EncounterRecord *> result;
	for(const auto &pair : records)
		if(pair.second.GetDisposition() == disposition)
			result.push_back(&pair.second);
	return result;
}



vector<const EncounterRecord *> EncounterLog::GetBySystem(const string &system) const
{
	vector<const EncounterRecord *> result;
	for(const auto &pair : records)
		if(pair.second.lastSystem == system)
			result.push_back(&pair.second);
	return result;
}



size_t EncounterLog::Size() const
{
	return records.size();
}



size_t EncounterLog::MaxRecords() const
{
	return maxRecords;
}



void EncounterLog::SetMaxRecords(size_t max)
{
	maxRecords = max;
	TrimToMaxSize();
}



void EncounterLog::TrimToMaxSize()
{
	if(records.size() <= maxRecords)
		return;

	// Find and remove the oldest (by lastSeen) records until we're under the limit.
	while(records.size() > maxRecords)
	{
		auto oldest = records.begin();
		for(auto it = records.begin(); it != records.end(); ++it)
		{
			if(it->second.lastSeen < oldest->second.lastSeen)
				oldest = it;
		}
		records.erase(oldest);
	}
}
