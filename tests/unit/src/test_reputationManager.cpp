/* test_reputationManager.cpp
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

#include "es-test.hpp"

// Include only the tested class's header.
#include "../../../source/ReputationManager.h"

// Include other necessary headers.
#include "../../../source/Date.h"



namespace { // test namespace

// #region mock data
// #endregion mock data



// #region unit tests
SCENARIO( "ReputationConfig defaults" , "[ReputationManager][Config]" ) {
	GIVEN( "a default reputation config" ) {
		ReputationConfig config;

		THEN( "positive decay rate should be reasonable" ) {
			REQUIRE( config.positiveDecayRate >= 0.0 );
			REQUIRE( config.positiveDecayRate <= 1.0 );
		}

		THEN( "negative recovery rate should be reasonable" ) {
			REQUIRE( config.negativeRecoveryRate >= 0.0 );
			REQUIRE( config.negativeRecoveryRate <= 1.0 );
		}

		THEN( "neutral point should be zero" ) {
			REQUIRE( config.neutralPoint == 0.0 );
		}

		THEN( "atrocity forgiveness should be disabled by default" ) {
			REQUIRE( config.forgivesAtrocities == false );
		}
	}
}

SCENARIO( "GovernmentReputationState tracking" , "[ReputationManager][State]" ) {
	GIVEN( "a new reputation state" ) {
		GovernmentReputationState state;
		Date testDate(1, 1, 3014);

		THEN( "it should have no atrocity" ) {
			REQUIRE( state.hasCommittedAtrocity == false );
		}

		THEN( "good deed count should be zero" ) {
			REQUIRE( state.goodDeedCount == 0 );
		}

		WHEN( "an event is recorded" ) {
			ReputationEvent event(testDate, -10.0, "attacked ship", false, true);
			state.RecordEvent(event);

			THEN( "event should be in history" ) {
				REQUIRE( state.recentEvents.size() == 1 );
			}

			THEN( "last interaction should be updated" ) {
				REQUIRE( state.lastInteraction == testDate );
			}
		}

		WHEN( "an atrocity is recorded" ) {
			ReputationEvent event(testDate, -50.0, "committed atrocity", true, true);
			state.RecordEvent(event);

			THEN( "atrocity flag should be set" ) {
				REQUIRE( state.hasCommittedAtrocity == true );
			}

			THEN( "atrocity date should be set" ) {
				REQUIRE( state.atrocityDate == testDate );
			}
		}
	}
}

SCENARIO( "ReputationManager threshold detection" , "[ReputationManager][Threshold]" ) {
	THEN( "very negative reputation should be WAR" ) {
		REQUIRE( ReputationManager::GetThreshold(-150.0) == ReputationThreshold::WAR );
	}

	THEN( "moderately negative reputation should be HOSTILE" ) {
		REQUIRE( ReputationManager::GetThreshold(-60.0) == ReputationThreshold::HOSTILE );
	}

	THEN( "slightly negative reputation should be UNFRIENDLY" ) {
		REQUIRE( ReputationManager::GetThreshold(-30.0) == ReputationThreshold::UNFRIENDLY );
	}

	THEN( "zero reputation should be NEUTRAL" ) {
		REQUIRE( ReputationManager::GetThreshold(0.0) == ReputationThreshold::NEUTRAL );
	}

	THEN( "moderately positive reputation should be FRIENDLY" ) {
		REQUIRE( ReputationManager::GetThreshold(30.0) == ReputationThreshold::FRIENDLY );
	}

	THEN( "high positive reputation should be ALLIED" ) {
		REQUIRE( ReputationManager::GetThreshold(60.0) == ReputationThreshold::ALLIED );
	}

	THEN( "very high positive reputation should be HONORED" ) {
		REQUIRE( ReputationManager::GetThreshold(150.0) == ReputationThreshold::HONORED );
	}
}

SCENARIO( "ReputationManager threshold crossing detection" , "[ReputationManager][Crossing]" ) {
	ReputationThreshold from, to;

	WHEN( "reputation doesn't cross a threshold" ) {
		bool crossed = ReputationManager::CrossesThreshold(10.0, 15.0, from, to);
		THEN( "it should return false" ) {
			REQUIRE( crossed == false );
		}
	}

	WHEN( "reputation crosses from NEUTRAL to FRIENDLY" ) {
		bool crossed = ReputationManager::CrossesThreshold(20.0, 30.0, from, to);
		THEN( "it should return true" ) {
			REQUIRE( crossed == true );
			REQUIRE( from == ReputationThreshold::NEUTRAL );
			REQUIRE( to == ReputationThreshold::FRIENDLY );
		}
	}

	WHEN( "reputation crosses from FRIENDLY to HOSTILE" ) {
		bool crossed = ReputationManager::CrossesThreshold(30.0, -60.0, from, to);
		THEN( "it should return true" ) {
			REQUIRE( crossed == true );
			REQUIRE( from == ReputationThreshold::FRIENDLY );
			REQUIRE( to == ReputationThreshold::HOSTILE );
		}
	}
}

SCENARIO( "ReputationManager threshold names" , "[ReputationManager][Names]" ) {
	THEN( "WAR should have correct name" ) {
		REQUIRE( ReputationManager::GetThresholdName(ReputationThreshold::WAR) == "at war" );
	}

	THEN( "HOSTILE should have correct name" ) {
		REQUIRE( ReputationManager::GetThresholdName(ReputationThreshold::HOSTILE) == "hostile" );
	}

	THEN( "NEUTRAL should have correct name" ) {
		REQUIRE( ReputationManager::GetThresholdName(ReputationThreshold::NEUTRAL) == "neutral" );
	}

	THEN( "FRIENDLY should have correct name" ) {
		REQUIRE( ReputationManager::GetThresholdName(ReputationThreshold::FRIENDLY) == "friendly" );
	}

	THEN( "ALLIED should have correct name" ) {
		REQUIRE( ReputationManager::GetThresholdName(ReputationThreshold::ALLIED) == "allied" );
	}

	THEN( "HONORED should have correct name" ) {
		REQUIRE( ReputationManager::GetThresholdName(ReputationThreshold::HONORED) == "honored" );
	}
}

SCENARIO( "ReputationManager creation and clearing" , "[ReputationManager][Management]" ) {
	GIVEN( "a new reputation manager" ) {
		ReputationManager manager;

		THEN( "it should have no known governments" ) {
			REQUIRE( manager.GetKnownGovernments().empty() );
		}

		WHEN( "state is created for a government" ) {
			auto &state = manager.GetOrCreateState(nullptr);
			(void)state;  // suppress unused warning

			WHEN( "manager is cleared" ) {
				manager.Clear();
				THEN( "it should be empty again" ) {
					REQUIRE( manager.GetKnownGovernments().empty() );
				}
			}
		}
	}
}

SCENARIO( "Recording good deeds" , "[ReputationManager][GoodDeed]" ) {
	GIVEN( "a reputation manager" ) {
		ReputationManager manager;
		Date testDate(1, 1, 3014);

		WHEN( "a good deed is recorded" ) {
			manager.RecordGoodDeed(nullptr, testDate);

			const GovernmentReputationState *state = manager.GetState(nullptr);
			THEN( "state should exist" ) {
				REQUIRE( state != nullptr );
			}
			THEN( "good deed count should be 1" ) {
				REQUIRE( state->goodDeedCount == 1 );
			}
			THEN( "last interaction should be updated" ) {
				REQUIRE( state->lastInteraction == testDate );
			}
		}
	}
}

SCENARIO( "Recording atrocities" , "[ReputationManager][Atrocity]" ) {
	GIVEN( "a reputation manager" ) {
		ReputationManager manager;
		Date testDate(1, 1, 3014);

		WHEN( "an atrocity is recorded" ) {
			manager.RecordAtrocity(nullptr, testDate);

			THEN( "there should be an unforgiven atrocity" ) {
				// Default config doesn't forgive atrocities
				REQUIRE( manager.HasUnforgivenAtrocity(nullptr) == true );
			}
		}
	}
}

SCENARIO( "Event history trimming" , "[ReputationManager][Trim]" ) {
	GIVEN( "a state with many events" ) {
		GovernmentReputationState state;
		Date testDate(1, 1, 3014);

		// Add 100 events.
		for(int i = 0; i < 100; ++i)
		{
			ReputationEvent event(testDate, -1.0, "event " + std::to_string(i));
			state.RecordEvent(event);
		}

		REQUIRE( state.recentEvents.size() == 100 );

		WHEN( "history is trimmed to 50" ) {
			state.TrimEventHistory(50);
			THEN( "only 50 events should remain" ) {
				REQUIRE( state.recentEvents.size() == 50 );
			}
		}
	}
}
// #endregion unit tests



} // test namespace
