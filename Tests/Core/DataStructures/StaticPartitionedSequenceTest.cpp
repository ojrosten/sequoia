////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StaticPartitionedSequenceTest.hpp"
#include "PartitionedDataTestingUtilities.hpp"
#include "Utilities/TestUtilities.hpp"

namespace sequoia::testing
{
  using namespace data_structures;

  namespace
  {
    template<std::size_t Npartitions, std::size_t Nelements>
    using byte_indexed_sequence
      = static_partitioned_sequence<
          int,
          Npartitions,
          Nelements,
          maths::static_monotonic_sequence<std::uint8_t, Npartitions, std::ranges::greater>>;

    template<std::size_t Npartitions, std::size_t Nelements>
    concept byte_indexable = requires { typename byte_indexed_sequence<Npartitions, Nelements>; };
  }

  [[nodiscard]]
  std::filesystem::path static_partitioned_sequence_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void static_partitioned_sequence_test::run_tests()
  {
    test_index_type_constraint();
    test_static_storage();
    test_index_type_limit();
  }

  void static_partitioned_sequence_test::test_index_type_constraint()
  {
    constexpr std::size_t limit{std::numeric_limits<std::uint8_t>::max()};

    STATIC_CHECK( byte_indexable<1, limit>);
    STATIC_CHECK(!byte_indexable<1, limit + 1>);

    STATIC_CHECK( byte_indexable<limit, 0>);
    STATIC_CHECK(!byte_indexable<limit + 1, 0>);
  }

  void static_partitioned_sequence_test::test_static_storage()
  {
    {
      using prediction_t = std::initializer_list<std::initializer_list<int>>;

      using storage_t = static_partitioned_sequence<int, 1, 1>;
      storage_t storage{{5}};
      check(equivalence, "", storage, prediction_t{{5}});

      storage_t storage2{{3}};
      check(equivalence, "", storage2, prediction_t{{3}});

      check_semantics("", storage, storage2);

      auto iter = storage.begin_partition(0);
      *iter = 4;
      check(equality, "", storage, storage_t{{4}});
    }

    {
      using prediction_t = std::initializer_list<std::initializer_list<double>>;

      using storage_t = static_partitioned_sequence<double, 3, 6>;
      storage_t storage{{1.2, 1.1}, {2.6, -7.8}, {0.0, -9.3}};
      check(equivalence, "", storage, prediction_t{{1.2, 1.1}, {2.6, -7.8}, {0.0, -9.3}});

      storage_t storage2{{1.2}, {1.1, 2.6, -7.8}, {0.0, -9.3}};
      check(equivalence, "", storage2, prediction_t{{1.2}, {1.1, 2.6, -7.8}, {0.0, -9.3}});

      check_semantics("", storage, storage2);
    }

    {
      using prediction_t = std::initializer_list<std::initializer_list<double>>;

      using storage_t = static_partitioned_sequence<double, 2, 6>;
      storage_t storage{{0.2,0.3,-0.4}, {0.8,-1.1,-1.4}}, storage2{{0.2,0.3}, {-0.4, 0.8,-1.1,-1.4}};

      check(equivalence, "", storage, prediction_t{{0.2,0.3,-0.4}, {0.8,-1.1,-1.4}});
      check(equivalence, "", storage2, prediction_t{{0.2,0.3}, {-0.4, 0.8,-1.1,-1.4}});

      check_semantics("", storage, storage2);
    }

    {
      using ndc = no_default_constructor;
      using prediction_t = std::initializer_list<std::initializer_list<ndc>>;

      {
        using storage_t = static_partitioned_sequence<no_default_constructor, 1, 1>;
        constexpr storage_t storage{{ndc{1}}};

        check(equivalence, "", storage, prediction_t{{ndc{1}}});
      }

      {
        using storage_t = static_partitioned_sequence<no_default_constructor, 3, 5>;
        constexpr storage_t storage2{{ndc{1}, ndc{1}}, {ndc{0}}, {ndc{2}, ndc{4}}};
        check(equivalence, "", storage2, prediction_t{{ndc{1}, ndc{1}}, {ndc{0}}, {ndc{2}, ndc{4}}});
      }
    }
  }

  void static_partitioned_sequence_test::test_index_type_limit()
  {
    constexpr std::size_t limit{std::numeric_limits<std::uint8_t>::max()};

    using one_partition  = byte_indexed_sequence<1, limit>;
    using two_partitions = byte_indexed_sequence<2, limit>;

    const auto makeOnePartition{
      [] <std::size_t... Is> (std::index_sequence<Is...>) {
        return one_partition{{static_cast<int>(Is)...}};
      }
    };

    const auto makeTwoEqualPartitions{
      [] <std::size_t... Is> (std::index_sequence<Is...>) {
        return two_partitions{{static_cast<int>(Is)...}, {static_cast<int>(Is)...}};
      }
    };

    check(equality,
          "A partition as long as the index type counts is admitted",
          makeOnePartition(std::make_index_sequence<limit>{}).size_of_partition(0),
          limit);

    check_exception_thrown<std::out_of_range>(
      "A partition longer than the index type counts throws",
      [&makeOnePartition]() { return makeOnePartition(std::make_index_sequence<limit + 1>{}); }
    );

    check_exception_thrown<std::out_of_range>(
      "Partitions each within the index type, but together beyond it, throw",
      [&makeTwoEqualPartitions]() { return makeTwoEqualPartitions(std::make_index_sequence<limit / 2 + 1>{}); }
    );
  }
}
