////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "PartitionIteratorTest.hpp"

#include "PartitionedDataTestingUtilities.hpp"

namespace sequoia::testing
{
  using namespace data_structures;
  using namespace object;

  [[nodiscard]]
  std::filesystem::path partition_iterator_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void partition_iterator_test::run_tests()
  {
    test_iterators();
  }

  void partition_iterator_test::test_iterators()
  {
    test_generic_iterator_properties<std::vector<int>, std::vector<int>::iterator>();
    test_generic_iterator_properties<std::vector<int>, std::vector<int>::const_iterator>();
  }

  template<class Container, std::bidirectional_iterator I>
  void partition_iterator_test::test_generic_iterator_properties()
  {
    Container vec{1, 2, 3};

    using p_i_t = utilities::iterator<I, utilities::identity_dereference_policy<I, partition_impl::partition_index_policy<false, std::size_t>>>;

    p_i_t iter(vec.begin(), 4u);

    if constexpr(std::is_same_v<I, typename Container::iterator>)
    {
      check(equality, report_line(""), iter.operator->(), &(*vec.begin()));
    }
    else
    {
      check(equality, report_line(""), iter.operator->(), &(*vec.cbegin()));
    }

    check(equality, report_line(""), vec[0], *iter);
    check(equality, report_line(""), iter.partition_index(), 4_sz);

    ++iter;
    check(equality, report_line(""), *iter, vec[1]);
    check(equality, report_line(""), iter.partition_index(), 4_sz);

    iter++;
    check(equality, report_line(""), *iter, vec[2]);

    --iter;
    check(equality, report_line(""), *iter, vec[1]);
    check(equality, report_line(""), iter.partition_index(), 4_sz);

    iter--;
    check(equality, report_line(""), *iter, vec[0]);

    check(equality, report_line(""), iter[0], vec[0]);
    check(equality, report_line(""), iter[1], vec[1]);
    check(equality, report_line(""), iter[2], vec[2]);

    iter += 1;
    check(equality, report_line(""), iter[0], vec[1]);

    iter -= 1;
    check(equality, report_line(""), iter[0], vec[0]);

    auto iter2 = iter + 2;
    check(equality, report_line(""), iter[0], vec[0]);
    check(equality, report_line(""), iter2[0], vec[2]);

    auto iter3 = iter2 - 2;
    check(equality, report_line(""), iter3[0], vec[0]);

    check(report_line(""), iter == iter3);
    check(report_line(""), iter2 != iter3);

    check(report_line(""), iter2 > iter);
    check(report_line(""), iter < iter2);
    check(report_line(""), iter >= iter3);
    check(report_line(""), iter <= iter3);

    check(equality, report_line(""), std::ptrdiff_t{-2}, std::ranges::distance(iter2, iter3));
    check(equality, report_line(""), std::ptrdiff_t{2}, std::ranges::distance(iter, iter2));

    auto std_iter = iter.base_iterator();

    check(equality, report_line(""), *std_iter, 1);
  }
}
