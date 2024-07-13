////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2024.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "HouseAllocationTest.hpp"
#include "Maths/ProbabilityTest.hpp"
#include "Maths/ProbabilityTestingDiagnostics.hpp"
#include "Maybe/MaybeTest.hpp"
#include "Maybe/MaybeTestingDiagnostics.hpp"
#include "Stuff/BarFreeTest.hpp"
#include "Stuff/FooTest.hpp"
#include "Stuff/FooTestingDiagnostics.hpp"
#include "Stuff/OldschoolTest.hpp"
#include "Stuff/OldschoolTestingDiagnostics.hpp"
#include "Unstable/FlipperFreeTest.hpp"
#include "Utilities/ContainerAllocationTest.hpp"
#include "Utilities/ContainerPerformanceTest.hpp"
#include "Utilities/Thing/UniqueThingTest.hpp"
#include "Utilities/Thing/UniqueThingTestingDiagnostics.hpp"
#include "Utilities/UsefulThingsFreeTest.hpp"
#include "Utilities/UtilitiesFreeTest.hpp"
#include "sequoia/TestFramework/TestRunner.hpp"
#include <iostream>

int main(int argc, char** argv)
{
	try
	{
		using namespace sequoia;
		using namespace testing;
		using namespace std::literals::chrono_literals;

		test_runner runner{argc, argv, "Oliver Jacob Rosten", "\t"};

		runner.add_test_suite(
			"Utilities",
			utilities_free_test{"Utilities Free Test"}
		);

		runner.add_test_suite(
			"Useful Things",
			useful_things_free_test{"Useful Things Free Test"}
		);

		runner.add_test_suite(
			"Bar",
			bar_free_test{"Bar Free Test"}
		);

		runner.add_test_suite(
			"Unstable",
			flipper_free_test{"Flipper Free Test"}
		);

		runner.add_test_suite(
			"Maybe",
			maybe_false_positive_test{"False Positive Test"},
			maybe_test{"Unit Test"}
		);

		runner.add_test_suite(
			"Oldschool",
			oldschool_false_positive_test{"False Positive Test"},
			oldschool_test{"Unit Test"}
		);

		runner.add_test_suite(
			"Probability",
			probability_false_positive_test{"False Positive Test"},
			probability_test{"Unit Test"}
		);

		runner.add_test_suite(
			"Foo",
			foo_false_positive_test{"False Positive Test"},
			foo_test{"Unit Test"}
		);

		runner.add_test_suite(
			"Unique Thing",
			unique_thing_false_positive_test{"False Positive Test"},
			unique_thing_test{"Unit Test"}
		);

		runner.add_test_suite(
			"Container",
			container_performance_test{"Container Performance Test"},
			container_allocation_test{"Allocation Test"}
		);

		runner.add_test_suite(
			"House",
			house_allocation_test{"Allocation Test"}
		);

		runner.execute(timer_resolution{1ms});
	}
	catch(const std::exception& e)
	{
		std::cout << e.what();
	}
	catch(...)
	{
		std::cout << "Unrecognized error\n"; 
	}

	return 0;
}

