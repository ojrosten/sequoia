#include "TestPartitionedData.hpp"

#include <array>
#include <complex>

namespace sequoia
{
  namespace unit_testing
  {
    void test_partitioned_data::run_tests()
    {
      using namespace data_sharing;
      using namespace data_structures;
      test_iterators<independent>();
      test_iterators<shared>();
      test_storage();
      test_init_lists<bucketed_storage, int>({{1}, {2}});
      test_init_lists<contiguous_storage, int>({{1}, {2}});

      test_init_lists<bucketed_storage, std::complex<double>>({{{1.0, 5.7}, {8.7, 3.3}}, {{2.0, -5.7}, {-0.7, 3.2}, {1.0, -1.0}}});
      test_init_lists<contiguous_storage, std::complex<double>>({{{1.0, 5.7}, {8.7, 3.3}}, {{2.0, -5.7}, {-0.7, 3.2}, {1.0, -1.0}}});

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


    template<template<class...> class Storage, class T>
    void test_partitioned_data::test_init_lists(std::initializer_list<std::initializer_list<T>> list)
    {
      using namespace data_structures;
      Storage<T,data_sharing::shared<T>> s{list};
      check_partitions(s, list);
    }

    template<class S, class T>
    bool test_partitioned_data::check_partitions(S& storage, std::initializer_list<std::initializer_list<T>> answers)
    {
      std::vector<std::vector<T>> ans;
      for(const auto& list : answers)
      {
        ans.emplace_back();
        for(const auto& element : list)
        {
          ans.back().emplace_back(element);
        }
      }

      return check_partitions(storage, ans);
    }

    template<class S, class T>
    bool test_partitioned_data::check_partitions(S& storage, const std::vector<std::vector<T>>& answers)
    {
      const auto numFailures{this->failures()};
        
      using p_iter = typename S::partition_iterator;
      using cp_iter = typename S::const_partition_iterator;
      using rp_iter = typename S::reverse_partition_iterator;
      using crp_iter = typename S::const_reverse_partition_iterator;

      const std::size_t numPartitions{answers.size()};
      check_equality<std::size_t>(numPartitions, storage.num_partitions(), "Storage does not have expected number of partitions");
      std::size_t nElements{};
      for(const auto& ans : answers)
      {
        nElements += ans.size();
      }
      check_equality<std::size_t>(nElements, storage.size(), "Storage does not have the expected number of elements");

      if(numPartitions == storage.num_partitions())
      {
        for(auto ansIter = answers.cbegin(); ansIter != answers.cend(); ++ansIter)
        {
          const std::size_t partitionIndex{static_cast<std::size_t>(std::distance(answers.cbegin(), ansIter))};
          const std::size_t partitionSize{ansIter->size()};
          const std::string indexStr{std::to_string(partitionIndex)};
          check_equality<std::size_t>(partitionSize, distance(storage.cbegin_partition(partitionIndex), storage.cend_partition(partitionIndex)), "Partition " + indexStr + " not of expceted size");
          {
            p_iter piter{storage.begin_partition(partitionIndex)};
            cp_iter cpiter{storage.cbegin_partition(partitionIndex)};
            for(auto aiter = ansIter->cbegin(); aiter != ansIter->cend(); ++aiter, ++piter, ++cpiter)
            {
              check_equality<T>(*aiter, *piter, "Results wrong for forward iteration through partition " + indexStr);
              check_equality<T>(*aiter, *cpiter, "Results wrong for const forward iteration through partition " + indexStr);
            }
          }
          {
            rp_iter rpiter{storage.rbegin_partition(partitionIndex)};
            crp_iter crpiter{storage.crbegin_partition(partitionIndex)};
            for(auto ariter = ansIter->crbegin(); ariter != ansIter->crend(); ++ariter, ++rpiter, ++crpiter)
            {
              check_equality<T>(*ariter, *rpiter, "Results wrong for reverse iteration through partition " + indexStr);
              check_equality<T>(*ariter, *crpiter, "Results wrong for const reverse iteration through partition " + indexStr);
            }
          }
        }
      }

      return this->failures() == numFailures;
    }

    template<class S, class T>
    bool test_partitioned_data::check_constexpr_partitions(S& storage, std::initializer_list<std::initializer_list<T>> answers)
    {
      std::vector<std::vector<T>> ans;
      for(const auto& list : answers)
      {
        ans.emplace_back();
        for(const auto& element : list)
        {
          ans.back().emplace_back(element);
        }
      }

      return check_constexpr_partitions(storage, ans);
    }

    template<class S, class T>
    bool test_partitioned_data::check_constexpr_partitions(const S& storage, const std::vector<std::vector<T>>& answers)
    {
      const auto numFailures{this->failures()};
        
      using cp_iter = typename S::const_partition_iterator;
      using crp_iter = typename S::const_reverse_partition_iterator;

      const std::size_t numPartitions{answers.size()};
      check_equality<std::size_t>(numPartitions, storage.num_partitions(), "Storage does not have expected number of partitions");
      std::size_t nElements{0};
      for(const auto& ans : answers)
      {
        nElements += ans.size();
      }
      check_equality<std::size_t>(nElements, storage.size(), "Storage does not have the expected number of elements");

      if(numPartitions == storage.num_partitions())
      {
        for(auto ansIter = answers.cbegin(); ansIter != answers.cend(); ++ansIter)
        {
          const std::size_t partitionIndex{static_cast<std::size_t>(std::distance(answers.cbegin(), ansIter))};
          const std::size_t partitionSize{ansIter->size()};
          const std::string indexStr{std::to_string(partitionIndex)};
          check_equality<std::size_t>(partitionSize, distance(storage.cbegin_partition(partitionIndex), storage.cend_partition(partitionIndex)), "Partition " + indexStr + " not of expceted size");
          {
            cp_iter cpiter{storage.cbegin_partition(partitionIndex)};
            for(auto aiter = ansIter->cbegin(); aiter != ansIter->cend(); ++aiter, ++cpiter)
            {
              check_equality<T>(*aiter, *cpiter, "Results wrong for const forward iteration through partition " + indexStr);
            }
          }
          {
            crp_iter crpiter{storage.crbegin_partition(partitionIndex)};
            for(auto ariter = ansIter->crbegin(); ariter != ansIter->crend(); ++ariter,++crpiter)
            {
              check_equality<T>(*ariter, *crpiter, "Results wrong for const reverse iteration through partition " + indexStr);
            }
          }
        }
      }

      return this->failures() == numFailures;
    }

    void test_partitioned_data::test_static_storage()
    {
      using namespace data_structures;

      {
        static_contiguous_storage<int, 1, 1> storage{{5}};
        check_partitions(storage, {{5}});

        static_contiguous_storage<int, 1, 1> storage2{{3}};
        check_partitions(storage2, {{3}});

        std::swap(storage, storage2);
        check_partitions(storage, {{3}});
        check_partitions(storage2, {{5}});

        check(storage != storage2);
        auto iter = storage.begin_partition(0);
        check_equality<int>(3, *iter);
        *iter = 5;
        check_equality<int>(5, *iter);
        check(storage == storage2);
      }

      {
        static_contiguous_storage<double, 3, 6> storage{{1.2, 1.1}, {2.6, -7.8}, {0.0, -9.3}};
        check_partitions(storage, {{1.2, 1.1}, {2.6, -7.8}, {0.0, -9.3}});

        static_contiguous_storage<double, 3, 6> storage2{{1.2}, {1.1, 2.6, -7.8}, {0.0, -9.3}};
        check_partitions(storage2, {{1.2}, {1.1, 2.6, -7.8}, {0.0, -9.3}});

        check(storage != storage2);
      }

      {
        static_contiguous_storage<double, 2, 6> storage{{0.2,0.3,-0.4}, {0.8,-1.1,-1.4}};
        check_partitions(storage, {{0.2,0.3,-0.4}, {0.8,-1.1,-1.4}});

        auto storage2 = storage;
        check_partitions(storage2, {{0.2,0.3,-0.4}, {0.8,-1.1,-1.4}});
      }

      {
        using ndc = no_default_constructor;
        constexpr static_contiguous_storage<no_default_constructor, 1, 1> storage{{1}};
        check_constexpr_partitions(storage, {{ndc{1}}});

        constexpr static_contiguous_storage<no_default_constructor, 3, 5> storage2{{1, 1}, {0}, {2, 4}};
        check_constexpr_partitions(storage2, {{ndc{1}, ndc{1}}, {ndc{0}}, {ndc{2}, ndc{4}}});
      }
    }
    
    void test_partitioned_data::test_storage()
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
    Storage test_partitioned_data::test_generic_storage()
    {
      using namespace data_structures;
      using namespace data_sharing;
      using answers_type = std::vector<std::vector<typename Storage::value_type>>;

      constexpr bool sharedData{std::is_same<typename Storage::sharing_policy_type, shared<typename Storage::value_type>>::value};

      const std::string prefix{type_to_string<Storage>::str()};
      this->failure_message_prefix(prefix);

      Storage storage;

      // Null

      check_exception_thrown<std::out_of_range>([&storage]() { storage.push_back_to_partition(0, 8); }, "No partition available to push back to");
      check_exception_thrown<std::out_of_range>([&storage]() { storage.insert_to_partition(storage.cbegin_partition(0), 1); }, "No partition available to insert into");
      check_equality<std::size_t>(0, storage.size());

      check_equality<std::size_t>(0, storage.num_partitions());
      check_equality<std::size_t>(0, storage.erase_slot(0));

      storage.add_slot();
      check_equality<std::size_t>(1, storage.num_partitions());
      // []

      check_exception_thrown<std::out_of_range>([&storage]() { storage.push_back_to_partition(1, 7); }, "Only one partition available so cannot push back to the second");

      check_equality<std::size_t>(0, storage.erase_slot(1));
      check_equality<std::size_t>(0, storage.erase_slot(0));
      check_equality<std::size_t>(0, storage.num_partitions());


      // Null

      storage.add_slot();
      storage.add_slot();
      check_equality<std::size_t>(2, storage.num_partitions());
      // [][]

      storage.push_back_to_partition(0, 2);
      check_equality<std::size_t>(1, storage.size());
      check_equality<int>(2, storage[0][0]);
      // [2][]

      check_partitions(storage, answers_type{{2}, {}});

      auto iter = storage.begin_partition(0);
      check_equality<int>(2, *iter);
      *iter = 3;
      // [3][]

      check_partitions(storage, answers_type{{3}, {}});

      storage.push_back_to_partition(1, 4);
      // [3][4]

      check_partitions(storage, answers_type{{3}, {4}});

      storage.add_slot();
      storage.push_back_to_partition(2, 9);
      storage.push_back_to_partition(2, -3);
      // [3][4][9,-3]

      check_partitions(storage, answers_type{{3}, {4}, {9, -3}});

      storage.insert_to_partition(storage.cbegin_partition(2), 2);
      // [3][4][2, 9,-3]

      check_partitions(storage, answers_type{{3}, {4}, {2, 9, -3}});

      storage.insert_to_partition(storage.cbegin_partition(2) + 1, 8);
      // [3][4][2, 8, 9,-3]

      check_partitions(storage, answers_type{{3}, {4}, {2, 8, 9, -3}});

      storage.insert_to_partition(storage.begin_partition(2) + 4, 7);
      // [3][4][2, 8, 9,-3, 7]

      check_partitions(storage, answers_type{{3}, {4}, {2, 8, 9, -3, 7}});

      storage.insert_to_partition(storage.cbegin_partition(2) + 5, 5);
      // [3][4][2, 8, 9,-3, 7, 5]

      check_partitions(storage, answers_type{{3}, {4}, {2, 8, 9, -3, 7, 5}});

      storage.add_slot();
      storage.insert_to_partition(storage.cbegin_partition(2), 1);
      // [3][4][1, 2, 8, 9,-3, 7, 5][]

      check_partitions(storage, answers_type{{3}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}});

      storage.insert_to_partition(storage.cbegin_partition(0), 12);
      // [12, 3][4][1, 2, 8, 9,-3, 7, 5][]

      check_partitions(storage, answers_type{{12, 3}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}});

      storage.insert_to_partition(storage.begin_partition(0) + 1, 13);
      // [12, 13, 3][4][1, 2, 8, 9,-3, 7, 5][]

      check_partitions(storage, answers_type{{12, 13, 3}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}});

      storage.insert_to_partition(storage.cbegin_partition(0) + 3, -8);
      // [12, 13, 3, -8][4][1, 2, 8, 9,-3, 7, 5][]

      check_partitions(storage, answers_type{{12, 13, 3, -8}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}});

      storage.erase_from_partition(0, 0);
      // [13, 3, -8][4][1, 2, 8, 9,-3, 7, 5][]

      check_partitions(storage, answers_type{{13, 3, -8}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}});

      storage.erase_from_partition(0, 2);
      // [13, 3][4][1, 2, 8, 9,-3, 7, 5][]

      check_partitions(storage, answers_type{{13, 3}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}});

      storage.erase_from_partition(storage.cbegin_partition(0));
      // [3][4][1, 2, 8, 9,-3, 7, 5][]

      check_partitions(storage, answers_type{{3}, {4}, {1, 2, 8, 9, -3, 7, 5}, {}});

      storage.insert_to_partition(storage.cbegin_partition(2), -2);
      storage.insert_to_partition(storage.cbegin_partition(3), -2);
      // [3][4][-2, 1, 2, 8, 9,-3, 7, 5][-2]

      check_partitions(storage, answers_type{{3}, {4}, {-2, 1, 2, 8, 9, -3, 7, 5}, {-2}});

      storage.insert_to_partition(storage.cbegin_partition(3) + 1,-4);
      storage.insert_to_partition(storage.cbegin_partition(3), -4);
      // [3][4][-2, 1, 2, 8, 9,-3, 7, 5][-4, -2,-4]

      check_partitions(storage, answers_type{{3}, {4}, {-2, 1, 2, 8, 9, -3, 7, 5}, {-4, -2, -4}});

      check_equality<std::size_t>(3, storage.erase_slot(3));
      check_equality<std::size_t>(8, storage.erase_slot(2));
      // [3][4]

      check_partitions(storage, answers_type{{3}, {4}});

      check_equality<std::size_t>(1, storage.insert_slot(1));
      // [3][][4]

      check_partitions(storage, answers_type{{3}, {}, {4}});

      check_equality<std::size_t>(0, storage.insert_slot(0));
      // [][3][][4]

      check_partitions(storage, answers_type{{}, {3}, {}, {4}});

      storage.insert_to_partition(storage.cbegin_partition(0), 1);
      storage.insert_to_partition(storage.cbegin_partition(2), storage.cbegin_partition(0));
      // [1][3][1][4]

      if(check_partitions(storage, answers_type{{1}, {3}, {1}, {4}}))
      {
        auto iter = storage.begin_partition(0);
        *iter = 2;
        // shared: [2][3][2][4], independent: [2][3][1][4]
        check_partitions(storage, sharedData ? answers_type{{2}, {3}, {2}, {4}} : answers_type{{2}, {3}, {1}, {4}});
        *iter = 1;
        // [1][3][1][4]
        check_partitions(storage, answers_type{{1}, {3}, {1}, {4}});

        iter = storage.begin_partition(2);
        *iter = -2;
        // shared: [-2][3][-2][4], independent: [-2][3][1][4]
        check_partitions(storage, sharedData ? answers_type{{-2}, {3}, {-2}, {4}} : answers_type{{1}, {3}, {-2}, {4}});
        *iter = 1;
        // [1][3][1][4]
        check_partitions(storage, answers_type{{1}, {3}, {1}, {4}});
      }

      storage.insert_to_partition(storage.cbegin_partition(0), 2);
      storage.insert_to_partition(storage.cbegin_partition(0) + 2, storage.cbegin_partition(0));
      // [2,1,2][3][1][4]

      check_partitions(storage, answers_type{{2, 1, 2}, {3}, {1}, {4}});

      storage.insert_to_partition(storage.cbegin_partition(2), 7);
      // [2,1,2][3][1, 7][4]

      check_partitions(storage, answers_type{{2, 1, 2}, {3}, {7,1}, {4}});

      check_equality<std::size_t>(2, storage.erase_slot(2));
      // [2,1,2][3][4]

      check_partitions(storage, answers_type{{2,1,2}, {3}, {4}});

      check_equality<std::size_t>(3, storage.erase_slot(0));
      // [3][4]

      check_partitions(storage, answers_type{{3}, {4}});

      storage.push_back_to_partition(0, -5);       
      storage.push_back_to_partition(1, --storage.cend_partition(0));
      // [3,-5][4,-5]

      if(check_partitions(storage, answers_type{{3, -5}, {4, -5}}))
      {
        auto iter = storage.begin_partition(0);
        ++iter;
        *iter = 2;
        // shared [3,2][4,2], independent: [3,2][4,-5]
        check_partitions(storage, sharedData ? answers_type{{3, 2}, {4, 2}} : answers_type{{3, 2}, {4, -5}});

        *iter = -5;
        // [3,-5][4,-5]
        check_partitions(storage, answers_type{{3, -5}, {4, -5}});

        iter = storage.begin_partition(1);
        ++iter;
        *iter = -2;
        // shared [3,-2][4,-2], independent: [3,-5][4,-2]
        check_partitions(storage, sharedData ? answers_type{{3, -2}, {4, -2}} : answers_type{{3, -5}, {4, -2}});

        *iter = -5;
        // [3,-5][4,-5]
        check_partitions(storage, answers_type{{3, -5}, {4, -5}});
      }

      storage.add_slot();
      storage.push_back_to_partition(2, 8);
      storage.push_back_to_partition(2, 9);
      // [3,-5][4,-5][8,9]

      check_partitions(storage, answers_type{{3, -5}, {4, -5}, {8, 9}});

      check_equality<std::size_t>(2, storage.erase_slot(1));
      // [3,-5][8,9]

      check_partitions(storage, answers_type{{3, -5}, {8, 9}});

      check_equality<std::size_t>(2, storage.erase_slot(0));
      // [8,9]

      check_partitions(storage, answers_type{{8, 9}});

      storage.add_slot();
      storage.push_back_to_partition(1, 4);
      storage.push_back_to_partition(1, -5);
      // [8,9][4,-5]

      check_partitions(storage, answers_type{{8, 9}, {4, -5}});

      {
        auto iter = storage.begin_partition(0);
        iter++;
        *iter = -5;
        auto next = storage.erase_from_partition(0, 0);
        check_equality<std::size_t>(-5, *next);
      }
      // [-5][4,-5]

      check_partitions(storage, answers_type{{-5}, {4, -5}});


      {
        auto next = storage.erase_from_partition(0, 2);
        check(next == storage.end_partition(0));
      }
      // [-5][4,-5]

      check_partitions(storage, answers_type{{-5}, {4, -5}});

      {
        auto next = storage.erase_from_partition(1, 1);
        check(next == storage.end_partition(1));
      }
      // [-5][4]

      check_partitions(storage, answers_type{{-5}, {4}});

      storage.push_back_to_partition(0, 6);
      storage.push_back_to_partition(1, --storage.cend_partition(0));
      // [-5,6][4,6]

      check_partitions(storage, answers_type{{-5, 6}, {4, 6}});

      storage.push_back_to_partition(0, -2);
      storage.push_back_to_partition(1, --storage.cend_partition(0));
      // [-5,6,-2][4,6,-2]

      check_partitions(storage, answers_type{{-5, 6, -2}, {4, 6, -2}});

      auto found{std::find_if(storage.begin_partition(0), storage.end_partition(0),
        [](const int& elt) { return elt == 6; }
      )};
      
      if(check(found != storage.end_partition(0)))
        check_equality<std::size_t>(6, *found);

      storage.erase_from_partition_if(0, [](const int& elt) { return elt == 6; });
      // [-5,-2][4,6,-2]

      check_partitions(storage, answers_type{{-5, -2}, {4, 6, -2}});
      
      auto found2{std::find_if(storage.cbegin_partition(0), storage.cend_partition(0),
        [](const int& elt) { return elt == 6; })
      };
      
      check(found2 == storage.cend_partition(0));
      check_equality<std::size_t>(5, storage.size());

      storage.push_back_to_partition(1, 7);
      storage.push_back_to_partition(1, --storage.cend_partition(1));

      // [-5,-2][4,6,-2,7,7]

      check_partitions(storage, answers_type{{-5, -2}, {4, 6, -2, 7, 7}});

      check_equality<std::size_t>(2, storage.erase_from_partition_if(1, [](const int& elt) { return elt == 7; }));
      // [-5,-2][4,6,-2]

      check_partitions(storage, answers_type{{-5, -2}, {4, 6, -2}});

      // now check copy constructor etc

      Storage storage2(storage);

      check(storage == storage2);
      storage2.push_back_to_partition(0, 7);
      // storage2: [-5,-2, 7][4,6,-2]
      check(storage != storage2);

      // Check original has been unchanged (bearing in mind
      // possible presence of shared_ptrs

      check_partitions(storage, answers_type{{-5, -2}, {4, 6, -2}});

      // Now check new instance

      check_partitions(storage2, answers_type{{-5, -2, 7}, {4, 6, -2}});

      std::swap(storage, storage2);

      check_partitions(storage2, answers_type{{-5, -2}, {4, 6, -2}});
      check_partitions(storage, answers_type{{-5, -2, 7}, {4, 6, -2}});

      storage = storage2;
      check(storage == storage2);

      check_partitions(storage2, answers_type{{-5, -2}, {4, 6, -2}});
      check_partitions(storage, answers_type{{-5, -2}, {4, 6, -2}});

      return storage;
    }

    template<template<class> class SharingPolicy>
    void test_partitioned_data::test_iterators()
    {
      using namespace data_structures;

      {
        std::vector<typename SharingPolicy<int>::handle_type> vec{SharingPolicy<int>::make(1), SharingPolicy<int>::make(2), SharingPolicy<int>::make(3)};
        test_generic_iterator_properties<std::vector, SharingPolicy<int>, partition_impl::mutable_reference>(vec);
        test_generic_iterator_properties<std::vector, SharingPolicy<int>, partition_impl::const_reference>(vec);
      }

      {
        using data = std::pair<bool, double>;

        std::vector<typename SharingPolicy<data>::handle_type> vec{SharingPolicy<data>::make(true, 1.0)};

        test_generic_iterator_deref<std::vector, SharingPolicy<std::pair<bool, double>>, partition_impl::mutable_reference>(vec);
        test_generic_iterator_deref<std::vector, SharingPolicy<std::pair<bool, double>>, partition_impl::const_reference>(vec);
      }
    }

    template<template<class...> class C, class SharingPolicy, template<class> class ReferencePolicy, class Arg>
    void test_partitioned_data::test_generic_iterator_properties(const Arg& v)
    {
      using namespace data_structures;
      using T = typename SharingPolicy::elementary_type;

      auto vec(v);

      using p_i_t = utilities::iterator<typename partition_impl::partition_iterator_generator<C, SharingPolicy, ReferencePolicy, false>::iterator, partition_impl::dereference_policy<SharingPolicy, ReferencePolicy>, partition_impl::partition_index_policy<false, std::size_t>>;

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

    template<template<class...> class C, class SharingPolicy, template<class> class ReferencePolicy, class Arg>
    void test_partitioned_data::test_generic_iterator_deref(const Arg& v)
    {
      using namespace data_structures;
      using T = typename SharingPolicy::elementary_type;

      auto vec(v);
      using p_i_t = utilities::iterator<typename partition_impl::partition_iterator_generator<C, SharingPolicy, ReferencePolicy, false>::iterator, partition_impl::dereference_policy<SharingPolicy, ReferencePolicy>, partition_impl::partition_index_policy<false, std::size_t>>;

      p_i_t iter(vec.begin(), 0u);

      check_equality<typename T::first_type>(SharingPolicy::get(v[0]).first, iter->first);
      check_equality<typename T::second_type>(SharingPolicy::get(v[0]).first, iter->second);
    }

    template<class T, class SharingPolicy, bool ThrowOnRangeError>
    void test_partitioned_data::test_contiguous_capacity()
    {
      using namespace data_structures;
       
      contiguous_storage<T, SharingPolicy> s{};
      check_equality<std::size_t>(0, s.capacity(), LINE(""));
      check_equality<std::size_t>(0, s.num_partitions_capacity(), LINE(""));

      s.reserve(4);
      check_equality<std::size_t>(4, s.capacity(), LINE(""));
      check_equality<std::size_t>(0, s.num_partitions_capacity(), LINE(""));

      s.reserve_partitions(8);
      check_equality<std::size_t>(4, s.capacity(), LINE(""));
      check_equality<std::size_t>(8, s.num_partitions_capacity(), LINE(""));

      s.shrink_to_fit();
      check_equality<std::size_t>(0, s.capacity(), LINE(""));
      check_equality<std::size_t>(0, s.num_partitions_capacity(), LINE(""));
    }

    template<class T, class SharingPolicy, bool ThrowOnRangeError>
    void test_partitioned_data::test_bucketed_capacity()
    {
      using namespace data_structures;
       
      bucketed_storage<T, SharingPolicy> s{};

      check_equality<std::size_t>(0, s.num_partitions_capacity(), LINE(""));
      if constexpr(ThrowOnRangeError) check_exception_thrown<std::out_of_range>([&s](){ s.partition_capacity(0); }, LINE(""));

      s.reserve_partitions(4);
      check_equality<std::size_t>(4, s.num_partitions_capacity(), LINE(""));
      if constexpr(ThrowOnRangeError) check_exception_thrown<std::out_of_range>([&s](){ s.partition_capacity(0); }, LINE(""));

      s.shrink_num_partitions_to_fit();
      check_equality<std::size_t>(0, s.num_partitions_capacity(), LINE("May fail if shrink to fit impl does not reduce capacity"));
      if constexpr(ThrowOnRangeError) check_exception_thrown<std::out_of_range>([&s](){ s.partition_capacity(0); }, LINE(""));

      s.add_slot();
      check_equality<std::size_t>(0, s.partition_capacity(0), LINE(""));
      if constexpr(ThrowOnRangeError) check_exception_thrown<std::out_of_range>([&s](){ s.partition_capacity(1); }, LINE(""));

      s.reserve_partition(0, 4);
      check_equality<std::size_t>(4, s.partition_capacity(0), LINE(""));
       
      s.shrink_to_fit(0);
      check_equality<std::size_t>(0, s.partition_capacity(0), LINE("May fail if shrink to fit impl does not reduce capacity"));
      if constexpr(ThrowOnRangeError) check_exception_thrown<std::out_of_range>([&s](){ s.shrink_to_fit(1); }, LINE(""));
    }
  }
}
