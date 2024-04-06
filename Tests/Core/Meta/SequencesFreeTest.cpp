////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "SequencesFreeTest.hpp"
#include "sequoia/Core/Meta/Sequences.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path sequences_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void sequences_free_test::run_tests()
  {
      test_filtered_sequence();
  }


  void sequences_free_test::test_filtered_sequence()
  {
      check(report_line("One element which survives"), []() {
          static_assert(std::is_same_v<make_filtered_sequence<void, std::type_identity, int>, std::index_sequence<0>>);

          return true;
          }()
              );

      check(report_line("One element which is filtered"), []() {
          static_assert(std::is_same_v<make_filtered_sequence<int, std::type_identity, int>, std::index_sequence<>>);

          return true;
          }()
              );

      check(report_line("Two elements, both of which survive"), []() {
          static_assert(std::is_same_v<make_filtered_sequence<void, std::type_identity, int, double>, std::index_sequence<0, 1>>);

          return true;
          }()
              );

      check(report_line("Two elements, the zeroth of which survives"), []() {
          static_assert(std::is_same_v<make_filtered_sequence<double, std::type_identity, int, double>, std::index_sequence<0>>);

          return true;
          }()
              );

      check(report_line("Two elements, the first of which survives"), []() {
          static_assert(std::is_same_v<make_filtered_sequence<int, std::type_identity, int, double>, std::index_sequence<1>>);

          return true;
          }()
              );

      check(report_line("Two elements, both of which are filtered"), []() {
          static_assert(std::is_same_v<make_filtered_sequence<int, std::type_identity, int, int>, std::index_sequence<>>);

          return true;
          }()
              );
  }
}
