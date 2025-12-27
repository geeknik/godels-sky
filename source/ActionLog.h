/* ActionLog.h
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
#include <deque>
#include <map>
#include <string>
#include <vector>

class DataNode;
class DataWriter;
class Government;



// A record of a single significant action performed by the player.
// This captures the context needed for governments to "remember" player behavior
// and generate appropriate consequences.
struct ActionRecord {
	// When this action occurred.
	Date date;
	// The type of event (bitmask from ShipEvent::Type).
	int eventType = 0;
	// The government that was affected by this action.
	const Government *targetGov = nullptr;
	// The system where this action took place.
	std::string systemName;
	// Number of crew killed in this action.
	int crewKilled = 0;
	// Total value of ships/cargo destroyed.
	int64_t valueDestroyed = 0;
	// Whether this action was witnessed by ships that could report it.
	bool wasWitnessed = true;

	// Default constructor.
	ActionRecord() = default;
	// Constructor with all fields.
	ActionRecord(const Date &date, int eventType, const Government *targetGov,
		const std::string &systemName, int crewKilled, int64_t valueDestroyed,
		bool wasWitnessed);
};



// Detected behavioral pattern types for the player.
// These are used to categorize overall player behavior.
enum class BehaviorPattern {
	UNKNOWN = 0,
	TRADER,        // Mostly peaceful trading activity
	PIRATE,        // Attacks merchants and civilians
	BOUNTY_HUNTER, // Attacks pirates and enemies of governments
	PROTECTOR,     // Assists other ships frequently
	WARMONGER,     // Attacks many different governments
	SABOTEUR       // Targets specific governments repeatedly
};



// Class that tracks significant player actions over time.
// This allows governments to "remember" patterns of behavior and escalate
// their responses accordingly. Actions are stored in a rolling window
// to prevent unbounded memory growth while maintaining recent history.
class ActionLog {
public:
	// Default maximum number of records to keep.
	static constexpr size_t DEFAULT_MAX_RECORDS = 1000;


public:
	ActionLog() = default;
	explicit ActionLog(size_t maxRecords);

	// Copying is prohibited to avoid expensive deep copies.
	ActionLog(const ActionLog &) = delete;
	ActionLog &operator=(const ActionLog &) = delete;
	// Moving is allowed.
	ActionLog(ActionLog &&) = default;
	ActionLog &operator=(ActionLog &&) = default;

	// Serialization support.
	void Load(const DataNode &node);
	void Save(DataWriter &out) const;

	// Clear all recorded actions.
	void Clear();

	// Record a new action.
	void Record(const ActionRecord &record);
	void Record(const Date &date, int eventType, const Government *targetGov,
		const std::string &systemName, int crewKilled = 0,
		int64_t valueDestroyed = 0, bool wasWitnessed = true);

	// Get the maximum number of records this log will store.
	size_t MaxRecords() const;
	// Set the maximum number of records (trims if necessary).
	void SetMaxRecords(size_t maxRecords);

	// Get the total number of records currently stored.
	size_t Size() const;
	// Check if the log is empty.
	bool IsEmpty() const;

	// Query methods for accessing action history.

	// Get all actions within the last N days from the given reference date.
	std::vector<const ActionRecord *> GetRecentActions(const Date &referenceDate,
		int days) const;

	// Get actions against a specific government within the last N days.
	std::vector<const ActionRecord *> GetActionsAgainst(const Government *gov,
		const Date &referenceDate, int days) const;

	// Count occurrences of a specific event type within the last N days.
	// eventType is a bitmask; this counts records where (record.eventType & eventType) != 0.
	int CountEventType(int eventType, const Date &referenceDate, int days) const;

	// Count total crew killed against a government within the last N days.
	int GetCrewKilledAgainst(const Government *gov, const Date &referenceDate,
		int days) const;

	// Get total value destroyed against a government within the last N days.
	int64_t GetValueDestroyedAgainst(const Government *gov, const Date &referenceDate,
		int days) const;

	// Pattern detection methods.

	// Analyze recent behavior to determine the dominant pattern.
	BehaviorPattern GetPatternScore(const Date &referenceDate, int days = 30) const;

	// Get a numeric "hostility score" against a specific government.
	// Higher values indicate more hostile behavior.
	// Range: 0.0 (no hostility) to 1.0+ (extreme hostility).
	double GetHostilityScore(const Government *gov, const Date &referenceDate,
		int days = 30) const;

	// Check if the player has shown a pattern of escalating violence.
	bool HasEscalationPattern(const Government *gov, const Date &referenceDate,
		int days = 30) const;

	// Get the number of distinct governments the player has attacked recently.
	int GetDistinctTargets(const Date &referenceDate, int days = 30) const;

	// Check if player actions were mostly witnessed (could affect reputation).
	double GetWitnessedRatio(const Date &referenceDate, int days = 30) const;


private:
	// Helper to check if a record falls within a date range.
	bool IsWithinRange(const ActionRecord &record, const Date &referenceDate,
		int days) const;

	// Trim the log to the maximum size, removing oldest entries first.
	void TrimToMaxSize();

	// Convert event type bitmask to/from string for serialization.
	static std::string EventTypeToString(int eventType);
	static int StringToEventType(const std::string &str);


private:
	// Rolling window of action records, newest at back.
	std::deque<ActionRecord> records;

	// Maximum number of records to retain.
	size_t maxRecords = DEFAULT_MAX_RECORDS;

	// Cached per-government statistics for faster queries.
	// Maps government pointer to count of hostile actions.
	// This cache is rebuilt on load and updated on record.
	mutable std::map<const Government *, int> hostileActionCache;
	mutable bool cacheValid = false;
};
