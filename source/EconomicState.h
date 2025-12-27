/* EconomicState.h
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
class System;



// Economic states that affect trade prices and availability in a system.
// States naturally drift back toward STABLE over time.
enum class EconomicStateType : int8_t {
	STABLE = 0,     // Normal economy, baseline prices
	BOOM,           // Thriving trade: better sell prices, cheaper buys
	BUST,           // Depressed economy: worse prices all around
	SHORTAGE,       // Supply crisis: specific commodity expensive
	SURPLUS,        // Oversupply: specific commodity cheap
	LOCKDOWN        // Trade suspended: black market only
};



// Types of economic events that can affect system state.
enum class EconomicEventType : int8_t {
	MERCHANT_DESTROYED,     // Merchant ship destroyed in system
	PIRATE_DESTROYED,       // Pirate ship destroyed in system
	TRADE_COMPLETED,        // Trade transaction completed
	LARGE_PURCHASE,         // Player bought significant cargo
	LARGE_SALE,             // Player sold significant cargo
	SMUGGLING_DETECTED,     // Illegal cargo detected
	CONVOY_ATTACKED,        // Multiple merchants attacked
	BLOCKADE_ACTIVE,        // System is blockaded
	RELIEF_DELIVERED,       // Humanitarian aid delivered
	WAR_STARTED,            // War declared affecting region
	WAR_ENDED               // Peace restored
};



// Record of an economic event for history tracking.
struct EconomicEvent {
	Date date;
	EconomicEventType type = EconomicEventType::TRADE_COMPLETED;
	int magnitude = 1;              // Severity/size of event
	std::string commodity;          // Affected commodity (if applicable)
	std::string description;        // Human-readable description
	bool playerCaused = false;      // Whether player triggered this

	EconomicEvent() = default;
	EconomicEvent(const Date &date, EconomicEventType type, int magnitude = 1,
		const std::string &commodity = "", bool playerCaused = false);
};



// Configuration for how a system's economy behaves.
struct EconomicConfig {
	// Days required for state to recover to STABLE.
	int boomRecoveryDays = 14;
	int bustRecoveryDays = 14;
	int shortageRecoveryDays = 7;
	int surplusRecoveryDays = 7;
	int lockdownRecoveryDays = 21;

	// Thresholds for triggering state changes (within rolling 7-day window).
	int merchantLossThreshold = 10;     // Losses to trigger BUST
	int pirateLossThreshold = 20;       // Kills to trigger BOOM
	int tradeVolumeThreshold = 5000;    // Tons traded to trigger BOOM
	int smugglingThreshold = 50;        // Units to trigger LOCKDOWN
	int bulkTradeThreshold = 500;       // Tons to trigger SHORTAGE/SURPLUS

	// Price modifiers for each state (multipliers).
	double boomBuyModifier = 0.90;      // -10% buy price
	double boomSellModifier = 1.10;     // +10% sell price
	double bustBuyModifier = 1.10;      // +10% buy price
	double bustSellModifier = 0.90;     // -10% sell price
	double shortageModifier = 1.50;     // +50% for affected commodity
	double surplusModifier = 0.70;      // -30% for affected commodity

	// Black market settings (during LOCKDOWN).
	double blackMarketBuyModifier = 1.50;   // +50% buy price
	double blackMarketSellModifier = 0.60;  // -40% sell price
	double blackMarketDetectionChance = 0.15; // 15% chance per trade

	// How far effects cascade to neighboring systems (in jumps).
	int cascadeRadius = 2;

	// Load configuration from data.
	void Load(const DataNode &node);
};



// Tracks the economic state of a single star system.
class SystemEconomy {
public:
	// Default configuration values.
	static constexpr int MAX_EVENT_HISTORY = 100;
	static constexpr int ROLLING_WINDOW_DAYS = 7;
	static constexpr double DAILY_COUNTER_DECAY = 0.85;


public:
	SystemEconomy() = default;

	// Load/save state.
	void Load(const DataNode &node);
	void Save(DataWriter &out) const;

	// Clear all state.
	void Clear();

	// Get current state.
	EconomicStateType GetState() const;
	const std::string &GetAffectedCommodity() const;
	int DaysInCurrentState() const;
	int GetStateStrength() const;

	// Get state name for display.
	static std::string GetStateName(EconomicStateType state);
	std::string GetStateDescription() const;

	// Calculate price modifier for a commodity.
	// Takes into account state and reputation.
	double GetPriceModifier(const std::string &commodity, bool buying) const;

	// Calculate reputation-based price modifier.
	double GetReputationModifier(double reputation, bool buying) const;

	// Check if trading is allowed (false during LOCKDOWN for non-black-market).
	bool IsTradingAllowed() const;
	bool IsBlackMarketOnly() const;

	// Black market pricing and risk.
	double GetBlackMarketModifier(bool buying) const;
	double GetBlackMarketDetectionChance() const;

	// Record an economic event.
	void RecordEvent(const EconomicEvent &event);
	void RecordEvent(EconomicEventType type, int magnitude = 1,
		const std::string &commodity = "", bool playerCaused = false);

	// Called once per day to update state and counters.
	// Returns true if state changed.
	bool StepDaily(const Date &date, const System *system);

	// Force a state change (for events, missions, etc.).
	void SetState(EconomicStateType state, const std::string &commodity = "",
		int strength = 100);

	// Get rolling counters for state transition logic.
	int GetMerchantLosses() const;
	int GetPirateLosses() const;
	int GetTradeVolume() const;
	int GetSmugglingLevel() const;

	// Get recent events.
	const std::vector<EconomicEvent> &GetRecentEvents() const;

	// Check if an event is significant enough to report.
	bool HasSignificantChange() const;

	// Get the last significant news headline for this system.
	const std::string &GetNewsHeadline() const;
	void SetNewsHeadline(const std::string &headline);
	void ClearNewsHeadline();

	// Propagate effects to neighboring systems within cascade radius.
	void PropagateEffects(const System *system, EconomicEventType type, int magnitude);


private:
	// Check thresholds and potentially change state.
	bool EvaluateStateTransition(const Date &date, const System *system);

	// Apply natural recovery toward STABLE.
	bool ApplyRecovery(const Date &date);

	// Simulate NPC economic activity (trade, piracy, etc.).
	void SimulateNPCActivity(const Date &date, const System *system);

	// Generate news headline for state change.
	void GenerateNewsHeadline(EconomicStateType oldState, EconomicStateType newState,
		const System *system);


private:
	// Current economic state.
	EconomicStateType state = EconomicStateType::STABLE;
	std::string affectedCommodity;  // For SHORTAGE/SURPLUS states
	int stateStrength = 0;          // How entrenched the state is (0-100)
	Date stateChangeDate;           // When state last changed

	// Rolling counters (decay daily).
	double merchantLosses = 0.0;
	double pirateLosses = 0.0;
	double tradeVolume = 0.0;
	double smugglingLevel = 0.0;

	// Event history.
	std::vector<EconomicEvent> recentEvents;

	// News headline for display.
	std::string newsHeadline;
	Date newsDate;

	// Configuration (shared across systems, but can be overridden).
	EconomicConfig config;

	// Whether there was a significant change recently.
	bool significantChange = false;
};



// Manager class for the global economic simulation.
// Tracks all system economies and handles cross-system effects.
class EconomicManager {
public:
	EconomicManager() = default;

	// Load/save global economic state.
	void Load(const DataNode &node);
	void Save(DataWriter &out) const;

	// Clear all state.
	void Clear();

	// Called once per day to update all systems. Returns news headlines for display.
	std::vector<std::string> StepDaily(const Date &date);

	// Get the economy for a specific system.
	SystemEconomy &GetSystemEconomy(const System *system);
	const SystemEconomy *GetSystemEconomy(const System *system) const;

	// Record an event in a system (convenience method).
	void RecordEvent(const System *system, EconomicEventType type,
		int magnitude = 1, const std::string &commodity = "", bool playerCaused = false);

	// Get systems in a particular state.
	std::vector<const System *> GetSystemsInState(EconomicStateType state) const;

	// Get all systems with active (non-STABLE) economies.
	std::vector<const System *> GetActiveEconomies() const;

	// Get recent news headlines across all systems.
	std::vector<std::pair<const System *, std::string>> GetRecentNews(int days = 7) const;

	// Set the default configuration for new systems.
	void SetDefaultConfig(const EconomicConfig &config);
	const EconomicConfig &GetDefaultConfig() const;

	// Check if black market is available in a system.
	bool HasBlackMarket(const System *system) const;

	// Get black market price modifier (typically higher than normal).
	double GetBlackMarketModifier() const;


private:
	// Per-system economy state.
	std::map<const System *, SystemEconomy> systemEconomies;

	// Default configuration.
	EconomicConfig defaultConfig;

	// Black market modifier (how much more expensive on black market).
	double blackMarketModifier = 1.5;

	// Systems known to have black markets.
	std::set<const System *> blackMarketSystems;
};
