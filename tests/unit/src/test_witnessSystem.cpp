/* test_witnessSystem.cpp
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
#include "../../../source/WitnessSystem.h"

// Include other necessary headers.
#include "../../../source/Point.h"



namespace { // test namespace

// #region mock data
// #endregion mock data



// #region unit tests
SCENARIO( "WitnessResult management" , "[WitnessSystem][WitnessResult]" ) {
	GIVEN( "an empty witness result" ) {
		WitnessResult result;

		THEN( "it should have no witnesses" ) {
			REQUIRE( result.HasWitnesses() == false );
			REQUIRE( result.WitnessCount() == 0 );
		}

		WHEN( "a witness is added" ) {
			WitnessInfo info;
			info.government = nullptr;
			info.distance = 1000.0;
			info.canReport = true;
			info.clarity = 0.8;
			result.AddWitness(info);

			THEN( "it should have one witness" ) {
				REQUIRE( result.HasWitnesses() == true );
				REQUIRE( result.WitnessCount() == 1 );
			}

			THEN( "max clarity should be correct" ) {
				REQUIRE( result.GetMaxClarity() == Catch::Approx(0.8) );
			}

			THEN( "closest distance should be correct" ) {
				REQUIRE( result.GetClosestWitnessDistance() == Catch::Approx(1000.0) );
			}
		}
	}
}

SCENARIO( "WitnessResult with multiple witnesses" , "[WitnessSystem][WitnessResult]" ) {
	GIVEN( "a result with multiple witnesses" ) {
		WitnessResult result;

		WitnessInfo info1;
		info1.distance = 2000.0;
		info1.clarity = 0.5;
		info1.canReport = true;
		result.AddWitness(info1);

		WitnessInfo info2;
		info2.distance = 500.0;
		info2.clarity = 0.9;
		info2.canReport = false;
		result.AddWitness(info2);

		THEN( "witness count should be 2" ) {
			REQUIRE( result.WitnessCount() == 2 );
		}

		THEN( "closest distance should be 500" ) {
			REQUIRE( result.GetClosestWitnessDistance() == Catch::Approx(500.0) );
		}

		THEN( "max clarity should be 0.9" ) {
			REQUIRE( result.GetMaxClarity() == Catch::Approx(0.9) );
		}
	}
}

SCENARIO( "WitnessResult clear witnessing check" , "[WitnessSystem][WitnessResult]" ) {
	GIVEN( "a result with low clarity witnesses" ) {
		WitnessResult result;

		WitnessInfo info;
		info.clarity = 0.3;
		result.AddWitness(info);

		THEN( "should not be clearly witnessed at default threshold" ) {
			REQUIRE( result.WasClearlyWitnessed() == false );
		}

		THEN( "should be clearly witnessed at lower threshold" ) {
			REQUIRE( result.WasClearlyWitnessed(0.2) == true );
		}
	}
}

SCENARIO( "WitnessReport stepping" , "[WitnessSystem][WitnessReport]" ) {
	GIVEN( "a witness report with 3 frames remaining" ) {
		WitnessReport report(nullptr, nullptr, 0, 3, nullptr, 10.0);

		WHEN( "stepped once" ) {
			bool ready = report.Step();
			THEN( "it should not be ready" ) {
				REQUIRE( ready == false );
			}
		}

		WHEN( "stepped three times" ) {
			report.Step();
			report.Step();
			bool ready = report.Step();
			THEN( "it should be ready" ) {
				REQUIRE( ready == true );
			}
		}
	}
}

SCENARIO( "WitnessSystem report queue" , "[WitnessSystem][Queue]" ) {
	GIVEN( "a witness system" ) {
		WitnessSystem system;

		THEN( "it should have no pending reports" ) {
			REQUIRE( system.GetPendingReports().empty() );
		}

		WHEN( "a report is queued" ) {
			WitnessReport report(nullptr, nullptr, 0, 2, nullptr, 5.0);
			system.QueueReport(report);

			THEN( "there should be one pending report" ) {
				REQUIRE( system.GetPendingReports().size() == 1 );
			}
		}
	}
}

SCENARIO( "WitnessSystem stepping reports" , "[WitnessSystem][Step]" ) {
	GIVEN( "a system with a report ready in 2 frames" ) {
		WitnessSystem system;
		WitnessReport report(nullptr, nullptr, 0, 2, nullptr, 5.0);
		system.QueueReport(report);

		WHEN( "stepped once" ) {
			auto ready = system.StepReports();
			THEN( "no reports should be ready" ) {
				REQUIRE( ready.empty() );
			}
			THEN( "report should still be pending" ) {
				REQUIRE( system.GetPendingReports().size() == 1 );
			}
		}

		WHEN( "stepped twice" ) {
			system.StepReports();
			auto ready = system.StepReports();
			THEN( "one report should be ready" ) {
				REQUIRE( ready.size() == 1 );
			}
			THEN( "no reports should be pending" ) {
				REQUIRE( system.GetPendingReports().empty() );
			}
		}
	}
}

SCENARIO( "WitnessSystem clear" , "[WitnessSystem][Clear]" ) {
	GIVEN( "a system with pending reports" ) {
		WitnessSystem system;
		system.QueueReport(WitnessReport(nullptr, nullptr, 0, 10, nullptr, 5.0));
		system.QueueReport(WitnessReport(nullptr, nullptr, 0, 20, nullptr, 10.0));
		REQUIRE( system.GetPendingReports().size() == 2 );

		WHEN( "system is cleared" ) {
			system.Clear();
			THEN( "all reports should be gone" ) {
				REQUIRE( system.GetPendingReports().empty() );
			}
		}
	}
}

SCENARIO( "Witness constants are reasonable" , "[WitnessSystem][Constants]" ) {
	THEN( "default witness range should be positive" ) {
		REQUIRE( WitnessConstants::DEFAULT_WITNESS_RANGE > 0.0 );
	}
	THEN( "cloak threshold should be between 0 and 1" ) {
		REQUIRE( WitnessConstants::CLOAK_WITNESS_THRESHOLD >= 0.0 );
		REQUIRE( WitnessConstants::CLOAK_WITNESS_THRESHOLD <= 1.0 );
	}
	THEN( "report chances should be between 0 and 1" ) {
		REQUIRE( WitnessConstants::CIVILIAN_REPORT_CHANCE >= 0.0 );
		REQUIRE( WitnessConstants::CIVILIAN_REPORT_CHANCE <= 1.0 );
		REQUIRE( WitnessConstants::MILITARY_REPORT_CHANCE >= 0.0 );
		REQUIRE( WitnessConstants::MILITARY_REPORT_CHANCE <= 1.0 );
	}
}
// #endregion unit tests



} // test namespace
