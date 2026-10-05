////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StreamingFreeTest.hpp"
#include "sequoia/PlatformSpecific/Preprocessor.hpp"
#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"
#include "sequoia/TestFramework/SumTypeCheckers.hpp"

#include <cstdint>
#include <fstream>
#include <functional>
#include <limits>
#include <sstream>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  [[nodiscard]]
  fs::path streaming_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void streaming_free_test::run_tests()
  {
    test_files();
    test_peek_for_more();
    test_parse_integer();
    test_extract_field();
    test_extract_text();
  }

  void streaming_free_test::test_files()
  {
    using namespace std::string_literals;

    check(equality, "", read_to_string(working_materials() /= "Foo.txt", std::ios_base::in), std::optional{"hello, World"s});
    check(equality, "", read_to_string(working_materials() /= "Bar.txt", std::ios_base::in), std::optional<std::string>{});

    write_to_file(working_materials() /= "Empty.txt", "", std::ios_base::out);
    check(equality, "An empty file", read_to_string(working_materials() /= "Empty.txt", std::ios_base::in), std::optional{""s});

    write_to_file(working_materials() /= "Lines.txt", "a\r\nb", std::ios_base::binary);
    check(equality, "Binary mode keeps every byte", read_to_string(working_materials() /= "Lines.txt", std::ios_base::binary), std::optional{"a\r\nb"s});

    check_exception_thrown<std::runtime_error>(
      reporter{""},
      [this]() { read_modify_write(working_materials() /= "Bar.txt", [](std::string& s) { capitalize(s);  }); });

    check_exception_thrown<std::runtime_error>(
      reporter{""},
      [this]() { write_to_file(working_materials() /= "Baz.txt", "Hello!", std::ios_base::out | std::ios_base::noreplace); });

    // Under Linux, /dev/full opens and then fails every write: the failure
    // this check is for. No portable path behaves so. Elsewhere a directory
    // stands in, failing at the open instead, so the check count is the same
    // on every platform.
    const fs::path unwritable{with_linux_v ? fs::path{"/dev/full"} : working_materials()};
    check("The stand-in for a failing write is present",
          with_linux_v ? fs::is_character_file(unwritable) : fs::is_directory(unwritable));
    check("A write which fails is reported", !try_write_to_file(unwritable, "Hello!", std::ios_base::out));

    read_modify_write(working_materials() /= "Foo.txt", [](std::string& s) { capitalize(s);  });
    check(equivalence, "", working_materials() /= "Foo.txt", predictive_materials() /= "Foo.txt");
  }

  void streaming_free_test::test_peek_for_more()
  {
    {
      std::stringstream s{};
      check("Peeking finds nothing more in an exhausted stream", !peek_for_more(s));
      check("A stream with nothing left fails", s.fail());
      check("A stream with nothing left is not bad", !s.bad());
    }

    {
      std::stringstream s{"a"};
      check("Peeking finds a character left", static_cast<bool>(peek_for_more(s)));
      check("A stream with a character left does not fail", s.good());
      check(equality, "The character left is not consumed", static_cast<char>(s.get()), 'a');
    }
  }

  void streaming_free_test::test_parse_integer()
  {
    check(equality, "A decimal integer", parse_integer<int>("42", "a count"), 42);
    check(equality, "A negative integer", parse_integer<int>("-7", "a count"), -7);
    check(equality, "The largest value of the type", parse_integer<std::uint8_t>("255", "a count"), std::uint8_t{255});

    check_exception_thrown<std::runtime_error>("Characters after the integer",
                                               []() { return parse_integer<int>("42x", "a count"); });
    check_exception_thrown<std::runtime_error>("Whitespace before the integer",
                                               []() { return parse_integer<int>(" 42", "a count"); });
    check_exception_thrown<std::runtime_error>("No characters at all",
                                               []() { return parse_integer<int>("", "a count"); });
    check_exception_thrown<std::runtime_error>("An integer beyond the type's range",
                                               []() { return parse_integer<std::uint8_t>("256", "a count"); });
    check_exception_thrown<std::runtime_error>("A negative integer, for an unsigned type",
                                               []() { return parse_integer<unsigned>("-1", "a count"); });
  }

  void streaming_free_test::test_extract_field()
  {
    {
      std::stringstream s{"key: value\n"};
      check(
        equality,
        "The rest of a line after its key",
        extract_field(s, "key: ", std::identity{}),
        std::string{"value"}
      );
    }

    auto extractKey{
      [](std::string text) {
        return [text{std::move(text)}]() {
          std::stringstream s{text};
          return extract_field(s, "key: ", std::identity{});
        };
      }
    };

    check_exception_thrown<std::runtime_error>("A line not beginning with the key", extractKey("other: value\n"));
    check_exception_thrown<std::runtime_error>("No line left", extractKey(""));
  }

  void streaming_free_test::test_extract_text()
  {
    {
      std::stringstream s{"a\nb\nrest\n"};
      check(equality, "Text holding a line break", extract_text(s, 3), std::string{"a\nb"});

      std::string next{};
      std::getline(s, next);
      check(equality, "The line break after the text is read too", next, std::string{"rest"});
    }

    {
      std::stringstream s{"\n"};
      check(equality, "No text, then a line break", extract_text(s, 0), std::string{});
    }

    auto extractThree{
      [](std::string text) {
        return [text{std::move(text)}]() {
          std::stringstream s{text};
          return extract_text(s, 3);
        };
      }
    };

    check_exception_thrown<std::runtime_error>("Fewer characters than asked for", extractThree("ab"));
    check_exception_thrown<std::runtime_error>("No line break after the text", extractThree("abcd\n"));
    check_exception_thrown<std::runtime_error>("The end of the stream after the text", extractThree("abc"));

    check_exception_thrown<std::runtime_error>(
      "A stream which has failed",
      []() {
        std::stringstream s{"abc\n"};
        s.setstate(std::ios_base::failbit);
        return extract_text(s, 3);
      }
    );

    check_exception_thrown<std::runtime_error>(
      "A length more than a stream iterator can count",
      []() {
        std::stringstream s{"abc\n"};
        return extract_text(s, std::numeric_limits<std::size_t>::max());
      }
    );
  }
}
