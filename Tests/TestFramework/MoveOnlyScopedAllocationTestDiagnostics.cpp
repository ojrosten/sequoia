////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "MoveOnlyScopedAllocationTestDiagnostics.hpp"
#include "MoveOnlyScopedAllocationTestDiagnosticUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view move_only_scoped_allocation_false_negative_diagnostics::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void move_only_scoped_allocation_false_negative_diagnostics::run_tests()
  {
    do_allocation_tests(*this);
  }

  template<bool PropagateMove, bool PropagateSwap>
  void move_only_scoped_allocation_false_negative_diagnostics::test_allocation()
  {
    test_regular_semantics<PropagateMove, PropagateSwap>();
  }

  template<bool PropagateMove, bool PropagateSwap>
  void move_only_scoped_allocation_false_negative_diagnostics::test_regular_semantics()
  {
    using beast
      = move_only_scoped_beast<shared_counting_allocator<char, true, PropagateMove, PropagateSwap>>;

    auto mutator{
      [](beast& b) {
        b.x.push_back("something too long for small string optimization");
      }
    };

    using allocator = typename beast::allocator_type;
    using info = move_only_allocation_info<beast, allocator>;

    auto allocGetter{
      [](const beast& b) {
        return b.x.get_allocator();
      }
    };

    check_semantics(LINE(""),
                    beast{},
                    beast{{"something too long for small string optimization"}},
                    beast{},
                    beast{{"something too long for small string optimization"}},
                    mutator, info{allocGetter, {{1_anp, 1_mu, 1_pm}, {1_anp, 1_mu, 1_pm}}}
    );
  }


  [[nodiscard]]
  std::string_view move_only_scoped_allocation_false_positive_diagnostics::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void move_only_scoped_allocation_false_positive_diagnostics::run_tests()
  {
    do_allocation_tests(*this);
  }

  template<bool PropagateMove, bool PropagateSwap>
  void move_only_scoped_allocation_false_positive_diagnostics::test_allocation()
  {
    test_regular_semantics<PropagateMove, PropagateSwap>();
  }

  template<bool PropagateMove, bool PropagateSwap>
  void move_only_scoped_allocation_false_positive_diagnostics::test_regular_semantics()
  {
    using beast
      = move_only_scoped_beast<shared_counting_allocator<char, true, PropagateMove, PropagateSwap>>;

    auto mutator{
      [](beast& b) {
        b.x.push_back("something too long for small string optimization");
      }
    };

    using allocator = typename beast::allocator_type;
    using info = move_only_allocation_info<beast, allocator>;

    auto allocGetter{
      [](const beast& b) {
        return b.x.get_allocator();
      }
    };

    if constexpr(!PropagateMove)
    {
      check_semantics(LINE("Incorrect assigment outer allocs"),
                      beast{},
                      beast{{"something too long for small string optimization"}},
                      beast{},
                      beast{{"something too long for small string optimization"}},
                      mutator, info{allocGetter, {{0_anp, 1_mu, 1_pm}, {1_anp, 1_mu, 1_pm}}}
      );
    }

    check_semantics(LINE("Incorrect mutation outer allocs"),
                    beast{},
                    beast{{"something too long for small string optimization"}},
                    beast{},
                    beast{{"something too long for small string optimization"}},
                    mutator, info{allocGetter, {{1_anp, 0_mu, 1_pm}, {1_anp, 1_mu, 1_pm}}}
    );

    check_semantics(LINE("Incorrect para move outer allocs"),
                    beast{},
                    beast{{"something too long for small string optimization"}},
                    beast{},
                    beast{{"something too long for small string optimization"}},
                    mutator, info{allocGetter, {{1_anp, 1_mu, 0_pm}, {1_anp, 1_mu, 1_pm}}}
    );

    if constexpr(!PropagateMove)
    {
      check_semantics(LINE("Incorrect assigment inner allocs"),
                      beast{},
                      beast{{"something too long for small string optimization"}},
                      beast{},
                      beast{{"something too long for small string optimization"}},
                      mutator, info{allocGetter, {{1_anp, 1_mu, 1_pm}, {0_anp, 1_mu, 1_pm}}}
      );
    }

    check_semantics(LINE("Incorrect mutation inner allocs"),
                    beast{},
                    beast{{"something too long for small string optimization"}},
                    beast{},
                    beast{{"something too long for small string optimization"}},
                    mutator, info{allocGetter, {{1_anp, 1_mu, 1_pm}, {1_anp, 0_mu, 1_pm}}}
    );

    check_semantics(LINE("Incorrect para move inner allocs"),
                    beast{},
                    beast{{"something too long for small string optimization"}},
                    beast{},
                    beast{{"something too long for small string optimization"}},
                    mutator, info{allocGetter, {{1_anp, 1_mu, 1_pm}, {1_anp, 1_mu, 0_pm}}}
    );
  }
}
