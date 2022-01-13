////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "MoveOnlyScopedAllocationTestDiagnostics.hpp"
#include "MoveOnlyTestDiagnosticsUtilities.hpp"
#include "ScopedAllocationTestDiagnosticsUtilities.hpp"

namespace sequoia::testing
{
  template<alloc InnerAllocator>
  using move_only_scoped_beast
    = typename scoped_beast_builder<move_only_beast, std::basic_string<char, std::char_traits<char>, InnerAllocator>>::beast;

  [[nodiscard]]
  std::string_view move_only_scoped_allocation_false_negative_diagnostics::source_file() const noexcept
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
    using beast = move_only_scoped_beast<shared_counting_allocator<char, true, PropagateMove, PropagateSwap>>;

    auto mutator{
      [](beast& b) {
        b.x.push_back("something too long for small string optimization");
      }
    };

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
                    mutator,
                    allocation_info{
                        allocGetter,
                        {0_pm, {1_pm, 1_mu}, {1_manp}},
                        {{0_pm, {1_pm, 1_mu}, {1_manp}, {0_containers, 1_containers, 2_postmutation}}}
                    }
    );

    auto[x,y]{check_semantics(LINE(""),
                              [](){ return beast{}; },
                              [](){ return beast{{"something too long for small string optimization"}}; },
                              mutator,
                              allocation_info{
                                  allocGetter,
                                  {0_pm, {1_pm, 1_mu}, {1_manp}},
                                  {{0_pm, {1_pm, 1_mu}, {1_manp}, {0_containers, 1_containers, 2_postmutation}}
                              }
                    }
              )
    };

    check(equality, LINE("check_semantics return value (x)"), x, beast{});
    check(equality, LINE("check_semantics return value (y)"), y, beast{{"something too long for small string optimization"}});
  }


  [[nodiscard]]
  std::string_view move_only_scoped_allocation_false_positive_diagnostics::source_file() const noexcept
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
                      mutator,
                      allocation_info{allocGetter,
                                     {0_pm, {1_pm, 1_mu}, {0_manp}},
                                     {{0_pm, {1_pm, 1_mu}, {1_manp}, {0_containers, 1_containers, 2_postmutation}}}
                      }
      );
    }

    check_semantics(LINE("Incorrect mutation outer allocs"),
                    beast{},
                    beast{{"something too long for small string optimization"}},
                    beast{},
                    beast{{"something too long for small string optimization"}},
                    mutator,
                    allocation_info{allocGetter,
                                    {0_pm, {1_pm, 0_mu}, {1_manp}},
                                    {{0_pm, {1_pm, 1_mu}, {1_manp}, {0_containers, 1_containers, 2_postmutation}}}
                    }
    );

    check_semantics(LINE("Incorrect para move outer allocs"),
                    beast{},
                    beast{{"something too long for small string optimization"}},
                    beast{},
                    beast{{"something too long for small string optimization"}},
                    mutator,
                    allocation_info{allocGetter,
                                    {0_pm, {0_pm, 1_mu}, {1_manp}},
                                    {{0_pm, {1_pm, 1_mu}, {1_manp}, {0_containers, 1_containers, 2_postmutation}}}
                    }
    );

    if constexpr(!PropagateMove)
    {
      check_semantics(LINE("Incorrect assigment inner allocs"),
                      beast{},
                      beast{{"something too long for small string optimization"}},
                      beast{},
                      beast{{"something too long for small string optimization"}},
                      mutator,
                      allocation_info{allocGetter,
                                      {0_pm, {1_pm, 1_mu}, {1_manp}},
                                      {{0_pm, {1_pm, 1_mu}, {0_manp}, {0_containers, 1_containers, 2_postmutation}}}
                      }
      );
    }

    check_semantics(LINE("Incorrect mutation inner allocs"),
                    beast{},
                    beast{{"something too long for small string optimization"}},
                    beast{},
                    beast{{"something too long for small string optimization"}},
                    mutator, allocation_info{allocGetter,
                                             {0_pm, {1_pm, 1_mu}, {1_manp}},
                                             {{0_pm, {1_pm, 0_mu}, {1_manp}, {0_containers, 1_containers, 2_postmutation}}}
                    }
    );

    check_semantics(LINE("Incorrect para move inner allocs"),
                    beast{},
                    beast{{"something too long for small string optimization"}},
                    beast{},
                    beast{{"something too long for small string optimization"}},
                    mutator,
                    allocation_info{allocGetter,
                                    {0_pm, {1_pm, 1_mu}, {1_manp}}, 
                                    {{0_pm, {0_pm, 1_mu}, {1_manp}, {0_containers, 1_containers, 2_postmutation}}}
                    }
    );
  }
}
