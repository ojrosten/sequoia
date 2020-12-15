////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ScopedAllocationTestDiagnostics.hpp"
#include "ScopedAllocationTestDiagnosticsUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view scoped_allocation_false_negative_diagnostics::source_file() const noexcept
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

    auto allocGetter{
      [](const beast& b) {
        return b.x.get_allocator();
      }
    };

    check_semantics(LINE(""),
                    beast{},
                    beast{ {"something too long for small string optimization"},
                           {"something else too long for small string optimization"}
                    },
                    mutator,
                    allocation_info{
                      allocGetter,
                        { {0_c, {1_c,1_mu}, {1_awp,1_anp}},
                           {0_c, {2_c,0_mu}, {2_awp,2_awp}, 0_containers, 2_containers}
                        }
                    }
    );
  }


  [[nodiscard]]
  std::string_view scoped_allocation_false_positive_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void scoped_allocation_false_positive_diagnostics::run_tests()
  {
    do_allocation_tests(*this);
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void scoped_allocation_false_positive_diagnostics::test_allocation()
  {
    test_regular_semantics<PropagateCopy, PropagateMove, PropagateSwap>();
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void scoped_allocation_false_positive_diagnostics::test_regular_semantics()
  {
    using beast
      = perfectly_scoped_beast<shared_counting_allocator<char, PropagateCopy, PropagateMove, PropagateSwap>>;

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

    check_semantics(LINE("Incorrect copy x outer allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {{1_c, {1_c,1_mu}, {1_awp,1_anp}}, {0_c, {2_c,0_mu}, {2_awp,2_awp}}}
      }
    );

    check_semantics(LINE("Incorrect copy y outer allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {{0_c, {0_c,1_mu}, {1_awp,1_anp}}, {0_c, {2_c,0_mu}, {2_awp,2_awp}}}
      }
    );

    check_semantics(LINE("Incorrect mutation outer allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {{0_c, {1_c,2_mu}, {1_awp,1_anp}}, {0_c, {2_c,0_mu}, {2_awp,2_awp}}}
      }
    );

    check_semantics(LINE("Incorrect assignment outer allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {{0_c, {1_c,1_mu}, {2_awp,3_anp}}, {0_c, {2_c,0_mu}, {2_awp,2_awp}}}
      }
    );

    check_semantics(LINE("Incorrect para copy x outer allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {{0_c, {1_c,1_mu, 2_pc}, {1_awp,1_anp}}, {0_c, {2_c,0_mu}, {2_awp,2_awp}}}
      }
    );

    check_semantics(LINE("Incorrect para move x outer allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {{0_c, {1_c,1_mu, 1_pc, 0_pm}, {1_awp,1_anp}}, {0_c, {2_c,0_mu}, {2_awp,2_awp}}}
      }
    );

    check_semantics(LINE("Incorrect copy x inner allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {{0_c, {1_c,1_mu}, {1_awp,1_anp}}, {4_c, {2_c,0_mu}, {2_awp,2_awp}}}
      }
    );

    check_semantics(LINE("Incorrect copy y inner allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {{0_c, {1_c,1_mu}, {1_awp,1_anp}}, {0_c, {0_c,0_mu}, {2_awp,2_awp}}}
      }
    );

    check_semantics(LINE("Incorrect mutation inner allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {{0_c, {1_c,1_mu}, {1_awp,1_anp}}, {0_c, {2_c,3_mu}, {2_awp,2_awp}}}
      }
    );

    check_semantics(LINE("Incorrect assignment inner allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {{0_c, {1_c,1_mu}, {1_awp,1_anp}}, {0_c, {2_c,0_mu}, {1_awp,0_awp}}}
      }
    );

    check_semantics(LINE("Incorrect para copy x inner allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {{0_c, {1_c,1_mu}, {1_awp,1_anp}}, {0_c, {2_c,0_mu,4_pc}, {2_awp,2_awp}}}
      }
    );

    check_semantics(LINE("Incorrect para move x inener allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {{0_c, {1_c,1_mu}, {1_awp,1_anp}}, {0_c, {2_c,0_mu,2_pc,0_pm}, {2_awp,2_awp}}}
      }
    );
  }
}
