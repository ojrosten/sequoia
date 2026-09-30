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
        return compilation_record{.object{file(record.object_index)}, .inputs{record.input_indices | std::views::transform(file) | std::ranges::to<std::vector>()}};
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

    void write_utf16le(std::ostream& out, std::u16string_view text)
    {
      for(const char16_t unit : text)
      {
        out.put(static_cast<char>(unit & 0xFF));
        out.put(static_cast<char>(unit >> 8));
      }
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

  /* Two logs, each UTF-16 behind a byte order mark. For each record with inputs, each log has a `^`-led line naming
     the first input. Beneath it, the read log lists the other inputs and the write log lists the object. Every path
     is spelled by `to_tracker_spelling`.
   */
  void write_tlogs(const fs::path& tlogDir, std::span<const compilation_record> records)
  {
    fs::create_directories(tlogDir);
    std::ofstream read{tlogDir / "CL.read.1.tlog", std::ios_base::binary}, write{tlogDir / "CL.write.1.tlog", std::ios_base::binary};
    if(!read || !write)
      throw std::runtime_error{"Unable to write tracking logs in " + tlogDir.generic_string()};

    for(auto& out : {&read, &write})
    {
      out->write("\xFF\xFE", 2);
    }

    for(const auto& record : records)
    {
      if(record.inputs.empty())
        continue;

      const auto source{u"^" + to_tracker_spelling(record.inputs.front()) + u"\r\n"};

      write_utf16le(read, source);
      for(const auto& input : record.inputs | std::views::drop(1))
      {
        write_utf16le(read, to_tracker_spelling(input) + u"\r\n");
      }

      write_utf16le(write, source);
      write_utf16le(write, to_tracker_spelling(record.object) + u"\r\n");
    }
  }
}
