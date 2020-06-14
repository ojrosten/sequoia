////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ScopedAllocationTestDiagnostics.hpp"
#include "ScopedAllocationTestDiagnosticUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view scoped_allocation_false_negative_diagnostics::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void scoped_allocation_false_negative_diagnostics::run_tests()
  {
    do_allocation_tests(*this);
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void scoped_allocation_false_negative_diagnostics::test_allocation()
  {
    test_regular_semantics<PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void scoped_allocation_false_negative_diagnostics::test_regular_semantics()
  {
    using beast
      = perfectly_scoped_beast<shared_counting_allocator<char, PropagateCopy, PropagateMove, PropagateSwap>>;

    auto mutator{
        [](beast& b) {
          b.x.push_back("baz");
        }
      };
    
    check_semantics(LINE(""), beast{}, beast{{"foo"}, {"bar"}}, mutator);

    using allocator = typename beast::allocator_type;
    using info = allocation_info<beast, allocator>;

    auto allocGetter{
      [](const beast& b) {
        return b.x.get_allocator();
      }
    };

    check_semantics(LINE(""), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, info{
        allocGetter, {{0_c, {1_c,1_mu}, {1_awp,1_anp}}, {0_c, {2_c,0_mu}, {2_awp,2_awp}}}
      });
  }
}
