////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "NumbersFreeTest.hpp"
#include "sequoia/TestFramework/SumTypeCheckers.hpp"
#include "sequoia/TextProcessing/Numbers.hpp"

#include <cmath>
#include <limits>

namespace sequoia::testing
{
  namespace
  {
    template<class T>
    concept extractable = requires(std::string_view text) { extract_numbers_from<T, 1>(text); };

    template<class T, std::size_t N>
    using numbers_t = std::optional<std::array<T, N>>;
  }

  [[nodiscard]]
  std::filesystem::path numbers_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void numbers_free_test::run_tests()
  {
    test_constraints();
    test_count();
    test_floating_point();
    test_integers();
    test_signs();
    test_out_of_range();
  }

  void numbers_free_test::test_constraints()
  {
    STATIC_CHECK( extractable<double>);
    STATIC_CHECK( extractable<int>);
    STATIC_CHECK( extractable<std::size_t>);
    STATIC_CHECK(!extractable<bool>);
    STATIC_CHECK(!extractable<char>);
    STATIC_CHECK(!extractable<const int>);
    STATIC_CHECK(!extractable<volatile double>);
    STATIC_CHECK(!extractable<std::string>);
  }

  void numbers_free_test::test_count()
  {
    check(equality,
          "Exactly as many numbers as asked for, in the order of the text",
          extract_numbers_from<double, 2>("fast 0.5s, slow 2s"),
          numbers_t<double, 2>{{0.5, 2.0}});

    check(equality,
          "Fewer numbers than asked for",
          extract_numbers_from<double, 2>("fast 0.5s"),
          numbers_t<double, 2>{});

    check(equality,
          "More numbers than asked for",
          extract_numbers_from<double, 2>("1, 2, 3"),
          numbers_t<double, 2>{});

    check(equality,
          "No numbers asked for, in empty text",
          extract_numbers_from<double, 0>(""),
          numbers_t<double, 0>{std::array<double, 0>{}});

    check(equality,
          "No numbers asked for, in text holding one",
          extract_numbers_from<double, 0>("1"),
          numbers_t<double, 0>{});
  }

  void numbers_free_test::test_floating_point()
  {
    constexpr auto infinity{std::numeric_limits<double>::infinity()};

    check(equality,
          "An integer read as a floating-point number",
          extract_numbers_from<double, 1>("Trials: 5"),
          numbers_t<double, 1>{{5.0}});

    check(equality,
          "An exponent",
          extract_numbers_from<double, 2>("1e+06 and 9.9e-05s"),
          numbers_t<double, 2>{{1e+06, 9.9e-05}});

    check(equality,
          "An inf inside a word",
          extract_numbers_from<double, 2>("infer 2"),
          numbers_t<double, 2>{{infinity, 2.0}});

    const auto nanInsideWord{extract_numbers_from<double, 1>("banana")};
    check("A nan inside a word", nanInsideWord && std::isnan((*nanInsideWord)[0]));
  }

  void numbers_free_test::test_integers()
  {
    check(equality,
          "A decimal point separates two integers",
          extract_numbers_from<int, 2>("1.5"),
          numbers_t<int, 2>{{1, 5}});

    check(equality,
          "No inf or nan is an integer",
          extract_numbers_from<int, 1>("inf nan 7"),
          numbers_t<int, 1>{{7}});
  }

  void numbers_free_test::test_signs()
  {
    check(equality,
          "A minus sign before a floating-point number",
          extract_numbers_from<double, 1>("slow -0.25s"),
          numbers_t<double, 1>{{-0.25}});

    check(equality,
          "A minus sign before a signed integer, even directly after another number",
          extract_numbers_from<int, 2>("5-3"),
          numbers_t<int, 2>{{5, -3}});

    check(equality,
          "A minus sign before an unsigned integer is skipped",
          extract_numbers_from<std::size_t, 1>("-5"),
          numbers_t<std::size_t, 1>{{5}});

    check(equality,
          "A hyphen before a letter is no sign",
          extract_numbers_from<double, 1>("Speed-up: 2"),
          numbers_t<double, 1>{{2.0}});
  }

  void numbers_free_test::test_out_of_range()
  {
    check(equality,
          "A floating-point number too large for the type; reading resumes at its second character",
          extract_numbers_from<double, 1>("1e400"),
          numbers_t<double, 1>{{400.0}});

    check(equality,
          "An integer too large for the type; reading resumes at its second character",
          extract_numbers_from<std::size_t, 1>("99999999999999999999"),
          numbers_t<std::size_t, 1>{{9'999'999'999'999'999'999uz}});
  }
}
