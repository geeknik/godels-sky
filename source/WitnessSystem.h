/* WitnessSystem.h
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

#include "Point.h"

#include <cstdint>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <vector>

class Government;
class Ship;
class System;



// Constants for witness detection.
namespace WitnessConstants {
	// Default range at which ships can witness events.
	constexpr double DEFAULT_WITNESS_RANGE = 5000.0;

	// Cloaking level above which a ship cannot be witnessed.
	constexpr double CLOAK_WITNESS_THRESHOLD = 0.5;

	// Minimum cloak level for the observer to be unable to witness.
	constexpr double OBSERVER_CLOAK_THRESHOLD = 0.8;

	// Time (in frames) for a witness report to be transmitted.
	constexpr int REPORT_TRANSMISSION_TIME = 60;

	// Chance that a civilian ship will report a crime (0.0 - 1.0).
	constexpr double CIVILIAN_REPORT_CHANCE = 0.7;

	// Chance that a military ship will report a crime (0.0 - 1.0).
	constexpr double MILITARY_REPORT_CHANCE = 0.95;

	// Range multiplier for sensor-equipped ships.
	constexpr double SENSOR_RANGE_MULTIPLIER = 1.5;
}



// Information about a single witness to an event.
struct WitnessInfo {
	// The ship that witnessed the event.
	std::shared_ptr<Ship> ship;
	// The government of the witness.
	const Government *government = nullptr;
	// Distance from the event.
	double distance = 0.0;
	// Whether this witness can report to authorities.
	bool canReport = true;
	// Whether this witness has special sensors (enhanced range).
	bool hasSensors = false;
	// How "clearly" the witness saw the event (0.0 - 1.0).
	// Affected by distance, cloaking, etc.
	double clarity = 1.0;

	WitnessInfo() = default;
	WitnessInfo(const std::shared_ptr<Ship> &ship, const Government *gov,
		double distance, bool canReport, bool hasSensors, double clarity);
};



// Result of checking for witnesses to an event.
class WitnessResult {
public:
	WitnessResult() = default;

	// Add a witness to this result.
	void AddWitness(const WitnessInfo &witness);

	// Check if there are any witnesses.
	bool HasWitnesses() const;

	// Check if there are witnesses from a specific government.
	bool HasWitnessFrom(const Government *gov) const;

	// Get the number of witnesses.
	size_t WitnessCount() const;

	// Get all witnesses.
	const std::vector<WitnessInfo> &GetWitnesses() const;

	// Get all governments that witnessed the event.
	std::set<const Government *> GetWitnessGovernments() const;

	// Get the governments that will report this to their allies.
	std::set<const Government *> GetReportingGovernments() const;

	// Check if the event was "clearly" witnessed (high clarity by any witness).
	bool WasClearlyWitnessed(double threshold = 0.7) const;

	// Get the highest clarity value among all witnesses.
	double GetMaxClarity() const;

	// Get the closest witness distance.
	double GetClosestWitnessDistance() const;

	// Check if eliminating all witnesses would prevent the report.
	bool CanSuppressReport() const;

	// Get the list of ships that must be eliminated to suppress the report.
	std::vector<std::shared_ptr<Ship>> GetSuppressibleWitnesses() const;


private:
	std::vector<WitnessInfo> witnesses;
};



// A pending report that will affect reputation after a delay.
struct WitnessReport {
	// The government filing the report.
	const Government *reportingGov = nullptr;
	// The government that was wronged (may be the same or an ally).
	const Government *victimGov = nullptr;
	// The type of event being reported.
	int eventType = 0;
	// Frames remaining until the report is processed.
	int framesRemaining = 0;
	// The system where the event occurred.
	const System *system = nullptr;
	// Reputation impact when processed.
	double reputationImpact = 0.0;
	// Whether this report can still be suppressed.
	bool canBeSuppressed = true;
	// The witnesses who can still report (for suppression).
	std::vector<std::shared_ptr<Ship>> activeWitnesses;

	WitnessReport() = default;
	WitnessReport(const Government *reportingGov, const Government *victimGov,
		int eventType, int frames, const System *system, double impact);

	// Step the report forward one frame. Returns true if it's time to process.
	bool Step();

	// Mark a witness as eliminated. Returns true if report is now suppressed.
	bool EliminateWitness(const Ship *ship);

	// Check if all witnesses have been eliminated.
	bool AllWitnessesEliminated() const;
};



// The witness system manages detection of crimes and their reporting.
// It integrates with the Engine to check for witnesses during combat
// and with Politics to apply delayed reputation effects.
class WitnessSystem {
public:
	WitnessSystem() = default;

	// Check for witnesses to an event at a specific location.
	// The perpetrator is the ship performing the action (usually player).
	// The victim is the ship being acted upon.
	static WitnessResult CheckWitnesses(
		const Point &eventLocation,
		const Ship *perpetrator,
		const Ship *victim,
		const std::list<std::shared_ptr<Ship>> &nearbyShips,
		double customRange = WitnessConstants::DEFAULT_WITNESS_RANGE);

	// Calculate if a specific ship can witness an event at a location.
	static bool CanWitness(
		const Ship &observer,
		const Point &eventLocation,
		const Ship *perpetrator,
		double range = WitnessConstants::DEFAULT_WITNESS_RANGE);

	// Calculate the clarity of observation (how well the witness saw the event).
	static double CalculateClarity(
		const Ship &observer,
		const Point &eventLocation,
		const Ship *perpetrator,
		double range);

	// Check if a ship has enhanced sensors for witnessing.
	static bool HasEnhancedSensors(const Ship &ship);

	// Get the effective witness range for a ship.
	static double GetWitnessRange(const Ship &ship);

	// Determine if a ship would report a crime to authorities.
	static bool WouldReport(const Ship &witness, const Government *victimGov);

	// Queue a witness report for delayed processing.
	void QueueReport(const WitnessReport &report);

	// Step all pending reports. Returns reports ready to be processed.
	std::vector<WitnessReport> StepReports();

	// Get all pending reports.
	const std::vector<WitnessReport> &GetPendingReports() const;

	// Mark a ship as eliminated (updates all pending reports).
	void NotifyShipDestroyed(const Ship *ship);

	// Clear all pending reports.
	void Clear();


private:
	// Pending reports waiting to be processed.
	std::vector<WitnessReport> pendingReports;
};
