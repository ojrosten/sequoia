////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PartitionedDataTest.hpp"

#include <array>
#include <complex>

namespace sequoia
{
  namespace unit_testing
  {
    void partitioned_data_test::run_tests()
    {
      using namespace data_sharing;
      using namespace data_structures;
      test_iterators<independent>();
      test_iterators<shared>();
      test_storage();

      test_static_storage();

      test_contiguous_capacity<int, independent<int>, true>();
      test_contiguous_capacity<int, shared<int>, true>();
      test_contiguous_capacity<int, independent<int>, false>();
      test_contiguous_capacity<int, shared<int>, false>();
      
      test_bucketed_capacity<int, independent<int>, true>();
      test_bucketed_capacity<int, shared<int>, true>();
      test_bucketed_capacity<int, independent<int>, false>();
      test_bucketed_capacity<int, shared<int>, false>();
    }
    
    void partitioned_data_test::test_static_storage()
    {
      using namespace data_structures;

      {
        using prediction_t = std::initializer_list<std::initializer_list<int>>;
        
        using storage_t = static_contiguous_storage<int, 1, 1>;
        storage_t storage{{5}};
        check_equivalence(storage, prediction_t{{5}}, LINE(""));
        
        storage_t storage2{{3}};
        check_equivalence(storage, prediction_t{{5}}, LINE(""));

        check_regular_semantics(storage, storage2, LINE("Regular Semantics"));

        auto iter = storage.begin_partition(0);
        *iter = 4;
        check_equality(storage, storage_t{{4}}, LINE(""));
      }

      {
        using prediction_t = std::initializer_list<std::initializer_list<double>>;
        
        using storage_t = static_contiguous_storage<double, 3, 6>;
        storage_t storage{{1.2, 1.1}, {2.6, -7.8}, {0.0, -9.3}};
        check_equivalence(storage, prediction_t{{1.2, 1.1}, {2.6, -7.8}, {0.0, -9.3}}, LINE(""));

        storage_t storage2{{1.2}, {1.1, 2.6, -7.8}, {0.0, -9.3}};
        check_equivalence(storage2, prediction_t{{1.2}, {1.1, 2.6, -7.8}, {0.0, -9.3}}, LINE(""));

        check_regular_semantics(storage, storage2, LINE("Regular Semantics"));
      }

      {
        using prediction_t = std::initializer_list<std::initializer_list<double>>;
        
        using storage_t = static_contiguous_storage<double, 2, 6>;   
        storage_t storage{{0.2,0.3,-0.4}, {0.8,-1.1,-1.4}}, storage2{{0.2,0.3}, {-0.4, 0.8,-1.1,-1.4}};
        
        check_equivalence(storage, prediction_t{{0.2,0.3,-0.4}, {0.8,-1.1,-1.4}}, LINE(""));
        check_equivalence(storage2, prediction_t{{0.2,0.3}, {-0.4, 0.8,-1.1,-1.4}}, LINE(""));

        check_regular_semantics(storage, storage2, LINE("Regular Semantics"));
      }

      {
        using ndc = no_default_constructor;
        using prediction_t = std::initializer_list<std::initializer_list<ndc>>;

        {
          using storage_t = static_contiguous_storage<no_default_constructor, 1, 1>;      
          constexpr storage_t storage{{1}};
          
          check_equivalence(storage, prediction_t{{ndc{1}}}, LINE(""));
        }

        {
          using storage_t = static_contiguous_storage<no_default_constructor, 3, 5>;
          constexpr storage_t storage2{{1, 1}, {0}, {2, 4}};
          check_equivalence(storage2, prediction_t{{ndc{1}, ndc{1}}, {ndc{0}}, {ndc{2}, ndc{4}}}, LINE(""));
        }
      }
    }
    
    void partitioned_data_test::test_storage()
    {
      using namespace data_structures;
      using namespace data_sharing;

      auto storage1 = test_generic_storage<bucketed_storage<int>>();
      auto storage2 = test_generic_storage<contiguous_storage<int>>();

      check(isomorphic(storage1, storage2));
      check(isomorphic(storage2, storage1));

      auto storage3 = test_generic_storage<bucketed_storage<int, independent<int>>>();
      auto storage4 = test_generic_storage<contiguous_storage<int, independent<int>>>();

      check(isomorphic(storage3, storage4));
    }

    template <class Storage>
    Storage partitioned_data_test::test_generic_storage()
    {
      using namespace data_structures;
      using namespace data_sharing;
      using value_type = typename Storage::value_type;
      using equivalent_type = std::initializer_list<std::initializer_list<value_type>>;

      constexpr bool sharedData{std::is_same<typename Storage::sharing_policy_type, shared<typename Storage::value_type>>::value};

      this->failure_message_prefix(type_to_string<Storage>::str());

      Storage storage;

      // Null
      check_equivalence(storage, equivalent_type{}, LINE(""));

      check_exception_thrown<std::out_of_range>([&storage]() {
          storage.push_back_to_partition(0, 8);
        }, LINE("No partition available to push back to")
      );
      
      check_exception_thrown<std::out_of_range>([&storage]() {
          storage.insert_to_partition(storage.cbegin_partition(0), 1);
        }, LINE("No partition available to insert into")
      );
      
      check_exception_thrown<std::out_of_range>([&storage]() {
          storage.swap_partitions(0,0);
        }, LINE("No partitions to swap")
      );
      
      storage.erase_slot(0);
      check_equivalence(storage, equivalent_type{}, LINE(""));

      storage.add_slot();
      // []
     
      check_equality(storage, Storage{{}}, LINE(""));
      check_regular_semantics(storage, Storage{}, LINE("Regular semantics"));

      check_exception_thrown<std::out_of_range>([&storage]() {
          storage.push_back_to_partition(1, 7);
        }, LINE("Only one partition available so cannot push back to the second")
      );

      check_exception_thrown<std::out_of_range>([&storage]() {
          storage.swap_partitions(0,1);
        }, LINE("No partitions to swap")
      );
      
      storage.swap_partitions(0,0);
      check_equality(storage, Storage{{}}, LINE(""));
      
      storage.erase_slot(1);
      storage.erase_slot(0);
      // Null
    
      check_equality(storage, Storage{}, LINE(""));

      storage.add_slot();
      storage.add_slot();
      // [][]
      
      check_equality(storage, Storage{{}, {}}, LINE(""));
      check_regular_semantics(storage, Storage{}, LINE(""));
      check_regular_semantics(storage, Storage{{}}, LINE(""));
      
      storage.swap_partitions(0,1);
      check_equality(storage, Storage{{}, {}}, LINE(""));
      
      storage.push_back_to_partition(0, 2);
      // [2][]
   
      check_equality(storage, Storage{{2}, {}}, LINE(""));
      check_regular_semantics(storage, Storage{{1},{}}, LINE("Regular semantics"));

      storage.swap_partitions(0,1);
      // [][2]
      
      check_equality(storage, Storage{{}, {2}});
      check_regular_semantics(storage, Storage{{2},{}}, LINE("Regular semantics"));

      storage.swap_partitions(0,1);
      // [2][]
      
      check_equality(storage, Storage{{2}, {}}, LINE(""));

      storage.swap_partitions(1,0);
      // [][2]
      
      check_equality(storage, Storage{{}, {2}}, LINE(""));
 
      storage.swap_partitions(1,0);
      // [2][]
      
      check_equality(storage, Storage{{2}, {}}, LINE(""));
      
      auto iter = storage.begin_partition(0);
      check_equality(2, *iter, LINE(""));
      *iter = 3;
      // [3][]

      check_equality(storage, Storage{{3}, {}}, LINE(""));
      
      storage.push_back_to_partition(1, 4);
      // [3][4]

      check_equality(storage, Storage{{3}, {4}}, LINE(""));
      check_regular_semantics(storage, Storage{{4},{3}}, LINE("Regular semantics"));

      storage.swap_partitions(1,0);
      // [4][3]
      
      check_equality(storage, Storage{{4}, {3}}, LINE(""));

      storage.swap_partitions(0,1);
      // [3]4]
      
      check_equality(storage, Storage{{3}, {4}}, LINE(""));

      storage.add_slot();
      storage.push_back_to_partition(2, 9);
      storage.push_back_to_partition(2, -3);
      // [3][4][9,-3]

      check_equality(storage, Storage{{3}, {4}, {9, -3}}, LINE(""));
      check_regular_semantics(storage, Storage{{4},{3}}, LINE("Regular semantics"));

      storage.swap_partitions(1,2);
      // [3][9,-3][4]
      
      check_equality(storage, Storage{{3}, {9, -3}, {4}}, LINE(""));

      storage.swap_partitions(2,1);
      // [3][4][9,-3]

      check_equality(storage, Storage{{3}, {4}, {9, -3}}, LINE(""));

      iter = storage.insert_to_partition(storage.cbegin_partition(2), 2);
      // [3][4][2, 9,-3]

      check_equality(storage, Storage{{3}, {4}, {2, 9, -3}}, LINE(""));
      check_equality(2, *iter, LINE(""));
      check_equality<std::size_t>(2, iter.partition_index(), LINE(""));

      storage.swap_partitions(0,2);
      // [2, 9,-3][4][3]

      check_equality(storage, Storage{{2, 9, -3}, {4}, {3}}, LINE(""));

      storage.swap_partitions(2,0);
      // [3][4][2, 9,-3]

      check_equality(storage, Storage{{3}, {4}, {2, 9, -3}}, LINE(""));

      iter = storage.insert_to_partition(storage.cbegin_partition(2) + 1, 8);
      // [3][4][2, 8, 9,-3]

      check_equality(storage, Storage{{3}, {4}, {2, 8, 9, -3}}, LINE(""));
      check_equality(8, *iter, LINE(""));
      check_equality<std::size_t>(2, iter.partition_index(), LINE(""));

      iter = storage.insert_to_partition(storage.begin_partition(2) + 4, 7);
      // [3][4][2, 8, 9,-3, 7]

      check_equality(storage, Storage{{3}, {4}, {2, 8, 9, -3, 7}}, LINE(""));
      check_equality(7, *iter, LINE(""));
      check_equality<std::size_t>(2, iter.partition_index(), LINE(""));

      storage.insert_to_partition(storage.cbegin_partition(2) + 5, 5);
      // [3][4][2, 8, 9,-3, 7, 5]

      check_equality(storage, Storage{{3}, {4}, {2, 8, 9, -3, 7, 5}}, LINE(""));

      storage.add_slot();
      storage.insert_to_partition(storage.cbegin_partition(2), 1);
      // [3][4][1, 2, 8, 9,-3, 7, 5][]

      check_equality(storage, Storage{{3}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}}, LINE(""));

      storage.insert_to_partition(storage.cbegin_partition(0), 12);
      // [12, 3][4][1, 2, 8, 9,-3, 7, 5][]

      check_equality(storage, Storage{{12, 3}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}}, LINE(""));

      storage.swap_partitions(2, 1);
      // [12, 3][1, 2, 8, 9,-3, 7, 5][4][]

      check_equality(storage, Storage{{12, 3}, {1, 2, 8, 9, -3, 7, 5}, {4}, {}}, LINE(""));

      storage.swap_partitions(0,1);
      // [1, 2, 8, 9,-3, 7, 5][12, 3][4][]

      check_equality(storage, Storage{{1, 2, 8, 9, -3, 7, 5}, {12, 3}, {4}, {}}, LINE(""));

      storage.swap_partitions(1,0);

      // [12, 3][1, 2, 8, 9,-3, 7, 5][4][]

      check_equality(storage, Storage{{12, 3}, {1, 2, 8, 9, -3, 7, 5}, {4}, {}}, LINE(""));

      storage.swap_partitions(1,2);

      // [12, 3][4][1, 2, 8, 9,-3, 7, 5][]

      check_equality(storage, Storage{{12, 3}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}}, LINE(""));
      
      storage.insert_to_partition(storage.begin_partition(0) + 1, 13);
      // [12, 13, 3][4][1, 2, 8, 9,-3, 7, 5][]

      check_equality(storage, Storage{{12, 13, 3}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}}, LINE(""));

      storage.insert_to_partition(storage.cbegin_partition(0) + 3, -8);
      // [12, 13, 3, -8][4][1, 2, 8, 9,-3, 7, 5][]

      check_equality(storage, Storage{{12, 13, 3, -8}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}}, LINE(""));

      iter = storage.erase_from_partition(0, 0);
      // [13, 3, -8][4][1, 2, 8, 9,-3, 7, 5][]

      check_equality(storage, Storage{{13, 3, -8}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}}, LINE(""));
      check_equality(13, *iter, LINE(""));
      check_equality<std::size_t>(0, iter.partition_index(), LINE(""));

      storage.erase_from_partition(0, 2);
      // [13, 3][4][1, 2, 8, 9,-3, 7, 5][]

      check_equality(storage, Storage{{13, 3}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}}, LINE(""));

      iter = storage.erase_from_partition(storage.cbegin_partition(0));
      // [3][4][1, 2, 8, 9,-3, 7, 5][]

      check_equality(storage, Storage{{3}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}}, LINE(""));
      check_equality(3, *iter, LINE(""));
      check_equality<std::size_t>(0, iter.partition_index(), LINE(""));

      storage.insert_to_partition(storage.cbegin_partition(2), -2);
      storage.insert_to_partition(storage.cbegin_partition(3), -2);
      // [3][4][-2, 1, 2, 8, 9,-3, 7, 5][-2]

      check_equality(storage, Storage{{3}, {4}, {-2, 1, 2, 8, 9, -3, 7, 5}, {-2}});

      storage.insert_to_partition(storage.cbegin_partition(3) + 1,-4);
      storage.insert_to_partition(storage.cbegin_partition(3), -4);
      // [3][4][-2, 1, 2, 8, 9,-3, 7, 5][-4, -2,-4]

      check_equality(storage, Storage{{3}, {4}, {-2, 1, 2, 8, 9, -3, 7, 5}, {-4, -2, -4}}, LINE(""));

      storage.erase_slot(3);
      storage.erase_slot(2);
      // [3][4]

      check_equality(storage, Storage{{3}, {4}}, LINE(""));

      storage.insert_slot(1);
      // [3][][4]

      check_equality(storage, Storage{{3}, {}, {4}}, LINE(""));

      storage.insert_slot(0);
      // [][3][][4]

      check_equality(storage, Storage{{}, {3}, {}, {4}}, LINE(""));

      storage.insert_to_partition(storage.cbegin_partition(0), 1);
      storage.insert_to_partition(storage.cbegin_partition(2), storage.cbegin_partition(0));
      // [1][3][1][4]

      if(check_equality(storage, Storage{{1}, {3}, {1}, {4}}, LINE("")))
      {
        auto iter = storage.begin_partition(0);
        *iter = 2;
        // shared: [2][3][2][4], independent: [2][3][1][4]
        check_equality(storage, sharedData ? Storage{{2}, {3}, {2}, {4}} : Storage{{2}, {3}, {1}, {4}}, LINE(""));
        check_regular_semantics(storage, Storage{{1}, {3}, {1}, {4}}, LINE(""));
        
        *iter = 1;
        // [1][3][1][4]
        check_equality(storage, Storage{{1}, {3}, {1}, {4}}, LINE(""));

        iter = storage.begin_partition(2);
        *iter = -2;
        // shared: [-2][3][-2][4], independent: [-2][3][1][4]
        check_equality(storage, sharedData ? Storage{{-2}, {3}, {-2}, {4}} : Storage{{1}, {3}, {-2}, {4}}, LINE(""));
        *iter = 1;
        // [1][3][1][4]
        check_equality(storage, Storage{{1}, {3}, {1}, {4}}, LINE(""));
      }

      storage.insert_to_partition(storage.cbegin_partition(0), 2);
      storage.insert_to_partition(storage.cbegin_partition(0) + 2, storage.cbegin_partition(0));
      // [2,1,2][3][1][4]

      check_equality(storage, Storage{{2, 1, 2}, {3}, {1}, {4}}, LINE(""));

      storage.insert_to_partition(storage.cbegin_partition(2), 7);
      // [2,1,2][3][1, 7][4]

      check_equality(storage, Storage{{2, 1, 2}, {3}, {7,1}, {4}}, LINE(""));

      storage.erase_slot(2);
      // [2,1,2][3][4]

      check_equality(storage, Storage{{2,1,2}, {3}, {4}}, LINE(""));

      storage.erase_slot(0);
      // [3][4]

      check_equality(storage, Storage{{3}, {4}}, LINE(""));

      storage.push_back_to_partition(0, -5);       
      storage.push_back_to_partition(1, --storage.cend_partition(0));
      // [3,-5][4,-5]

      if(check_equality(storage, Storage{{3, -5}, {4, -5}}, LINE("")))
      {
        auto iter = storage.begin_partition(0);
        ++iter;
        *iter = 2;
        // shared [3,2][4,2], independent: [3,2][4,-5]
        check_equality(storage, sharedData ? Storage{{3, 2}, {4, 2}} : Storage{{3, 2}, {4, -5}}, LINE(""));
        check_regular_semantics(storage, Storage{{3, -5}, {4, -5}}, LINE(""));
        
        *iter = -5;
        // [3,-5][4,-5]
        check_equality(storage, Storage{{3, -5}, {4, -5}});

        iter = storage.begin_partition(1);
        ++iter;
        *iter = -2;
        // shared [3,-2][4,-2], independent: [3,-5][4,-2]
        check_equality(storage, sharedData ? Storage{{3, -2}, {4, -2}} : Storage{{3, -5}, {4, -2}}, LINE(""));

        *iter = -5;
        // [3,-5][4,-5]
        check_equality(storage, Storage{{3, -5}, {4, -5}}, LINE(""));
      }

      storage.add_slot();
      storage.push_back_to_partition(2, 8);
      storage.push_back_to_partition(2, 9);
      // [3,-5][4,-5][8,9]

      check_equality(storage, Storage{{3, -5}, {4, -5}, {8, 9}}, LINE(""));

      storage.erase_slot(1);
      // [3,-5][8,9]

      check_equality(storage, Storage{{3, -5}, {8, 9}}, LINE(""));

      storage.erase_slot(0);
      // [8,9]

      check_equality(storage, Storage{{8, 9}}, LINE(""));

      storage.add_slot();
      storage.push_back_to_partition(1, 4);
      storage.push_back_to_partition(1, -5);
      // [8,9][4,-5]

      check_equality(storage, Storage{{8, 9}, {4, -5}}, LINE(""));

      {
        auto iter = storage.begin_partition(0);
        iter++;
        *iter = -5;
        auto next = storage.erase_from_partition(0, 0);
        check_equality(-5, *next, LINE(""));
      }
      // [-5][4,-5]

      check_equality(storage, Storage{{-5}, {4, -5}}, LINE(""));


      {
        auto next = storage.erase_from_partition(0, 2);
        check(next == storage.end_partition(0), LINE(""));
      }
      // [-5][4,-5]

      check_equality(storage, Storage{{-5}, {4, -5}}, LINE(""));

      {
        auto next = storage.erase_from_partition(1, 1);
        check(next == storage.end_partition(1));
      }
      // [-5][4]

      check_equality(storage, Storage{{-5}, {4}}, LINE(""));

      storage.push_back_to_partition(0, 6);
      storage.push_back_to_partition(1, --storage.cend_partition(0));
      // [-5,6][4,6]

      check_equality(storage, Storage{{-5, 6}, {4, 6}}, LINE(""));

      storage.push_back_to_partition(0, -2);
      storage.push_back_to_partition(1, --storage.cend_partition(0));
      // [-5,6,-2][4,6,-2]

      check_equality(storage, Storage{{-5, 6, -2}, {4, 6, -2}}, LINE(""));

      auto found{std::find_if(storage.begin_partition(0), storage.end_partition(0),
        [](const int& elt) { return elt == 6; }
      )};
      
      if(check(found != storage.end_partition(0)))
        check_equality(6, *found, LINE(""));

      erase_from_partition_if(storage, 0, [](const int& elt) { return elt == 6; });
      // [-5,-2][4,6,-2]

      check_equality(storage, Storage{{-5, -2}, {4, 6, -2}});
      
      auto found2{std::find_if(storage.cbegin_partition(0), storage.cend_partition(0),
        [](const int& elt) { return elt == 6; })
      };
      
      check(found2 == storage.cend_partition(0));
      check_equality<std::size_t>(5, storage.size());

      storage.push_back_to_partition(1, 7);
      storage.push_back_to_partition(1, --storage.cend_partition(1));

      // [-5,-2][4,6,-2,7,7]

      check_equality(storage, Storage{{-5, -2}, {4, 6, -2, 7, 7}}, LINE(""));

      erase_from_partition_if(storage, 1, [](const int& elt) { return elt == 7; });
      // [-5,-2][4,6,-2]

      check_equality(storage, Storage{{-5, -2}, {4, 6, -2}}, LINE(""));

      return storage;
    }

    template<template<class> class SharingPolicy>
    void partitioned_data_test::test_iterators()
    {
      using namespace data_structures;

      {
        std::vector<typename SharingPolicy<int>::handle_type> vec{SharingPolicy<int>::make(1), SharingPolicy<int>::make(2), SharingPolicy<int>::make(3)};
        test_generic_iterator_properties<traits, SharingPolicy<int>, partition_impl::mutable_reference>(vec);
        test_generic_iterator_properties<traits, SharingPolicy<int>, partition_impl::const_reference>(vec);
      }

      {
        using data = std::pair<bool, double>;

        std::vector<typename SharingPolicy<data>::handle_type> vec{SharingPolicy<data>::make(true, 1.0)};

        test_generic_iterator_deref<traits, SharingPolicy<std::pair<bool, double>>, partition_impl::mutable_reference>(vec);
        test_generic_iterator_deref<traits, SharingPolicy<std::pair<bool, double>>, partition_impl::const_reference>(vec);
      }
    }

    template<class Traits, class SharingPolicy, template<class> class ReferencePolicy, class Arg>
    void partitioned_data_test::test_generic_iterator_properties(const Arg& v)
    {
      using namespace data_structures;
      using T = typename SharingPolicy::elementary_type;

      auto vec(v);

      using p_i_t
        = utilities::iterator<
            typename partition_impl::partition_iterator_generator<Traits, SharingPolicy, ReferencePolicy, false>::iterator,
            partition_impl::dereference_policy<SharingPolicy, ReferencePolicy, partition_impl::partition_index_policy<false, std::size_t>>
          >;

      p_i_t iter(vec.begin(), 4u);

      check_equality<T>(SharingPolicy::get(v[0]),*iter);
      check_equality<std::size_t>(4, iter.partition_index());

      ++iter;
      check_equality<T>(SharingPolicy::get(v[1]), *iter);
      check_equality<std::size_t>(4, iter.partition_index());

      iter++;
      check_equality<T>(SharingPolicy::get(v[2]), *iter);

      --iter;
      check_equality<T>(SharingPolicy::get(v[1]), *iter);
      check_equality<std::size_t>(4, iter.partition_index());

      iter--;
      check_equality<T>(SharingPolicy::get(v[0]), *iter);

      check_equality<T>(SharingPolicy::get(v[0]), iter[0]);
      check_equality<T>(SharingPolicy::get(v[1]), iter[1]);
      check_equality<T>(SharingPolicy::get(v[2]), iter[2]);

      iter+=1;
      check_equality<T>(SharingPolicy::get(v[1]), iter[0]);

      iter-=1;
      check_equality<T>(SharingPolicy::get(v[0]), iter[0]);

      auto iter2 = iter + 2;
      check_equality<T>(SharingPolicy::get(v[0]), iter[0]);
      check_equality<T>(SharingPolicy::get(v[2]), iter2[0]);

      auto iter3 = iter2 - 2;
      check_equality<T>(SharingPolicy::get(v[0]), iter3[0]);

      check(iter == iter3);
      check(iter2 != iter3);

      check(iter2 > iter);
      check(iter < iter2);
      check(iter >= iter3);
      check(iter <= iter3);

      check_equality<std::ptrdiff_t>(-2, distance(iter2, iter3));
      check_equality<std::ptrdiff_t>(2, distance(iter, iter2));

      auto std_iter = iter.base_iterator();

      check_equality<T>(1, SharingPolicy::get(*std_iter));
    }

    template<class Traits, class SharingPolicy, template<class> class ReferencePolicy, class Arg>
    void partitioned_data_test::test_generic_iterator_deref(const Arg& v)
    {
      using namespace data_structures;
      using T = typename SharingPolicy::elementary_type;

      auto vec(v);
      using p_i_t
        = utilities::iterator<
            typename partition_impl::partition_iterator_generator<Traits, SharingPolicy, ReferencePolicy, false>::iterator,
          partition_impl::dereference_policy<SharingPolicy, ReferencePolicy, partition_impl::partition_index_policy<false, std::size_t>>
          >;

      p_i_t iter(vec.begin(), 0u);

      check_equality<typename T::first_type>(SharingPolicy::get(v[0]).first, iter->first);
      check_equality<typename T::second_type>(SharingPolicy::get(v[0]).first, iter->second);
    }

    template<class T, class SharingPolicy, bool ThrowOnRangeError>
    void partitioned_data_test::test_contiguous_capacity()
    {
      using namespace data_structures;
       
      contiguous_storage<T, SharingPolicy> s{};
      check_equality(s.capacity(), 0ul, LINE(""));
      check_equality(s.num_partitions_capacity(), 0ul, LINE(""));

      s.reserve(4);
      check_equality(s.capacity(), 4ul, LINE(""));
      check_equality(s.num_partitions_capacity(), 0ul, LINE(""));

      s.reserve_partitions(8);
      check_equality(s.capacity(), 4ul, LINE(""));
      check_equality(s.num_partitions_capacity(), 8ul, LINE(""));

      s.shrink_to_fit();
      check_equality(s.capacity(), 0ul, LINE(""));
      check_equality(s.num_partitions_capacity(), 0ul, LINE(""));
    }

    template<class T, class SharingPolicy, bool ThrowOnRangeError>
    void partitioned_data_test::test_bucketed_capacity()
    {
      using namespace data_structures;
       
      bucketed_storage<T, SharingPolicy> s{};

      check_equality(s.num_partitions_capacity(), 0ul, LINE(""));
      if constexpr(ThrowOnRangeError) check_exception_thrown<std::out_of_range>([&s](){ return s.partition_capacity(0); }, LINE(""));

      s.reserve_partitions(4);
      check_equality(s.num_partitions_capacity(), 4ul, LINE(""));
      if constexpr(ThrowOnRangeError) check_exception_thrown<std::out_of_range>([&s](){ return s.partition_capacity(0); }, LINE(""));

      s.shrink_num_partitions_to_fit();
      check_equality(s.num_partitions_capacity(), 0ul, LINE("May fail if shrink to fit impl does not reduce capacity"));
      if constexpr(ThrowOnRangeError) check_exception_thrown<std::out_of_range>([&s](){ return s.partition_capacity(0); }, LINE(""));

      s.add_slot();
      check_equality(s.partition_capacity(0), 0ul, LINE(""));
      if constexpr(ThrowOnRangeError) check_exception_thrown<std::out_of_range>([&s](){ return s.partition_capacity(1); }, LINE(""));

      s.reserve_partition(0, 4);
      check_equality(s.partition_capacity(0), 4ul, LINE(""));
       
      s.shrink_to_fit(0);
      check_equality(s.partition_capacity(0), 0ul, LINE("May fail if shrink to fit impl does not reduce capacity"));
      if constexpr(ThrowOnRangeError) check_exception_thrown<std::out_of_range>([&s](){ s.shrink_to_fit(1); }, LINE(""));
    }
  }
}
