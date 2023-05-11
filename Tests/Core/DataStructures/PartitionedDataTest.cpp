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
    std::filesystem::path partitioned_data_test::source_file() const noexcept
    {
      return std::source_location::current().file_name();
    }

    void partitioned_data_test::run_tests()
    {
      test_iterators<by_value>();
      test_iterators<shared>();
      test_storage();

      test_static_storage();

      test_contiguous_capacity<int, by_value<int>, true>();
      test_contiguous_capacity<int, shared<int>, true>();
      test_contiguous_capacity<int, by_value<int>, false>();
      test_contiguous_capacity<int, shared<int>, false>();

      test_bucketed_capacity<int, by_value<int>, true>();
      test_bucketed_capacity<int, shared<int>, true>();
      test_bucketed_capacity<int, by_value<int>, false>();
      test_bucketed_capacity<int, shared<int>, false>();
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

    void partitioned_data_test::test_storage()
    {
      {
        auto storage1 = test_generic_storage<bucketed_sequence<int, shared<int>>>();
        auto storage2 = test_generic_storage<partitioned_sequence<int,shared<int>>>();

        check(report_line(""), isomorphic(storage1, storage2));
        check(report_line(""), isomorphic(storage2, storage1));
      }

      {
        auto storage1 = test_generic_storage<bucketed_sequence<int, by_value<int>>>();
        auto storage2 = test_generic_storage<partitioned_sequence<int, by_value<int>>>();

        check(report_line(""), isomorphic(storage1, storage2));
      }
    }

    template <class Storage>
    Storage partitioned_data_test::test_generic_storage()
    {
      using value_type = typename Storage::value_type;
      using equivalent_type = std::initializer_list<std::initializer_list<value_type>>;

      constexpr bool sharedData{std::is_same<typename Storage::handler_type, shared<typename Storage::value_type>>::value};

      Storage storage;

      // Null
      check(equivalence, report_line(""), storage, equivalent_type{});

      check_exception_thrown<std::out_of_range>(
         report_line("No partition available to push back to"),
         [&storage]() {
           storage.push_back_to_partition(0, 8);
         }
      );

      check_exception_thrown<std::out_of_range>(
        report_line("No partition available to insert into"),
        [&storage]() {
          storage.insert_to_partition(storage.cbegin_partition(0), 1);
        }
      );

      check_exception_thrown<std::out_of_range>(
        report_line("No partitions to swap"),
        [&storage]() {
          storage.swap_partitions(0,0);
        }
      );

      storage.erase_slot(0);
      check(equivalence, report_line(""), storage, equivalent_type{});

      storage.add_slot();
      // []

      check(equality, report_line(""), storage, Storage{{}});
      check_semantics(report_line("Regular semantics"), storage, Storage{});

      check_exception_thrown<std::out_of_range>(
        report_line("Only one partition available so cannot push back to the second"),
        [&storage]() {
          storage.push_back_to_partition(1, 7);
        }
      );

      check_exception_thrown<std::out_of_range>(
        report_line("No partitions to swap"),
        [&storage]() {
          storage.swap_partitions(0,1);
        }
      );

      storage.swap_partitions(0,0);
      check(equality, report_line(""), storage, Storage{{}});

      storage.erase_slot(1);
      storage.erase_slot(0);
      // Null

      check(equality, report_line(""), storage, Storage{});

      storage.add_slot();
      storage.add_slot();
      // [][]

      check(equivalence, report_line(""), storage, equivalent_type{{}, {}});
      check(equality, report_line(""), storage, Storage{{}, {}});
      check_semantics(report_line(""), storage, Storage{});
      check_semantics(report_line(""), storage, Storage{{}});

      storage.swap_partitions(0,1);
      check(equality, report_line(""), storage, Storage{{}, {}});

      storage.push_back_to_partition(0, 2);
      // [2][]

      check(equivalence, report_line(""), storage, equivalent_type{{2}, {}});
      check(equality, report_line(""), storage, Storage{{2}, {}});
      check_semantics(report_line("Regular semantics"), storage, Storage{{1},{}});

      storage.swap_partitions(0,1);
      // [][2]

      check(equivalence, report_line(""), storage, equivalent_type{{}, {2}});
      check(equality, report_line(""), storage, Storage{{}, {2}});
      check_semantics(report_line("Regular semantics"), storage, Storage{{2},{}});

      storage.swap_partitions(0,1);
      // [2][]

      check(equality, report_line(""), storage, Storage{{2}, {}});

      storage.swap_partitions(1,0);
      // [][2]

      check(equality, report_line(""), storage, Storage{{}, {2}});

      storage.swap_partitions(1,0);
      // [2][]

      check(equality, report_line(""), storage, Storage{{2}, {}});

      auto iter = storage.begin_partition(0);
      check(equality, report_line(""), *iter, 2);
      *iter = 3;
      // [3][]

      check(equality, report_line(""), storage, Storage{{3}, {}});

      storage.push_back_to_partition(1, 4);
      // [3][4]

      check(equivalence, report_line(""), storage, equivalent_type{{3}, {4}});
      check(equality, report_line(""), storage, Storage{{3}, {4}});
      check_semantics(report_line("Regular semantics"), storage, Storage{{4},{3}});

      storage.swap_partitions(1,0);
      // [4][3]

      check(equivalence, report_line(""), storage, equivalent_type{{4}, {3}});
      check(equality, report_line(""), storage, Storage{{4}, {3}});

      storage.swap_partitions(0,1);
      // [3]4]

      check(equality, report_line(""), storage, Storage{{3}, {4}});

      storage.add_slot();
      storage.push_back_to_partition(2, 9);
      storage.push_back_to_partition(2, -3);
      // [3][4][9,-3]

      check(equivalence, report_line(""), storage, equivalent_type{{3}, {4}, {9, -3}});
      check(equality, report_line(""), storage, Storage{{3}, {4}, {9, -3}});
      check_semantics(report_line("Regular semantics"), storage, Storage{{4},{3}});

      storage.swap_partitions(1,2);
      // [3][9,-3][4]

      check(equality, report_line(""), storage, Storage{{3}, {9, -3}, {4}});

      storage.swap_partitions(2,1);
      // [3][4][9,-3]

      check(equality, report_line(""), storage, Storage{{3}, {4}, {9, -3}});

      iter = storage.insert_to_partition(storage.cbegin_partition(2), 2);
      // [3][4][2, 9,-3]

      check(equality, report_line(""), storage, Storage{{3}, {4}, {2, 9, -3}});
      check(equality, report_line(""), *iter, 2);
      check(equality, report_line(""), iter.partition_index(), 2_sz);

      storage.swap_partitions(0,2);
      // [2, 9,-3][4][3]

      check(equality, report_line(""), storage, Storage{{2, 9, -3}, {4}, {3}});

      storage.swap_partitions(2,0);
      // [3][4][2, 9,-3]

      check(equality, report_line(""), storage, Storage{{3}, {4}, {2, 9, -3}});

      iter = storage.insert_to_partition(storage.cbegin_partition(2) + 1, 8);
      // [3][4][2, 8, 9,-3]

      check(equality, report_line(""), storage, Storage{{3}, {4}, {2, 8, 9, -3}});
      check(equality, report_line(""), *iter, 8);
      check(equality, report_line(""), iter.partition_index(), 2_sz);

      iter = storage.insert_to_partition(storage.begin_partition(2) + 4, 7);
      // [3][4][2, 8, 9,-3, 7]

      check(equality, report_line(""), storage, Storage{{3}, {4}, {2, 8, 9, -3, 7}});
      check(equality, report_line(""), *iter, 7);
      check(equality, report_line(""), iter.partition_index(), 2_sz);

      storage.insert_to_partition(storage.cbegin_partition(2) + 5, 5);
      // [3][4][2, 8, 9,-3, 7, 5]

      check(equality, report_line(""), storage, Storage{{3}, {4}, {2, 8, 9, -3, 7, 5}});

      storage.add_slot();
      storage.insert_to_partition(storage.cbegin_partition(2), 1);
      // [3][4][1, 2, 8, 9,-3, 7, 5][]

      check(equality, report_line(""), storage, Storage{{3}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}});

      storage.insert_to_partition(storage.cbegin_partition(0), 12);
      // [12, 3][4][1, 2, 8, 9,-3, 7, 5][]

      check(equality, report_line(""), storage, Storage{{12, 3}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}});

      storage.swap_partitions(2, 1);
      // [12, 3][1, 2, 8, 9,-3, 7, 5][4][]

      check(equality, report_line(""), storage, Storage{{12, 3}, {1, 2, 8, 9, -3, 7, 5}, {4}, {}});

      storage.swap_partitions(0,1);
      // [1, 2, 8, 9,-3, 7, 5][12, 3][4][]

      check(equality, report_line(""), storage, Storage{{1, 2, 8, 9, -3, 7, 5}, {12, 3}, {4}, {}});

      storage.swap_partitions(1,0);

      // [12, 3][1, 2, 8, 9,-3, 7, 5][4][]

      check(equality, report_line(""), storage, Storage{{12, 3}, {1, 2, 8, 9, -3, 7, 5}, {4}, {}});

      storage.swap_partitions(1,2);

      // [12, 3][4][1, 2, 8, 9,-3, 7, 5][]

      check(equality, report_line(""), storage, Storage{{12, 3}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}});

      storage.insert_to_partition(storage.begin_partition(0) + 1, 13);
      // [12, 13, 3][4][1, 2, 8, 9,-3, 7, 5][]

      check(equality, report_line(""), storage, Storage{{12, 13, 3}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}});

      storage.insert_to_partition(storage.cbegin_partition(0) + 3, -8);
      // [12, 13, 3, -8][4][1, 2, 8, 9,-3, 7, 5][]

      check(equality, report_line(""), storage, Storage{{12, 13, 3, -8}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}});

      iter = storage.erase_from_partition(0, 0);
      // [13, 3, -8][4][1, 2, 8, 9,-3, 7, 5][]

      check(equality, report_line(""), storage, Storage{{13, 3, -8}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}});
      check(equality, report_line(""), *iter, 13);
      check(equality, report_line(""), iter.partition_index(), 0_sz);

      storage.erase_from_partition(0, 2);
      // [13, 3][4][1, 2, 8, 9,-3, 7, 5][]

      check(equality, report_line(""), storage, Storage{{13, 3}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}});

      iter = storage.erase_from_partition(storage.cbegin_partition(0));
      // [3][4][1, 2, 8, 9,-3, 7, 5][]

      check(equality, report_line(""), storage, Storage{{3}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}});
      check(equality, report_line(""), *iter, 3);
      check(equality, report_line(""), iter.partition_index(), 0_sz);

      storage.insert_to_partition(storage.cbegin_partition(2), -2);
      storage.insert_to_partition(storage.cbegin_partition(3), -2);
      // [3][4][-2, 1, 2, 8, 9,-3, 7, 5][-2]

      check(equality, report_line(""), storage, Storage{{3}, {4}, {-2, 1, 2, 8, 9, -3, 7, 5}, {-2}});

      storage.insert_to_partition(storage.cbegin_partition(3) + 1,-4);
      storage.insert_to_partition(storage.cbegin_partition(3), -4);
      // [3][4][-2, 1, 2, 8, 9,-3, 7, 5][-4, -2,-4]

      check(equality, report_line(""), storage, Storage{{3}, {4}, {-2, 1, 2, 8, 9, -3, 7, 5}, {-4, -2, -4}});

      storage.erase_slot(3);
      storage.erase_slot(2);
      // [3][4]

      check(equality, report_line(""), storage, Storage{{3}, {4}});

      storage.insert_slot(1);
      // [3][][4]

      check(equality, report_line(""), storage, Storage{{3}, {}, {4}});

      storage.insert_slot(0);
      // [][3][][4]

      check(equality, report_line(""), storage, Storage{{}, {3}, {}, {4}});

      storage.insert_to_partition(storage.cbegin_partition(0), 1);
      storage.insert_to_partition(storage.cbegin_partition(2), storage.cbegin_partition(0));
      // [1][3][1][4]

      if(check(equality, report_line(""), storage, Storage{{1}, {3}, {1}, {4}}))
      {
        iter = storage.begin_partition(0);
        *iter = 2;
        // shared: [2][3][2][4], by_value: [2][3][1][4]
        check(equality, report_line(""), storage, sharedData ? Storage{{2}, {3}, {2}, {4}} : Storage{{2}, {3}, {1}, {4}});
        check_semantics(report_line(""), storage, Storage{{1}, {3}, {1}, {4}});

        *iter = 1;
        // [1][3][1][4]
        check(equality, report_line(""), storage, Storage{{1}, {3}, {1}, {4}});

        iter = storage.begin_partition(2);
        *iter = -2;
        // shared: [-2][3][-2][4], by_value: [-2][3][1][4]
        check(equality, report_line(""), storage, sharedData ? Storage{{-2}, {3}, {-2}, {4}} : Storage{{1}, {3}, {-2}, {4}});
        *iter = 1;
        // [1][3][1][4]
        check(equality, report_line(""), storage, Storage{{1}, {3}, {1}, {4}});
      }

      storage.insert_to_partition(storage.cbegin_partition(0), 2);
      storage.insert_to_partition(storage.cbegin_partition(0) + 2, storage.cbegin_partition(0));
      // [2,1,2][3][1][4]

      check(equality, report_line(""), storage, Storage{{2, 1, 2}, {3}, {1}, {4}});

      storage.insert_to_partition(storage.cbegin_partition(2), 7);
      // [2,1,2][3][1, 7][4]

      check(equality, report_line(""), storage, Storage{{2, 1, 2}, {3}, {7,1}, {4}});

      storage.erase_slot(2);
      // [2,1,2][3][4]

      check(equality, report_line(""), storage, Storage{{2,1,2}, {3}, {4}});

      storage.erase_slot(0);
      // [3][4]

      check(equality, report_line(""), storage, Storage{{3}, {4}});

      storage.push_back_to_partition(0, -5);
      storage.push_back_to_partition(1, --storage.cend_partition(0));
      // [3,-5][4,-5]

      if(check(equality, report_line(""), storage, Storage{{3, -5}, {4, -5}}))
      {
        iter = storage.begin_partition(0);
        ++iter;
        *iter = 2;
        // shared [3,2][4,2], by_value: [3,2][4,-5]
        check(equality, report_line(""), storage, sharedData ? Storage{{3, 2}, {4, 2}} : Storage{{3, 2}, {4, -5}});
        check_semantics(report_line(""), storage, Storage{{3, -5}, {4, -5}});

        *iter = -5;
        // [3,-5][4,-5]
        check(equality, report_line(""), storage, Storage{{3, -5}, {4, -5}});

        iter = storage.begin_partition(1);
        ++iter;
        *iter = -2;
        // shared [3,-2][4,-2], by_value: [3,-5][4,-2]
        check(equality, report_line(""), storage, sharedData ? Storage{{3, -2}, {4, -2}} : Storage{{3, -5}, {4, -2}});

        *iter = -5;
        // [3,-5][4,-5]
        check(equality, report_line(""), storage, Storage{{3, -5}, {4, -5}});
      }

      storage.add_slot();
      storage.push_back_to_partition(2, 8);
      storage.push_back_to_partition(2, 9);
      // [3,-5][4,-5][8,9]

      check(equality, report_line(""), storage, Storage{{3, -5}, {4, -5}, {8, 9}});

      storage.erase_slot(1);
      // [3,-5][8,9]

      check(equality, report_line(""), storage, Storage{{3, -5}, {8, 9}});

      storage.erase_slot(0);
      // [8,9]

      check(equality, report_line(""), storage, Storage{{8, 9}});

      storage.add_slot();
      storage.push_back_to_partition(1, 4);
      storage.push_back_to_partition(1, -5);
      // [8,9][4,-5]

      check(equality, report_line(""), storage, Storage{{8, 9}, {4, -5}});

      {
        iter = storage.begin_partition(0);
        iter++;
        *iter = -5;
        auto next = storage.erase_from_partition(0, 0);
        check(equality, report_line(""), *next, -5);
      }
      // [-5][4,-5]

      check(equality, report_line(""), storage, Storage{{-5}, {4, -5}});


      {
        auto next = storage.erase_from_partition(0, 2);
        check(report_line(""), next == storage.end_partition(0));
      }
      // [-5][4,-5]

      check(equality, report_line(""), storage, Storage{{-5}, {4, -5}});

      {
        auto next = storage.erase_from_partition(1, 1);
        check(report_line(""), next == storage.end_partition(1));
      }
      // [-5][4]

      check(equality, report_line(""), storage, Storage{{-5}, {4}});

      storage.push_back_to_partition(0, 6);
      storage.push_back_to_partition(1, --storage.cend_partition(0));
      // [-5,6][4,6]

      check(equality, report_line(""), storage, Storage{{-5, 6}, {4, 6}});

      storage.push_back_to_partition(0, -2);
      storage.push_back_to_partition(1, --storage.cend_partition(0));
      // [-5,6,-2][4,6,-2]

      check(equality, report_line(""), storage, Storage{{-5, 6, -2}, {4, 6, -2}});

      auto found{std::ranges::find_if(storage.partition(0), [](const int& elt) { return elt == 6; })};

      if(check(report_line(""), found != storage.end_partition(0)))
        check(equality, report_line(""), *found, 6);

      erase_from_partition_if(storage, 0, [](const int& elt) { return elt == 6; });
      // [-5,-2][4,6,-2]

      check(equality, report_line(""), storage, Storage{{-5, -2}, {4, 6, -2}});

      auto found2{std::ranges::find_if(storage.cpartition(0), [](const int& elt) { return elt == 6; })};

      check(report_line(""), found2 == storage.cend_partition(0));
      check(equality, report_line(""), storage.size(), 5_sz);

      storage.push_back_to_partition(1, 7);
      storage.push_back_to_partition(1, --storage.cend_partition(1));

      // [-5,-2][4,6,-2,7,7]

      check(equality, report_line(""), storage, Storage{{-5, -2}, {4, 6, -2, 7, 7}});

      erase_from_partition_if(storage, 1, [](const int& elt) { return elt == 7; });
      // [-5,-2][4,6,-2]

      check(equality, report_line(""), storage, Storage{{-5, -2}, {4, 6, -2}});

      return storage;
    }

    template<template<class> class Handler>
    void partitioned_data_test::test_iterators()
    {
      test_generic_iterator_properties<traits, Handler, partition_impl::mutable_reference>();
      test_generic_iterator_properties<traits, Handler, partition_impl::const_reference>();
    }

    template<class Traits, template<class> class Handler, template<class> class ReferencePolicy>
    void partitioned_data_test::test_generic_iterator_properties()
    {
      using container_t = std::vector<typename Handler<int>::product_type>;

      container_t vec{Handler<int>::producer_type::make(1), Handler<int>::producer_type::make(2), Handler<int>::producer_type::make(3)};

      using p_i_t
        = utilities::iterator<
            typename partition_impl::partition_iterator_generator<Traits, Handler<int>, ReferencePolicy, false>::iterator,
            partition_impl::dereference_policy_for<Handler<int>, ReferencePolicy, partition_impl::partition_index_policy<false, std::size_t>>
          >;

      p_i_t iter(vec.begin(), 4u);

      if constexpr(std::is_same_v<ReferencePolicy<int>, partition_impl::mutable_reference<int>>)
      {
        check(equality, report_line(""), iter.operator->(), &Handler<int>::get(*vec.begin()));
      }
      else
      {
        check(equality, report_line(""), iter.operator->(), &Handler<int>::get(*vec.cbegin()));
      }


      check(equality, report_line(""), Handler<int>::get(vec[0]),*iter);
      check(equality, report_line(""), iter.partition_index(), 4_sz);

      ++iter;
      check(equality, report_line(""), *iter, Handler<int>::get(vec[1]));
      check(equality, report_line(""), iter.partition_index(), 4_sz);

      iter++;
      check(equality, report_line(""), *iter, Handler<int>::get(vec[2]));

      --iter;
      check(equality, report_line(""), *iter, Handler<int>::get(vec[1]));
      check(equality, report_line(""), iter.partition_index(), 4_sz);

      iter--;
      check(equality, report_line(""), *iter, Handler<int>::get(vec[0]));

      check(equality, report_line(""), iter[0], Handler<int>::get(vec[0]));
      check(equality, report_line(""), iter[1], Handler<int>::get(vec[1]));
      check(equality, report_line(""), iter[2], Handler<int>::get(vec[2]));

      iter+=1;
      check(equality, report_line(""), iter[0], Handler<int>::get(vec[1]));

      iter-=1;
      check(equality, report_line(""), iter[0], Handler<int>::get(vec[0]));

      auto iter2 = iter + 2;
      check(equality, report_line(""), iter[0], Handler<int>::get(vec[0]));
      check(equality, report_line(""), iter2[0], Handler<int>::get(vec[2]));

      auto iter3 = iter2 - 2;
      check(equality, report_line(""), iter3[0], Handler<int>::get(vec[0]));

      check(report_line(""), iter == iter3);
      check(report_line(""), iter2 != iter3);

      check(report_line(""), iter2 > iter);
      check(report_line(""), iter < iter2);
      check(report_line(""), iter >= iter3);
      check(report_line(""), iter <= iter3);

      check(equality, report_line(""), std::ptrdiff_t{-2}, distance(iter2, iter3));
      check(equality, report_line(""), std::ptrdiff_t{2}, distance(iter, iter2));

      auto std_iter = iter.base_iterator();

      check(equality, report_line(""), Handler<int>::get(*std_iter), 1);
    }

    template<class T, class Handler, bool ThrowOnRangeError>
    void partitioned_data_test::test_contiguous_capacity()
    {
      partitioned_sequence<T, Handler> s{};
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

    template<class T, class Handler, bool ThrowOnRangeError>
    void partitioned_data_test::test_bucketed_capacity()
    {
      bucketed_sequence<T, Handler> s{};

      check(equality, report_line(""), s.num_partitions_capacity(), 0_sz);
      if constexpr(ThrowOnRangeError) check_exception_thrown<std::out_of_range>(report_line(""), [&s](){ return s.partition_capacity(0); });

      s.reserve_partitions(4);
      check(equality, report_line(""), s.num_partitions_capacity(), 4_sz);
      if constexpr(ThrowOnRangeError) check_exception_thrown<std::out_of_range>(report_line(""), [&s](){ return s.partition_capacity(0); });

      s.shrink_num_partitions_to_fit();
      check(equality, report_line("May fail if shrink to fit impl does not reduce capacity"), s.num_partitions_capacity(), 0_sz);
      if constexpr(ThrowOnRangeError) check_exception_thrown<std::out_of_range>(report_line(""), [&s](){ return s.partition_capacity(0); });

      s.add_slot();
      check(equality, report_line(""), s.partition_capacity(0), 0_sz);
      if constexpr(ThrowOnRangeError) check_exception_thrown<std::out_of_range>(report_line(""), [&s](){ return s.partition_capacity(1); });

      s.reserve_partition(0, 4);
      check(equality, report_line(""), s.partition_capacity(0), 4_sz);

      s.shrink_to_fit(0);
      check(equality, report_line("May fail if shrink to fit impl does not reduce capacity"), s.partition_capacity(0), 0_sz);
      if constexpr(ThrowOnRangeError) check_exception_thrown<std::out_of_range>(report_line(""), [&s](){ s.shrink_to_fit(1); });
    }
  }
}
