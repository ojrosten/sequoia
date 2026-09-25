////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "OutputFreeTest.hpp"
#include "sequoia/TestFramework/Output.hpp"

#include <array>
#include <concepts>
#include <limits>
#include <ranges>
#include <utility>

namespace sequoia::testing
{
  using namespace std::string_literals;
  namespace fs = std::filesystem;

  namespace
  {
    /** The text after the first `open` and before the first `close` */
    [[nodiscard]]
    std::string text_between(std::string_view text, char open, char close)
    {
      const auto start{text.find(open) + 1};
      return std::string{text.substr(start, text.find(close) - start)};
    }
  }

  namespace template_argument_fixtures
  {
    template<std::floating_point T>
    struct bounds
    {
      T lower, upper;
    };

    template<class T, class U>
    struct two_members
    {
      T first;
      U second;
    };

    struct inner
    {
      float value;
    };

    struct outer
    {
      inner nested;
      float value;
    };

    struct point3
    {
      float x, y;
    };

    enum class member_enum { a, b };

    template<class T, class U>
    struct enclosing
    {
      enum class nested { a, b };
    };

    template<auto Value>
    struct value_holder {};

    template<class T>
    struct type_holder {};

    template<auto Value, class T>
    struct value_and_type_holder {};

    struct inff {};

    struct inff_leading {};

    struct trailing_inff {};

    struct nanf {};

    namespace scope_ending_in_2
    {
      struct inff {};
    }

    constexpr float       float_infinity{std::numeric_limits<float>::infinity()};
    constexpr double      double_infinity{std::numeric_limits<double>::infinity()};
    constexpr long double long_double_infinity{std::numeric_limits<long double>::infinity()};

    constexpr float       float_nan{std::numeric_limits<float>::quiet_NaN()};
    constexpr double      double_nan{std::numeric_limits<double>::quiet_NaN()};
    constexpr long double long_double_nan{std::numeric_limits<long double>::quiet_NaN()};
  }

  [[nodiscard]]
  std::filesystem::path output_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void output_free_test::run_tests()
  {
    test_emphasise();
    test_display_character();
    test_tidy_name();
    test_template_argument_values();
    test_array_iterators();
    test_relative_reporting_path();
    test_absolute_reporting_path();
  }

  void output_free_test::test_emphasise()
  {
    check(equality, "Emphasis", emphasise("foo"), "--foo--"s);
    check(equality, "Nothing to emphasise", emphasise(""), ""s);
  }

  void output_free_test::test_display_character()
  {
    check(equality, "", display_character('\n'), "'\\n'"s);
    check(equality, "", display_character('\t'), "'\\t'"s);
    check(equality, "", display_character('\0'), "'\\0'"s);
    check(equality, "", display_character(' '), "' '"s);
  }

  void output_free_test::test_tidy_name()
  {
    check(equality, "", tidy_name("(some enum)0", clang_type{}), "0"s);
    check(equality, "", tidy_name("<0f>",         clang_type{}), "<0>"s);
    check(equality, "", tidy_name("<0ul>",        clang_type{}), "<0>"s);
    check(equality, "", tidy_name("<1ul, 0f>",    clang_type{}), "<1, 0>"s);
    check(equality, "", tidy_name("<2ul, 1f>",    clang_type{}), "<2, 1>"s);
    check(equality, "", tidy_name("<textued_2d>", clang_type{}), "<textued_2d>"s);
    check(equality, "", tidy_name("struct foo", msvc_type{}), "foo"s);
    check(equality, "MSVC function type",         tidy_name("int __cdecl(void)",    msvc_type{}), "int ()"s);
    check(equality, "MSVC pointer to function",   tidy_name("int (__cdecl*)(void)", msvc_type{}), "int (*)()"s);
    check(equality, "MSVC reference to function", tidy_name("int (__cdecl&)(void)", msvc_type{}), "int (&)()"s);
    check(equality, "MSVC parameter list kept",   tidy_name("int (__cdecl*)(int)",  msvc_type{}), "int (*)(int)"s);
    check(equality,
          "MSVC pointer to function as a template argument",
          tidy_name("reset_on_move<int (__cdecl*)(void),0>", msvc_type{}),
          "reset_on_move<int (*)(), 0>"s);
    check(equality, "", tidy_name("",  gcc_type{}), ""s);
    check(equality, "", tidy_name(" ", gcc_type{}), " "s);
    check(equality,
          "",
          tidy_name(std::format("<(float)[{}]>", std::format("{:X}", std::bit_cast<int>(3.14f))), gcc_type{}),
          std::format("<{:.6f}>", 3.14f)
    );

    check(equality,
          "",
          tidy_name("coordinates<my_vec_space<1ul>>",  gcc_type{}),
          "coordinates<my_vec_space<1> >"s
    );

    const std::string normalizedLong{(sizeof(unsigned long) == sizeof(unsigned long long)) ? "long long" : "long"};

    check(equality, "long",                                   tidy_name("long",             clang_type{}), normalizedLong);
    check(equality, "unsigned long",                          tidy_name("unsigned long",    clang_type{}), "unsigned " + normalizedLong);
    check(equality, "long long",                              tidy_name("long long",        clang_type{}), "long long"s);
    check(equality, "long as a template argument",            tidy_name("vector<long>",     clang_type{}), "vector<" + normalizedLong + ">");
    check(equality, "long twice",                             tidy_name("pair<long, long>", clang_type{}), "pair<" + normalizedLong + ", " + normalizedLong + ">");

    check(equality, "An identifier containing long",          tidy_name("my_long_type",     clang_type{}), "my_long_type"s);
    check(equality, "An identifier ending in long",           tidy_name("prolong",          clang_type{}), "prolong"s);
    check(equality, "An identifier beginning with long",      tidy_name("longitude",        clang_type{}), "longitude"s);
    check(equality, "An identifier containing long, beside a long", tidy_name("belongs_to<long>", clang_type{}), "belongs_to<" + normalizedLong + ">");

    // The top hex nibble of a negative float is 8 through F, so its hex form begins with a letter
    check(equality,
          "A negative float",
          tidy_name("<(float)[C048F5C3]>", gcc_type{}),
          "<-3.140000>"s
    );

    check(equality,
          "A negative float of unit magnitude",
          tidy_name("<(float)[BF800000]>", gcc_type{}),
          "<-1.000000>"s
    );

    check(equality,
          "A hex form with no decimal digit at all",
          tidy_name("<(float)[FF]>", gcc_type{}),
          "<0.000000>"s
    );

    check(equality,
          "A double whose hex form has no decimal digit",
          tidy_name("<(double)[FF]>", gcc_type{}),
          "<0.000000>"s
    );

    // The top bit of a negative double is set, so its hex form overflows a signed 64-bit read
    check(equality,
          "A negative double",
          tidy_name("<(double)[C00921FB54442D18]>", gcc_type{}),
          "<-3.141593>"s
    );

    check(equality,
          "Negative infinity as a double",
          tidy_name("<(double)[FFF0000000000000]>", gcc_type{}),
          "<-inf>"s
    );

    check(equality,
          "A positive float, as the control",
          tidy_name("<(float)[4048F5C3]>", gcc_type{}),
          "<3.140000>"s
    );
  }

  void output_free_test::test_template_argument_values()
  {
    using namespace template_argument_fixtures;
    using nested = enclosing<int, float>::nested;

    const std::string fixtureNamespace{"sequoia::testing::template_argument_fixtures::"};

    check(equality,
          "Float infinities in a class-type template argument",
          demangle<value_holder<bounds<float>{-float_infinity, float_infinity}>>(),
          std::format("{0}value_holder<{0}bounds<float>{{-inf, inf}}>", fixtureNamespace)
    );

    check(equality,
          "Double infinities in a class-type template argument",
          demangle<value_holder<bounds<double>{-double_infinity, double_infinity}>>(),
          std::format("{0}value_holder<{0}bounds<double>{{-inf, inf}}>", fixtureNamespace)
    );

    check(equality,
          "A float infinity as a template argument",
          demangle<value_holder<float_infinity>>(),
          std::format("{0}value_holder<inf>", fixtureNamespace)
    );

    check(equality,
          "A type named inff",
          demangle<type_holder<inff>>(),
          std::format("{0}type_holder<{0}inff>", fixtureNamespace)
    );

    check(equality,
          "A type whose name begins with inff",
          demangle<type_holder<inff_leading>>(),
          std::format("{0}type_holder<{0}inff_leading>", fixtureNamespace)
    );

    check(equality,
          "A type whose name ends with inff",
          demangle<type_holder<trailing_inff>>(),
          std::format("{0}type_holder<{0}trailing_inff>", fixtureNamespace)
    );

    check(equality,
          "Long double values in a class-type template argument",
          demangle<value_holder<bounds<long double>{-long_double_infinity, 1.5L}>>(),
          std::format("{0}value_holder<{0}bounds<long double>{{-inf, 1.500000}}>", fixtureNamespace)
    );

    check(equality,
          "A long double as a template argument",
          demangle<value_holder<1.5L>>(),
          std::format("{0}value_holder<1.500000>", fixtureNamespace)
    );

    check(equality,
          "Positive float NaNs in a class-type template argument",
          demangle<value_holder<bounds<float>{float_nan, float_nan}>>(),
          std::format("{0}value_holder<{0}bounds<float>{{nan, nan}}>", fixtureNamespace)
    );

    check(equality,
          "Negative float NaNs in a class-type template argument",
          demangle<value_holder<bounds<float>{-float_nan, -float_nan}>>(),
          std::format("{0}value_holder<{0}bounds<float>{{-nan, -nan}}>", fixtureNamespace)
    );

    check(equality,
          "Negative double NaNs in a class-type template argument",
          demangle<value_holder<bounds<double>{-double_nan, -double_nan}>>(),
          std::format("{0}value_holder<{0}bounds<double>{{-nan, -nan}}>", fixtureNamespace)
    );

    check(equality,
          "Negative long double NaNs in a class-type template argument",
          demangle<value_holder<bounds<long double>{-long_double_nan, -long_double_nan}>>(),
          std::format("{0}value_holder<{0}bounds<long double>{{-nan, -nan}}>", fixtureNamespace)
    );

    check(equality,
          "A negative float NaN as a template argument",
          demangle<value_holder<-float_nan>>(),
          std::format("{0}value_holder<-nan>", fixtureNamespace)
    );

    {
      const auto obtained{demangle<value_and_type_holder<-float_nan, nanf>>()};
      check(equality,
            "A type named nanf beside a float NaN",
            obtained.substr(obtained.rfind(", ") + 2),
            std::format("{0}nanf>", fixtureNamespace)
      );
    }

    check(equality,
          "A type named inff in a scope whose name ends with a digit",
          demangle<type_holder<scope_ending_in_2::inff>>(),
          std::format("{0}type_holder<{0}scope_ending_in_2::inff>", fixtureNamespace)
    );

    check(equality,
          "An integer and a float in a class-type template argument",
          demangle<value_holder<two_members<int, float>{1, 2.0f}>>(),
          std::format("{0}value_holder<{0}two_members<int, float>{{1, 2.000000}}>", fixtureNamespace)
    );

    check(equality,
          "A bool and a char in a class-type template argument",
          demangle<value_holder<two_members<bool, char>{true, 'a'}>>(),
          std::format("{0}value_holder<{0}two_members<bool, char>{{1, 97}}>", fixtureNamespace)
    );

    check(equality,
          "An enumeration and an unsigned in a class-type template argument",
          demangle<value_holder<two_members<member_enum, unsigned>{member_enum::b, 3u}>>(),
          std::format("{0}value_holder<{0}two_members<{0}member_enum, unsigned int>{{1, 3}}>", fixtureNamespace)
    );

    check(equality,
          "An enumeration nested in a class template, in a class-type template argument",
          demangle<value_holder<two_members<nested, int>{nested::b, 3}>>(),
          std::format("{0}value_holder<{0}two_members<{0}enclosing<int, float>::nested, int>{{1, 3}}>",
                      fixtureNamespace)
    );

    check(equality,
          "Negative float and double NaNs in one class-type template argument",
          demangle<value_holder<two_members<float, double>{-float_nan, -double_nan}>>(),
          std::format("{0}value_holder<{0}two_members<float, double>{{-nan, -nan}}>", fixtureNamespace)
    );

    check(equality,
          "A positive float NaN beside a negative double NaN",
          demangle<value_holder<two_members<float, double>{float_nan, -double_nan}>>(),
          std::format("{0}value_holder<{0}two_members<float, double>{{nan, -nan}}>", fixtureNamespace)
    );

    check(equality,
          "A class-type template argument whose name holds a digit",
          demangle<value_holder<point3{1.0f, 2.0f}>>(),
          std::format("{0}value_holder<{0}point3{{1.000000, 2.000000}}>", fixtureNamespace)
    );

    {
      const auto obtained{demangle<value_holder<bounds<float>{float_nan, -float_nan}>>()};
      const auto first{text_between(obtained, '{', ',')};
      check("A positive float NaN beside a negative one is rendered as nan, or as libc++abi's nanf",
            (first == "nan") || (first == "nanf"));
    }

    check(equality,
          "A class template member of a class-type template argument",
          demangle<value_holder<two_members<two_members<int, int>, int>{{1, 2}, 3}>>(),
          std::format("{0}value_holder<{0}two_members<{0}two_members<int, int>, int>"
                      "{{{0}two_members<int, int>{{1, 2}}, 3}}>",
                      fixtureNamespace)
    );

    check(equality,
          "A nested class-type template argument",
          demangle<value_holder<outer{{1.0f}, 2.0f}>>(),
          std::format("{0}value_holder<{0}outer{{{0}inner{{1.000000}}, 2.000000}}>", fixtureNamespace)
    );

    check(equality,
          "MSVC's float infinities in a class-type template argument",
          tidy_name("struct value_holder<struct bounds<float>{float:-inf,float:inf}>", msvc_type{}),
          "value_holder<bounds<float>{-inf, inf}>"s
    );

    check(equality,
          "MSVC's double infinities in a class-type template argument",
          tidy_name("struct value_holder<struct bounds<double>{double:-inf,double:inf}>", msvc_type{}),
          "value_holder<bounds<double>{-inf, inf}>"s
    );

    check(equality,
          "MSVC's finite floats in a class-type template argument",
          tidy_name("struct value_holder<struct bounds<float>{float:0.000000,float:1.500000}>", msvc_type{}),
          "value_holder<bounds<float>{0.000000, 1.500000}>"s
    );

    check(equality,
          "MSVC's long doubles in a class-type template argument",
          tidy_name(
            "struct value_holder<struct bounds<long double>{long double:-inf,long double:1.500000}>",
            msvc_type{}
          ),
          "value_holder<bounds<long double>{-inf, 1.500000}>"s
    );

    check(equality,
          "MSVC's floats in a nested class-type template argument",
          tidy_name("struct value_holder<struct outer{struct inner{float:1.000000},float:2.000000}>", msvc_type{}),
          "value_holder<outer{inner{1.000000}, 2.000000}>"s
    );

    check(equality,
          "MSVC's integer and float in a class-type template argument",
          tidy_name("struct value_holder<struct two_members<int,float>{int:1,float:2.000000}>", msvc_type{}),
          "value_holder<two_members<int, float>{1, 2.000000}>"s
    );

    check(equality,
          "MSVC's bool and char in a class-type template argument",
          tidy_name("struct value_holder<struct two_members<bool,char>{bool:1,char:97}>", msvc_type{}),
          "value_holder<two_members<bool, char>{1, 97}>"s
    );

    check(equality,
          "MSVC's enumeration and unsigned in a class-type template argument",
          tidy_name(
            "struct value_holder<struct two_members<enum member_enum,unsigned int>{enum member_enum:1,unsigned int:3}>",
            msvc_type{}
          ),
          "value_holder<two_members<member_enum, unsigned int>{1, 3}>"s
    );

    check(equality,
          "MSVC's class template member of a class-type template argument",
          tidy_name(
            "struct value_holder<struct two_members<struct two_members<int,int>,int>"
            "{struct two_members<int,int>{int:1,int:2},int:3}>",
            msvc_type{}
          ),
          "value_holder<two_members<two_members<int, int>, int>{two_members<int, int>{1, 2}, 3}>"s
    );

    check(equality,
          "MSVC's qualified enumeration member of a class-type template argument",
          tidy_name("struct value_holder<struct two_members<int,enum ns::E>{int:1,enum ns::E:1}>", msvc_type{}),
          "value_holder<two_members<int, ns::E>{1, 1}>"s
    );

    check(equality,
          "MSVC's enumeration nested in a class template, in a class-type template argument",
          tidy_name(
            "struct value_holder<struct two_members<enum enclosing<int,float>::nested,int>"
            "{enum enclosing<int,float>::nested:1,int:3}>",
            msvc_type{}
          ),
          "value_holder<two_members<enclosing<int, float>::nested, int>{1, 3}>"s
    );

    check(equality,
          "MSVC's NaNs in a class-type template argument",
          tidy_name("struct value_holder<struct bounds<float>{float:nan,float:-nan(ind)}>", msvc_type{}),
          "value_holder<bounds<float>{nan, -nan}>"s
    );

    check(equality,
          "MSVC's negative NaN as a template argument",
          tidy_name("struct value_holder<-nan(ind)>", msvc_type{}),
          "value_holder<-nan>"s
    );
  }

  void output_free_test::test_array_iterators()
  {
    check(equality,
          "A subrange of a std::array's const iterators",
          demangle<std::ranges::subrange<std::array<double, 2>::const_iterator>>(),
          "std::ranges::subrange<double const*, double const*, 1>"s
    );

    check(equality,
          "A subrange of a std::array's const reverse iterators",
          demangle<std::ranges::subrange<std::array<double, 2>::const_reverse_iterator>>(),
          "std::ranges::subrange<std::reverse_iterator<double const*>, std::reverse_iterator<double const*>, 1>"s
    );

    check(equality,
          "A subrange of a std::array's mutable iterators",
          demangle<std::ranges::subrange<std::array<int, 12>::iterator>>(),
          "std::ranges::subrange<int*, int*, 1>"s
    );

    check(equality,
          "A subrange of the const iterators of a std::array of class template type",
          demangle<std::ranges::subrange<std::array<std::pair<int, float>, 3>::const_iterator>>(),
          "std::ranges::subrange<std::pair<int, float> const*, std::pair<int, float> const*, 1>"s
    );

    check(equality,
          "A std::array's const iterator over a std::array's const iterators",
          demangle<std::array<std::array<int, 2>::const_iterator, 3>::const_iterator>(),
          "int const* const*"s
    );

    check(equality,
          "A std::array's const iterator over a std::array's mutable iterators",
          demangle<std::array<std::array<int, 2>::iterator, 3>::const_iterator>(),
          "int* const*"s
    );

    check(equality,
          "A const std::array iterator in a pair",
          demangle<std::pair<const std::array<int, 2>::iterator, int>>(),
          "std::pair<int* const, int>"s
    );

    check(equality,
          "MSVC's std::array const iterator over a std::array's const iterators",
          tidy_name("class std::_Array_const_iterator<class std::_Array_const_iterator<int,2>,3>", msvc_type{}),
          "int const* const*"s
    );

    check(equality,
          "MSVC's std::array const iterator over a std::array's mutable iterators",
          tidy_name("class std::_Array_const_iterator<class std::_Array_iterator<int,2>,3>", msvc_type{}),
          "int* const*"s
    );

    check(equality,
          "MSVC's const std::array iterator in a pair",
          tidy_name("struct box<struct std::pair<class std::_Array_iterator<int,2> const ,int> >", msvc_type{}),
          "box<std::pair<int* const, int> >"s
    );

    check(equality,
          "MSVC's pointer to a const std::array const iterator",
          tidy_name("struct box<class std::_Array_const_iterator<int,2> const * __ptr64>", msvc_type{}),
          "box<int const* const*>"s
    );

    check(equality,
          "MSVC's subrange of a std::array's const iterators",
          tidy_name(
            "class std::ranges::subrange<class std::_Array_const_iterator<double,2>,"
            "class std::_Array_const_iterator<double,2>,1>",
            msvc_type{}
          ),
          "std::ranges::subrange<double const*, double const*, 1>"s
    );

    check(equality,
          "MSVC's subrange of a std::array's const reverse iterators",
          tidy_name(
            "class std::ranges::subrange<class std::reverse_iterator<class std::_Array_const_iterator<double,2> >,"
            "class std::reverse_iterator<class std::_Array_const_iterator<double,2> >,1>",
            msvc_type{}
          ),
          "std::ranges::subrange<std::reverse_iterator<double const*>, std::reverse_iterator<double const*>, 1>"s
    );

    check(equality,
          "MSVC's subrange of a one-element std::array's const iterators",
          tidy_name(
            "class std::ranges::subrange<class std::_Array_const_iterator<float,1>,"
            "class std::_Array_const_iterator<float,1>,1>",
            msvc_type{}
          ),
          "std::ranges::subrange<float const*, float const*, 1>"s
    );

    check(equality,
          "MSVC's subrange of a std::array's mutable iterators",
          tidy_name(
            "class std::ranges::subrange<class std::_Array_iterator<int,12>,class std::_Array_iterator<int,12>,1>",
            msvc_type{}
          ),
          "std::ranges::subrange<int*, int*, 1>"s
    );

    check(equality,
          "MSVC's subrange of the const iterators of a std::array of class template type",
          tidy_name(
            "class std::ranges::subrange<class std::_Array_const_iterator<struct std::pair<int,float>,3>,"
            "class std::_Array_const_iterator<struct std::pair<int,float>,3>,1>",
            msvc_type{}
          ),
          "std::ranges::subrange<std::pair<int, float> const*, std::pair<int, float> const*, 1>"s
    );
  }

  void output_free_test::test_relative_reporting_path()
  {
    check(equality, "No ..",     path_for_reporting(      "Tests/foo.cpp", ""), fs::path{"Tests/foo.cpp"});
    check(equality, "Single ..", path_for_reporting(   "../Tests/foo.cpp", ""), fs::path{"Tests/foo.cpp"});
    check(equality, "Double ..", path_for_reporting("../../Tests/foo.cpp", ""), fs::path{"Tests/foo.cpp"});
  }

  void output_free_test::test_absolute_reporting_path()
  {
    const auto root{fs::current_path().root_path()};
    const auto testRepo{root / "Tests"}, file{testRepo / "foo.cpp"};

    check(equality, "File in repo", path_for_reporting(file, testRepo), fs::path{"Tests/foo.cpp"});
  }
}
