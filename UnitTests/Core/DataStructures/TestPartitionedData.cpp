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

      check(storage1 == storage2);
      check(storage2 == storage1);
      check(!(storage1 != storage2));
      check(!(storage2 != storage1));

      auto storage3 = test_generic_storage<bucketed_storage<int, independent<int>>>();
      auto storage4 = test_generic_storage<contiguous_storage<int, independent<int>>>();

      check(storage3 == storage4);
    }
  }
}
