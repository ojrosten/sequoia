////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "SequencesFreeTest.hpp"
#include "sequoia/Core/Meta/Sequences.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path sequences_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void sequences_free_test::run_tests()
  {
    test_concat_sequences();
    test_filtered_sequence();
    test_rotate_sequence();
    test_reverse_sequence();
    test_shift_sequence();
    test_sequence_partial_sum();
  }

  void sequences_free_test::test_concat_sequences()
  {
    STATIC_CHECK(std::is_same_v<concat_sequences_t<std::index_sequence<>, std::index_sequence<>>,
                                std::index_sequence<>>);
    STATIC_CHECK(std::is_same_v<concat_sequences_t<std::index_sequence<42>, std::index_sequence<>>,
                                std::index_sequence<42>>);
    STATIC_CHECK(std::is_same_v<concat_sequences_t<std::index_sequence<>, std::index_sequence<42>>,
                                std::index_sequence<42>>);
    STATIC_CHECK(std::is_same_v<concat_sequences_t<std::index_sequence<0>, std::index_sequence<0>>,
                                std::index_sequence<0, 0>>);
    STATIC_CHECK(std::is_same_v<concat_sequences_t<std::index_sequence<0, 2>, std::index_sequence<1>>,
                                std::index_sequence<0, 2, 1>>);
    STATIC_CHECK(std::is_same_v<concat_sequences_t<std::index_sequence<0, 1>, std::index_sequence<2, 3>>,
                                std::index_sequence<0, 1, 2, 3>>);
  }

  void sequences_free_test::test_filtered_sequence()
  {
    STATIC_CHECK(std::is_same_v<make_filtered_sequence<void, std::type_identity, int>,
                                std::index_sequence<0>>);
    STATIC_CHECK(std::is_same_v<make_filtered_sequence<int, std::type_identity, int>,
                                std::index_sequence<>>);
    STATIC_CHECK(std::is_same_v<make_filtered_sequence<void, std::type_identity, int, double>,
                                std::index_sequence<0, 1>>);
    STATIC_CHECK(std::is_same_v<make_filtered_sequence<double, std::type_identity, int, double>,
                                std::index_sequence<0>>);
    STATIC_CHECK(std::is_same_v<make_filtered_sequence<int, std::type_identity, int, double>,
                                std::index_sequence<1>>);
    STATIC_CHECK(std::is_same_v<make_filtered_sequence<int, std::type_identity, int, int>,
                                std::index_sequence<>>);
  }

  void sequences_free_test::test_rotate_sequence()
  {
    STATIC_CHECK(std::is_same_v<rotate_sequence_t<std::index_sequence<>>, std::index_sequence<>>);
    STATIC_CHECK(std::is_same_v<rotate_sequence_t<std::index_sequence<42>>, std::index_sequence<42>>);
    STATIC_CHECK(std::is_same_v<rotate_sequence_t<std::index_sequence<0, 1>>, std::index_sequence<1, 0>>);
    STATIC_CHECK(std::is_same_v<rotate_sequence_t<std::index_sequence<0, 1, 2>>, std::index_sequence<1, 2, 0>>);
  }

  void sequences_free_test::test_reverse_sequence()
  {
    STATIC_CHECK(std::is_same_v<reverse_sequence_t<std::index_sequence<>>, std::index_sequence<>>);
    STATIC_CHECK(std::is_same_v<reverse_sequence_t<std::index_sequence<42>>, std::index_sequence<42>>);
    STATIC_CHECK(std::is_same_v<reverse_sequence_t<std::index_sequence<42, 7>>, std::index_sequence<7, 42>>);
    STATIC_CHECK(std::is_same_v<reverse_sequence_t<std::index_sequence<0, 1, 2>>, std::index_sequence<2, 1, 0>>);
  }

  void sequences_free_test::test_shift_sequence()
  {
    STATIC_CHECK(std::is_same_v<shift_sequence_t<std::index_sequence<>, 42>, std::index_sequence<>>);
    STATIC_CHECK(std::is_same_v<shift_sequence_t<std::index_sequence<0>, 42>, std::index_sequence<42>>);
    STATIC_CHECK(std::is_same_v<shift_sequence_t<std::index_sequence<0, 7>, 42>, std::index_sequence<42, 49>>);
  }

  void sequences_free_test::test_sequence_partial_sum()
  {
    STATIC_CHECK(std::is_same_v<sequence_partial_sum_t<std::index_sequence<>>,
                                std::index_sequence<>>);
    STATIC_CHECK(std::is_same_v<sequence_partial_sum_t<std::index_sequence<42>>,
                                std::index_sequence<42>>);
    STATIC_CHECK(std::is_same_v<sequence_partial_sum_t<std::index_sequence<42, 7>>,
                                std::index_sequence<42, 49>>);
    STATIC_CHECK(std::is_same_v<sequence_partial_sum_t<std::index_sequence<42, 7, 12>>,
                                std::index_sequence<42, 49, 61>>);
  }
}
