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
runner.register_test<utilities_free_test>();runner.register_test<useful_things_free_test>();runner.register_test<bar_free_test>();runner.register_test<flipper_free_test>();runner.register_test<maybe_false_negative_test>();
		runner.register_test<maybe_test>();runner.register_test<oldschool_false_negative_test>();
		runner.register_test<oldschool_test>();runner.register_test<probability_false_negative_test>();
		runner.register_test<probability_test>();runner.register_test<foo_false_negative_test>();
		runner.register_test<foo_test>();runner.register_test<unique_thing_false_negative_test>();
		runner.register_test<unique_thing_test>();runner.register_test<container_allocation_test>();runner.register_test<house_allocation_test>();runner.register_test<container_performance_test>();
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

