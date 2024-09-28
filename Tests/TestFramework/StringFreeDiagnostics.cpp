////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "StringFreeDiagnostics.hpp"
#include "sequoia/TestFramework/ConcreteTypeCheckers.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path string_false_positive_free_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void string_false_positive_free_diagnostics::run_tests()
  {
    test_strings<std::string>();
    test_strings<std::string_view>();
    test_wstrings<std::wstring>();
    test_wstrings<std::wstring_view>();

    test_string_equivalences();
    test_wstring_equivalences();
  }

  template<class String>
  void string_false_positive_free_diagnostics::test_strings()
  {
    check(equality, report("Empty and non-empty strings"), String{""}, String{"Hello, World!"});
    check(equality, report("Empty and non-empty strings"), String{"Hello, World!"}, String{""});
    check(equality, report("Strings of differing length"), String{"what?!"}, String{"Hello, World!"});
    check(equality, report("Differing strings of same length"), String{"Hello, world?"}, String{"Hello, World!"});
    check(equality, report("Advice"), String{"Foo"}, String{"Bar"}, tutor{[](const String&, const String&) { return "Foo, my old nemesis"; }});

    const String longMessage{"This is a message which is sufficiently long for only a segment to be included when a string diff is performed"};
    const String longMessageWithDiffNearMiddle{"This is a message which is sufficiently long for only a segment to be incluxed when a string diff is performed"};
    const String longMessageWithDiffNearEnd{"This is a message which is sufficiently long for only a segment to be included when a string diff isxperformed"};

    check(equality, report("Empty string compared with long string"), String{""}, longMessage);
    check(equality, report("Long string with empty string"), longMessage, String{""});

    check(equality, report("Short string compared with long string"), String{"This is a mess"}, longMessage);
    check(equality, report("Long string with short string"), longMessage, String{"This is a mess"});

    check(equality, report("Strings differing by a newline"), String{"Hello\nWorld"}, String{"Hello, World"});
    check(equality, report("Strings differing by a newline"), String{"Hello, World"}, String{"Hello\nWorld"});
    check(equality, report("Strings differing by a newline at the start"), String{"\nHello, World"}, String{"Hello, World"});
    check(equality, report("Strings differing by a newline at the start"), String{"Hello, World"}, String{"\nHello, World"});
    check(equality, report("Empty string compared with newline"), String{""}, String{"\n"});
    check(equality, report("Empty string compared with newline"), String{"\n"}, String{""});
    check(equality, report("Strings differing from newline onwards"), String{"Hello, World"}, String{"Hello\n"});
    check(equality, report("Strings differing from newline onwards"), String{"Hello\n"}, String{"Hello, World"});
    check(equality, report("Strings differing from newline onwards"), String{"Hello, World"}, String{"Hello\nPeople"});
    check(equality, report("Strings differing from newline onwards"), String{"Hello\nPeople"}, String{"Hello, World"});
    check(equality, report("Output suppressed by a new line"), String{"Hello  World\nAnd so forth"}, String{"Hello, World\nAnd so forth"});
    check(equality, report("Difference on the second line"), String{"Hello, World\nAnd so furth"}, String{"Hello, World\nAnd so forth"});
    check(equality, report("Missing line"), String{"Hello, World\nAnd so forth"}, String{"Hello, World\n\nAnd so forth"});
    check(equality, report("Extra line"), String{"Hello, World\n\nAnd so forth"}, String{"Hello, World\nAnd so forth"});

    check(equality, report("Long strings compared with difference near middle"), longMessageWithDiffNearMiddle, longMessage);
    check(equality, report("Long strings compared with difference near middle"), longMessage, longMessageWithDiffNearMiddle);

    check(equality, report("Long strings compared with difference near end"), longMessageWithDiffNearEnd, longMessage);
    check(equality, report("Long strings compared with difference near end"), longMessage, longMessageWithDiffNearEnd);

    check(equivalence, report(""), String{"foo"}, "fo");
    check(equivalence, report(""), String{"foo"}, "fob", tutor{[](char, char) {
        return "Sort your chars out!";
      }});
  }

  template<class String>
  void string_false_positive_free_diagnostics::test_wstrings()
  {
    check(equality, report("Differing strings of same length"), String{L"Hello, world?"}, String{L"Hello, World!"});
    check(equality, report("Advice"), String{L"Foo"}, String{L"Bar"}, tutor{[](const String&, const String&) { return "Foo, my old nemesis"; }});
    check(equality, report("Missing line"), String{L"Hello, World\nAnd so forth"}, String{L"Hello, World\n\nAnd so forth"});
    check(equality, report("Extra line"), String{L"Hello, World\n\nAnd so forth"}, String{L"Hello, World\nAnd so forth"});
  }

  void string_false_positive_free_diagnostics::test_string_equivalences()
  {
    using namespace std::string_literals;
    using namespace std::string_view_literals;
    check(equivalence, report(""), "foo"s, "fob"sv);
    check(equivalence, report(""), "foo"sv, "fob"s);
  }

  void string_false_positive_free_diagnostics::test_wstring_equivalences()
  {
    check(equivalence, report(""), std::wstring{L"foo"}, std::wstring_view{L"fob"});
    check(equivalence, report(""), std::wstring_view{L"foo"}, std::wstring{L"fob"});
  }

  [[nodiscard]]
  std::filesystem::path string_false_negative_free_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void string_false_negative_free_diagnostics::run_tests()
  {
    test_strings();

    test_string_equivalences();
    test_wstring_equivalences();
  }

  void string_false_negative_free_diagnostics::test_strings()
  {
    check(equality, report("Differing strings"), std::string{"Hello, World!"}, std::string{"Hello, World!"});
    check(equivalence, report(""), std::string{"foo"}, "foo");
  }

  void string_false_negative_free_diagnostics::test_string_equivalences()
  {
    using namespace std::string_literals;
    using namespace std::string_view_literals;
    check(equivalence, report(""), "foo"s, "foo"sv);
  }

  void string_false_negative_free_diagnostics::test_wstring_equivalences()
  {
    check(equivalence, report(""), std::wstring{L"foo"}, std::wstring_view{L"foo"});
  }
}
