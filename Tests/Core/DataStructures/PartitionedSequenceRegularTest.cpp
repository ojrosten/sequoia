////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PartitionedSequenceRegularTest.hpp"
#include "ElementTestingUtilities.hpp"
#include "PartitionedDataGenericTests.hpp"

#include "sequoia/Core/DataStructures/PartitionedData.hpp"

#include <forward_list>
#include <map>
#include <memory>
#include <span>

namespace sequoia::testing
{
  namespace
  {
    struct non_assignable_element
    {
      const int value{};
    };

    struct move_only_element
    {
      int value{};

      move_only_element() = default;

      move_only_element(move_only_element&&) noexcept = default;

      move_only_element& operator=(move_only_element&&) noexcept = default;
    };

    struct assign_only_element
    {
      int value{};

      assign_only_element() = default;

      assign_only_element(const assign_only_element&) = delete;

      assign_only_element& operator=(const assign_only_element&) = default;
    };

    template<class T>
    struct copy_propagating_allocator
    {
      using value_type                             = T;
      using propagate_on_container_copy_assignment = std::true_type;

      constexpr copy_propagating_allocator() = default;

      template<class U>
      constexpr copy_propagating_allocator(const copy_propagating_allocator<U>&) noexcept {}

      [[nodiscard]]
      constexpr T* allocate(const std::size_t n) { return std::allocator<T>{}.allocate(n); }

      constexpr void deallocate(T* p, const std::size_t n) { std::allocator<T>{}.deallocate(p, n); }

      [[nodiscard]]
      friend constexpr bool operator==(const copy_propagating_allocator&, const copy_propagating_allocator&) noexcept = default;
    };

    using namespace partitioned_data;

    // A standard container's size type is std::size_t; these carry only what the constraint reads.
    template<std::integral SizeType>
    struct fake_container
    {
      using size_type = SizeType;
    };

    template<std::integral IndexType, std::integral SizeType>
    struct fake_partitions
    {
      using value_type     = IndexType;
      using size_type      = SizeType;
      using compare_type   = std::ranges::greater;
      using const_iterator = const IndexType*;

      const_iterator cbegin() const;
      const_iterator cend() const;

      void translate(const_iterator, const_iterator, std::ptrdiff_t);
    };

    struct untranslatable_partitions : fake_partitions<std::size_t, std::size_t>
    {
      void translate(const_iterator, const_iterator, std::ptrdiff_t) = delete;
    };

    struct equal_bound_refusing_partitions : fake_partitions<std::size_t, std::size_t>
    {
      using compare_type = std::ranges::greater_equal;
    };

    struct rising_bound_refusing_partitions : fake_partitions<std::size_t, std::size_t>
    {
      using compare_type = std::ranges::less;
    };

    // Its end is a sentinel, while translate takes two iterators, as the members pass them
    struct sentinel_ended_partitions : fake_partitions<std::size_t, std::size_t>
    {
      struct sentinel {};

      sentinel cend() const;
    };

    struct runtime_comparison
    {
      bool operator()(std::size_t a, std::size_t b) const { return a > b; }
    };

    struct runtime_compared_partitions : fake_partitions<std::size_t, std::size_t>
    {
      using compare_type = runtime_comparison;
    };

    using greater_bounds = maths::monotonic_sequence<std::size_t, std::ranges::greater>;

    // Its bounds are reachable only through cbegin and cend
    struct const_reachable_partitions : greater_bounds
    {
      using greater_bounds::greater_bounds;

      const_iterator begin() const = delete;
      const_iterator end() const = delete;
    };

    // Overwritable only from a span, to which a view of the bounds does not convert
    struct span_overwritten_partitions : greater_bounds
    {
      using greater_bounds::greater_bounds;

      void overwrite(std::span<const std::size_t> values);
    };

    template<class Container, class Partitions>
    concept sequence_admits = requires { typename data_structures::partitioned_sequence<int, Container, Partitions>; };

    template<class PartitionedData>
    struct partitioned_operations : partitioned_data_operations<PartitionedData>
    {
      using data_type = partitioned_data_operations<PartitionedData>::data_type;

      static void execute(regular_test& t)
      {
        auto trg{partitioned_data_operations<PartitionedData>::make_transition_graph(t)};

        // begin 'empty'
        trg.join(data_description::empty,
          data_description::empty,
          t.report(""),
          [&t](data_type d) -> data_type {
            t.check(equality, "", d.capacity(), 0uz);
            t.check(equality, "", d.num_partitions_capacity(), 0uz);

            d.reserve(4);
            t.check(equality, "", d.capacity(), 4uz);
            t.check(equality, "", d.num_partitions_capacity(), 0uz);

            d.reserve_partitions(8);
            t.check(equality, "", d.capacity(), 4uz);
            t.check(equality, "", d.num_partitions_capacity(), 8uz);

            d.shrink_to_fit();
            t.check(equality, "May fail if shrink to fit impl does not reduce capacity", d.capacity(), 0uz);
            t.check(equality, "May fail if shrink to fit impl does not reduce capacity", d.num_partitions_capacity(), 0uz);

            return d;
          }
        );

        // end 'empty'
        // begin 'empty_partition'

        trg.join(data_description::empty_partition,
                 data_description::empty_partition,
                 t.report(""),
                 [&t](data_type d) -> data_type {
                   auto i{d.erase_from_partition(d.cbegin_partition(1))};
                   t.check(equality, "Erase from non-existent partition", i, d.begin_partition(1));
                   return d;
                 }
          );

        // end 'empty_partition'

        auto checker{
            [&t](std::string_view description, const data_type& obtained, const data_type& prediction, const data_type& parent, std::size_t host, std::size_t target) {
              t.check(equality, {description, no_source_location}, obtained, prediction);
              if(host != target) t.check_semantics({description, no_source_location}, prediction, parent);
            }
        };

        transition_checker<data_type>::check(t.report(""), trg, checker);
      }
    };
  }

  [[nodiscard]]
  std::filesystem::path partitioned_sequence_regular_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void partitioned_sequence_regular_test::run_tests()
  {
    using namespace data_structures;
    test_copyability();
    test_index_type_constraint();
    test_partitions_constraint();
    test_allocator_extended_move();
    test_reset();
    test_permute_partitions_constraints();
    partitioned_operations<partitioned_sequence<int>>::execute(*this);
  }

  void partitioned_sequence_regular_test::test_permute_partitions_constraints()
  {
    using sequence_t = data_structures::partitioned_sequence<int>;

    STATIC_CHECK( permute_partitions_accepts<sequence_t, std::vector<std::size_t>>);
    STATIC_CHECK(!permute_partitions_accepts<sequence_t, input_only_destinations_t>);
    STATIC_CHECK(!permute_partitions_accepts<sequence_t, non_const_iterable_destinations_t>);
    STATIC_CHECK(!permute_partitions_accepts<sequence_t, std::forward_list<std::size_t>>);
  }

  void partitioned_sequence_regular_test::test_copyability()
  {
    using namespace data_structures;
    using move_only_sequence = partitioned_sequence<move_only_element>;
    using copyable_sequence  = partitioned_sequence<int>;
    using move_only_vector_sequence = partitioned_sequence<std::vector<move_only_element>>;

    STATIC_CHECK(!std::is_copy_constructible_v<move_only_sequence>);
    STATIC_CHECK(!std::is_copy_assignable_v<move_only_sequence>);
    STATIC_CHECK(!std::is_constructible_v<move_only_sequence,
                                          const move_only_sequence&,
                                          move_only_sequence::allocator_type,
                                          move_only_sequence::partitions_allocator_type>);
    STATIC_CHECK( std::is_nothrow_move_constructible_v<move_only_sequence>);

    STATIC_CHECK( std::is_copy_constructible_v<copyable_sequence>);
    STATIC_CHECK( std::is_copy_assignable_v<copyable_sequence>);

    STATIC_CHECK( std::is_copy_constructible_v<partitioned_sequence<non_assignable_element>>);
    STATIC_CHECK(!std::is_copy_assignable_v<partitioned_sequence<non_assignable_element>>);
    STATIC_CHECK( std::is_constructible_v<copyable_sequence,
                                          const copyable_sequence&,
                                          copyable_sequence::allocator_type,
                                          copyable_sequence::partitions_allocator_type>);

    STATIC_CHECK(!std::is_copy_constructible_v<partitioned_sequence<assign_only_element>>);
    STATIC_CHECK(!std::is_copy_assignable_v<partitioned_sequence<assign_only_element>>);
    STATIC_CHECK(!std::is_copy_assignable_v<static_partitioned_sequence<assign_only_element, 1, 1>>);

    STATIC_CHECK(!std::is_copy_constructible_v<move_only_vector_sequence>);
    STATIC_CHECK(!std::is_copy_assignable_v<move_only_vector_sequence>);
    STATIC_CHECK(!std::is_constructible_v<move_only_vector_sequence,
                                          const move_only_vector_sequence&,
                                          move_only_vector_sequence::allocator_type,
                                          move_only_vector_sequence::partitions_allocator_type>);

    STATIC_CHECK( std::is_copy_constructible_v<partitioned_sequence<std::vector<int>>>);
    STATIC_CHECK( std::is_copy_assignable_v<partitioned_sequence<std::vector<int>>>);
    STATIC_CHECK( std::is_copy_assignable_v<partitioned_sequence<std::map<int, int>>>);
  }

  void partitioned_sequence_regular_test::test_index_type_constraint()
  {
    STATIC_CHECK( sequence_admits<std::vector<int>, maths::monotonic_sequence<std::size_t,  std::ranges::greater>>);
    STATIC_CHECK(!sequence_admits<std::vector<int>, maths::monotonic_sequence<std::uint8_t, std::ranges::greater>>);

    STATIC_CHECK( sequence_admits<fake_container<std::uint8_t>,  fake_partitions<std::uint8_t,  std::uint8_t>>);
    STATIC_CHECK( sequence_admits<fake_container<std::uint8_t>,  fake_partitions<std::uint16_t, std::uint8_t>>);
    STATIC_CHECK(!sequence_admits<fake_container<std::uint8_t>,  fake_partitions<std::int8_t,   std::uint8_t>>);
    STATIC_CHECK(!sequence_admits<fake_container<std::uint16_t>, fake_partitions<std::uint8_t,  std::uint8_t>>);
    STATIC_CHECK(!sequence_admits<fake_container<std::uint8_t>,  fake_partitions<std::uint8_t,  std::uint16_t>>);
    STATIC_CHECK(!sequence_admits<fake_container<std::uint8_t>,  fake_partitions<std::int16_t,  std::uint8_t>>);

    // Each index type below counts to both size types, so only its being bool or a character type refuses it.
    STATIC_CHECK(!sequence_admits<fake_container<bool>,        fake_partitions<bool, bool>>);
    STATIC_CHECK(!sequence_admits<fake_container<std::int8_t>, fake_partitions<char, std::int8_t>>);
  }

  void partitioned_sequence_regular_test::test_partitions_constraint()
  {
    STATIC_CHECK( sequence_admits<fake_container<std::size_t>, fake_partitions<std::size_t, std::size_t>>);
    STATIC_CHECK( sequence_admits<fake_container<std::size_t>, sentinel_ended_partitions>);
    STATIC_CHECK(!sequence_admits<fake_container<std::size_t>, untranslatable_partitions>);
    STATIC_CHECK(!sequence_admits<std::vector<int>, maths::monotonic_sequence<std::size_t, last_digit_greater>>);
    STATIC_CHECK(!sequence_admits<fake_container<std::size_t>, equal_bound_refusing_partitions>);
    STATIC_CHECK(!sequence_admits<fake_container<std::size_t>, rising_bound_refusing_partitions>);
    STATIC_CHECK(!sequence_admits<fake_container<std::size_t>, runtime_compared_partitions>);

    using span_overwritten_sequence
      = data_structures::partitioned_sequence<int, std::vector<int>, span_overwritten_partitions>;
    STATIC_CHECK(!permute_partitions_accepts<span_overwritten_sequence, std::vector<std::size_t>>);

    STATIC_CHECK(
      [](){
        data_structures::partitioned_sequence<int, std::vector<int>, const_reachable_partitions> s{};
        s.add_slot();
        s.push_back_to_partition(0, 1);
        return s.size_of_partition(0) == 1;
      }()
    );
  }

  void partitioned_sequence_regular_test::test_allocator_extended_move()
  {
    using sequence = data_structures::partitioned_sequence<constructible_from_anything>;

    STATIC_CHECK(
      [](){
        sequence source{{1, 2}, {3}};
        const sequence moved{std::move(source), sequence::allocator_type{}, sequence::partitions_allocator_type{}};
        return moved == sequence{{1, 2}, {3}};
      }()
    );
  }

  void partitioned_sequence_regular_test::test_reset()
  {
    using element  = constructible_from_anything;
    using sequence = data_structures::partitioned_sequence<element,
                                                           std::vector<element, copy_propagating_allocator<element>>,
                                                           maths::monotonic_sequence<std::size_t,
                                                                                     std::ranges::greater,
                                                                                     std::vector<std::size_t, copy_propagating_allocator<std::size_t>>>>;

    STATIC_CHECK(
      [](){
        sequence toReset{{1, 2}, {3}};
        toReset.reset(sequence::allocator_type{}, sequence::partitions_allocator_type{});
        return toReset == sequence{};
      }()
    );
  }
}
