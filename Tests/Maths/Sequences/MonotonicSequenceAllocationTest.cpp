////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "MonotonicSequenceAllocationTest.hpp"
#include "MonotonicSequenceTestingUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path monotonic_sequence_allocation_test::source_file() const
  {
    return std::source_location::current().file_name();
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
    using sequence = monotonic_sequence<int, std::ranges::less, std::vector<int, allocator>>;

    auto getter{
      [](const sequence& s){ return s.get_allocator(); }
    };

    auto mutator{
      [](sequence& seq){
        const auto val{seq.empty() ? 0 : seq.back() - 1};
        seq.push_back(val);
      }
    };

    auto[s,t]{check_semantics(report(""),
                              [](){ return sequence(allocator{}); },
                              [](){ return sequence{{4, 3}, allocator{}}; },
                              mutator,
                              allocation_info{getter, {0_c, {1_c, 1_mu}, {1_anp, 1_awp}}})};

    check(equivalence, report(""), s, std::initializer_list<int>{});
    check(equivalence, report(""), t, std::initializer_list<int>{4, 3});
  }

}
