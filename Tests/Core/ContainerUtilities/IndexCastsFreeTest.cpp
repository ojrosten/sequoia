////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "IndexCastsFreeTest.hpp"
#include "sequoia/Core/ContainerUtilities/IndexCasts.hpp"

namespace sequoia::testing
{
  using namespace utilities;

  namespace
  {
    template<class Index>
    inline constexpr bool to_difference_accepts_v{
      requires(Index index) { to_difference(index); }
    };

    enum class unsigned_enum : unsigned {};
  }

  [[nodiscard]]
  std::filesystem::path index_casts_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void index_casts_free_test::run_tests()
  {
    // Integer types, not integral ones: the line std::in_range draws
    STATIC_CHECK( to_difference_accepts_v<unsigned char>);
    STATIC_CHECK( to_difference_accepts_v<std::uint16_t>);
    STATIC_CHECK( to_difference_accepts_v<std::uint32_t>);
    STATIC_CHECK( to_difference_accepts_v<std::size_t>);
    STATIC_CHECK( to_difference_accepts_v<int>);
    STATIC_CHECK( to_difference_accepts_v<std::ptrdiff_t>);
    STATIC_CHECK(!to_difference_accepts_v<bool>);
    STATIC_CHECK(!to_difference_accepts_v<char>);
    STATIC_CHECK(!to_difference_accepts_v<char8_t>);
    STATIC_CHECK(!to_difference_accepts_v<char32_t>);
    STATIC_CHECK(!to_difference_accepts_v<double>);
    STATIC_CHECK(!to_difference_accepts_v<unsigned_enum>);

    // noexcept precisely where no information can be lost
    STATIC_CHECK( noexcept(to_difference(std::uint16_t{})));
    STATIC_CHECK( noexcept(to_difference(int{})));
    STATIC_CHECK(!noexcept(to_difference(std::size_t{})));

    STATIC_CHECK(to_difference(std::size_t{42}) == 42);

    constexpr auto maxDifference{std::numeric_limits<std::ptrdiff_t>::max()};
    constexpr auto maxDifferenceAsIndex{static_cast<std::size_t>(maxDifference)};
    constexpr auto maxNarrowIndex{std::numeric_limits<std::uint16_t>::max()};

    check_exception_thrown<std::domain_error>(
      "Above the range of std::ptrdiff_t",
      []() { return to_difference(maxDifferenceAsIndex + 1); }
    );

    check(equality, "Greatest ptrdiff_t", to_difference(maxDifferenceAsIndex), maxDifference);
    check(equality, "Greatest uint16_t",  to_difference(maxNarrowIndex),       std::ptrdiff_t{maxNarrowIndex});
    check(equality, "Negative int",       to_difference(-1),                   std::ptrdiff_t{-1});
  }
}
