////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/TestRunner.hpp"

#include <iostream>

namespace myProject::testing{};

int main(int argc, char** argv)
{
	auto code{sequoia::testing::return_code::incomplete_run};

	try
	{
		using namespace myProject::testing;
		using namespace std::literals::chrono_literals;

		sequoia::testing::test_runner runner{argc, argv, "Oliver J. Rosten", "\t"};

        runner.add_test_suite(
            "Maybe",
            maybe_false_negative_test{},
            maybe_test{}
        );

        runner.add_test_suite(
            "Iterator",
            foo_allocation_test{},
            foo_test{},
            foo_false_negative_test{},
            iterator_false_negative_test{},
            iterator_test{}
        );

        runner.add_test_suite(
            "Widget",
            widget_false_negative_test{},
            widget_test{}
        );

        runner.add_test_suite(
            "Probability",
            probability_false_negative_test{},
            probability_test{}
        );

        runner.add_test_suite(
            "Angle",
            angle_false_negative_free_diagnostics{},
            angle_false_positive_free_diagnostics{},
            angle_false_negative_test{},
            angle_test{}
        );

        runner.add_test_suite(
            "Human",
            human_false_negative_test{},
            human_test{}
        );

        runner.add_test_suite(
            "Thingummy",
            thingummy_false_negative_test{},
            thingummy_test{}
        );

        runner.add_test_suite(
            "Container",
            container_performance_test{},
            container_allocation_test{},
            container_false_negative_test{},
            container_test{}
        );

        runner.add_test_suite(
            "partners",
            couple_false_negative_test{},
            couple_test{}
        );

        runner.add_test_suite(
            "Things",
            things_false_negative_test{},
            things_test{}
        );

        runner.add_test_suite(
            "Variadic",
            variadic_false_negative_test{},
            variadic_test{}
        );

        runner.add_test_suite(
            "Multiple",
            multiple_false_negative_test{},
            multiple_test{}
        );

        runner.add_test_suite(
            "Cloud",
            cloud_false_negative_test{},
            cloud_test{}
        );

        runner.add_test_suite(
            "Utilities",
            utilities_free_test{}
        );

        runner.add_test_suite(
            "Bazzer",
            bazagain_free_test{},
            bazzer_free_test{}
        );

        runner.add_test_suite(
            "Doohicky",
            doohicky_free_test{}
        );

        runner.add_test_suite(
            "Global",
            global_free_test{}
        );

        runner.add_test_suite(
            "Defs",
            defs_free_test{}
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

