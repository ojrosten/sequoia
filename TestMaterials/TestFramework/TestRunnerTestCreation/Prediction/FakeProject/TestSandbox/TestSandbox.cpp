////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ShareIncludes.hpp"

#include <iostream>

int main(int argc, char** argv)
{
  using namespace sequoia;
  using namespace testing;
  namespace fs = std::filesystem;

  try
  {    
    test_runner runner{argc,
                       argv,
                       "Oliver J. Rosten",
                       working_path().append("TestChamberMain.cpp"),
                       sibling_path("TestShared").append("SharedIncludes.hpp"),
                       sibling_path("Tests"),
                       sibling_path("Source")
    };

    runner.add_test_family(
      "Maybe",
      maybe_false_positive_test("False Positive Test"),
      maybe_test("Unit Test")
    );

    runner.add_test_family(
      "Iterator",
      foo_allocation_test("Allocation Test"),
      foo_test("Unit Test"),
      foo_false_positive_test("False Positive Test"),
      iterator_false_positive_test("False Positive Test"),
      iterator_test("Unit Test")
    );

    runner.add_test_family(
      "partners",
      couple_false_positive_test("False Positive Test"),
      couple_test("Unit Test")
    );

    runner.add_test_family(
      "Utilities",
      utilities_free_test("Free Test")
    );

    runner.add_test_family(
      "Bazzer",
      bazzer_test("Free Test")
    );

    runner.add_test_family(
      "Container",
      container_performance_test("Performance Test"),
      container_allocation_test("Allocation Test"),
      container_false_positive_test("False Positive Test"),
      container_test("Unit Test")
    );

    runner.add_test_family(
      "Variadic",
      variadic_false_positive_test("False Positive Test"),
      variadic_test("Unit Test")
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

