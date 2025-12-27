/* EncounterRecord.h
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

#pragma once

#include "Date.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

class DataNode;
class DataWriter;



// Disposition categories describing the relationship between the player and an NPC.
// These are derived from the history of interactions and help NPCs "remember" the player.
enum class NPCDisposition {
	UNKNOWN = 0,    // No significant history
	FRIENDLY,       // Positive interactions (assists, trades)
	NEUTRAL,        // Mixed or minimal interactions
	WARY,           // Some negative interactions
	HOSTILE,        // Significant negative history
	NEMESIS,        // Extreme negative history (multiple attacks)
	GRATEFUL,       // Player has saved this NPC
	INDEBTED        // Player has provided significant help
};



// A record of encounters between the player and a specific NPC.
// This tracks the history of interactions to allow NPCs to "remember" the player
// and respond appropriately based on past behavior.
struct EncounterRecord {
	// When the player first encountered this NPC.
	Date firstMet;
	// When the player last saw this NPC.
	Date lastSeen;
	// The system where the last encounter occurred.
	std::string lastSystem;
	// Total number of times this NPC has been encountered.
	int timesEncountered = 0;
	// Bitmask of events that have occurred between player and this NPC.
	// Uses ShipEvent::Type values.
	int eventsBitmask = 0;
	// For named persons (unique NPCs), store their name for save/load.
	std::string personName;
	// The NPC's UUID for identification across encounters.
	std::string npcUuid;

	// Derived statistics for quick access.
	int timesAssisted = 0;      // Player assisted this NPC
	int timesAttacked = 0;      // Player attacked this NPC
	int timesScanned = 0;       // Player scanned this NPC
	bool wasDisabled = false;   // Player disabled this NPC at some point
	bool wasBoarded = false;    // Player boarded this NPC
	bool playerWasAssisted = false;  // This NPC assisted the player

	// Economic interactions (for merchants).
	int64_t totalTradeValue = 0;  // Total credits exchanged in trades

	int combatEncounters = 0;
	int playerFleeCount = 0;
	int playerAfterburnerUseCount = 0;
	int playerMissileUseCount = 0;
	int playerBeamUseCount = 0;
	double averageCombatRange = 0.;

	// Default constructor.
	EncounterRecord() = default;

	// Constructor for initial encounter.
	explicit EncounterRecord(const Date &date, const std::string &system = "",
		const std::string &uuid = "");

	// Serialization.
	void Load(const DataNode &node);
	void Save(DataWriter &out) const;

	// Update the record with a new encounter.
	void RecordEncounter(const Date &date, const std::string &system);

	// Record a specific event type.
	void RecordEvent(int eventType);

	// Record a trade transaction.
	void RecordTrade(int64_t value);

	// Calculate the NPC's disposition toward the player based on interaction history.
	NPCDisposition GetDisposition() const;

	// Get a text description of the disposition.
	static std::string DispositionName(NPCDisposition disposition);

	// Check if the NPC would recognize the player (enough encounters).
	bool WouldRecognizePlayer() const;

	// Check if there's significant negative history.
	bool HasNegativeHistory() const;

	// Check if there's significant positive history.
	bool HasPositiveHistory() const;

	// Get a "threat level" the NPC perceives from the player (0.0 - 1.0+).
	double GetPerceivedThreat() const;

	// Get a "friendship level" the NPC has with the player (0.0 - 1.0).
	double GetFriendshipLevel() const;

	void RecordCombatEncounter(bool playerFled, bool usedAfterburner,
		bool usedMissiles, bool usedBeams, double combatRange);
	bool PlayerLikelyToFlee() const;
	bool PlayerUsesAfterburner() const;
	bool PlayerPrefersMissiles() const;
	double GetPreferredCombatRange() const;
};



// Manager class for a collection of encounter records.
// This is typically owned by PlayerInfo to track all NPC encounters.
class EncounterLog {
public:
	// Default maximum number of records to keep.
	static constexpr size_t DEFAULT_MAX_RECORDS = 500;

	EncounterLog() = default;
	explicit EncounterLog(size_t maxRecords);

	// Copying is prohibited.
	EncounterLog(const EncounterLog &) = delete;
	EncounterLog &operator=(const EncounterLog &) = delete;
	// Moving is allowed.
	EncounterLog(EncounterLog &&) = default;
	EncounterLog &operator=(EncounterLog &&) = default;

	// Serialization.
	void Load(const DataNode &node);
	void Save(DataWriter &out) const;

	// Clear all records.
	void Clear();

	// Get or create an encounter record for a specific NPC UUID.
	EncounterRecord &GetOrCreate(const std::string &uuid, const Date &date,
		const std::string &system = "");

	// Get a record if it exists (const version, returns nullptr if not found).
	const EncounterRecord *Get(const std::string &uuid) const;

	// Check if we have a record for this NPC.
	bool HasRecord(const std::string &uuid) const;

	// Remove a record.
	void Remove(const std::string &uuid);

	// Get all NPCs with a specific disposition.
	std::vector<const EncounterRecord *> GetByDisposition(NPCDisposition disposition) const;

	// Get all NPCs encountered in a specific system.
	std::vector<const EncounterRecord *> GetBySystem(const std::string &system) const;

	// Get the total number of records.
	size_t Size() const;

	// Get/set the maximum number of records.
	size_t MaxRecords() const;
	void SetMaxRecords(size_t max);


private:
	// Trim the log to maximum size, removing least-recently-seen entries.
	void TrimToMaxSize();

	// Map from NPC UUID to encounter record.
	std::map<std::string, EncounterRecord> records;

	// Maximum number of records to retain.
	size_t maxRecords = DEFAULT_MAX_RECORDS;
};
