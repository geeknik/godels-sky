/* WitnessSystem.cpp
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

#include "WitnessSystem.h"

#include "Government.h"
#include "Ship.h"
#include "System.h"

#include <algorithm>
#include <cmath>

using namespace std;



WitnessInfo::WitnessInfo(const shared_ptr<Ship> &ship, const Government *gov,
	double distance, bool canReport, bool hasSensors, double clarity)
	: ship(ship), government(gov), distance(distance), canReport(canReport),
	hasSensors(hasSensors), clarity(clarity)
{
}



void WitnessResult::AddWitness(const WitnessInfo &witness)
{
	witnesses.push_back(witness);
}



bool WitnessResult::HasWitnesses() const
{
	return !witnesses.empty();
}



bool WitnessResult::HasWitnessFrom(const Government *gov) const
{
	for(const WitnessInfo &w : witnesses)
		if(w.government == gov)
			return true;
	return false;
}



size_t WitnessResult::WitnessCount() const
{
	return witnesses.size();
}



const vector<WitnessInfo> &WitnessResult::GetWitnesses() const
{
	return witnesses;
}



set<const Government *> WitnessResult::GetWitnessGovernments() const
{
	set<const Government *> result;
	for(const WitnessInfo &w : witnesses)
		if(w.government)
			result.insert(w.government);
	return result;
}



set<const Government *> WitnessResult::GetReportingGovernments() const
{
	set<const Government *> result;
	for(const WitnessInfo &w : witnesses)
		if(w.government && w.canReport)
			result.insert(w.government);
	return result;
}



bool WitnessResult::WasClearlyWitnessed(double threshold) const
{
	for(const WitnessInfo &w : witnesses)
		if(w.clarity >= threshold)
			return true;
	return false;
}



double WitnessResult::GetMaxClarity() const
{
	double maxClarity = 0.0;
	for(const WitnessInfo &w : witnesses)
		maxClarity = max(maxClarity, w.clarity);
	return maxClarity;
}



double WitnessResult::GetClosestWitnessDistance() const
{
	if(witnesses.empty())
		return -1.0;

	double closest = witnesses[0].distance;
	for(const WitnessInfo &w : witnesses)
		closest = min(closest, w.distance);
	return closest;
}



bool WitnessResult::CanSuppressReport() const
{
	// Can suppress if all witnesses that can report are eliminable.
	for(const WitnessInfo &w : witnesses)
		if(w.canReport && !w.ship)
			return false;  // Can't suppress if witness ship is not tracked.
	return true;
}



vector<shared_ptr<Ship>> WitnessResult::GetSuppressibleWitnesses() const
{
	vector<shared_ptr<Ship>> result;
	for(const WitnessInfo &w : witnesses)
		if(w.canReport && w.ship)
			result.push_back(w.ship);
	return result;
}



WitnessReport::WitnessReport(const Government *reportingGov, const Government *victimGov,
	int eventType, int frames, const System *system, double impact)
	: reportingGov(reportingGov), victimGov(victimGov), eventType(eventType),
	framesRemaining(frames), system(system), reputationImpact(impact)
{
}



bool WitnessReport::Step()
{
	if(framesRemaining > 0)
		--framesRemaining;
	return framesRemaining <= 0;
}



bool WitnessReport::EliminateWitness(const Ship *ship)
{
	if(!canBeSuppressed)
		return false;

	auto it = find_if(activeWitnesses.begin(), activeWitnesses.end(),
		[ship](const shared_ptr<Ship> &s) { return s.get() == ship; });

	if(it != activeWitnesses.end())
		activeWitnesses.erase(it);

	return AllWitnessesEliminated();
}



bool WitnessReport::AllWitnessesEliminated() const
{
	return canBeSuppressed && activeWitnesses.empty();
}



WitnessResult WitnessSystem::CheckWitnesses(
	const Point &eventLocation,
	const Ship *perpetrator,
	const Ship *victim,
	const list<shared_ptr<Ship>> &nearbyShips,
	double customRange)
{
	WitnessResult result;

	for(const auto &ship : nearbyShips)
	{
		// Skip the perpetrator and victim themselves.
		if(ship.get() == perpetrator || ship.get() == victim)
			continue;

		// Skip destroyed or disabled ships that can't observe.
		if(ship->IsDestroyed() || ship->IsDisabled())
			continue;

		// Check if this ship can witness the event.
		double range = GetWitnessRange(*ship);
		if(customRange > 0)
			range = customRange;

		if(!CanWitness(*ship, eventLocation, perpetrator, range))
			continue;

		// Calculate observation clarity.
		double distance = ship->Position().Distance(eventLocation);
		double clarity = CalculateClarity(*ship, eventLocation, perpetrator, range);

		// Determine if this witness can/will report.
		const Government *gov = ship->GetGovernment();
		bool canReport = WouldReport(*ship, victim ? victim->GetGovernment() : nullptr);
		bool hasSensors = HasEnhancedSensors(*ship);

		result.AddWitness(WitnessInfo(ship, gov, distance, canReport, hasSensors, clarity));
	}

	return result;
}



bool WitnessSystem::CanWitness(
	const Ship &observer,
	const Point &eventLocation,
	const Ship *perpetrator,
	double range)
{
	// Check if observer is within range.
	double distance = observer.Position().Distance(eventLocation);
	if(distance > range)
		return false;

	// Check if observer is too cloaked to see anything.
	if(observer.Cloaking() >= WitnessConstants::OBSERVER_CLOAK_THRESHOLD)
		return false;

	// Check if perpetrator is too cloaked to be seen.
	if(perpetrator && perpetrator->Cloaking() >= WitnessConstants::CLOAK_WITNESS_THRESHOLD)
		return false;

	return true;
}



double WitnessSystem::CalculateClarity(
	const Ship &observer,
	const Point &eventLocation,
	const Ship *perpetrator,
	double range)
{
	double distance = observer.Position().Distance(eventLocation);

	// Base clarity decreases with distance.
	double clarity = 1.0 - (distance / range);
	clarity = max(0.0, min(1.0, clarity));

	// Perpetrator cloaking reduces clarity.
	if(perpetrator)
	{
		double cloaking = perpetrator->Cloaking();
		clarity *= (1.0 - cloaking * 0.8);
	}

	// Enhanced sensors improve clarity.
	if(HasEnhancedSensors(observer))
		clarity = min(1.0, clarity * 1.3);

	return clarity;
}



bool WitnessSystem::HasEnhancedSensors(const Ship &ship)
{
	// Ships with scanning outfits have enhanced witness capability.
	// Check for cargo/outfit scanners.
	return ship.Attributes().Get("cargo scan power") > 0 ||
		ship.Attributes().Get("outfit scan power") > 0 ||
		ship.Attributes().Get("tactical scan power") > 0;
}



double WitnessSystem::GetWitnessRange(const Ship &ship)
{
	double range = WitnessConstants::DEFAULT_WITNESS_RANGE;

	if(HasEnhancedSensors(ship))
		range *= WitnessConstants::SENSOR_RANGE_MULTIPLIER;

	return range;
}



bool WitnessSystem::WouldReport(const Ship &witness, const Government *victimGov)
{
	const Government *witnessGov = witness.GetGovernment();
	if(!witnessGov)
		return false;

	// Determine report probability based on government type.
	double reportChance = WitnessConstants::CIVILIAN_REPORT_CHANCE;

	// Military governments are more likely to report.
	// Check if this is a "law enforcement" type government.
	if(witnessGov->IsEnemy() || witnessGov->TrueName().find("Navy") != string::npos ||
		witnessGov->TrueName().find("Militia") != string::npos ||
		witnessGov->TrueName().find("Police") != string::npos)
	{
		reportChance = WitnessConstants::MILITARY_REPORT_CHANCE;
	}

	// Pirates and hostile governments don't report crimes.
	if(witnessGov->IsEnemy())
		return false;

	// Allied governments always report crimes against their allies.
	if(victimGov && !witnessGov->IsEnemy(victimGov))
		reportChance = 1.0;

	// For simplicity, return true if report chance is high.
	// In a full implementation, this would use random chance.
	return reportChance >= 0.5;
}



void WitnessSystem::QueueReport(const WitnessReport &report)
{
	pendingReports.push_back(report);
}



vector<WitnessReport> WitnessSystem::StepReports()
{
	vector<WitnessReport> ready;

	auto it = pendingReports.begin();
	while(it != pendingReports.end())
	{
		// Check if all witnesses were eliminated.
		if(it->AllWitnessesEliminated())
		{
			it = pendingReports.erase(it);
			continue;
		}

		// Step the report.
		if(it->Step())
		{
			ready.push_back(*it);
			it = pendingReports.erase(it);
		}
		else
		{
			++it;
		}
	}

	return ready;
}



const vector<WitnessReport> &WitnessSystem::GetPendingReports() const
{
	return pendingReports;
}



void WitnessSystem::NotifyShipDestroyed(const Ship *ship)
{
	for(WitnessReport &report : pendingReports)
		report.EliminateWitness(ship);
}



void WitnessSystem::Clear()
{
	pendingReports.clear();
}
