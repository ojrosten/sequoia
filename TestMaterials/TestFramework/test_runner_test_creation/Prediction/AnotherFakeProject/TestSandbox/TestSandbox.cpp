////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

import std;
import sequoia.test_framework;

namespace myProject::testing{};

int main(int argc, char** argv)
{
	auto code{sequoia::testing::return_code::incomplete_run};

	try
	{
		using namespace myProject::testing;
		using namespace std::literals::chrono_literals;

		sequoia::testing::test_runner runner{argc, argv, "Oliver J. Rosten", "\t"};
runner.register_test<maybe_false_negative_test>();
        runner.register_test<maybe_test>();runner.register_test<iterator_false_negative_test>();
        runner.register_test<iterator_test>();runner.register_test<widget_false_negative_test>();
        runner.register_test<widget_test>();runner.register_test<probability_false_negative_test>();
        runner.register_test<probability_test>();runner.register_test<angle_false_negative_test>();
        runner.register_test<angle_test>();runner.register_test<human_false_negative_test>();
        runner.register_test<human_test>();runner.register_test<thingummy_false_negative_test>();
        runner.register_test<thingummy_test>();runner.register_test<container_false_negative_test>();
        runner.register_test<container_test>();runner.register_test<couple_false_negative_test>();
        runner.register_test<couple_test>();runner.register_test<things_false_negative_test>();
        runner.register_test<things_test>();runner.register_test<foo_false_negative_test>();
        runner.register_test<foo_test>();runner.register_test<variadic_false_negative_test>();
        runner.register_test<variadic_test>();runner.register_test<multiple_false_negative_test>();
        runner.register_test<multiple_test>();runner.register_test<cloud_false_negative_test>();
        runner.register_test<cloud_test>();runner.register_test<utilities_free_test>();runner.register_test<bazzer_free_test>();runner.register_test<bazagain_free_test>();runner.register_test<doohicky_free_test>();runner.register_test<global_free_test>();runner.register_test<defs_free_test>();runner.register_test<angle_false_positive_free_diagnostics>();
        runner.register_test<angle_false_negative_free_diagnostics>();runner.register_test<container_allocation_test>();runner.register_test<foo_allocation_test>();runner.register_test<container_performance_test>();
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

