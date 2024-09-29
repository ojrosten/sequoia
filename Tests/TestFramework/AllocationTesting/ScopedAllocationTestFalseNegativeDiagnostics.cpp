////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ScopedAllocationTestFalseNegativeDiagnostics.hpp"
#include "ScopedAllocationTestDiagnosticsUtilities.hpp"
#include "AllocationTestDiagnosticsUtilities.hpp"

namespace sequoia::testing
{
  namespace
  {
    template<alloc InnerAllocator>
    using perfectly_scoped_beast
      = typename scoped_beast_builder<perfectly_normal_beast, std::basic_string<char, std::char_traits<char>, InnerAllocator>>::beast;
  }

  [[nodiscard]]
  std::filesystem::path scoped_allocation_false_negative_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void scoped_allocation_false_negative_diagnostics::run_tests()
  {
    do_allocation_tests(*this);
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void scoped_allocation_false_negative_diagnostics::test_allocation()
  {
    test_perfectly_scoped<PropagateCopy, PropagateMove, PropagateSwap>();
    test_perfectly_branched<PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void scoped_allocation_false_negative_diagnostics::test_perfectly_scoped()
  {
    using beast = perfectly_scoped_beast<shared_counting_allocator<char, PropagateCopy, PropagateMove, PropagateSwap>>;

    auto mutator{
      [](beast& b) {
        b.x.push_back("baz");
      }
    };

    auto allocGetter{
      [](const beast& b) {
        return b.x.get_allocator();
      }
    };

    check_semantics("",
                    beast{},
                    beast{ {"something too long for small string optimization"},
                           {"something else too long for small string optimization"}
                    },
                    mutator,
                    allocation_info{
                      allocGetter,
                      {0_c, {1_c,1_mu}, {1_anp,1_awp}},
                      { {0_c, {2_c,0_mu}, {2_anp,2_awp}, {0_containers, 2_containers, 3_postmutation}} }
                    }
    );

    auto[s,t]{check_semantics("",
                    [](){ return beast{}; },
                    [](){ return beast{ {"something too long for small string optimization"},
                                         {"something else too long for small string optimization"}};
                    },
                    mutator,
                    allocation_info{
                      allocGetter,
                      {0_c, {1_c,1_mu}, {1_anp,1_awp}},
                      { {0_c, {2_c,0_mu}, {2_anp,2_awp}, {0_containers, 2_containers, 3_postmutation}} }
                    }
    )};

    check(equality, "check_semantics return value (x)", s, beast{});
    check(equality, "check_semantics return value (y)",
                   t,
                   beast{{"something too long for small string optimization"},
                                         {"something else too long for small string optimization"}});
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void scoped_allocation_false_negative_diagnostics::test_perfectly_branched()
  {
    using inner_allocator = shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>;
    using inner_beast = perfectly_normal_beast<int, inner_allocator>;
    using inner_type = std::tuple<inner_beast, inner_beast>;

    using outer_allocator = shared_counting_allocator<inner_type, PropagateCopy, PropagateMove, PropagateSwap>;
    using beast = perfectly_normal_beast<inner_type, std::scoped_allocator_adaptor<outer_allocator, inner_allocator>>;

    auto getter{[](const beast& b) { return b.x.get_allocator(); }};

    auto mutator{
      [](beast& b) {
        b.x.reserve(10);
        b.x.push_back({{},{}});
      }
    };

    check_semantics("",
      beast{},
      beast{{{1}, {2}}},
      mutator,
      allocation_info{
        getter,
        {0_c, {1_c,1_mu}, {1_anp,1_awp}},
        { {0_c, {2_c,0_mu}, {2_anp,2_awp}, {0_containers, 2_containers, 4_postmutation}} }
      }
    );
  }
}
