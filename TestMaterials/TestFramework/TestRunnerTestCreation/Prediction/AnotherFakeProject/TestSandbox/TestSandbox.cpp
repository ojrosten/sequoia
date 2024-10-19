////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "sequoia/TestFramework/TestRunner.hpp"

#include <iostream>

namespace myProject::testing{};

int main(int argc, char** argv)
{
	try
	{
		using namespace myProject::testing;
		using namespace std::literals::chrono_literals;

		sequoia::testing::test_runner runner{argc, argv, "Oliver J. Rosten", "\t"};

        runner.add_test_suite(
            "Maybe",
            maybe_false_positive_test{"False Positive Test"},
            maybe_test{"Unit Test"}
        );

        runner.add_test_suite(
            "Iterator",
            foo_allocation_test{"Allocation Test"},
            foo_test{"Unit Test"},
            foo_false_positive_test{"False Positive Test"},
            iterator_false_positive_test{"False Positive Test"},
            iterator_test{"Unit Test"}
        );

        runner.add_test_suite(
            "Widget",
            widget_false_positive_test{"False Positive Test"},
            widget_test{"Unit Test"}
        );

        runner.add_test_suite(
            "Probability",
            probability_false_positive_test{"False Positive Test"},
            probability_test{"Unit Test"}
        );

        runner.add_test_suite(
            "Angle",
            angle_false_positive_free_diagnostics{"Angle False Positive Free Diagnostics"},
            angle_false_negative_free_diagnostics{"Angle False Negative Free Diagnostics"},
            angle_false_positive_test{"False Positive Test"},
            angle_test{"Unit Test"}
        );

        runner.add_test_suite(
            "Human",
            human_false_positive_test{"False Positive Test"},
            human_test{"Unit Test"}
        );

        runner.add_test_suite(
            "Thingummy",
            thingummy_false_positive_test{"False Positive Test"},
            thingummy_test{"Unit Test"}
        );

        runner.add_test_suite(
            "Container",
            container_performance_test{"Container Performance Test"},
            container_allocation_test{"Allocation Test"},
            container_false_positive_test{"False Positive Test"},
            container_test{"Unit Test"}
        );

        runner.add_test_suite(
            "partners",
            couple_false_positive_test{"False Positive Test"},
            couple_test{"Unit Test"}
        );

        runner.add_test_suite(
            "Things",
            things_false_positive_test{"False Positive Test"},
            things_test{"Unit Test"}
        );

        runner.add_test_suite(
            "Variadic",
            variadic_false_positive_test{"False Positive Test"},
            variadic_test{"Unit Test"}
        );

        runner.add_test_suite(
            "Multiple",
            multiple_false_positive_test{"False Positive Test"},
            multiple_test{"Unit Test"}
        );

        runner.add_test_suite(
            "Cloud",
            cloud_false_positive_test{"False Positive Test"},
            cloud_test{"Unit Test"}
        );

        runner.add_test_suite(
            "Utilities",
            utilities_free_test{"Utilities Free Test"}
        );

        runner.add_test_suite(
            "Bazzer",
            bazagain_free_test{"Bazagain Free Test"},
            bazzer_free_test{"Bazzer Free Test"}
        );

        runner.add_test_suite(
            "Doohicky",
            doohicky_free_test{"Doohicky Free Test"}
        );

        runner.add_test_suite(
            "Global",
            global_free_test{"Global Free Test"}
        );

        runner.add_test_suite(
            "Defs",
            defs_free_test{"Defs Free Test"}
        );

		runner.execute(sequoia::timer_resolution{1ms});
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

