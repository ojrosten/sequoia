////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for BuildArtefacts.hpp
 */

#include "sequoia/TestFramework/BuildArtefacts.hpp"

#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TextProcessing/Patterns.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <ranges>
#include <set>
#include <span>
#include <stdexcept>
#include <string_view>
#include <tuple>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  std::ostream& operator<<(std::ostream& s, const compilation_record& record)
  {
    s << record.object.generic_string();
    for(const auto& input : record.inputs) s << "\n  " << input.generic_string();

    return s;
  }

  namespace
  {
    /* The log's layout, as ninja's deps_log.cc writes it:

       1. The signature line, then a version word. Version 4, which ninja has written since 1.11,
          is supported; earlier versions are not.
       2. Records, each led by a size word whose high bit says which of two kinds it is.
       3. A path record: the path, NUL-padded to a multiple of four bytes, then a checksum which is
          the bitwise complement of the path's id. Ids are assigned by position, counting from zero.
       4. A deps record: the id of the object, an eight-byte time stamp, then the ids of the inputs.

       Words are in the host's byte order, since the log is only ever read on the machine which
       wrote it. Three things follow for the reader:

       1. The log is append-only, so the last deps record for an object is the one which holds, as
          when ninja reads it.
       2. If ninja was interrupted while appending, the last record may be incomplete, its size word
          promising more bytes than the file holds. Ninja ignores such a record and truncates the
          file; this reader ignores it too, returning the records before it.
       3. Paths are as the compiler reported them, so a relative one is relative to the build
          directory.
     */
    constexpr std::string_view signature{"# ninjadeps\n"};
    constexpr std::uint32_t deps_flag{0x80000000u}, max_record_size{(1u << 19) - 1};
    constexpr std::size_t word_size{sizeof(std::uint32_t)};

    [[nodiscard]]
    std::string malformed_error(const fs::path& file, std::string_view what)
    {
      return std::format("Unable to read {}: {}", file.generic_string(), what);
    }

    [[nodiscard]]
    std::string read(const fs::path& file)
    {
      auto text{read_to_string(file, std::ios_base::binary)};
      if(!text)
        throw std::runtime_error{"Unable to open " + file.generic_string()};

      return std::move(*text);
    }

    [[nodiscard]]
    std::vector<std::byte> read_bytes(const fs::path& file)
    {
      return read(file) | std::views::transform([](char c){ return static_cast<std::byte>(c); })
                        | std::ranges::to<std::vector>();
    }

    [[nodiscard]]
    bool begins_with(std::span<const std::byte> bytes, std::string_view text)
    {
      auto same{[](std::byte b, char c){ return std::to_integer<char>(b) == c; }};

      return std::ranges::equal(bytes | std::views::take(text.size()), text, same);
    }

    /// What a record's leading word says of it
    struct record_header
    {
      std::size_t size{};
      bool is_deps{};
    };

    /// A dependency log's bytes, each read of which is checked against its end
    class dependency_log
    {
    public:
      explicit dependency_log(const fs::path& file) : m_File{file}, m_Bytes{read_bytes(file)} {}

      [[nodiscard]]
      std::size_t size() const noexcept { return m_Bytes.size(); }

      [[nodiscard]]
      bool begins_with(std::string_view text) const { return testing::begins_with(m_Bytes, text); }

      [[nodiscard]]
      std::uint32_t word(std::size_t at) const
      {
        if(at + word_size > m_Bytes.size())
          throw std::runtime_error{error("a word past the end")};

        std::uint32_t word{};
        std::memcpy(&word, m_Bytes.data() + at, sizeof(word));
        return word;
      }

      [[nodiscard]]
      record_header header(std::size_t at) const
      {
        const auto word{this->word(at)};
        const record_header header{.size{word & ~deps_flag}, .is_deps{(word & deps_flag) != 0}};
        if(header.size > max_record_size)
          throw std::runtime_error{error("a record exceeds ninja's maximum size")};

        return header;
      }

      /// The text of `count` bytes, less the NUL padding which ends it
      [[nodiscard]]
      std::string text(std::size_t at, std::size_t count) const
      {
        if(at + count > m_Bytes.size())
          throw std::runtime_error{error("text past the end")};

        return m_Bytes | std::views::drop(at)
                       | std::views::take(count)
                       | std::views::transform([](std::byte b){ return std::to_integer<char>(b); })
                       | std::views::take_while([](char c){ return c != '\0'; })
                       | std::ranges::to<std::string>();
      }

      [[nodiscard]]
      std::string error(std::string_view what) const { return malformed_error(m_File, what); }
    private:
      fs::path m_File;
      std::vector<std::byte> m_Bytes;
    };

    /// A character of a ninja file is escaped when an odd number of `$` immediately precede it, `$$` being an escaped dollar.
    [[nodiscard]]
    bool escaped_at(std::string_view text, std::size_t i)
    {
      auto dollars{
          text.substr(0, i)
        | std::views::reverse
        | std::views::take_while([](char c){ return c == '$'; })
      };

      return std::ranges::distance(dollars) % 2 == 1;
    }

    /// The path a ninja token spells: `$ ` stands for a space, `$:` for a colon and `$$` for a dollar.
    [[nodiscard]]
    std::string plain_spelling(std::string_view token)
    {
      auto kept{
        [token](const auto& indexed) {
          const auto [i, c]{indexed};
          return (c != '$') || escaped_at(token, static_cast<std::size_t>(i));
        }
      };

      return token | std::views::enumerate
                   | std::views::filter(kept)
                   | std::views::values
                   | std::ranges::to<std::string>();
    }

    /// Splits on ninja's bare spaces, so that `a$ b` is one token, and gives each token's plain spelling.
    [[nodiscard]]
    std::vector<std::string> tokens(std::string_view text)
    {
      auto isSeparator{
        [text](const auto& indexed) {
          const auto [i, c]{indexed};
          return (c == ' ') && !escaped_at(text, static_cast<std::size_t>(i));
        }
      };

      auto neitherSeparates{[isSeparator](const auto& lhs, const auto& rhs){ return !isSeparator(lhs) && !isSeparator(rhs); }};
      auto isToken{[isSeparator](auto chunk){ return !isSeparator(*chunk.begin()); }};
      auto spelling{[](auto chunk){ return plain_spelling(chunk | std::views::values | std::ranges::to<std::string>()); }};

      return text | std::views::enumerate
                  | std::views::chunk_by(neitherSeparates)
                  | std::views::filter(isToken)
                  | std::views::transform(spelling)
                  | std::ranges::to<std::vector>();
    }
  }

  [[nodiscard]]
  std::vector<compilation_record> read_ninja_deps(const fs::path& file)
  {
    const dependency_log log{file};
    if(!log.begins_with(signature))
      throw std::runtime_error{log.error("not a ninja dependency log")};

    std::size_t pos{signature.size()};
    if(const auto version{log.word(pos)}; version != 4)
      throw std::runtime_error{log.error(std::format("version {} is not supported", version))};

    pos += word_size;

    std::vector<fs::path> paths{};
    std::map<std::uint32_t, std::vector<std::uint32_t>> deps{};

    while(pos + word_size <= log.size())
    {
      constexpr std::size_t stampWidth{8};
      const auto [size, isDeps]{log.header(pos)};
      pos += word_size;
      // An incomplete final record, which an interrupted ninja leaves
      if(pos + size > log.size())
        break;

      if(isDeps)
      {
        if((size < word_size + stampWidth) || ((size - word_size - stampWidth) % word_size))
          throw std::runtime_error{log.error("a deps record of impossible size")};

        const auto outputId{log.word(pos)};
        if(outputId >= paths.size())
          throw std::runtime_error{log.error("a deps record names an output not yet seen")};

        auto inputIds{
            std::views::iota(pos + word_size + stampWidth, pos + size)
          | std::views::stride(word_size)
          | std::views::transform([&log](std::size_t at){ return log.word(at); })
          | std::ranges::to<std::vector>()
        };

        if(std::ranges::any_of(inputIds, [&paths](std::uint32_t id){ return id >= paths.size(); }))
          throw std::runtime_error{log.error("a deps record names an input not yet seen")};

        deps[outputId] = std::move(inputIds);
      }
      else
      {
        if(size < word_size)
          throw std::runtime_error{log.error("a path record of impossible size")};

        auto path{log.text(pos, size - word_size)};

        if(log.word(pos + size - word_size) != ~static_cast<std::uint32_t>(paths.size()))
          throw std::runtime_error{log.error("a path record's checksum does not match its position")};

        paths.emplace_back(std::move(path));
      }

      pos += size;
    }

    auto pathOf{[&paths](std::uint32_t id){ return paths[id]; }};

    return deps | std::views::transform(
                    [pathOf](const auto& record){
                      const auto& [objectId, inputIds]{record};
                      return
                        compilation_record{
                          .object{pathOf(objectId)},
                          .inputs{inputIds | std::views::transform(pathOf) | std::ranges::to<std::vector>()}
                        };
                    }
                  )
                | std::ranges::to<std::vector>();
  }

  namespace
  {
    constexpr std::string_view byte_order_mark{"\xFF\xFE"};

    /// The tracker writes UTF-16, little-endian; a path is built from the code units themselves, so nothing is lost in a narrow encoding
    [[nodiscard]]
    std::u16string decode_utf16le(std::span<const std::byte> bytes)
    {
      constexpr std::size_t unitWidth{sizeof(char16_t)};

      auto unit{
        [](const auto& pair) {
          const auto [low, high]{pair};
          constexpr auto bitsPerByte{std::numeric_limits<unsigned char>::digits};
          return static_cast<char16_t>(std::to_integer<unsigned>(low) | (std::to_integer<unsigned>(high) << bitsPerByte));
        }
      };

      // The low byte of each unit, the high byte of each; zip ends at the shorter, so a stray final byte is dropped
      return std::views::zip(bytes | std::views::stride(unitWidth), bytes | std::views::drop(1) | std::views::stride(unitWidth))
           | std::views::transform(unit)
           | std::ranges::to<std::u16string>();
    }

    /// `CL.read.1.tlog`, or `CL.11932.read.1.tlog` where MSBuild has numbered the target's logs
    [[nodiscard]]
    bool is_tlog(const fs::path& file, std::string_view kind)
    {
      auto name{file.filename().string()};
      std::ranges::transform(name, name.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
      if(!name.starts_with("cl.") || !name.ends_with(".tlog"))
        return false;

      std::string_view rest{std::string_view{name}.substr(3)};
      const auto number{rest.find_first_not_of("0123456789")};
      if((number > 0) && (number != std::string_view::npos) && (rest[number] == '.'))
        rest.remove_prefix(number + 1);

      return rest.starts_with(std::string{kind}.append("."));
    }

    /// The tracker's logs: under each `^`-led line naming one or more sources, the files that compilation touched
    [[nodiscard]]
    std::map<std::u16string, std::vector<std::u16string>> read_tlog_entries(const fs::path& tlogDir, std::string_view kind)
    {
      std::map<std::u16string, std::vector<std::u16string>> entries{};
      for(const auto& entry : fs::directory_iterator(tlogDir))
      {
        if(!is_tlog(entry.path(), kind))
          continue;

        const auto encoded{read_bytes(entry.path())};
        if(!begins_with(encoded, byte_order_mark))
          throw std::runtime_error{malformed_error(entry.path(), "not the tracker's UTF-16")};

        const auto text{decode_utf16le(std::span{encoded}.subspan(byte_order_mark.size()))};
        auto withoutReturn{
          [](auto lineRange) {
            const std::u16string_view line{lineRange};
            return line.ends_with(u'\r') ? line.substr(0, line.size() - 1) : line;
          }
        };

        auto lines{
            std::views::split(text, u'\n')
          | std::views::transform(withoutReturn)
          | std::views::filter([](std::u16string_view line){ return !line.empty(); })
        };

        // A `^` line and the lines beneath it are one group; whatever precedes the first `^` names no source and is skipped
        auto beneathSameSource{[](std::u16string_view, std::u16string_view next){ return !next.starts_with(u'^'); }};
        for(auto group : lines | std::views::chunk_by(beneathSameSource))
        {
          const std::u16string_view head{*group.begin()};
          if(!head.starts_with(u'^'))
            continue;

          const auto sources{std::views::split(head.substr(1), u'|') | std::ranges::to<std::vector<std::u16string>>()};

          // A source is an entry even where it touched nothing else
          for(const auto& source : sources) entries.try_emplace(source);

          for(const std::u16string_view line : group | std::views::drop(1))
          {
            for(const auto& source : sources) entries[source].emplace_back(line);
          }
        }
      }

      return entries;
    }

    /** The tracker spells paths in upper case; the directories know how they are really spelled, and
        are asked once each:

        1. A filesystem which finds a file whatever its case answers through `weakly_canonical`.
        2. One which does not - ext4, where the fixtures also run - is walked a component at a time,
           each matched against its directory's entries without regard to case.
        3. What no directory holds keeps its spelling.
     */
    class case_recoverer
    {
    public:
      [[nodiscard]]
      const fs::path& operator()(const std::u16string& spelling)
      {
        auto found{m_Recovered.find(spelling)};
        if(found == m_Recovered.end())
        {
          const fs::path asSpelled{spelling};
          std::error_code error{};
          auto canonical{fs::weakly_canonical(asSpelled, error)};
          if(error)
            canonical = asSpelled;

          found = m_Recovered.emplace(spelling, fs::exists(canonical) ? std::move(canonical) : walk(canonical)).first;
        }

        return found->second;
      }
    private:
      std::map<std::u16string, fs::path> m_Recovered{};
      std::map<fs::path, std::vector<fs::path>> m_Listings{};

      [[nodiscard]]
      static bool equal_ignoring_case(const std::string& lhs, const std::string& rhs)
      {
        return std::ranges::equal(lhs, rhs, [](unsigned char l, unsigned char r){ return std::tolower(l) == std::tolower(r); });
      }

      /// The entry of `dir` spelled as `name` but for case, if there is one; the directory is listed once
      [[nodiscard]]
      std::optional<fs::path> entry_ignoring_case(const fs::path& dir, const fs::path& name)
      {
        auto listing{m_Listings.find(dir)};
        if(listing == m_Listings.end())
        {
          std::error_code error{};
          auto names{
              fs::directory_iterator{dir, error}
            | std::views::transform([](const fs::directory_entry& entry){ return entry.path().filename(); })
            | std::ranges::to<std::vector>()
          };

          listing = m_Listings.emplace(dir, std::move(names)).first;
        }

        const auto spelled{name.string()};
        auto sameButForCase{[&spelled](const fs::path& candidate){ return equal_ignoring_case(candidate.string(), spelled); }};
        const auto match{std::ranges::find_if(listing->second, sameButForCase)};

        return (match != listing->second.end()) ? std::optional{*match} : std::nullopt;
      }

      /// Components are matched against their directories until one is not found; those beyond it keep their spelling
      [[nodiscard]]
      fs::path walk(const fs::path& p)
      {
        using progress = std::pair<fs::path, bool>;

        auto step{
          [this](const progress& sofar, const fs::path& component) -> progress {
            const auto& [recovered, matching]{sofar};
            if(!matching)
              return {recovered / component, false};

            if(fs::exists(recovered / component))
              return {recovered / component, true};

            if(const auto match{entry_ignoring_case(recovered, component)})
              return {recovered / *match, true};

            return {recovered / component, false};
          }
        };

        return std::ranges::fold_left(p.relative_path(), progress{p.root_path(), true}, step).first;
      }
    };

    [[nodiscard]]
    bool has_extension(const fs::path& p, std::string_view ext)
    {
      auto found{p.extension().string()};
      std::ranges::transform(found, found.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

      return found == ext;
    }

    /// The value of `set(NAME "value")` in a CMake script
    [[nodiscard]]
    std::optional<std::string> cmake_script_value(std::string_view text, std::string_view name)
    {
      const auto [first, last]{find_sandwiched_text(text, std::string{"set("}.append(name).append(" \""), "\"")};
      if(first < last)
        return std::string{text.substr(first, last - first)};

      return std::nullopt;
    }

    /// The value of `NAME:TYPE=value` in a CMake cache
    [[nodiscard]]
    std::optional<std::string> cmake_cache_value(std::string_view text, std::string_view name)
    {
      const auto [first, last]{find_sandwiched_text(text, std::string{"\n"}.append(name).append(":"), "\n")};
      if(first >= last)
        return std::nullopt;

      auto entry{text.substr(first, last - first)};
      if(entry.ends_with('\r'))
        entry.remove_suffix(1);

      const auto equals{entry.find('=')};
      if(equals == std::string_view::npos)
        return std::nullopt;

      return std::string{entry.substr(equals + 1)};
    }

    /// A line of a ninja file with its trailing whitespace and carriage return removed
    [[nodiscard]]
    std::string_view trimmed(std::string_view line)
    {
      return line.substr(0, line.find_last_not_of(" \t\r") + 1);
    }

    /// The position of the first bare `sought`, one not escaped by a `$` before it
    [[nodiscard]]
    std::size_t find_bare(std::string_view text, char sought)
    {
      auto isBare{
        [text, sought](const auto& indexed) {
          const auto [i, c]{indexed};
          return (c == sought) && !escaped_at(text, static_cast<std::size_t>(i));
        }
      };

      const auto indexed{text | std::views::enumerate};
      const auto found{std::ranges::find_if(indexed, isBare)};

      return (found == indexed.end()) ? std::string_view::npos : static_cast<std::size_t>(std::get<0>(*found));
    }

    /** What `build.ninja` says of the current build: each object's source, keyed by the object's generic
        spelling. A build statement is `build <outputs> [| <implicit outputs>]: <rule> <inputs> [| <implicit>] [|| <order-only>]`.
     */
    [[nodiscard]]
    std::map<std::string, std::string> read_ninja_sources(const fs::path& buildFile)
    {
      constexpr std::string_view keyword{"build "};
      std::map<std::string, std::string> sourceOf{};

      const auto text{read(buildFile)};
      auto statements{
          std::views::split(text, '\n')
        | std::views::transform([](auto lineRange){ return trimmed(std::string_view{lineRange}); })
        | std::views::filter([keyword](std::string_view line){ return line.starts_with(keyword); })
      };

      for(const std::string_view statement : statements)
      {
        const auto colon{find_bare(statement, ':')};
        if(colon == std::string_view::npos)
          throw std::runtime_error{malformed_error(buildFile, "a build statement with no rule")};

        auto outputs{tokens(statement.substr(keyword.size(), colon - keyword.size()))};
        if(const auto implicit{std::ranges::find(outputs, std::string_view{"|"})}; implicit != outputs.end())
          outputs.erase(implicit, outputs.end());

        // The rule comes first, and a `|` or `||` ends the explicit inputs, so the source is the second token unless that is one of them
        const auto inputs{tokens(statement.substr(colon + 1))};
        if((inputs.size() > 1) && (inputs[1] != "|") && (inputs[1] != "||"))
        {
          // On Windows the generator writes `CMakeFiles\Foo.dir\Bar.cpp.obj` where the log, canonicalized by ninja,
          // has `CMakeFiles/Foo.dir/Bar.cpp.obj`
          for(const auto& output : outputs) sourceOf.emplace(fs::path{output}.generic_string(), inputs[1]);
        }
      }

      return sourceOf;
    }
  }

  /* The tracker's logs, as MSBuild writes them:

     1. `CL.read.*.tlog` lists, under each source, every file the compiler read; `CL.write.*.tlog`
        lists what it wrote, which is where the object is named.
     2. A source is a line beginning `^`. Sources compiled by one invocation share a line, separated
        by `|`, and so share what is listed beneath it.
     3. Both are UTF-16 with a byte order mark, and spell paths in upper case, so each path is put
        through the filesystem to recover its case.

     Hence, where sources share their writes, each object is given to the source whose stem or name
     it bears; what cannot be told apart is refused rather than guessed.
   */
  [[nodiscard]]
  std::vector<compilation_record> read_tlogs(const fs::path& tlogDir)
  {
    const auto reads{read_tlog_entries(tlogDir, "read")};
    const auto writes{read_tlog_entries(tlogDir, "write")};
    case_recoverer recover{};

    std::vector<compilation_record> records{};
    for(const auto& [source, read] : reads)
    {
      compilation_record record{};
      const auto& sourcePath{recover(source)};

      if(const auto written{writes.find(source)}; written != writes.end())
      {
        const auto objects{
            written->second
          | std::views::transform([&recover](const std::u16string& w){ return recover(w); })
          | std::views::filter([](const fs::path& path){ return has_extension(path, ".obj"); })
          | std::ranges::to<std::vector>()
        };

        // Sources compiled by one invocation share their writes; each object bears its source's stem, or its whole name where stems collide
        auto bearsSourcesName{
          [&sourcePath](const fs::path& o){ return (o.stem() == sourcePath.stem()) || (o.stem() == sourcePath.filename()); }
        };
        auto own{std::ranges::find_if(objects, bearsSourcesName)};
        if((own == objects.end()) && (objects.size() == 1))
          own = objects.begin();

        if(own != objects.end())
          record.object = *own;
        else if(!objects.empty())
          throw std::runtime_error{
            std::format("The tracker's log in {} does not say which of {} objects {} produced",
                        tlogDir.generic_string(),
                        objects.size(),
                        sourcePath.filename().generic_string())
          };
      }

      if(record.object.empty())
        continue;

      // The source first, then what was read in the order the tracker recorded it, the source itself among them or not
      std::set<fs::path> seen{sourcePath};
      record.inputs.push_back(sourcePath);
      for(const auto& r : read)
      {
        if(const auto& path{recover(r)}; seen.insert(path).second)
          record.inputs.push_back(path);
      }

      records.push_back(std::move(record));
    }

    return records;
  }

  [[nodiscard]]
  build_tree read_build_tree(const fs::path& cacheFile)
  {
    build_tree tree{.build_directory{cacheFile.parent_path()}};

    tree.generator = cmake_cache_value(read(cacheFile), "CMAKE_GENERATOR").value_or("");

    if(const auto cmakeFiles{tree.build_directory / "CMakeFiles"}; fs::is_directory(cmakeFiles))
    {
      for(const auto& entry : fs::directory_iterator(cmakeFiles))
      {
        if(const auto info{entry.path() / "CMakeCXXCompiler.cmake"}; entry.is_directory() && fs::exists(info))
        {
          // Canonical, so that a directory reached through a symlink - macOS's SDK is one - compares equal to the files beneath it
          auto canonical{
            [](std::string_view dir) {
              std::error_code error{};
              const auto canonicalDir{fs::weakly_canonical(fs::path{dir}, error)};
              return error ? fs::path{dir}.lexically_normal() : canonicalDir;
            }
          };

          const auto directories{cmake_script_value(read(info), "CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES").value_or("")};
          tree.implicit_include_directories.append_range(
              std::views::split(directories, ';')
            | std::views::transform([](auto dirRange){ return std::string_view{dirRange}; })
            | std::views::filter([](std::string_view dir){ return !dir.empty(); })
            | std::views::transform(canonical)
          );
        }
      }
    }

    return tree;
  }

  /* By generator:

     1. Ninja: the objects `build.ninja` names, each with its source from its statement - which the
        dependency log omits where the compiler is MSVC - and the rest of its inputs from the log.
        The log's other records are not read: it is append-only, so it describes objects the build
        no longer has.
     2. Visual Studio: every `*.tlog` directory beneath the build directory in the executable's
        configuration, which is the name of the directory holding it.
     3. `Ninja Multi-Config` keeps its statements elsewhere and is not understood; nor is any other.
   */
  [[nodiscard]]
  std::vector<compilation_record> read_compilations(const build_tree& tree, const fs::path& executable)
  {
    std::vector<compilation_record> records{};

    if(tree.generator == "Ninja")
    {
      const auto log{tree.build_directory / ".ninja_deps"};
      if(!fs::exists(log))
        throw std::runtime_error{std::format("{} has no dependency log; has anything been built?", tree.build_directory.generic_string())};

      const auto sourceOf{read_ninja_sources(tree.build_directory / "build.ninja")};

      const auto logged{read_ninja_deps(log)};
      auto named{[&sourceOf](const compilation_record& record){ return sourceOf.contains(record.object.generic_string()); }};

      // MSVC reports the headers alone, so the source is added from the statement where the log lacks it
      auto withSource{
        [&sourceOf](compilation_record record) {
          if(const fs::path source{sourceOf.at(record.object.generic_string())}; !std::ranges::contains(record.inputs, source))
            record.inputs.insert(record.inputs.begin(), source);

          return record;
        }
      };

      records = logged | std::views::filter(named)
                       | std::views::transform(withSource)
                       | std::ranges::to<std::vector>();

      /* A log every one of whose records fails to match a statement is not an empty build: it is two
         spellings of one tree, and what follows would select nothing, forever, without a word.
       */
      if(records.empty() && !logged.empty())
        throw std::runtime_error{
          std::format("None of the objects {} records is named by build.ninja; are the two spelled differently?", log.generic_string())
        };
    }
    else if(tree.generator.starts_with("Visual Studio"))
    {
      const auto configuration{executable.parent_path().filename()};
      for(const auto& entry : fs::recursive_directory_iterator(tree.build_directory))
      {
        if(!entry.is_directory() || (entry.path().extension() != ".tlog") || (entry.path().parent_path().filename() != configuration))
          continue;

        records.append_range(read_tlogs(entry.path()));
      }
    }
    else
    {
      throw std::runtime_error{
        std::format("The build in {} was written by the {} generator, whose record of dependencies is not understood; "
                    "Ninja's and Visual Studio's are",
                    tree.build_directory.generic_string(),
                    tree.generator)
      };
    }

    return records;
  }
}
