/* ReputationManager.h
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
#include <set>
#include <string>
#include <vector>

class DataNode;
class DataWriter;
class Government;



// Threshold levels for reputation that trigger special behaviors.
enum class ReputationThreshold {
	WAR = -100,        // Permanent enemy status
	HOSTILE = -50,     // Actively hostile, will attack on sight
	UNFRIENDLY = -25,  // Will deny services but not attack
	NEUTRAL = 0,       // Default state
	FRIENDLY = 25,     // Friendly, may offer assistance
	ALLIED = 50,       // Strong ally, will defend player
	HONORED = 100      // Maximum positive reputation
};



// Configuration for how a government handles reputation decay and recovery.
struct ReputationConfig {
	// Rate at which positive reputation decays toward neutral (per day).
	// 0.0 = no decay, 1.0 = reputation halves each day.
	double positiveDecayRate = 0.01;

	// Rate at which negative reputation recovers toward neutral (per day).
	double negativeRecoveryRate = 0.005;

	// The "neutral point" toward which reputation drifts.
	// Usually 0.0, but some governments might have a different baseline.
	double neutralPoint = 0.0;

	// Minimum reputation before atrocities are considered "unforgiven".
	double atrocityThreshold = -50.0;

	// Whether this government ever forgives atrocities over time.
	bool forgivesAtrocities = false;

	// Days required for atrocity forgiveness (if allowed).
	int atrocityForgivenessDays = 365;

	// Reputation gained/lost affects allies at this rate (0.0-1.0).
	double allyReputationFactor = 0.5;

	// How fast this government "remembers" the player (slows decay).
	// 0.0 = forgets quickly, 1.0 = never forgets.
	double memoryStrength = 0.5;

	// Load configuration from data.
	void Load(const DataNode &node);
};



// Record of a significant reputation event for a government.
struct ReputationEvent {
	Date date;
	double change = 0.0;
	std::string reason;
	bool wasAtrocity = false;
	bool wasWitnessed = true;

	ReputationEvent() = default;
	ReputationEvent(const Date &date, double change, const std::string &reason = "",
		bool wasAtrocity = false, bool wasWitnessed = true);
};



// Tracks when the player crossed important reputation thresholds.
struct ThresholdCrossing {
	const Government *government = nullptr;
	ReputationThreshold fromThreshold;
	ReputationThreshold toThreshold;
	Date date;
	double oldReputation = 0.0;
	double newReputation = 0.0;

	ThresholdCrossing() = default;
	ThresholdCrossing(const Government *gov, ReputationThreshold from, ReputationThreshold to,
		const Date &date, double oldRep, double newRep);
};



// Per-government reputation state that extends the base Politics data.
struct GovernmentReputationState {
	// Last time we interacted with this government.
	Date lastInteraction;

	// Whether the player has committed an atrocity against this government.
	bool hasCommittedAtrocity = false;

	// Date of the atrocity (for forgiveness timing).
	Date atrocityDate;

	// Count of good deeds performed (slows decay).
	int goodDeedCount = 0;

	// Recent reputation changes for history.
	std::vector<ReputationEvent> recentEvents;

	// Highest reputation ever achieved (for decay calculations).
	double peakReputation = 0.0;

	// Lowest reputation ever reached.
	double troughReputation = 0.0;

	// Days since last positive interaction.
	int daysSincePositiveInteraction = 0;

	// Load/save state.
	void Load(const DataNode &node);
	void Save(DataWriter &out) const;

	// Record a reputation event.
	void RecordEvent(const ReputationEvent &event);

	// Trim old events from history.
	void TrimEventHistory(int maxEvents = 50);
};



// Manager class for enhanced reputation mechanics.
// This works alongside the existing Politics class to add decay, memory,
// and more sophisticated reputation tracking.
class ReputationManager {
public:
	// Default configuration values.
	static constexpr int MAX_EVENT_HISTORY = 50;
	static constexpr double DEFAULT_DECAY_RATE = 0.01;
	static constexpr double DEFAULT_RECOVERY_RATE = 0.005;


public:
	ReputationManager() = default;

	// Load/save the manager state.
	void Load(const DataNode &node);
	void Save(DataWriter &out) const;

	// Clear all state.
	void Clear();

	// Set up default configuration for a government.
	void SetConfig(const Government *gov, const ReputationConfig &config);

	// Get the configuration for a government (uses defaults if not set).
	const ReputationConfig &GetConfig(const Government *gov) const;

	// Called once per day to apply reputation decay/recovery.
	// Returns any threshold crossings that occurred.
	std::vector<ThresholdCrossing> StepDaily(const Date &currentDate);

	// Record a reputation change for tracking purposes.
	void RecordChange(const Government *gov, const Date &date, double change,
		const std::string &reason = "", bool wasAtrocity = false, bool wasWitnessed = true);

	// Record a good deed (slows decay).
	void RecordGoodDeed(const Government *gov, const Date &date);

	// Record an atrocity.
	void RecordAtrocity(const Government *gov, const Date &date);

	// Check if the player has committed an unforgiving atrocity against this government.
	bool HasUnforgivenAtrocity(const Government *gov) const;

	// Get the state for a specific government.
	const GovernmentReputationState *GetState(const Government *gov) const;
	GovernmentReputationState &GetOrCreateState(const Government *gov);

	// Calculate the effective decay rate for a government.
	// This accounts for memory strength, recent interactions, good deeds, etc.
	double GetEffectiveDecayRate(const Government *gov) const;

	// Calculate the effective recovery rate for negative reputation.
	double GetEffectiveRecoveryRate(const Government *gov) const;

	// Get the reputation threshold for a given value.
	static ReputationThreshold GetThreshold(double reputation);

	// Get the name of a threshold level.
	static std::string GetThresholdName(ReputationThreshold threshold);

	// Check if a change crosses a threshold.
	static bool CrossesThreshold(double oldRep, double newRep, ReputationThreshold &from,
		ReputationThreshold &to);

	// Get recent events for a government.
	std::vector<ReputationEvent> GetRecentEvents(const Government *gov, int days = 30) const;

	// Get all governments the player has interacted with.
	std::vector<const Government *> GetKnownGovernments() const;


private:
	// Apply decay to a single government.
	// Returns the new reputation value.
	double ApplyDecay(const Government *gov, double currentRep, const Date &currentDate);

	// Default configuration for governments without specific settings.
	ReputationConfig defaultConfig;

	// Per-government configuration.
	std::map<const Government *, ReputationConfig> configs;

	// Per-government reputation state.
	std::map<const Government *, GovernmentReputationState> states;
};
