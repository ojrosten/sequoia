////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ScopedAllocationTestDiagnostics.hpp"
#include "ScopedAllocationTestDiagnosticsUtilities.hpp"
#include "AllocationTestDiagnosticsUtilities.hpp"

namespace sequoia::testing
{
  template<class InnerAllocator>
  using perfectly_scoped_beast
    = typename scoped_beast_builder<perfectly_normal_beast, std::basic_string<char, std::char_traits<char>, InnerAllocator>>::beast;

  template<class InnerAllocator>
  using perfectly_mixed_beast
    = typename scoped_beast_builder<perfectly_normal_beast, perfectly_sharing_beast<int, std::shared_ptr<int>, InnerAllocator>>::beast;

  template<class InnerAllocator>
  using weirdly_mixed_beast
    = typename scoped_beast_builder<perfectly_normal_beast, inefficient_para_copy<int, InnerAllocator>>::beast;

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
    test_perfectly_scoped<PropagateCopy, PropagateMove, PropagateSwap>();
    test_perfectly_mixed<PropagateCopy, PropagateMove, PropagateSwap>();
    test_weirdly_mixed<PropagateCopy, PropagateMove, PropagateSwap>();
    test_perfectly_branched<PropagateCopy, PropagateMove, PropagateSwap>();
    test_three_level_scoped<PropagateCopy, PropagateMove, PropagateSwap>();
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

    check_semantics(LINE(""),
                    beast{},
                    beast{ {"something too long for small string optimization"},
                           {"something else too long for small string optimization"}
                    },
                    mutator,
                    allocation_info{
                      allocGetter,
                      {0_c, {1_c,1_mu}, {1_awp,1_anp}},
                      { {0_c, {2_c,0_mu}, {2_awp,2_anp}, {0_containers, 2_containers, 3_postmutation}} }
                    }
    );

    auto[s,t]{check_semantics(LINE(""),
                    [](){ return beast{}; },
                    [](){ return beast{ {"something too long for small string optimization"},
                                         {"something else too long for small string optimization"}};
                    },
                    mutator,
                    allocation_info{
                      allocGetter,
                      {0_c, {1_c,1_mu}, {1_awp,1_anp}},
                      { {0_c, {2_c,0_mu}, {2_awp,2_anp}, {0_containers, 2_containers, 3_postmutation}} }
                    }
    )};
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void scoped_allocation_false_negative_diagnostics::test_perfectly_mixed()
  {
    using inner_allocator = shared_counting_allocator<std::shared_ptr<int>, PropagateCopy, PropagateMove, PropagateSwap>;
    using beast = perfectly_mixed_beast<inner_allocator>;

    auto getter{[](const beast& b) { return b.x.get_allocator(); }};

    auto mutator{
      [](beast& b) {
        b.x.reserve(10);
        b.x.push_back({});
      }
    };

    check_semantics(LINE(""),
      beast{},
      beast{{1}},
      mutator,
      allocation_info{
        getter,
        {0_c, {1_c,1_mu}, {1_awp,1_anp}},
        { {0_c, {1_c,0_mu}, {1_awp,1_anp}, {0_containers, 1_containers, 2_postmutation}} }
      }
    );

    check_semantics(LINE(""),
      beast{},
      beast{{1}, {2, 3}},
      mutator,
      allocation_info{
        getter,
        {0_c, {1_c,1_mu}, {1_awp,1_anp}},
        { {0_c, {2_c,0_mu}, {2_awp,2_anp}, {0_containers, 2_containers, 3_postmutation}} }
      }
    );

    check_semantics(LINE(""),
      beast{{1}},
      beast{},
      mutator,
      allocation_info{
        getter,
        {1_c, {0_c,1_mu}, {0_awp,0_anp}},
        { {1_c, {0_c,0_mu}, {0_awp,0_anp}, {1_containers, 0_containers, 1_postmutation}} }
      }
    );

    check_semantics(LINE(""),
      beast{{2}},
      beast{{1}},
      mutator,
      allocation_info{
        getter,
        {1_c, {1_c,1_mu}, {1_awp,0_anp}},
        { {1_c, {1_c,0_mu}, {1_awp,1_anp,0_clm,0_ma}, {1_containers, 1_containers, 2_postmutation}} }
      }
    );

    check_semantics(LINE(""),
      beast{{2}},
      beast{{1}, {5,6}},
      mutator,
      allocation_info{
        getter,
        {1_c, {1_c,1_mu}, {1_awp,1_anp}},
        { {1_c, {2_c,0_mu}, {2_awp,2_anp}, {1_containers, 2_containers, 3_postmutation}} }
      }
    );

    check_semantics(LINE(""),
      beast{{2}, {7,8}},
      beast{{1}},
      mutator,
      allocation_info{
        getter,
        {1_c, {1_c,1_mu}, {1_awp,0_anp}},
        { {2_c, {1_c,0_mu}, {1_awp,1_anp,0_clm,0_ma}, {2_containers, 1_containers, 2_postmutation}} }
      }
    );

    check_semantics(LINE(""),
      beast{{2}, {7,8}},
      beast{{1}, {5,6}},
      mutator,
      allocation_info{
        getter,
        {1_c, {1_c,1_mu}, {1_awp,0_anp}},
        { {2_c, {2_c,0_mu}, {2_awp,2_anp,0_clm,0_ma}, {2_containers, 2_containers, 3_postmutation}} }
      }
    );
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void scoped_allocation_false_negative_diagnostics::test_weirdly_mixed()
  {
    using inner_allocator = shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>;
    using beast = weirdly_mixed_beast<inner_allocator>;

    auto getter{[](const beast& b) { return b.x.get_allocator(); }};

    auto mutator{
      [](beast& b) {
        b.x.reserve(10);
        b.x.push_back({});
      }
    };

    check_semantics(LINE(""),
      beast{},
      beast{{1}},
      mutator,
      allocation_info{
        getter,
        {0_c, {1_c,1_mu}, {1_awp,1_anp}},
        { {0_c, {2_c,0_mu,2_pc,1_pm}, {2_awp,2_anp,1_clm,0_ma}, {0_containers, 1_containers, 2_postmutation}} }
      }
    );
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

    check_semantics(LINE(""),
      beast{},
      beast{{{1}, {2}}},
      mutator,
      allocation_info{
        getter,
        {0_c, {1_c,1_mu}, {1_awp,1_anp}},
        { {0_c, {2_c,0_mu}, {2_awp,2_anp}, {0_containers, 2_containers, 4_postmutation}} }
      }
    );
  }

  template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  void scoped_allocation_false_negative_diagnostics::test_three_level_scoped()
  {
    using innermost_allocator = shared_counting_allocator<int, PropagateCopy, PropagateMove, PropagateSwap>;
    using innermost_type = perfectly_normal_beast<int, innermost_allocator>;

    using middle_allocator = shared_counting_allocator<innermost_type, PropagateCopy, PropagateMove, PropagateSwap>;
    using middle_type = perfectly_normal_beast<innermost_type, std::scoped_allocator_adaptor<middle_allocator, innermost_allocator>>;

    using outer_allocator = shared_counting_allocator<middle_type, PropagateCopy, PropagateMove, PropagateSwap>;
    using beast = perfectly_normal_beast<middle_type, std::scoped_allocator_adaptor<outer_allocator, middle_allocator, innermost_allocator>>;

    auto getter{[](const beast& b) { return b.x.get_allocator(); }};

    check_semantics(LINE(""),
      beast{},
      beast{{{1}}},
      [](beast& b) { b.x.push_back({{2}}); },
      allocation_info{
        getter,
        {0_c, {1_c,1_mu}, {1_awp,1_anp}},
        { {0_c, {1_c,1_mu}, {1_awp,1_anp}, {0_containers, 1_containers, 2_postmutation}},
          {0_c, {1_c,1_mu}, {1_awp,1_anp}, {0_containers, 1_containers, 1_postmutation}}
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
        allocGetter, {1_c, {1_c,1_mu}, {1_awp,1_anp}}, {{0_c, {2_c,0_mu}, {2_awp,2_anp}, {0_containers, 2_containers, 3_postmutation}}}
      }
    );

    check_semantics(LINE("Incorrect copy y outer allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {0_c, {0_c,1_mu}, {1_awp,1_anp}}, {{0_c, {2_c,0_mu}, {2_awp,2_anp}, {0_containers, 2_containers, 3_postmutation}}}
      }
    );

    check_semantics(LINE("Incorrect mutation outer allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {0_c, {1_c,2_mu}, {1_awp,1_anp}}, {{0_c, {2_c,0_mu}, {2_awp,2_anp}, {0_containers, 2_containers, 3_postmutation}}}
      }
    );

    check_semantics(LINE("Incorrect assignment outer allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {0_c, {1_c,1_mu}, {2_awp,3_anp}}, {{0_c, {2_c,0_mu}, {2_awp,2_anp}, {0_containers, 2_containers, 3_postmutation}}}
      }
    );

    check_semantics(LINE("Incorrect para copy x outer allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {0_c, {1_c,1_mu, 2_pc}, {1_awp,1_anp}}, {{0_c, {2_c,0_mu}, {2_awp,2_anp}, {0_containers, 2_containers, 3_postmutation}}}
      }
    );

    check_semantics(LINE("Incorrect para move x outer allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {0_c, {1_c,1_mu, 1_pc, 0_pm}, {1_awp,1_anp}}, {{0_c, {2_c,0_mu}, {2_awp,2_anp}, {0_containers, 2_containers, 3_postmutation}}}
      }
    );

    check_semantics(LINE("Incorrect copy x inner allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {0_c, {1_c,1_mu}, {1_awp,1_anp}}, {{4_c, {2_c,0_mu}, {2_awp,2_anp}, {0_containers, 2_containers, 3_postmutation}}}
      }
    );

    check_semantics(LINE("Incorrect copy y inner allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {0_c, {1_c,1_mu}, {1_awp,1_anp}}, {{0_c, {0_c,0_mu}, {2_awp,2_anp}, {0_containers, 2_containers, 3_postmutation}}}
      }
    );

    check_semantics(LINE("Incorrect mutation inner allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {0_c, {1_c,1_mu}, {1_awp,1_anp}}, {{0_c, {2_c,3_mu}, {2_awp,2_anp}, {0_containers, 2_containers, 3_postmutation}}}
      }
    );

    check_semantics(LINE("Incorrect assignment inner allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {0_c, {1_c,1_mu}, {1_awp,1_anp}}, {{0_c, {2_c,0_mu}, {1_awp,0_anp}, {0_containers, 2_containers, 3_postmutation}}}
      }
    );

    check_semantics(LINE("Incorrect para copy x inner allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {0_c, {1_c,1_mu}, {1_awp,1_anp}}, {{0_c, {2_c,0_mu,4_pc}, {2_awp,2_anp}, {0_containers, 2_containers, 3_postmutation}}}
      }
    );

    check_semantics(LINE("Incorrect para move x inner allocs"), beast{}, beast{{"something too long for small string optimization"}, {"something else too long for small string optimization"}}, mutator, allocation_info{
        allocGetter, {0_c, {1_c,1_mu}, {1_awp,1_anp}}, {{0_c, {2_c,0_mu,2_pc,0_pm}, {2_awp,2_anp}, {0_containers, 2_containers, 3_postmutation}}}
      }
    );
  }
}
