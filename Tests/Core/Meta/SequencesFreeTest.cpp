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
    test_concat_sequences();
    test_filtered_sequence();
    test_rotate_sequence();
    test_reverse_sequence();
    test_shift_sequence();
    test_sequence_partial_sum();
  }

  void sequences_free_test::test_concat_sequences()
  {
    check("Concat two empty sequences", [](){
        static_assert(std::is_same_v<concat_sequences_t<std::index_sequence<>, std::index_sequence<>>, std::index_sequence<>>);
        return true;
      }()
    );

    check("Concat empty left-hand sequence", [](){
        static_assert(std::is_same_v<concat_sequences_t<std::index_sequence<42>, std::index_sequence<>>, std::index_sequence<42>>);
        return true;
      }()
    );

    check("Concat empty right-hand sequence", [](){
        static_assert(std::is_same_v<concat_sequences_t<std::index_sequence<>, std::index_sequence<42>>, std::index_sequence<42>>);
        return true;
      }()
    );

    check("Concat {0}, {0}", [](){
        static_assert(std::is_same_v<concat_sequences_t<std::index_sequence<0>, std::index_sequence<0>>, std::index_sequence<0, 0>>);
        return true;
      }()
    );

    check("Concat {0, 2}, {1}", [](){
        static_assert(std::is_same_v<concat_sequences_t<std::index_sequence<0, 2>, std::index_sequence<1>>, std::index_sequence<0, 2, 1>>);
        return true;
      }()
    );

    check("Concat {0, 1}, {2, 3}", [](){
        static_assert(std::is_same_v<concat_sequences_t<std::index_sequence<0, 1>, std::index_sequence<2, 3>>, std::index_sequence<0, 1, 2, 3>>);
        return true;
      }()
    );
  }

  void sequences_free_test::test_filtered_sequence()
  {
    check("One element which survives", []() {
        static_assert(std::is_same_v<make_filtered_sequence<void, std::type_identity, int>, std::index_sequence<0>>);
        return true;
      }()
    );

    check("One element which is filtered", []() {
        static_assert(std::is_same_v<make_filtered_sequence<int, std::type_identity, int>, std::index_sequence<>>);
        return true;
      }()
     );

    check("Two elements, both of which survive", []() {
        static_assert(std::is_same_v<make_filtered_sequence<void, std::type_identity, int, double>, std::index_sequence<0, 1>>);
        return true;
      }()
    );

    check("Two elements, the zeroth of which survives", []() {
        static_assert(std::is_same_v<make_filtered_sequence<double, std::type_identity, int, double>, std::index_sequence<0>>);
        return true;
      }()
    );

    check("Two elements, the first of which survives", []() {
        static_assert(std::is_same_v<make_filtered_sequence<int, std::type_identity, int, double>, std::index_sequence<1>>);
        return true;
      }()
    );

    check("Two elements, both of which are filtered", []() {
        static_assert(std::is_same_v<make_filtered_sequence<int, std::type_identity, int, int>, std::index_sequence<>>);
        return true;
      }()
    );
  }

  void sequences_free_test::test_rotate_sequence()
  {
    check("Rotate empty sequence", [](){
        static_assert(std::is_same_v<rotate_sequence_t<std::index_sequence<>>, std::index_sequence<>>);
        return true;
      }()
    );

    check("Rotate one element sequence", [](){
        static_assert(std::is_same_v<rotate_sequence_t<std::index_sequence<42>>, std::index_sequence<42>>);
        return true;
      }()
    );

    check("Rotate two element sequence", [](){
        static_assert(std::is_same_v<rotate_sequence_t<std::index_sequence<0, 1>>, std::index_sequence<1, 0>>);
        return true;
      }()
    );

    check("Rotate three element sequence", [](){
        static_assert(std::is_same_v<rotate_sequence_t<std::index_sequence<0, 1, 2>>, std::index_sequence<1, 2, 0>>);
        return true;
      }()
    );
  }

  void sequences_free_test::test_reverse_sequence()
  {
    check("Reverse empty sequence", [](){
        static_assert(std::is_same_v<reverse_sequence_t<std::index_sequence<>>, std::index_sequence<>>);
        return true;
      }()
    );

    check("Reverse one element sequence", [](){
        static_assert(std::is_same_v<reverse_sequence_t<std::index_sequence<42>>, std::index_sequence<42>>);
        return true;
      }()
    );

    check("Reverse two element sequence", [](){
        static_assert(std::is_same_v<reverse_sequence_t<std::index_sequence<42, 7>>, std::index_sequence<7, 42>>);
        return true;
      }()
    );

    check("Reverse three element sequence", [](){
        static_assert(std::is_same_v<reverse_sequence_t<std::index_sequence<0, 1, 2>>, std::index_sequence<2, 1, 0>>);
        return true;
      }()
    );
  }

  void sequences_free_test::test_shift_sequence()
  {
    check("",
          [](){
            static_assert(std::is_same_v<shift_sequence_t<std::index_sequence<>, 42>, std::index_sequence<>>);
            return true;
          }()
    );

    check("",
          [](){
            static_assert(std::is_same_v<shift_sequence_t<std::index_sequence<0>, 42>, std::index_sequence<42>>);
            return true;
          }()
    );

    check("",
          [](){
            static_assert(std::is_same_v<shift_sequence_t<std::index_sequence<0, 7>, 42>, std::index_sequence<42, 49>>);
            return true;
          }()
    );
  }

  void sequences_free_test::test_sequence_partial_sum()
  {
    check("Partial sum of empty sequence", [](){
        static_assert(std::is_same_v<sequence_partial_sum_t<std::index_sequence<>>, std::index_sequence<>>);
        return true;
      }()
    );

    check("Partial sum of one element sequence", [](){
        static_assert(std::is_same_v<sequence_partial_sum_t<std::index_sequence<42>>, std::index_sequence<42>>);
        return true;
      }()
    );

    check("Partial sum of two element sequence", [](){
        static_assert(std::is_same_v<sequence_partial_sum_t<std::index_sequence<42, 7>>, std::index_sequence<42, 49>>);
        return true;
      }()
    );

    check("Partial sum of three element sequence", [](){
        static_assert(std::is_same_v<sequence_partial_sum_t<std::index_sequence<42, 7, 12>>, std::index_sequence<42, 49, 61>>);
        return true;
      }()
    );
  }
}
