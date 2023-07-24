////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ScopedAllocationTestFalseNegativeDiagnosticsThreeLevel.hpp"
#include "ScopedAllocationTestDiagnosticsUtilities.hpp"
#include "AllocationTestDiagnosticsUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path scoped_allocation_false_negative_diagnostics_three_level::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void scoped_allocation_false_negative_diagnostics_three_level::run_tests()
  {
    do_allocation_tests(*this);
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void scoped_allocation_false_negative_diagnostics_three_level::test_allocation()
  {
    test_three_level_scoped<PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void scoped_allocation_false_negative_diagnostics_three_level::test_three_level_scoped()
  {
    using innermost_allocator = shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>;
    using innermost_type = perfectly_normal_beast<int, innermost_allocator>;

    using middle_allocator = shared_counting_allocator<innermost_type, PropagateCopy, PropagateMove, PropagateSwap>;
    using middle_type = perfectly_normal_beast<innermost_type, std::scoped_allocator_adaptor<middle_allocator, innermost_allocator>>;

    using outer_allocator = shared_counting_allocator<middle_type, PropagateCopy, PropagateMove, PropagateSwap>;
    using beast = perfectly_normal_beast<middle_type, std::scoped_allocator_adaptor<outer_allocator, middle_allocator, innermost_allocator>>;

    auto getter{[](const beast& b) { return b.x.get_allocator(); }};

    check_semantics(report_line(""),
      beast{},
      beast{{{1}}},
      [](beast& b) { b.x.push_back({{2}}); },
      allocation_info{
        getter,
        {0_c, {1_c,1_mu}, {1_anp,1_awp}},
        { {0_c, {1_c,1_mu}, {1_anp,1_awp}, {0_containers, 1_containers, 2_postmutation}},
          {0_c, {1_c,1_mu}, {1_anp,1_awp}, {0_containers, 1_containers, 1_postmutation}}
        }
      }
    );
  }
}
