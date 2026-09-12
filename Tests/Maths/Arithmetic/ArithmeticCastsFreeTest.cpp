////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ArithmeticCastsFreeTest.hpp"
#include "sequoia/Maths/Arithmetic/ArithmeticCasts.hpp"

namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
    template<class To, class From>
    inline constexpr bool has_checked_conversion_v{
      requires(From from) { checked_conversion_to<To>(from); }
    };
  }

  [[nodiscard]]
  std::filesystem::path arithmetic_casts_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void arithmetic_casts_free_test::run_tests()
  {
    // Integer types, not integral ones: the line std::in_range draws
    STATIC_CHECK( has_checked_conversion_v<int,          int>);
    STATIC_CHECK( has_checked_conversion_v<int,          std::int8_t>);
    STATIC_CHECK( has_checked_conversion_v<std::int8_t,  int>);
    STATIC_CHECK( has_checked_conversion_v<int,          std::uint8_t>);
    STATIC_CHECK( has_checked_conversion_v<std::uint8_t, int>);
    STATIC_CHECK(!has_checked_conversion_v<int,          bool>);
    STATIC_CHECK(!has_checked_conversion_v<bool,         int>);
    STATIC_CHECK(!has_checked_conversion_v<int,          char>);
    STATIC_CHECK(!has_checked_conversion_v<char,         int>);
    STATIC_CHECK(!has_checked_conversion_v<int,          char32_t>);
    STATIC_CHECK(!has_checked_conversion_v<char32_t,     int>);
    STATIC_CHECK(!has_checked_conversion_v<int,          double>);
    STATIC_CHECK(!has_checked_conversion_v<double,       int>);

    // noexcept precisely where no information can be lost
    STATIC_CHECK( noexcept(checked_conversion_to<int>(int{})));
    STATIC_CHECK( noexcept(checked_conversion_to<std::int64_t>(std::uint32_t{})));
    STATIC_CHECK(!noexcept(checked_conversion_to<std::uint32_t>(std::int64_t{})));
    STATIC_CHECK(!noexcept(checked_conversion_to<std::int32_t>(std::uint32_t{})));

    check_exception_thrown<std::domain_error>(
      "Above the range of a narrower signed type",
      []() { return checked_conversion_to<std::int32_t>(std::numeric_limits<std::int64_t>::max()); }
    );

    check_exception_thrown<std::domain_error>(
      "Above the range of a signed type of the same width",
      []() { return checked_conversion_to<std::int32_t>(std::numeric_limits<std::uint32_t>::max()); }
    );

    check_exception_thrown<std::domain_error>(
      "Above the range of a narrower unsigned type",
      []() { return checked_conversion_to<std::uint32_t>(std::numeric_limits<std::uint64_t>::max()); }
    );

    check_exception_thrown<std::domain_error>(
      "Negative, to an unsigned type",
      []() { return checked_conversion_to<std::uint32_t>(std::int32_t{-1}); }
    );

    check_exception_thrown<std::domain_error>(
      "Below the range of a narrower signed type",
      []() { return checked_conversion_to<std::int32_t>(std::numeric_limits<std::int64_t>::lowest()); }
    );

    check(equality, "Identity", checked_conversion_to<int>(42), 42);

    check(equality, "Greatest value of a narrower signed type",   checked_conversion_to<std:: int32_t>(std::numeric_limits<std::int64_t >::   max() >> 32), std::numeric_limits<std::int32_t >::   max()      );
    check(equality, "Least value of a narrower signed type",      checked_conversion_to<std:: int32_t>(std::numeric_limits<std::int64_t >::lowest() >> 32), std::numeric_limits<std::int32_t >::lowest()      );
    check(equality, "Greatest value, widened",                    checked_conversion_to<std::int64_t >(std::numeric_limits<std::int32_t> ::   max()      ), std::numeric_limits<std::int64_t >::   max() >> 32);
    check(equality, "Least value, widened",                       checked_conversion_to<std::int64_t >(std::numeric_limits<std::int32_t> ::lowest()      ), std::numeric_limits<std::int64_t >::lowest() >> 32);
    check(equality, "Greatest value of a narrower unsigned type", checked_conversion_to<std::uint32_t>(std::numeric_limits<std::uint64_t>::   max() >> 32), std::numeric_limits<std::uint32_t>::   max()      );
    check(equality, "Greatest signed value, from unsigned",       checked_conversion_to<std:: int32_t>(std::numeric_limits<std::uint32_t>::   max()  /  2), std::numeric_limits<std::int32_t >::   max()      );
  }
}
