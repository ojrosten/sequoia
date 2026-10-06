////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StreamingFreeTest.hpp"
#include "Utilities/TestUtilities.hpp"

#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/TestFramework/Macros.hpp"

import std;
import sequoia.platform_specific;
import sequoia.streaming;
import sequoia.test_framework;
import sequoia.text_processing;

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
    test_replace_contents();
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

    // Under Linux, Unwritable links to /dev/full, which opens and then fails
    // every write: the failure this check is for. No portable path behaves
    // so. Elsewhere a directory stands in, failing at the open instead, so
    // the check count and the message are the same on every platform.
    const auto unwritable{scratchpad_materials() /= "Unwritable"};
    if constexpr(with_linux_v)
    {
      fs::create_symlink("/dev/full", unwritable);
      check("The stand-in for a failing write is present", fs::is_character_file(unwritable));
    }
    else
    {
      fs::create_directory(unwritable);
      check("The stand-in for a failing write is present", fs::is_directory(unwritable));
    }

    check_exception_thrown<std::runtime_error>(
      "A write which fails",
      [&unwritable]() { write_to_file(unwritable, "Hello!", std::ios_base::out); });

    read_modify_write(working_materials() /= "Foo.txt", [](std::string& s) { capitalize(s);  });
    check(equivalence, "", working_materials() /= "Foo.txt", predictive_materials() /= "Foo.txt");
  }

  void streaming_free_test::test_replace_contents()
  {
    using namespace std::string_literals;

    const auto file{scratchpad_materials() /= "Replaced.txt"};
    const auto partial{fs::path{file} += ".partial"};

    write_to_file(file, "Previous", std::ios_base::out);
    check(equality,
          "A quiet replacement which succeeds",
          replace_contents_quietly(file, "Quiet", write_mode::text),
          std::optional<fs::path>{});
    check(equality, "Quietly replaced contents", read_to_string(file, std::ios_base::in), std::optional{"Quiet"s});

    replace_contents(file, "Replacement", write_mode::text);
    check(equality, "Replaced contents", read_to_string(file, std::ios_base::in), std::optional{"Replacement"s});
    check("No temporary file remains after a replacement", !fs::exists(partial));

    {
      // A client's entries hold the first two names tried for the created
      // file: a file, then a symbolic link whose target does not exist. The
      // names are the implementation's choice, which these checks pin.
      // Making a symbolic link under Windows needs a privilege, so there a
      // directory stands in for the link.
      const transient_file clientsFile{partial, "The client's"};
      const auto secondName{fs::path{file} += ".1.partial"};
      if constexpr(with_windows_v)
        fs::create_directory(secondName);
      else
        fs::create_symlink(scratchpad_materials() /= "Nowhere", secondName);

      replace_contents(file, "Beside the client's", write_mode::text);
      check(equality,
            "A replacement beside a client's entries",
            read_to_string(file, std::ios_base::in),
            std::optional{"Beside the client's"s});
      check(equality,
            "A replacement leaves a client's file at <file>.partial",
            read_to_string(partial, std::ios_base::in),
            std::optional{"The client's"s});
      check("A replacement leaves a client's entry at <file>.1.partial",
            with_windows_v ? fs::is_directory(secondName) : fs::is_symlink(secondName));
      check("No temporary file remains beside a client's entries", !fs::exists(fs::path{file} += ".2.partial"));
    }

    // No file can be created in a directory which does not exist
    const auto orphan{scratchpad_materials() /= "Absent/Replaced.txt"};
    check(equality,
          "A quiet replacement whose write fails",
          replace_contents_quietly(orphan, "Lost", write_mode::text),
          std::optional{fs::path{orphan} += ".partial"});
    check_exception_thrown<std::runtime_error>(
      "A replacement whose write fails",
      [&orphan]() { replace_contents(orphan, "Lost", write_mode::text); });

    const auto directory{scratchpad_materials() /= "Directory"};
    fs::create_directory(directory);

    check(equality,
          "A quiet replacement whose rename fails",
          replace_contents_quietly(directory, "Lost", write_mode::text),
          std::optional{directory});
    check("A quiet replacement whose rename fails removes its temporary file",
          !fs::exists(fs::path{directory} += ".partial"));
    check_exception_thrown<std::runtime_error>(
      "A replacement whose rename fails",
      [&directory]() { replace_contents(directory, "Lost", write_mode::text); });
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
