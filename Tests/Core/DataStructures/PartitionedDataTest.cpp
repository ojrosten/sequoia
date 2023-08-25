////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "sequoia/TestFramework/TestUtilities.hpp"
#include "PartitionedDataTest.hpp"

#include <array>
#include <complex>

namespace sequoia
{
  namespace testing
  {
    using namespace data_structures;
    using namespace object;

    [[nodiscard]]
    std::filesystem::path partitioned_data_test::source_file() const
    {
      return std::source_location::current().file_name();
    }

    void partitioned_data_test::run_tests()
    {
      test_iterators();
      test_static_storage();

      test_contiguous_capacity<int>();
      test_contiguous_capacity<int>();

      test_bucketed_capacity<int>();
      test_bucketed_capacity<int>();
    }

    void partitioned_data_test::test_static_storage()
    {
      {
        using prediction_t = std::initializer_list<std::initializer_list<int>>;

        using storage_t = static_partitioned_sequence<int, 1, 1>;
        storage_t storage{{5}};
        check(equivalence, report_line(""), storage, prediction_t{{5}});

        storage_t storage2{{3}};
        check(equivalence, report_line(""), storage2, prediction_t{{3}});

        check_semantics(report_line(""), storage, storage2);

        auto iter = storage.begin_partition(0);
        *iter = 4;
        check(equality, report_line(""), storage, storage_t{{4}});
      }

      {
        using prediction_t = std::initializer_list<std::initializer_list<double>>;

        using storage_t = static_partitioned_sequence<double, 3, 6>;
        storage_t storage{{1.2, 1.1}, {2.6, -7.8}, {0.0, -9.3}};
        check(equivalence, report_line(""), storage, prediction_t{{1.2, 1.1}, {2.6, -7.8}, {0.0, -9.3}});

        storage_t storage2{{1.2}, {1.1, 2.6, -7.8}, {0.0, -9.3}};
        check(equivalence, report_line(""), storage2, prediction_t{{1.2}, {1.1, 2.6, -7.8}, {0.0, -9.3}});

        check_semantics(report_line(""), storage, storage2);
      }

      {
        using prediction_t = std::initializer_list<std::initializer_list<double>>;

        using storage_t = static_partitioned_sequence<double, 2, 6>;
        storage_t storage{{0.2,0.3,-0.4}, {0.8,-1.1,-1.4}}, storage2{{0.2,0.3}, {-0.4, 0.8,-1.1,-1.4}};

        check(equivalence, report_line(""), storage, prediction_t{{0.2,0.3,-0.4}, {0.8,-1.1,-1.4}});
        check(equivalence, report_line(""), storage2, prediction_t{{0.2,0.3}, {-0.4, 0.8,-1.1,-1.4}});

        check_semantics(report_line(""), storage, storage2);
      }

      {
        using ndc = no_default_constructor;
        using prediction_t = std::initializer_list<std::initializer_list<ndc>>;

        {
          using storage_t = static_partitioned_sequence<no_default_constructor, 1, 1>;
          constexpr storage_t storage{{ndc{1}}};

          check(equivalence, report_line(""), storage, prediction_t{{ndc{1}}});
        }

        {
          using storage_t = static_partitioned_sequence<no_default_constructor, 3, 5>;
          constexpr storage_t storage2{{ndc{1}, ndc{1}}, {ndc{0}}, {ndc{2}, ndc{4}}};
          check(equivalence, report_line(""), storage2, prediction_t{{ndc{1}, ndc{1}}, {ndc{0}}, {ndc{2}, ndc{4}}});
        }
      }
    }

    void partitioned_data_test::test_iterators()
    {
      test_generic_iterator_properties<traits<int>, partition_impl::mutable_reference>();
      test_generic_iterator_properties<traits<int>, partition_impl::const_reference>();
    }

    template<class Traits, template<class> class ReferencePolicy>
    void partitioned_data_test::test_generic_iterator_properties()
    {
      using container_t = std::vector<int>;

      container_t vec{1, 2, 3};

      using p_i_t
        = utilities::iterator<
            typename partition_impl::partition_iterator_generator<Traits, ReferencePolicy, false>::iterator,
            utilities::identity_dereference_policy<typename partition_impl::partition_iterator_generator<Traits, ReferencePolicy, false>::iterator,
                                                   partition_impl::partition_index_policy<false, std::size_t>>
          >;

      p_i_t iter(vec.begin(), 4u);

      if constexpr(std::is_same_v<ReferencePolicy<int>, partition_impl::mutable_reference<int>>)
      {
        check(equality, report_line(""), iter.operator->(), &(*vec.begin()));
      }
      else
      {
        check(equality, report_line(""), iter.operator->(), &(*vec.cbegin()));
      }


      check(equality, report_line(""), vec[0],*iter);
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

      iter+=1;
      check(equality, report_line(""), iter[0], vec[1]);

      iter-=1;
      check(equality, report_line(""), iter[0], vec[0]);

      auto iter2 = iter + 2;
      check(equality, report_line(""), iter[0],  vec[0]);
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

    template<class T>
    void partitioned_data_test::test_contiguous_capacity()
    {
      partitioned_sequence<T> s{};
      check(equality, report_line(""), s.capacity(), 0_sz);
      check(equality, report_line(""), s.num_partitions_capacity(), 0_sz);

      s.reserve(4);
      check(equality, report_line(""), s.capacity(), 4_sz);
      check(equality, report_line(""), s.num_partitions_capacity(), 0_sz);

      s.reserve_partitions(8);
      check(equality, report_line(""), s.capacity(), 4_sz);
      check(equality, report_line(""), s.num_partitions_capacity(), 8_sz);

      s.shrink_to_fit();
      check(equality, report_line(""), s.capacity(), 0_sz);
      check(equality, report_line(""), s.num_partitions_capacity(), 0_sz);
    }

    template<class T>
    void partitioned_data_test::test_bucketed_capacity()
    {
      bucketed_sequence<T> s{};

      check(equality, report_line(""), s.num_partitions_capacity(), 0_sz);
      check(equality, report_line(""), s.partition_capacity(0), 0_sz);

      s.reserve_partitions(4);
      check(equality, report_line(""), s.num_partitions_capacity(), 4_sz);
      check(equality, report_line(""), s.partition_capacity(0), 0_sz);

      s.shrink_num_partitions_to_fit();
      check(equality, report_line("May fail if shrink to fit impl does not reduce capacity"), s.num_partitions_capacity(), 0_sz);
      check(equality, report_line(""), s.partition_capacity(0), 0_sz);

      s.add_slot();
      check(equality, report_line(""), s.partition_capacity(0), 0_sz);
      check(equality, report_line(""), s.partition_capacity(1), 0_sz);

      s.reserve_partition(0, 4);
      check(equality, report_line(""), s.partition_capacity(0), 4_sz);

      s.shrink_to_fit(0);
      check(equality, report_line("May fail if shrink to fit impl does not reduce capacity"), s.partition_capacity(0), 0_sz);
    }
  }
}
