////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "MonotonicSequenceAllocationTest.hpp"
#include "MonotonicSequenceTestingUtilities.hpp"

namespace sequoia::unit_testing
{
  [[nodiscard]]
  std::string_view monotonic_sequence_allocation_test::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void monotonic_sequence_allocation_test::run_tests()
  {
    do_allocation_tests(*this);
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void monotonic_sequence_allocation_test::test_allocation()
  {
    using namespace maths;

    using allocator = shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>;
    using sequence = monotonic_sequence<int, std::less<int>, std::vector<int, allocator>>;
  
    sequence s(allocator{});
    check_equivalence(LINE(""), s, std::initializer_list<int>{});
    check_equality(LINE(""), s.get_allocator().allocs(), 0);

    sequence t{{4, 3}, allocator{}};
    check_equivalence(LINE(""), t, std::initializer_list<int>{4, 3});
    check_equality(LINE(""), t.get_allocator().allocs(), 1);

    auto getter{
      [](const sequence& s){
        return s.get_allocator();
      }
    };

    auto mutator{
      [](sequence& seq){
        const auto val{seq.empty() ? 0 : seq.back() - 1};
        seq.push_back(val);
      }
    };
    check_semantics(LINE(""), s, t, mutator, allocation_info<sequence, allocator>{getter, {0_c, {1_c, 1_mu}, {1_asp, 1_as}}});
  }

}
