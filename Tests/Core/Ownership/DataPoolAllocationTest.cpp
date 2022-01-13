////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DataPoolAllocationTest.hpp"
#include "DataPoolTestingUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view data_pool_allocation_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void data_pool_allocation_test::run_tests()
  {
    do_allocation_tests(*this);
  }

  template<bool PropagateMove, bool PropagateSwap>
  void data_pool_allocation_test::test_allocation()
  {
    using namespace ownership;
    using pool_t = data_pool<int, shared_counting_allocator<int, true, PropagateMove, PropagateSwap>>;
    using prediction_t = typename value_tester<pool_t>::prediction_type;

    pool_t pool{};
    auto elt{pool.make(-1)};
    check(weak_equivalence, LINE(""), pool, prediction_t{{-1, 1}});
    check(equality, LINE(""), elt.get(), -1);

    pool_t clonePool{};
    auto cloneElt{clonePool.make(-1)};
    check(weak_equivalence, LINE(""), clonePool, prediction_t{{-1, 1}});
    check(equality, LINE(""), cloneElt.get(), -1);

    auto allocGetter{
      [](const pool_t& pool){
        return pool.get_allocator();
      }
    };

    auto mutator{
      [](pool_t& pool) {
        auto e{pool.make(4)};
      }
    };

    check_semantics(LINE(""), pool_t{}, std::move(clonePool), pool_t{}, pool, mutator,
                    allocation_info{allocGetter, {0_pm, {1_pm, 1_mu}, {1_manp}}});
  }
}
