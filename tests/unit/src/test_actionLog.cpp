/* test_actionLog.cpp
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
#include "../../../source/ActionLog.h"

// Include other necessary headers.
#include "../../../source/Date.h"
#include "../../../source/ShipEvent.h"



namespace { // test namespace

// #region mock data
// #endregion mock data



// #region unit tests
SCENARIO( "Creating an ActionLog" , "[ActionLog][Creation]" ) {
	GIVEN( "an empty action log" ) {
		ActionLog log;
		THEN( "it should be empty" ) {
			REQUIRE( log.IsEmpty() );
			REQUIRE( log.Size() == 0 );
		}
		THEN( "it should have the default max records" ) {
			REQUIRE( log.MaxRecords() == ActionLog::DEFAULT_MAX_RECORDS );
		}
	}
	GIVEN( "an action log with custom max records" ) {
		ActionLog log(500);
		THEN( "it should have the custom max records" ) {
			REQUIRE( log.MaxRecords() == 500 );
		}
	}
}

SCENARIO( "Recording actions in ActionLog" , "[ActionLog][Record]" ) {
	GIVEN( "an empty action log" ) {
		ActionLog log;
		Date testDate(1, 1, 3014);

		WHEN( "a single action is recorded" ) {
			log.Record(testDate, ShipEvent::DESTROY, nullptr, "Test System", 5, 100000, true);
			THEN( "the log should contain one record" ) {
				REQUIRE( log.Size() == 1 );
				REQUIRE( !log.IsEmpty() );
			}
		}

		WHEN( "an ActionRecord is recorded" ) {
			ActionRecord record(testDate, ShipEvent::DISABLE, nullptr, "Another System", 2, 50000, false);
			log.Record(record);
			THEN( "the log should contain the record" ) {
				REQUIRE( log.Size() == 1 );
			}
		}
	}
}

SCENARIO( "ActionLog size limiting" , "[ActionLog][Limits]" ) {
	GIVEN( "an action log with max 5 records" ) {
		ActionLog log(5);
		Date testDate(1, 1, 3014);

		WHEN( "6 records are added" ) {
			for(int i = 0; i < 6; ++i)
				log.Record(testDate, ShipEvent::DESTROY, nullptr, "System " + std::to_string(i));

			THEN( "only 5 records should remain" ) {
				REQUIRE( log.Size() == 5 );
			}
		}

		WHEN( "max records is changed to smaller value" ) {
			for(int i = 0; i < 5; ++i)
				log.Record(testDate, ShipEvent::DESTROY, nullptr, "System " + std::to_string(i));
			log.SetMaxRecords(3);

			THEN( "log should be trimmed to new size" ) {
				REQUIRE( log.Size() == 3 );
			}
		}
	}
}

SCENARIO( "Querying recent actions" , "[ActionLog][Query]" ) {
	GIVEN( "an action log with dated records" ) {
		ActionLog log;
		Date day1(1, 1, 3014);
		Date day5(5, 1, 3014);
		Date day10(10, 1, 3014);
		Date day15(15, 1, 3014);

		log.Record(day1, ShipEvent::DESTROY, nullptr, "System A");
		log.Record(day5, ShipEvent::DISABLE, nullptr, "System B");
		log.Record(day10, ShipEvent::ASSIST, nullptr, "System C");

		WHEN( "querying last 5 days from day 15" ) {
			auto actions = log.GetRecentActions(day15, 5);
			THEN( "only day 10 action should be returned" ) {
				REQUIRE( actions.size() == 1 );
			}
		}

		WHEN( "querying last 15 days from day 15" ) {
			auto actions = log.GetRecentActions(day15, 15);
			THEN( "all actions should be returned" ) {
				REQUIRE( actions.size() == 3 );
			}
		}
	}
}

SCENARIO( "Counting event types" , "[ActionLog][Count]" ) {
	GIVEN( "an action log with various event types" ) {
		ActionLog log;
		Date testDate(1, 1, 3014);

		log.Record(testDate, ShipEvent::DESTROY, nullptr, "System A");
		log.Record(testDate, ShipEvent::DESTROY, nullptr, "System B");
		log.Record(testDate, ShipEvent::DISABLE, nullptr, "System C");
		log.Record(testDate, ShipEvent::ASSIST, nullptr, "System D");

		WHEN( "counting DESTROY events" ) {
			int count = log.CountEventType(ShipEvent::DESTROY, testDate, 30);
			THEN( "should return 2" ) {
				REQUIRE( count == 2 );
			}
		}

		WHEN( "counting ASSIST events" ) {
			int count = log.CountEventType(ShipEvent::ASSIST, testDate, 30);
			THEN( "should return 1" ) {
				REQUIRE( count == 1 );
			}
		}
	}
}

SCENARIO( "Clearing the ActionLog" , "[ActionLog][Clear]" ) {
	GIVEN( "an action log with records" ) {
		ActionLog log;
		Date testDate(1, 1, 3014);

		log.Record(testDate, ShipEvent::DESTROY, nullptr, "System A");
		log.Record(testDate, ShipEvent::DISABLE, nullptr, "System B");
		REQUIRE( log.Size() == 2 );

		WHEN( "the log is cleared" ) {
			log.Clear();
			THEN( "it should be empty" ) {
				REQUIRE( log.IsEmpty() );
				REQUIRE( log.Size() == 0 );
			}
		}
	}
}

SCENARIO( "Witness ratio calculation" , "[ActionLog][WitnessRatio]" ) {
	GIVEN( "an action log with witnessed and unwitnessed actions" ) {
		ActionLog log;
		Date testDate(1, 1, 3014);

		log.Record(testDate, ShipEvent::DESTROY, nullptr, "System A", 0, 0, true);
		log.Record(testDate, ShipEvent::DESTROY, nullptr, "System B", 0, 0, true);
		log.Record(testDate, ShipEvent::DESTROY, nullptr, "System C", 0, 0, false);
		log.Record(testDate, ShipEvent::DESTROY, nullptr, "System D", 0, 0, false);

		WHEN( "calculating witness ratio" ) {
			double ratio = log.GetWitnessedRatio(testDate, 30);
			THEN( "should return 0.5 (half witnessed)" ) {
				REQUIRE( ratio == Catch::Approx(0.5) );
			}
		}
	}
}
// #endregion unit tests



} // test namespace
