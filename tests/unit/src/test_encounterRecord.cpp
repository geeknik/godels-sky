/* test_encounterRecord.cpp
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
#include "../../../source/EncounterRecord.h"

// Include other necessary headers.
#include "../../../source/Date.h"
#include "../../../source/ShipEvent.h"



namespace { // test namespace

// #region mock data
// #endregion mock data



// #region unit tests
SCENARIO( "Creating an EncounterRecord" , "[EncounterRecord][Creation]" ) {
	GIVEN( "default construction" ) {
		EncounterRecord record;
		THEN( "it should have zero encounters" ) {
			REQUIRE( record.timesEncountered == 0 );
			REQUIRE( record.timesAssisted == 0 );
			REQUIRE( record.timesAttacked == 0 );
		}
	}
	GIVEN( "construction with date and system" ) {
		Date testDate(1, 1, 3014);
		EncounterRecord record(testDate, "Sol", "uuid-123");

		THEN( "it should have one encounter" ) {
			REQUIRE( record.timesEncountered == 1 );
		}
		THEN( "it should store the system" ) {
			REQUIRE( record.lastSystem == "Sol" );
		}
		THEN( "it should store the UUID" ) {
			REQUIRE( record.npcUuid == "uuid-123" );
		}
	}
}

SCENARIO( "Recording encounters" , "[EncounterRecord][Encounter]" ) {
	GIVEN( "an encounter record" ) {
		Date day1(1, 1, 3014);
		Date day5(5, 1, 3014);
		EncounterRecord record(day1, "Sol", "uuid-123");

		WHEN( "a new encounter is recorded" ) {
			record.RecordEncounter(day5, "Alpha Centauri");
			THEN( "encounter count should increase" ) {
				REQUIRE( record.timesEncountered == 2 );
			}
			THEN( "last seen date should update" ) {
				REQUIRE( record.lastSeen == day5 );
			}
			THEN( "last system should update" ) {
				REQUIRE( record.lastSystem == "Alpha Centauri" );
			}
		}
	}
}

SCENARIO( "Recording events" , "[EncounterRecord][Events]" ) {
	GIVEN( "a new encounter record" ) {
		Date testDate(1, 1, 3014);
		EncounterRecord record(testDate, "Sol", "uuid-123");

		WHEN( "an ASSIST event is recorded" ) {
			record.RecordEvent(ShipEvent::ASSIST);
			THEN( "times assisted should increase" ) {
				REQUIRE( record.timesAssisted == 1 );
			}
			THEN( "event bitmask should include ASSIST" ) {
				REQUIRE( (record.eventsBitmask & ShipEvent::ASSIST) != 0 );
			}
		}

		WHEN( "a DESTROY event is recorded" ) {
			record.RecordEvent(ShipEvent::DESTROY);
			THEN( "times attacked should increase" ) {
				REQUIRE( record.timesAttacked == 1 );
			}
		}

		WHEN( "a DISABLE event is recorded" ) {
			record.RecordEvent(ShipEvent::DISABLE);
			THEN( "was disabled should be true" ) {
				REQUIRE( record.wasDisabled == true );
			}
			THEN( "times attacked should increase" ) {
				REQUIRE( record.timesAttacked == 1 );
			}
		}

		WHEN( "a BOARD event is recorded" ) {
			record.RecordEvent(ShipEvent::BOARD);
			THEN( "was boarded should be true" ) {
				REQUIRE( record.wasBoarded == true );
			}
		}
	}
}

SCENARIO( "Getting disposition from history" , "[EncounterRecord][Disposition]" ) {
	GIVEN( "a fresh encounter record" ) {
		Date testDate(1, 1, 3014);
		EncounterRecord record(testDate, "Sol", "uuid-123");

		WHEN( "no events have occurred" ) {
			THEN( "disposition should be NEUTRAL" ) {
				REQUIRE( record.GetDisposition() == NPCDisposition::NEUTRAL );
			}
		}

		WHEN( "player has assisted multiple times" ) {
			record.RecordEvent(ShipEvent::ASSIST);
			record.RecordEvent(ShipEvent::ASSIST);
			record.RecordEvent(ShipEvent::ASSIST);
			THEN( "disposition should be GRATEFUL or higher" ) {
				auto disp = record.GetDisposition();
				REQUIRE( (disp == NPCDisposition::GRATEFUL || disp == NPCDisposition::INDEBTED) );
			}
		}

		WHEN( "player has attacked multiple times" ) {
			record.RecordEvent(ShipEvent::DESTROY);
			record.RecordEvent(ShipEvent::DESTROY);
			record.RecordEvent(ShipEvent::DISABLE);
			THEN( "disposition should be HOSTILE or worse" ) {
				auto disp = record.GetDisposition();
				REQUIRE( (disp == NPCDisposition::HOSTILE || disp == NPCDisposition::NEMESIS) );
			}
		}
	}
}

SCENARIO( "Checking recognition threshold" , "[EncounterRecord][Recognition]" ) {
	GIVEN( "an encounter record" ) {
		Date testDate(1, 1, 3014);
		EncounterRecord record(testDate, "Sol", "uuid-123");

		WHEN( "only one encounter" ) {
			THEN( "player should not be recognized" ) {
				REQUIRE( record.WouldRecognizePlayer() == false );
			}
		}

		WHEN( "three encounters" ) {
			record.RecordEncounter(testDate, "Sol");
			record.RecordEncounter(testDate, "Sol");
			THEN( "player should be recognized" ) {
				REQUIRE( record.WouldRecognizePlayer() == true );
			}
		}

		WHEN( "player disabled the NPC" ) {
			record.RecordEvent(ShipEvent::DISABLE);
			THEN( "player should be recognized" ) {
				REQUIRE( record.WouldRecognizePlayer() == true );
			}
		}
	}
}

SCENARIO( "EncounterLog management" , "[EncounterLog][Management]" ) {
	GIVEN( "an empty encounter log" ) {
		EncounterLog log;
		Date testDate(1, 1, 3014);

		THEN( "size should be zero" ) {
			REQUIRE( log.Size() == 0 );
		}

		WHEN( "getting or creating a record" ) {
			auto &record = log.GetOrCreate("uuid-123", testDate, "Sol");
			THEN( "a new record should exist" ) {
				REQUIRE( log.Size() == 1 );
				REQUIRE( log.HasRecord("uuid-123") );
			}
			THEN( "the record should have correct data" ) {
				REQUIRE( record.npcUuid == "uuid-123" );
				REQUIRE( record.lastSystem == "Sol" );
			}
		}

		WHEN( "checking for non-existent record" ) {
			const EncounterRecord *record = log.Get("non-existent");
			THEN( "it should return nullptr" ) {
				REQUIRE( record == nullptr );
			}
		}
	}
}
// #endregion unit tests



} // test namespace
