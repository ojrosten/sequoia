////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

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
#include "Utilities/ContainerAllocationTest.hpp"
#include "Utilities/ContainerPerformanceTest.hpp"
#include "Utilities/UniqueThingTest.hpp"
#include "Utilities/UniqueThingTestingDiagnostics.hpp"
#include "Utilities/UsefulThingsFreeTest.hpp"
#include "Utilities/UtilitiesFreeTest.hpp"
#include "sequoia/TestFramework/TestRunner.hpp"
#include <iostream>

int main(int argc, char** argv)
{
  using namespace sequoia;
  using namespace testing;

  try
  {
    const auto root{project_root(argc, argv)};
    test_runner runner{argc,
                       argv,
                       "Oliver Jacob Rosten",
                       root/"TestAll"/"TestMain.cpp",
                       root/"TestAll"/"TestMain.cpp",
                       project_paths{root}
    };
 
    runner.add_test_family(
      "Utilities",
      utilities_free_test("Free Test")
    );

    runner.add_test_family(
      "UsefulThings",
      useful_things_free_test("Free Test")
    );

    runner.add_test_family(
      "Bar",
      bar_free_test("Free Test")
    );

    runner.add_test_family(
      "Maybe",
      maybe_false_positive_test("False Positive Test"),
      maybe_test("Unit Test")
    );

    runner.add_test_family(
      "Oldschool",
      oldschool_false_positive_test("False Positive Test"),
      oldschool_test("Unit Test")
    );

    runner.add_test_family(
      "Probability",
      probability_false_positive_test("False Positive Test"),
      probability_test("Unit Test")
    );

    runner.add_test_family(
      "Foo",
      foo_false_positive_test("False Positive Test"),
      foo_test("Unit Test")
    );

    runner.add_test_family(
      "UniqueThing",
      unique_thing_false_positive_test("False Positive Test"),
      unique_thing_test("Unit Test")
    );

    runner.add_test_family(
      "Container",
      container_performance_test("Performance Test"),
      container_allocation_test("Allocation Test")
    );

    runner.add_test_family(
      "House",
      house_allocation_test("Allocation Test")
    );

    runner.execute();
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

