/* ReputationManager.cpp
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

#include "ReputationManager.h"

#include "DataNode.h"
#include "DataWriter.h"
#include "GameData.h"
#include "Government.h"

#include <algorithm>
#include <cmath>

using namespace std;



void ReputationConfig::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);

		if(key == "positive decay rate" && child.Size() >= 2)
			positiveDecayRate = child.Value(1);
		else if(key == "negative recovery rate" && child.Size() >= 2)
			negativeRecoveryRate = child.Value(1);
		else if(key == "neutral point" && child.Size() >= 2)
			neutralPoint = child.Value(1);
		else if(key == "atrocity threshold" && child.Size() >= 2)
			atrocityThreshold = child.Value(1);
		else if(key == "forgives atrocities" && child.Size() >= 2)
			forgivesAtrocities = (child.Value(1) != 0);
		else if(key == "atrocity forgiveness days" && child.Size() >= 2)
			atrocityForgivenessDays = child.Value(1);
		else if(key == "ally reputation factor" && child.Size() >= 2)
			allyReputationFactor = child.Value(1);
		else if(key == "memory strength" && child.Size() >= 2)
			memoryStrength = child.Value(1);
	}
}



ReputationEvent::ReputationEvent(const Date &date, double change, const string &reason,
	bool wasAtrocity, bool wasWitnessed)
	: date(date), change(change), reason(reason), wasAtrocity(wasAtrocity),
	wasWitnessed(wasWitnessed)
{
}



ThresholdCrossing::ThresholdCrossing(const Government *gov, ReputationThreshold from,
	ReputationThreshold to, const Date &date, double oldRep, double newRep)
	: government(gov), fromThreshold(from), toThreshold(to), date(date),
	oldReputation(oldRep), newReputation(newRep)
{
}



void GovernmentReputationState::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		const string &key = child.Token(0);

		if(key == "last interaction" && child.Size() >= 4)
			lastInteraction = Date(child.Value(1), child.Value(2), child.Value(3));
		else if(key == "has committed atrocity" && child.Size() >= 2)
			hasCommittedAtrocity = (child.Value(1) != 0);
		else if(key == "atrocity date" && child.Size() >= 4)
			atrocityDate = Date(child.Value(1), child.Value(2), child.Value(3));
		else if(key == "good deed count" && child.Size() >= 2)
			goodDeedCount = child.Value(1);
		else if(key == "peak reputation" && child.Size() >= 2)
			peakReputation = child.Value(1);
		else if(key == "trough reputation" && child.Size() >= 2)
			troughReputation = child.Value(1);
		else if(key == "days since positive" && child.Size() >= 2)
			daysSincePositiveInteraction = child.Value(1);
		else if(key == "event")
		{
			ReputationEvent event;
			for(const DataNode &grand : child)
			{
				const string &eventKey = grand.Token(0);
				if(eventKey == "date" && grand.Size() >= 4)
					event.date = Date(grand.Value(1), grand.Value(2), grand.Value(3));
				else if(eventKey == "change" && grand.Size() >= 2)
					event.change = grand.Value(1);
				else if(eventKey == "reason" && grand.Size() >= 2)
					event.reason = grand.Token(1);
				else if(eventKey == "atrocity" && grand.Size() >= 2)
					event.wasAtrocity = (grand.Value(1) != 0);
				else if(eventKey == "witnessed" && grand.Size() >= 2)
					event.wasWitnessed = (grand.Value(1) != 0);
			}
			recentEvents.push_back(event);
		}
	}
}



void GovernmentReputationState::Save(DataWriter &out) const
{
	out.Write("reputation state");
	out.BeginChild();
	{
		if(lastInteraction)
			out.Write("last interaction", lastInteraction.Day(),
				lastInteraction.Month(), lastInteraction.Year());
		if(hasCommittedAtrocity)
		{
			out.Write("has committed atrocity", 1);
			if(atrocityDate)
				out.Write("atrocity date", atrocityDate.Day(),
					atrocityDate.Month(), atrocityDate.Year());
		}
		if(goodDeedCount > 0)
			out.Write("good deed count", goodDeedCount);
		if(peakReputation != 0.0)
			out.Write("peak reputation", peakReputation);
		if(troughReputation != 0.0)
			out.Write("trough reputation", troughReputation);
		if(daysSincePositiveInteraction > 0)
			out.Write("days since positive", daysSincePositiveInteraction);

		for(const ReputationEvent &event : recentEvents)
		{
			out.Write("event");
			out.BeginChild();
			{
				out.Write("date", event.date.Day(), event.date.Month(), event.date.Year());
				out.Write("change", event.change);
				if(!event.reason.empty())
					out.Write("reason", event.reason);
				if(event.wasAtrocity)
					out.Write("atrocity", 1);
				out.Write("witnessed", event.wasWitnessed ? 1 : 0);
			}
			out.EndChild();
		}
	}
	out.EndChild();
}



void GovernmentReputationState::RecordEvent(const ReputationEvent &event)
{
	recentEvents.push_back(event);
	lastInteraction = event.date;

	if(event.change > 0)
		daysSincePositiveInteraction = 0;

	if(event.wasAtrocity)
	{
		hasCommittedAtrocity = true;
		atrocityDate = event.date;
	}
}



void GovernmentReputationState::TrimEventHistory(int maxEvents)
{
	if(recentEvents.size() > static_cast<size_t>(maxEvents))
	{
		recentEvents.erase(recentEvents.begin(),
			recentEvents.begin() + (recentEvents.size() - maxEvents));
	}
}



void ReputationManager::Load(const DataNode &node)
{
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "default config")
			defaultConfig.Load(child);
		else if(child.Token(0) == "government" && child.Size() >= 2)
		{
			const Government *gov = GameData::Governments().Get(child.Token(1));
			for(const DataNode &grand : child)
			{
				if(grand.Token(0) == "config")
				{
					ReputationConfig config;
					config.Load(grand);
					configs[gov] = config;
				}
				else if(grand.Token(0) == "state")
				{
					GovernmentReputationState state;
					state.Load(grand);
					states[gov] = std::move(state);
				}
			}
		}
	}
}



void ReputationManager::Save(DataWriter &out) const
{
	out.Write("reputation manager");
	out.BeginChild();
	{
		// Only save states that have meaningful data.
		for(const auto &pair : states)
		{
			if(!pair.first)
				continue;

			out.Write("government", pair.first->TrueName());
			out.BeginChild();
			{
				pair.second.Save(out);
			}
			out.EndChild();
		}
	}
	out.EndChild();
}



void ReputationManager::Clear()
{
	configs.clear();
	states.clear();
}



void ReputationManager::SetConfig(const Government *gov, const ReputationConfig &config)
{
	if(gov)
		configs[gov] = config;
}



const ReputationConfig &ReputationManager::GetConfig(const Government *gov) const
{
	auto it = configs.find(gov);
	return (it != configs.end()) ? it->second : defaultConfig;
}



vector<ThresholdCrossing> ReputationManager::StepDaily(const Date &currentDate)
{
	vector<ThresholdCrossing> crossings;

	// Process each government the player has interacted with.
	for(auto &pair : states)
	{
		const Government *gov = pair.first;
		if(!gov)
			continue;

		GovernmentReputationState &state = pair.second;

		// Increment days since positive interaction.
		++state.daysSincePositiveInteraction;

		// Get current reputation.
		double currentRep = gov->Reputation();
		double oldRep = currentRep;

		// Apply decay/recovery.
		double newRep = ApplyDecay(gov, currentRep, currentDate);

		// Check for threshold crossing.
		ReputationThreshold from, to;
		if(CrossesThreshold(oldRep, newRep, from, to))
		{
			crossings.emplace_back(gov, from, to, currentDate, oldRep, newRep);
		}

		// Apply the change through the government's reputation setter.
		if(newRep != oldRep)
			gov->SetReputation(newRep);

		// Check for atrocity forgiveness.
		if(state.hasCommittedAtrocity)
		{
			const ReputationConfig &config = GetConfig(gov);
			if(config.forgivesAtrocities && state.atrocityDate)
			{
				int daysSinceAtrocity = currentDate - state.atrocityDate;
				if(daysSinceAtrocity >= config.atrocityForgivenessDays)
				{
					state.hasCommittedAtrocity = false;
				}
			}
		}

		// Trim event history.
		state.TrimEventHistory(MAX_EVENT_HISTORY);
	}

	return crossings;
}



void ReputationManager::RecordChange(const Government *gov, const Date &date,
	double change, const string &reason, bool wasAtrocity, bool wasWitnessed)
{
	if(!gov)
		return;

	GovernmentReputationState &state = GetOrCreateState(gov);
	state.RecordEvent(ReputationEvent(date, change, reason, wasAtrocity, wasWitnessed));

	// Update peak/trough tracking.
	double currentRep = gov->Reputation();
	if(currentRep > state.peakReputation)
		state.peakReputation = currentRep;
	if(currentRep < state.troughReputation)
		state.troughReputation = currentRep;
}



void ReputationManager::RecordGoodDeed(const Government *gov, const Date &date)
{
	if(!gov)
		return;

	GovernmentReputationState &state = GetOrCreateState(gov);
	++state.goodDeedCount;
	state.lastInteraction = date;
	state.daysSincePositiveInteraction = 0;
}



void ReputationManager::RecordAtrocity(const Government *gov, const Date &date)
{
	if(!gov)
		return;

	GovernmentReputationState &state = GetOrCreateState(gov);
	state.hasCommittedAtrocity = true;
	state.atrocityDate = date;
	state.lastInteraction = date;
}



bool ReputationManager::HasUnforgivenAtrocity(const Government *gov) const
{
	auto it = states.find(gov);
	if(it == states.end())
		return false;

	if(!it->second.hasCommittedAtrocity)
		return false;

	const ReputationConfig &config = GetConfig(gov);
	return !config.forgivesAtrocities;
}



const GovernmentReputationState *ReputationManager::GetState(const Government *gov) const
{
	auto it = states.find(gov);
	return (it != states.end()) ? &it->second : nullptr;
}



GovernmentReputationState &ReputationManager::GetOrCreateState(const Government *gov)
{
	return states[gov];
}



double ReputationManager::GetEffectiveDecayRate(const Government *gov) const
{
	const ReputationConfig &config = GetConfig(gov);
	double rate = config.positiveDecayRate;

	auto it = states.find(gov);
	if(it != states.end())
	{
		const GovernmentReputationState &state = it->second;

		// Memory strength reduces decay.
		rate *= (1.0 - config.memoryStrength);

		// Good deeds slow decay.
		if(state.goodDeedCount > 0)
		{
			double reduction = min(0.5, state.goodDeedCount * 0.05);
			rate *= (1.0 - reduction);
		}

		// Recent positive interaction slows decay.
		if(state.daysSincePositiveInteraction < 7)
		{
			double reduction = 0.3 * (1.0 - state.daysSincePositiveInteraction / 7.0);
			rate *= (1.0 - reduction);
		}
	}

	return max(0.0, rate);
}



double ReputationManager::GetEffectiveRecoveryRate(const Government *gov) const
{
	const ReputationConfig &config = GetConfig(gov);
	double rate = config.negativeRecoveryRate;

	auto it = states.find(gov);
	if(it != states.end())
	{
		const GovernmentReputationState &state = it->second;

		// Atrocities significantly slow recovery.
		if(state.hasCommittedAtrocity)
			rate *= 0.1;

		// Long time since last interaction helps recovery.
		if(state.daysSincePositiveInteraction > 30)
		{
			double bonus = min(0.5, (state.daysSincePositiveInteraction - 30) * 0.01);
			rate *= (1.0 + bonus);
		}
	}

	return max(0.0, rate);
}



ReputationThreshold ReputationManager::GetThreshold(double reputation)
{
	if(reputation <= static_cast<int>(ReputationThreshold::WAR))
		return ReputationThreshold::WAR;
	if(reputation <= static_cast<int>(ReputationThreshold::HOSTILE))
		return ReputationThreshold::HOSTILE;
	if(reputation <= static_cast<int>(ReputationThreshold::UNFRIENDLY))
		return ReputationThreshold::UNFRIENDLY;
	if(reputation < static_cast<int>(ReputationThreshold::FRIENDLY))
		return ReputationThreshold::NEUTRAL;
	if(reputation < static_cast<int>(ReputationThreshold::ALLIED))
		return ReputationThreshold::FRIENDLY;
	if(reputation < static_cast<int>(ReputationThreshold::HONORED))
		return ReputationThreshold::ALLIED;
	return ReputationThreshold::HONORED;
}



string ReputationManager::GetThresholdName(ReputationThreshold threshold)
{
	switch(threshold)
	{
		case ReputationThreshold::WAR:
			return "at war";
		case ReputationThreshold::HOSTILE:
			return "hostile";
		case ReputationThreshold::UNFRIENDLY:
			return "unfriendly";
		case ReputationThreshold::NEUTRAL:
			return "neutral";
		case ReputationThreshold::FRIENDLY:
			return "friendly";
		case ReputationThreshold::ALLIED:
			return "allied";
		case ReputationThreshold::HONORED:
			return "honored";
	}
	return "unknown";
}



bool ReputationManager::CrossesThreshold(double oldRep, double newRep,
	ReputationThreshold &from, ReputationThreshold &to)
{
	from = GetThreshold(oldRep);
	to = GetThreshold(newRep);
	return from != to;
}



vector<ReputationEvent> ReputationManager::GetRecentEvents(const Government *gov, int days) const
{
	vector<ReputationEvent> result;

	auto it = states.find(gov);
	if(it == states.end())
		return result;

	// Note: Without a reference date, we return all stored events.
	// In practice, the caller should filter by date if needed.
	return it->second.recentEvents;
}



vector<const Government *> ReputationManager::GetKnownGovernments() const
{
	vector<const Government *> result;
	for(const auto &pair : states)
		if(pair.first)
			result.push_back(pair.first);
	return result;
}



double ReputationManager::ApplyDecay(const Government *gov, double currentRep,
	const Date &currentDate)
{
	const ReputationConfig &config = GetConfig(gov);
	double neutralPoint = config.neutralPoint;

	if(currentRep > neutralPoint)
	{
		// Positive reputation decays toward neutral.
		double rate = GetEffectiveDecayRate(gov);
		double decay = (currentRep - neutralPoint) * rate;
		double newRep = currentRep - decay;

		// Don't decay below neutral point.
		return max(neutralPoint, newRep);
	}
	else if(currentRep < neutralPoint)
	{
		// Negative reputation recovers toward neutral.
		double rate = GetEffectiveRecoveryRate(gov);
		double recovery = (neutralPoint - currentRep) * rate;
		double newRep = currentRep + recovery;

		// Don't recover above neutral point.
		return min(neutralPoint, newRep);
	}

	return currentRep;
}
