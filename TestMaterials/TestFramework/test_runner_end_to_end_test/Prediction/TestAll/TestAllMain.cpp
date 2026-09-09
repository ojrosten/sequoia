////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2026.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include <iostream>
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

namespace generatedProject::testing{};

int main(int argc, char** argv)
{
	auto code{sequoia::testing::return_code::incomplete_run};

	try
	{
		using namespace generatedProject::testing;
		using namespace std::literals::chrono_literals;

		sequoia::testing::test_runner runner{argc, argv, "Oliver Jacob Rosten", "\t"};

		runner.add_test_suite(
			"Utilities",
			utilities_free_test{}
		);

		runner.add_test_suite(
			"Useful Things",
			useful_things_free_test{}
		);

		runner.add_test_suite(
			"Bar",
			bar_free_test{}
		);

		runner.add_test_suite(
			"Unstable",
			flipper_free_test{}
		);

		runner.add_test_suite(
			"Maybe",
			maybe_false_negative_test{},
			maybe_test{}
		);

		runner.add_test_suite(
			"Oldschool",
			oldschool_false_negative_test{},
			oldschool_test{}
		);

		runner.add_test_suite(
			"Probability",
			probability_false_negative_test{},
			probability_test{}
		);

		runner.add_test_suite(
			"Foo",
			foo_false_negative_test{},
			foo_test{}
		);

		runner.add_test_suite(
			"Unique Thing",
			unique_thing_false_negative_test{},
			unique_thing_test{}
		);

		runner.add_test_suite(
			"Container",
			container_performance_test{},
			container_allocation_test{}
		);

		runner.add_test_suite(
			"House",
			house_allocation_test{}
		);

		code = runner.execute(sequoia::timer_resolution{1ms});
	}
	catch(const std::exception& e)
	{
		std::cout << e.what();
	}
	catch(...)
	{
		std::cout << "Unrecognized error\n"; 
	}

	return sequoia::testing::to_exit_code(code);
}

