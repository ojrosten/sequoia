////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "IteratorTest.hpp"
#include "IteratorTestingUtilities.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <vector>

namespace sequoia::testing
{
  namespace
  {
    template<std::input_or_output_iterator Iterator>
    class scaling_dereference_policy
    {
    public:
      using value_type = std::iter_value_t<Iterator>;
      using reference  = value_type;

      constexpr scaling_dereference_policy() = default;
      constexpr scaling_dereference_policy(const value_type scale) : m_Scale{scale} {}
      constexpr scaling_dereference_policy(const scaling_dereference_policy&) = default;

      [[nodiscard]]
      constexpr value_type scale() const noexcept { return m_Scale; }

      // This must be public (not that there's really a problem with that) due to MSVC bug:
      // https://developercommunity.visualstudio.com/t/Compiler-crash-attempting-to-detect-prot/10361090
      [[nodiscard]]
      constexpr reference get(const Iterator& i) const
      {
        return (*i) * m_Scale;
      }

      [[nodiscard]]
      friend constexpr bool operator==(const scaling_dereference_policy&, const scaling_dereference_policy&) noexcept = default;
    protected:
      constexpr scaling_dereference_policy(scaling_dereference_policy&&) noexcept = default;

      ~scaling_dereference_policy() = default;

      constexpr scaling_dereference_policy& operator=(const scaling_dereference_policy&)     = default;
      constexpr scaling_dereference_policy& operator=(scaling_dereference_policy&&) noexcept = default;
    private:
      value_type m_Scale{1};
    };

    template<class Iterator, class DerefPolicy>
    concept scaling_iterator = sequoia::utilities::dereference_policy_for<DerefPolicy, Iterator>
      && requires(DerefPolicy & d) { d.scale(); };

    // Post-increment returns void, so this input iterator does not meet the
    // C++17 input iterator requirements, and std::iterator_traits is empty for
    // it. Wrapping it checks that neither utilities::iterator nor
    // identity_dereference_policy reads std::iterator_traits. Where Reference
    // is a reference type, operator* returns a reference to the iterator's
    // own count, as std::istream_iterator's operator* does to its own value.
    template<class Reference>
    class basic_counting_input_iterator
    {
    public:
      using value_type       = int;
      using difference_type  = std::ptrdiff_t;
      using iterator_concept = std::input_iterator_tag;

      constexpr basic_counting_input_iterator() = default;

      constexpr explicit basic_counting_input_iterator(const value_type count)
        : m_Count{count}
      {}

      [[nodiscard]]
      constexpr Reference operator*() const noexcept { return m_Count; }

      constexpr basic_counting_input_iterator& operator++() noexcept
      {
        ++m_Count;
        return *this;
      }

      constexpr void operator++(int) noexcept { ++m_Count; }

      [[nodiscard]]
      friend constexpr bool operator==(const basic_counting_input_iterator&,
                                       const basic_counting_input_iterator&) noexcept = default;
    private:
      value_type m_Count{};
    };

    using counting_input_iterator = basic_counting_input_iterator<int>;
    using stashing_input_iterator = basic_counting_input_iterator<const int&>;

    template<class I>
    constexpr bool has_iterator_traits_difference_type_v{
      requires { typename std::iterator_traits<I>::difference_type; }
    };

    class counting_input_iterator_without_equality
    {
    public:
      using value_type      = int;
      using difference_type = std::ptrdiff_t;

      [[nodiscard]]
      constexpr value_type operator*() const noexcept { return m_Count; }

      constexpr counting_input_iterator_without_equality& operator++() noexcept
      {
        ++m_Count;
        return *this;
      }

      constexpr void operator++(int) noexcept { ++m_Count; }
    private:
      value_type m_Count{};
    };

    // An auxiliary data policy with == and <=>, so that a wrapper over a base
    // iterator lacking either has a policy's comparison to fall back to.
    struct comparable_data_policy
    {
      [[nodiscard]]
      friend constexpr auto operator<=>(const comparable_data_policy&,
                                        const comparable_data_policy&) noexcept = default;
    };

    template<std::input_iterator Iterator>
    using iterator_with_comparable_data_policy
      = utilities::iterator<Iterator, utilities::identity_dereference_policy<Iterator, comparable_data_policy>>;

    struct aggregate_data_policy
    {
      int datum{};
    };

    template<std::input_iterator Iterator>
    using iterator_with_aggregate_data_policy
      = utilities::iterator<Iterator, utilities::identity_dereference_policy<Iterator, aggregate_data_policy>>;

    // A dereference policy other than the identity. Like the policy of
    // iterator_with_comparable_data_policy, it derives from
    // comparable_data_policy.
    template<std::input_or_output_iterator Iterator>
    struct negating_dereference_policy : comparable_data_policy
    {
      using value_type = std::iter_value_t<Iterator>;
      using reference  = value_type;

      [[nodiscard]]
      constexpr static reference get(const Iterator& i) { return -*i; }
    };

    struct element_with_overloaded_address_of
    {
      constexpr std::nullptr_t operator&() const noexcept { return nullptr; }
    };

    template<class I>
    constexpr bool has_arrow_operator_v{
      requires (I i) { i.operator->(); }
    };

    template<class I, class J>
    constexpr bool has_equality_with_v{
      requires (const I& i, const J& j) { i == j; }
    };

    template<class I, class J>
    constexpr bool has_three_way_comparison_with_v{
      requires (const I& i, const J& j) { i <=> j; }
    };
  }

  [[nodiscard]]
  std::filesystem::path iterator_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void iterator_test::run_tests()
  {
    test_nested_bidirectional_iterator();
    test_comparisons_the_base_iterator_lacks();
    test_comparisons_with_a_const_iterator();
    test_comparisons_without_a_conversion();
    test_aggregate_data_policy();
    test_stashing_input_iterator();
    test_arrow_operator_with_overloaded_address_of();

    test_iterator();
    test_const_iterator();
    test_reverse_iterator();
    test_const_reverse_iterator();

    test_const_scaling_iterator();
    test_const_reverse_scaling_iterator();

    test_traitless_input_iterator();
  }

  void iterator_test::test_nested_bidirectional_iterator()
  {
    using namespace utilities;

    using base_iter_t   = std::list<int>::iterator;
    using iter_t        = iterator<base_iter_t, identity_dereference_policy<base_iter_t>>;
    using nested_iter_t = iterator<iter_t, identity_dereference_policy<iter_t>>;

    STATIC_CHECK( std::bidirectional_iterator<nested_iter_t>);
    STATIC_CHECK(!std::random_access_iterator<nested_iter_t>);
  }

  void iterator_test::test_comparisons_the_base_iterator_lacks()
  {
    using iter_without_equality_t = iterator_with_comparable_data_policy<counting_input_iterator_without_equality>;

    STATIC_CHECK( std::input_iterator<iter_without_equality_t>);
    STATIC_CHECK(!std::equality_comparable<iter_without_equality_t>);

    using iter_without_ordering_t = iterator_with_comparable_data_policy<counting_input_iterator>;

    STATIC_CHECK( std::equality_comparable<iter_without_ordering_t>);
    STATIC_CHECK(!std::three_way_comparable<iter_without_ordering_t>);
  }

  void iterator_test::test_comparisons_with_a_const_iterator()
  {
    using namespace utilities;

    using i_t  = std::array<int, 3>::iterator;
    using ci_t = std::array<int, 3>::const_iterator;

    using custom_iter_t  = iterator<i_t,  identity_dereference_policy<i_t>>;
    using custom_citer_t = iterator<ci_t, identity_dereference_policy<ci_t>>;

    STATIC_CHECK(std::equality_comparable_with<custom_iter_t, custom_citer_t>);
    STATIC_CHECK(std::equality_comparable_with<custom_citer_t, custom_iter_t>);
    STATIC_CHECK(std::three_way_comparable_with<custom_iter_t, custom_citer_t>);
    STATIC_CHECK(std::three_way_comparable_with<custom_citer_t, custom_iter_t>);
    STATIC_CHECK(std::sentinel_for<custom_citer_t, custom_iter_t>);
    STATIC_CHECK(std::sentinel_for<custom_iter_t, custom_citer_t>);

    STATIC_CHECK([](){
      std::array<int, 3> a{};
      const custom_iter_t  i{a.begin()};
      const custom_citer_t ci{a.cbegin()}, cj{a.cbegin() + 1};
      return (i == ci) && (ci == i) && (i != cj) && (cj != i) && ((i <=> cj) < 0) && ((cj <=> i) > 0);
    }());

    // Unlike null_data_policy, comparable_data_policy declares <=>.
    using comparable_data_iter_t  = iterator_with_comparable_data_policy<i_t>;
    using comparable_data_citer_t = iterator_with_comparable_data_policy<ci_t>;

    STATIC_CHECK(std::three_way_comparable_with<comparable_data_iter_t, comparable_data_citer_t>);
    STATIC_CHECK(std::three_way_comparable_with<comparable_data_citer_t, comparable_data_iter_t>);

    STATIC_CHECK([](){
      std::array<int, 3> a{};
      const comparable_data_iter_t  i{a.begin()};
      const comparable_data_citer_t cj{a.cbegin() + 1};
      return (i != cj) && (cj != i) && ((i <=> cj) < 0) && ((cj <=> i) > 0);
    }());

    // The base iterators compare, but neither iterator converts to the other.
    using scaling_citer_t = iterator<ci_t, scaling_dereference_policy<ci_t>>;

    STATIC_CHECK( has_equality_with_v<custom_iter_t, custom_citer_t>);
    STATIC_CHECK( has_equality_with_v<custom_citer_t, custom_iter_t>);
    STATIC_CHECK(!has_equality_with_v<custom_iter_t, scaling_citer_t>);
    STATIC_CHECK(!has_equality_with_v<scaling_citer_t, custom_iter_t>);
    STATIC_CHECK( has_three_way_comparison_with_v<custom_iter_t, custom_citer_t>);
    STATIC_CHECK( has_three_way_comparison_with_v<custom_citer_t, custom_iter_t>);
    STATIC_CHECK(!has_three_way_comparison_with_v<custom_iter_t, scaling_citer_t>);
    STATIC_CHECK(!has_three_way_comparison_with_v<scaling_citer_t, custom_iter_t>);
  }

  void iterator_test::test_comparisons_without_a_conversion()
  {
    using namespace utilities;

    // In each pair, neither iterator converts to the other. Both share a
    // public base class declaring the comparison.
    using vec_i_t  = std::vector<int>::iterator;
    using list_i_t = std::list<int>::iterator;

    using custom_vec_iter_t  = iterator<vec_i_t,  identity_dereference_policy<vec_i_t>>;
    using custom_list_iter_t = iterator<list_i_t, identity_dereference_policy<list_i_t>>;

    STATIC_CHECK(!has_equality_with_v<custom_vec_iter_t, custom_list_iter_t>);
    STATIC_CHECK(!has_equality_with_v<custom_list_iter_t, custom_vec_iter_t>);

    // Unlike the first pair's, these base iterators compare.
    using i_t  = std::array<int, 3>::iterator;
    using ci_t = std::array<int, 3>::const_iterator;

    using comparable_data_iter_t = iterator_with_comparable_data_policy<i_t>;
    using negating_citer_t       = iterator<ci_t, negating_dereference_policy<ci_t>>;

    STATIC_CHECK(!has_equality_with_v<comparable_data_iter_t, negating_citer_t>);
    STATIC_CHECK(!has_equality_with_v<negating_citer_t, comparable_data_iter_t>);
    STATIC_CHECK(!has_three_way_comparison_with_v<comparable_data_iter_t, negating_citer_t>);
    STATIC_CHECK(!has_three_way_comparison_with_v<negating_citer_t, comparable_data_iter_t>);
  }

  void iterator_test::test_aggregate_data_policy()
  {
    using namespace utilities;

    using i_t  = std::array<int, 3>::iterator;
    using ci_t = std::array<int, 3>::const_iterator;

    using custom_iter_t          = iterator<i_t,  identity_dereference_policy<i_t>>;
    using custom_citer_t         = iterator<ci_t, identity_dereference_policy<ci_t>>;
    using aggregate_data_iter_t  = iterator_with_aggregate_data_policy<i_t>;
    using aggregate_data_citer_t = iterator_with_aggregate_data_policy<ci_t>;

    STATIC_CHECK( std::convertible_to<aggregate_data_iter_t, aggregate_data_citer_t>);
    STATIC_CHECK(!std::convertible_to<custom_iter_t, aggregate_data_citer_t>);
    STATIC_CHECK(!std::convertible_to<aggregate_data_iter_t, custom_citer_t>);

    STATIC_CHECK(!has_equality_with_v<custom_iter_t, aggregate_data_citer_t>);
    STATIC_CHECK(!has_equality_with_v<aggregate_data_citer_t, custom_iter_t>);
    STATIC_CHECK(!has_equality_with_v<aggregate_data_iter_t, custom_citer_t>);
    STATIC_CHECK(!has_equality_with_v<custom_citer_t, aggregate_data_iter_t>);

    STATIC_CHECK([](){
      std::array<int, 3> a{};
      const aggregate_data_citer_t ci{a.cbegin(), 2};
      return ci.datum == 2;
    }());
  }

  void iterator_test::test_stashing_input_iterator()
  {
    using namespace utilities;

    using iter_t = iterator<stashing_input_iterator, identity_dereference_policy<stashing_input_iterator>>;

    // A constant evaluation, so that a read through a dangling reference
    // fails to compile.
    STATIC_CHECK([](){
      const iter_t i{stashing_input_iterator{3}};
      return (*i == 3) && (*i.operator->() == 3);
    }());
  }

  void iterator_test::test_arrow_operator_with_overloaded_address_of()
  {
    using namespace utilities;

    using i_t    = std::array<element_with_overloaded_address_of, 2>::iterator;
    using iter_t = iterator<i_t, identity_dereference_policy<i_t>>;

    STATIC_CHECK([](){
      std::array<element_with_overloaded_address_of, 2> a{};
      const iter_t i{a.begin() + 1};
      return i.operator->() == std::addressof(a[1]);
    }());
  }

  void iterator_test::test_traitless_input_iterator()
  {
    using namespace utilities;

    using iter_t = iterator<counting_input_iterator, identity_dereference_policy<counting_input_iterator>>;

    STATIC_CHECK( std::input_iterator<counting_input_iterator>);
    STATIC_CHECK(!has_iterator_traits_difference_type_v<counting_input_iterator>);
    STATIC_CHECK( std::is_same_v<iter_t::difference_type, std::ptrdiff_t>);
    STATIC_CHECK( std::is_same_v<iter_t::value_type, int>);
    STATIC_CHECK( std::is_same_v<iter_t::reference, int>);
    STATIC_CHECK( std::is_same_v<iter_t::pointer, void>);
    STATIC_CHECK(!has_arrow_operator_v<iter_t>);
    STATIC_CHECK( std::input_iterator<iter_t>);

    iter_t i{counting_input_iterator{3}};
    check(equality, "Dereferencing an iterator over an input iterator without iterator traits", *i, 3);

    ++i;
    check(equality, "Pre-incrementing an iterator over an input iterator without iterator traits", *i, 4);

    check(equality, "Post-incrementing an iterator over an input iterator without iterator traits", *i++, 4);
    check(equality, "Post-increment advances an iterator over an input iterator without iterator traits", *i, 5);
  }

  void iterator_test::test_iterator()
  {
    using namespace utilities;

    using i_t = std::array<int, 3>::iterator;
    using custom_iter_t = iterator<i_t, identity_dereference_policy<i_t, null_data_policy>>;

    STATIC_CHECK(std::is_same_v<custom_iter_t::difference_type, std::ptrdiff_t>);
    STATIC_CHECK(std::is_same_v<custom_iter_t::value_type, int>);
    STATIC_CHECK(std::is_same_v<custom_iter_t::pointer, int*>);
    STATIC_CHECK(std::is_same_v<custom_iter_t::reference, int&>);
    STATIC_CHECK(std::random_access_iterator<custom_iter_t>);
    STATIC_CHECK(std::permutable<custom_iter_t>);

    std::array<int, 3> a{3, 0, 1};
    basic_checks<custom_iter_t>(a.begin(), a.end(), a.data(), "Custom iterator from iterator");

    custom_iter_t i{a.begin() + 1};

    *i = 5;
    // 3 5 1

    check(equality, "Check changing pointee", a, {3, 5, 1});
    check(equality, "", *i, 5);
    check(equality, "", i[-1], 3);
    check(equality, "", i[0], 5);
    check(equality, "", i[1], 1);

    i[-1] = 7;
    // 7 5 1

    check(equality, "Check changing pointee via []", a, {7, 5, 1});
    check(equality, "", *i, 5);
    check(equality, "", i[-1], 7);
    check(equality, "", i[0], 5);
    check(equality, "", i[1], 1);

    std::ranges::sort(custom_iter_t{a.begin()}, custom_iter_t{a.end()});
    // 1 5 7

    check(equality, "", i[-1], 1);
    check(equality, "", i[0], 5);
    check(equality, "", i[1], 7);

    i = 1 + i;
    check(equality, "", *i, 7);
  }

  void iterator_test::test_const_iterator()
  {
    using namespace utilities;

    using ci_t = std::array<int, 3>::const_iterator;
    using custom_citer_t = iterator<ci_t, identity_dereference_policy<ci_t, null_data_policy>>;

    STATIC_CHECK( std::is_same_v<custom_citer_t::difference_type, std::ptrdiff_t>);
    STATIC_CHECK( std::is_same_v<custom_citer_t::value_type, int>);
    STATIC_CHECK( std::is_same_v<custom_citer_t::pointer, const int*>);
    STATIC_CHECK( std::is_same_v<custom_citer_t::reference, const int&>);
    STATIC_CHECK( std::random_access_iterator<custom_citer_t>);
    STATIC_CHECK(!std::permutable<custom_citer_t>);

    std::array<int, 3> a{3, 0, 1};
    basic_checks<custom_citer_t>(a.cbegin(), a.cend(), &*a.cbegin(), "Custom const_iterator from const_iterator");
    basic_checks<custom_citer_t>(a.begin(), a.end(), &*a.cbegin(), "Custom const_iterator from iterator");

    using i_t = std::array<int, 3>::iterator;
    using custom_iter_t = iterator<i_t, identity_dereference_policy<ci_t, null_data_policy>>;

    basic_checks<custom_citer_t>(custom_iter_t{a.begin()}, custom_iter_t{a.end()}, &*a.cbegin(), "Custom const_iterator from custom iterator");
  }

  void iterator_test::test_reverse_iterator()
  {
    using namespace utilities;

    using ri_t = std::array<int, 3>::reverse_iterator;
    using custom_riter_t = iterator<ri_t, identity_dereference_policy<ri_t, null_data_policy>>;

    STATIC_CHECK(std::is_same_v<custom_riter_t::difference_type, std::ptrdiff_t>);
    STATIC_CHECK(std::is_same_v<custom_riter_t::value_type, int>);
    STATIC_CHECK(std::is_same_v<custom_riter_t::pointer, int*>);
    STATIC_CHECK(std::is_same_v<custom_riter_t::reference, int&>);
    STATIC_CHECK(std::random_access_iterator<custom_riter_t>);
    STATIC_CHECK(std::permutable<custom_riter_t>);

    std::array<int, 3> a{3, 0, 1};
    basic_checks<custom_riter_t>(a.rbegin(), a.rend(), &*a.rbegin(), "Custom reverse_iterator from reverse_iterator");

    custom_riter_t i{a.rbegin()};

    *i = 5;
    // 3 0 5

    check(equality, "Check changing pointee", a, {3, 0, 5});
    check(equality, "", *i, 5);
    check(equality, "", i[0], 5);
    check(equality, "", i[1], 0);
    check(equality, "", i[2], 3);

    i[2] = 7;
    // 7 0 5

    check(equality, "Check changing pointee via []", a, {7, 0, 5});
    check(equality, "", *i, 5);
    check(equality, "", i[0], 5);
    check(equality, "", i[1], 0);
    check(equality, "", i[2], 7);
  }

  void iterator_test::test_const_reverse_iterator()
  {
    using namespace utilities;

    using cri_t = std::array<int, 3>::const_reverse_iterator;
    using custom_criter_t = iterator<cri_t, identity_dereference_policy<cri_t, null_data_policy>>;

    STATIC_CHECK( std::is_same_v<custom_criter_t::difference_type, std::ptrdiff_t>);
    STATIC_CHECK( std::is_same_v<custom_criter_t::value_type, int>);
    STATIC_CHECK( std::is_same_v<custom_criter_t::pointer, const int*>);
    STATIC_CHECK( std::is_same_v<custom_criter_t::reference, const int&>);
    STATIC_CHECK( std::random_access_iterator<custom_criter_t>);
    STATIC_CHECK(!std::permutable<custom_criter_t>);

    std::array<int, 3> a{3, 0, 1};
    basic_checks<custom_criter_t>(a.crbegin(), a.crend(), &*a.crbegin(), "Custom const_reverse_iterator from const_reverse_iterator");
    basic_checks<custom_criter_t>(a.rbegin(), a.rend(), &*a.crbegin(), "Custom const_reverse_iterator from reverse_iterator");

    using ri_t = std::array<int, 3>::reverse_iterator;
    using custom_riter_t = iterator<ri_t, identity_dereference_policy<ri_t, null_data_policy>>;

    basic_checks<custom_criter_t>(custom_riter_t{a.rbegin()}, custom_riter_t{a.rend()}, &*a.crbegin(), "Custom const_reverse_iterator from custom reverse_iterator");
  }

  void iterator_test::test_const_scaling_iterator()
  {
    using namespace utilities;

    using ci_t = std::array<int, 3>::const_iterator;
    using custom_citer_t = iterator<ci_t, scaling_dereference_policy<ci_t>>;

    STATIC_CHECK( std::is_same_v<custom_citer_t::difference_type, std::ptrdiff_t>);
    STATIC_CHECK( std::is_same_v<custom_citer_t::value_type, int>);
    STATIC_CHECK( std::is_same_v<custom_citer_t::pointer, void>);
    STATIC_CHECK( std::random_access_iterator<custom_citer_t>);
    STATIC_CHECK(!std::permutable<custom_citer_t>);

    std::array<int, 3> a{3, 0, 1};
    basic_checks<custom_citer_t>(a.cbegin(), a.cend(), &*a.cbegin(), "Custom scaling iterator from const_iterator", 3);

    basic_checks<custom_citer_t>(a.begin(), a.end(), &*a.cbegin(), "Custom scaling iterator from iterator", -1);

    custom_citer_t i{a.cend(), 1}, j{a.cend(), 2};
    check("Custom iterators should compare equal if they point to the same thing, irrespective of any other state", i == j);
  }

  void iterator_test::test_const_reverse_scaling_iterator()
  {
    using namespace utilities;

    using cri_t = std::array<int, 3>::const_reverse_iterator;
    using custom_criter_t = iterator<cri_t, scaling_dereference_policy<cri_t>>;

    STATIC_CHECK( std::is_same_v<custom_criter_t::difference_type, std::ptrdiff_t>);
    STATIC_CHECK( std::is_same_v<custom_criter_t::value_type, int>);
    STATIC_CHECK( std::is_same_v<custom_criter_t::pointer, void>);
    STATIC_CHECK( std::random_access_iterator<custom_criter_t>);
    STATIC_CHECK(!std::permutable<custom_criter_t>);

    std::array<int, 3> a{3, 0, 1};
    basic_checks<custom_criter_t>(a.crbegin(), a.crend(), &*a.crbegin(), "Custom reverse scaling iterator from const_reverse_iterator", -1);

    basic_checks<custom_criter_t>(a.rbegin(), a.rend(), &*a.crbegin(), "Custom reverse scaling iterator from reverse_iterator", 3);

    custom_criter_t i{a.crend(), 1}, j{a.crend(), 2};
    check("Custom reverse iterators should compare equal if they point to the same thing, irrespective of any other state", i == j);
  }

  template<
    std::input_or_output_iterator CustomIter,
    std::input_or_output_iterator Iter,
    std::sentinel_for<Iter> Sentinel,
    class... Args,
    class Pointer
  >
  void iterator_test::basic_checks(Iter begin, Sentinel end, Pointer pBegin, std::string_view message, Args... args)
  {
    using namespace std;

    using value_t = std::iterator_traits<Iter>::value_type;
    using deref_pol = CustomIter::dereference_policy_type;

    if(!check(equality, append_lines(message, "Contract violated"), distance(begin, end), std::ptrdiff_t{3}))
      return;

    CustomIter i{begin, args...};

    STATIC_CHECK(std::totally_ordered<CustomIter>);

    const auto scale{
      []([[maybe_unused]] CustomIter iter) -> value_t {
        if constexpr(scaling_iterator<typename CustomIter::base_iterator_type, deref_pol>)
        {
          return iter.scale();
        }
        else
        {
          return {1};
        }
      }(i)
    };

    check(equality, message, *i, *begin * scale);
    check(equality, message, i[0], begin[0] * scale);
    check(equality, message, i[1], begin[1] * scale);
    check(equality, message, i[2], begin[2] * scale);

    if constexpr(has_arrow_operator_v<CustomIter>)
    {
      check(equality, append_lines(message, "Operator ->"), i.operator->(), pBegin);
    }

    CustomIter j{end, args...};
    check_semantics(append_lines(message, "Regular semantics; one iterator at end"), i, j, std::weak_ordering::less);

    check(message, i < j);
    check(message, j > i);
    check(message, i <= j);
    check(message, j >= i);
    check(equality, append_lines(message, "Check non-zero distance"), distance(i, j), distance(begin, end));
    check(equality, append_lines(message, "Check non-zero difference"), j - i, distance(begin, end));
    check(equality, append_lines(message, "Check negative difference"), i - j, -distance(begin, end));

    check(equality, message, *++i, begin[1] * scale);
    check(equality, message, *i++, begin[1] * scale);
    check(equality, message, *i, begin[2] * scale);
    check(message, ++i == j);
    check(message, i <= j);
    check(message, j >= i);

    check(equality, message, *--i, begin[2] * scale);
    check(equality, message, *i--, begin[2] * scale);
    check(equality, message, *i, begin[1] * scale);

    j = i - 1;
    check(equality, message, *i, begin[1] * scale);
    check(equality, message, *j, begin[0] * scale);
    check_semantics(append_lines(message, "Regular semantics"), i, j, std::weak_ordering::greater);

    i = j + 2;
    check(equality, message, *i, begin[2] * scale);

    i -= 1;
    check(equality, message, *i, begin[1] * scale);

    j += 1;
    check(equality, message, *j, begin[1] * scale);

    check(message, i == j);
    check<std::int64_t>(equality, append_lines(message, "Check for distance of zero"), distance(i, j), 0);
  }
}
