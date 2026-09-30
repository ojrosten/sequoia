////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file
    \brief Definitions for BuildArtefactsTestingUtilities.hpp
 */

#include "BuildArtefactsTestingUtilities.hpp"

#include "sequoia/TextProcessing/Characters.hpp"

#include <cstdint>
#include <fstream>
#include <map>
#include <ranges>
#include <stdexcept>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  std::ostream& operator<<(std::ostream& s, const compilation_record& record)
  {
    s << record.object.generic_string();
    for(const auto& input : record.inputs)
    {
      s << "\n  " << input.generic_string();
    }

    return s;
  }

  [[nodiscard]]
  std::vector<compilation_record> expand(const compilations& c)
  {
    auto spelledOut{
      [&c](const compilations::record& record) {
        auto file{[&c](compilations::file_index i){ return c.files.at(i); }};
        return compilation_record{
                 .object{file(record.object_index)},
                 .inputs{record.input_indices | std::views::transform(file) | std::ranges::to<std::vector>()}
               };
      }
    };

    return c.records | std::views::transform(spelledOut) | std::ranges::to<std::vector>();
  }

  namespace
  {
    void write_word(std::ostream& out, std::uint32_t word)
    {
      out.write(reinterpret_cast<const char*>(&word), sizeof(word));
    }
  }

  /* The layout the reader understands, re-spelled: the signature line and version word, a path
     record for each path as it is first met - the path NUL-padded to a multiple of four bytes, then
     the complement of its id - and a deps record per object, its size word carrying the high bit,
     with a zero time stamp. Should this and the reader disagree, the round trip in
     build_artefacts_free_test says so, and the reader is held to a log ninja wrote as well.
   */
  void write_ninja_deps(const fs::path& log, std::span<const compilation_record> records)
  {
    std::ofstream out{log, std::ios_base::binary};
    if(!out)
      throw std::runtime_error{"Unable to open " + log.generic_string()};

    constexpr std::string_view signature{"# ninjadeps\n"};
    constexpr std::uint32_t depsFlag{0x80000000u};

    out.write(signature.data(), static_cast<std::streamsize>(signature.size()));
    write_word(out, 4);

    std::map<std::string, std::uint32_t> ids{};
    auto idOf{
      [&](const fs::path& p) {
        const auto key{p.generic_string()};
        if(const auto found{ids.find(key)}; found != ids.end())
          return found->second;

        const auto id{static_cast<std::uint32_t>(ids.size())};
        ids.emplace(key, id);

        const std::size_t padding{(4 - key.size() % 4) % 4};
        write_word(out, static_cast<std::uint32_t>(key.size() + padding + 4));
        out.write(key.data(), static_cast<std::streamsize>(key.size()));
        for(std::size_t i{}; i < padding; ++i)
        {
          out.put('\0');
        }
        write_word(out, ~id);

        return id;
      }
    };

    for(const auto& record : records)
    {
      const auto objectId{idOf(record.object)};
      const auto inputIds{record.inputs | std::views::transform(idOf) | std::ranges::to<std::vector>()};

      write_word(out, depsFlag | static_cast<std::uint32_t>(4 * (1 + 2 + inputIds.size())));
      write_word(out, objectId);
      write_word(out, 0);
      write_word(out, 0);
      for(const auto id : inputIds)
      {
        write_word(out, id);
      }
    }
  }

  [[nodiscard]]
  std::u16string to_tracker_spelling(const fs::path& p)
  {
    constexpr char16_t asciiEnd{0x80};
    auto upper{
      [](char16_t c){ return (c < asciiEnd) ? static_cast<char16_t>(to_uppercase(static_cast<char>(c))) : c; }
    };

    return p.u16string() | std::views::transform(upper) | std::ranges::to<std::u16string>();
  }

  [[nodiscard]]
  std::u16string tracker_line(const fs::path& file)
  {
    return to_tracker_spelling(file) + u"\r\n";
  }

  [[nodiscard]]
  std::u16string tracker_sources_line(std::initializer_list<fs::path> sources)
  {
    auto spelling{[](const fs::path& source){ return to_tracker_spelling(source); }};
    const auto spellings{
        sources
      | std::views::transform(spelling)
      | std::views::join_with(u'|')
      | std::ranges::to<std::u16string>()
    };

    return u"^" + spellings + u"\r\n";
  }

  void write_tlog(const fs::path& log, std::u16string_view text)
  {
    std::ofstream out{log, std::ios_base::binary};
    if(!out)
      throw std::runtime_error{"Unable to write tracker log " + log.generic_string()};

    out.write("\xFF\xFE", 2);
    for(const char16_t unit : text)
    {
      out.put(static_cast<char>(unit & 0xFF));
      out.put(static_cast<char>(unit >> 8));
    }
  }

  /* Two logs. For each record with inputs, each log has a line naming the first input. Beneath it, the read log lists
     the other inputs and the write log lists the object.
   */
  void write_tlogs(const fs::path& tlogDir, std::span<const compilation_record> records)
  {
    std::u16string read{}, write{};
    for(const auto& record : records)
    {
      if(record.inputs.empty())
        continue;

      const auto source{tracker_sources_line({record.inputs.front()})};

      read.append(source);
      for(const auto& input : record.inputs | std::views::drop(1))
      {
        read.append(tracker_line(input));
      }

      write.append(source).append(tracker_line(record.object));
    }

    fs::create_directories(tlogDir);
    write_tlog(tlogDir / "CL.read.1.tlog",  read);
    write_tlog(tlogDir / "CL.write.1.tlog", write);
  }
}
