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
#include "sequoia/TestFramework/CMakeCache.hpp"

#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TextProcessing/Patterns.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    /// Hashes a path's spelling, so that a lookup keyed on a path's native string can take a view of one
    struct spelling_hash
    {
      using is_transparent = void;

      [[nodiscard]]
      std::size_t operator()(std::basic_string_view<fs::path::value_type> spelling) const noexcept
      {
        return std::hash<std::basic_string_view<fs::path::value_type>>{}(spelling);
      }
    };

    /** Numbers files as a reader assembles a `compilations`: the file numbered `i` becomes `files[i]`
        of the result.

        A file is identified by its path exactly as spelled. Two spellings of one file - `C:\a\b.cpp`
        and `C:/a/b.cpp`, a path through a symlink and its target - are two entries, so a reader must
        spell a file the same way each time it names it. A file's index is assigned when the file is
        first inserted, and never changes.
     */
    class path_table
    {
    public:
      path_table() = default;

      /// The files of a `compilations`, numbered as they already are
      /// \pre No two of `files` are spelled alike
      explicit path_table(std::vector<fs::path> files)
        : m_Files{std::move(files)}
        , m_IndexOfSpelling{index_by_spelling(m_Files)}
      {}

      /// The index of `p`, which is inserted if the table lacks it
      [[nodiscard]]
      compilations::file_index insert(const fs::path& p)
      {
        if(const auto found{m_IndexOfSpelling.find(std::basic_string_view{p.native()})}; found != m_IndexOfSpelling.end())
          return found->second;

        m_Files.push_back(p);
        return m_IndexOfSpelling.emplace(p.native(), m_Files.size() - 1).first->second;
      }

      /// \pre `i` is an index this table has given
      [[nodiscard]]
      const fs::path& operator[](compilations::file_index i) const { return m_Files[i]; }

      /// The files, numbered as they are here; the table is left empty
      [[nodiscard]]
      std::vector<fs::path> release_files() &&
      {
        m_IndexOfSpelling.clear();
        return std::move(m_Files);
      }
    private:
      using spelling_index = std::unordered_map<fs::path::string_type, compilations::file_index, spelling_hash, std::ranges::equal_to>;

      // The files in index order, and the inverse of that numbering, so that `insert` can tell in one
      // lookup whether a file is already numbered - which the tracker reader asks once per input of every record
      std::vector<fs::path> m_Files{};
      spelling_index m_IndexOfSpelling{};

      [[nodiscard]]
      static spelling_index index_by_spelling(const std::vector<fs::path>& files)
      {
        spelling_index indices{};
        for(const auto [i, file] : std::views::enumerate(files))
        {
          indices.emplace(file.native(), static_cast<compilations::file_index>(i));
        }

        return indices;
      }
    };
  }

  namespace
  {
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

    /// What a record's leading word says of the record
    struct record_header
    {
      std::size_t size{};
      bool is_deps{};
    };

    /// A dependency log's bytes; every read is checked against the end of the log
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

    /// A line of a ninja file with its trailing whitespace and carriage return removed
    [[nodiscard]]
    std::string_view trimmed(std::string_view line)
    {
      return line.substr(0, line.find_last_not_of(" \t\r") + 1);
    }

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

    /// A predicate on the enumerated characters of `text`: this one is `sought`, and bare
    [[nodiscard]]
    auto is_bare(std::string_view text, char sought)
    {
      return [text, sought](const auto& indexed) {
        const auto [i, c]{indexed};
        return (c == sought) && !escaped_at(text, static_cast<std::size_t>(i));
      };
    }

    /// The position of the first bare `sought`
    [[nodiscard]]
    std::size_t find_bare(std::string_view text, char sought)
    {
      const auto indexed{text | std::views::enumerate};
      const auto found{std::ranges::find_if(indexed, is_bare(text, sought))};

      return (found == indexed.end()) ? std::string_view::npos : static_cast<std::size_t>(std::get<0>(*found));
    }

    /// The path a ninja token spells: `$ ` stands for a space, `$:` for a colon and `$$` for a dollar.
    [[nodiscard]]
    std::string plain_spelling(std::string_view token)
    {
      return token | std::views::enumerate
                   | std::views::filter(std::not_fn(is_bare(token, '$')))
                   | std::views::values
                   | std::ranges::to<std::string>();
    }

    /// Splits on ninja's bare spaces, so that `a$ b` is one token, and gives each token's plain spelling.
    [[nodiscard]]
    std::vector<std::string> tokens(std::string_view text)
    {
      const auto isSeparator{is_bare(text, ' ')};
      auto neitherSeparates{[isSeparator](const auto& lhs, const auto& rhs){ return !isSeparator(lhs) && !isSeparator(rhs); }};
      auto isToken{[isSeparator](auto chunk){ return !isSeparator(*chunk.begin()); }};
      auto spelling{[](auto chunk){ return plain_spelling(chunk | std::views::values | std::ranges::to<std::string>()); }};

      return text | std::views::enumerate
                  | std::views::chunk_by(neitherSeparates)
                  | std::views::filter(isToken)
                  | std::views::transform(spelling)
                  | std::ranges::to<std::vector>();
    }

    /** What `build.ninja` says of the current build: each object file's source, both in generic spelling,
        keyed by the object file. A build statement is `build <outputs> [| <implicit outputs>]: <rule> <inputs> [| <implicit>] [|| <order-only>]`.
     */
    [[nodiscard]]
    std::map<std::string, std::string> read_ninja_sources(const fs::path& buildFile)
    {
      constexpr std::string_view keyword{"build "};
      std::map<std::string, std::string> sourcesByObjectFile{};

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

        const auto outputs{
            tokens(statement.substr(keyword.size(), colon - keyword.size()))
          | std::views::take_while([](std::string_view token){ return token != "|"; })
          | std::ranges::to<std::vector>()
        };

        // The rule comes first, and a `|` or `||` ends the explicit inputs, so the source is the second token
        // unless the second token is `|` or `||`
        const auto inputs{tokens(statement.substr(colon + 1))};
        if((inputs.size() > 1) && (inputs[1] != "|") && (inputs[1] != "||"))
        {
          // Both in generic form: on Windows the generator writes `CMakeFiles\Foo.dir\Bar.cpp.obj` and
          // `C:\Users\...\Bar.cpp`, where ninja canonicalizes what it logs to `CMakeFiles/Foo.dir/Bar.cpp.obj`
          // and `C:/Users/.../Bar.cpp`, and the two records are joined on these spellings
          const auto source{fs::path{inputs[1]}.generic_string()};
          for(const auto& output : outputs)
          {
            sourcesByObjectFile.try_emplace(fs::path{output}.generic_string(), source);
          }
        }
      }

      return sourcesByObjectFile;
    }

    /** The log Ninja keeps for a build directory: every object file the log has ever known, including
        those the build no longer has, in the order the log first names them; the files likewise.

        \throws std::runtime_error if the log cannot be read.
     */
    [[nodiscard]]
    compilations read_ninja_deps(const fs::path& file)
    {
      const dependency_log log{file};
      if(!log.begins_with(signature))
        throw std::runtime_error{log.error("not a ninja dependency log")};

      std::size_t pos{signature.size()};
      if(const auto version{log.word(pos)}; version != 4)
        throw std::runtime_error{log.error(std::format("version {} is not supported", version))};

      pos += word_size;

      std::vector<fs::path> paths{};
      std::unordered_set<std::string> spellings{};
      std::map<std::uint32_t, std::vector<compilations::file_index>> deps{};

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
            | std::views::transform([&log](std::size_t at){ return compilations::file_index{log.word(at)}; })
            | std::ranges::to<std::vector>()
          };

          if(std::ranges::any_of(inputIds, [&paths](compilations::file_index id){ return id >= paths.size(); }))
            throw std::runtime_error{log.error("a deps record names an input not yet seen")};

          deps.insert_or_assign(outputId, std::move(inputIds));
        }
        else
        {
          if(size < word_size)
            throw std::runtime_error{log.error("a path record of impossible size")};

          auto path{log.text(pos, size - word_size)};

          if(log.word(pos + size - word_size) != ~static_cast<std::uint32_t>(paths.size()))
            throw std::runtime_error{log.error("a path record's checksum does not match its position")};

          if(!spellings.insert(path).second)
            throw std::runtime_error{log.error("a path record repeats a spelling")};

          paths.emplace_back(std::move(path));
        }

        pos += size;
      }

      auto record{
        [](auto& dep) {
          auto& [objectId, inputIds]{dep};
          return compilations::record{.object_index{objectId}, .input_indices{std::move(inputIds)}};
        }
      };

      return compilations{
        .files{std::move(paths)},
        .records{deps | std::views::transform(record) | std::ranges::to<std::vector>()}
      };
    }

    constexpr std::string_view byte_order_mark{"\xFF\xFE"};

    /// The tracker writes UTF-16, little-endian; a path is built from the code units themselves, so nothing is lost in a narrow encoding
    [[nodiscard]]
    std::u16string decode_utf16le(std::span<const std::byte> bytes)
    {
      constexpr std::size_t unitWidth{sizeof(char16_t)};
      using code_unit_bytes = std::pair<std::byte, std::byte>;

      auto toCodeUnit{
        [](code_unit_bytes lowHigh) {
          const auto [low, high]{lowHigh};
          constexpr auto bitsPerByte{std::numeric_limits<std::underlying_type_t<std::byte>>::digits};
          return static_cast<char16_t>(std::to_integer<unsigned>(low) | (std::to_integer<unsigned>(high) << bitsPerByte));
        }
      };

      // The low byte of each unit, the high byte of each; zip ends at the shorter, so a stray final byte is dropped
      return std::views::zip(bytes | std::views::stride(unitWidth), bytes | std::views::drop(1) | std::views::stride(unitWidth))
           | std::views::transform(toCodeUnit)
           | std::ranges::to<std::u16string>();
    }

    [[nodiscard]]
    std::string lowercase(std::string text)
    {
      std::ranges::transform(text, text.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
      return text;
    }

    /// `CL.read.1.tlog`, or `CL.11932.read.1.tlog` where MSBuild has numbered the target's logs
    [[nodiscard]]
    bool is_tlog(const fs::path& file, std::string_view kind)
    {
      constexpr std::string_view prefix{"cl."}, suffix{".tlog"};

      const auto name{lowercase(file.filename().string())};
      if(!name.starts_with(prefix) || !name.ends_with(suffix))
        return false;

      const auto afterPrefix{std::string_view{name}.substr(prefix.size())};
      const auto digitsEnd{afterPrefix.find_first_not_of("0123456789")};
      const bool numbered{(digitsEnd > 0) && (digitsEnd != std::string_view::npos) && (afterPrefix[digitsEnd] == '.')};
      const auto rest{numbered ? afterPrefix.substr(digitsEnd + 1) : afterPrefix};

      return rest.starts_with(std::string{kind}.append("."));
    }

    /// The files a compilation touched, under each of its sources as the tracker spells them
    using tlog_entries = std::map<std::u16string, std::vector<std::u16string>>;

    /// The tracker's logs: under each `^`-led line naming one or more sources, the files that compilation touched
    [[nodiscard]]
    tlog_entries read_tlog_entries(const fs::path& tlogDir, std::string_view kind)
    {
      tlog_entries entries{};
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

        auto namesSources{[](std::u16string_view line){ return line.starts_with(u'^'); }};

        // Whatever precedes the first `^` names no source and is skipped
        auto lines{
            std::views::split(text, u'\n')
          | std::views::transform(withoutReturn)
          | std::views::filter([](std::u16string_view line){ return !line.empty(); })
          | std::views::drop_while(std::not_fn(namesSources))
        };

        // A `^` line and the lines beneath it are one group, so each group begins with the line naming its sources
        auto beneathSameSource{[namesSources](std::u16string_view, std::u16string_view next){ return !namesSources(next); }};
        for(auto group : lines | std::views::chunk_by(beneathSameSource))
        {
          const std::u16string_view head{group.front()};

          // A source is an entry even where the compilation touched nothing else
          auto entryFor{[&entries](auto sourceRange){ return entries.try_emplace(std::u16string{std::u16string_view{sourceRange}}).first; }};
          const auto sourceEntries{std::views::split(head.substr(1), u'|') | std::views::transform(entryFor) | std::ranges::to<std::vector>()};

          for(const std::u16string_view line : group | std::views::drop(1))
          {
            for(const auto sourceEntry : sourceEntries)
            {
              sourceEntry->second.emplace_back(line);
            }
          }
        }
      }

      return entries;
    }

    /** The tracker spells paths in upper case; the directories know how the paths are really spelled, and
        are asked once each:

        1. A filesystem which finds a file whatever its case answers through `weakly_canonical`.
        2. One which does not - ext4, where the fixtures also run - is walked a component at a time,
           each matched against its directory's entries without regard to case.
        3. What no directory holds keeps its spelling.
     */
    class path_spelling_recoverer
    {
    public:
      [[nodiscard]]
      const fs::path& operator()(const std::u16string& spelling)
      {
        if(const auto found{m_Recovered.find(spelling)}; found != m_Recovered.end())
          return found->second;

        return m_Recovered.try_emplace(spelling, recover(spelling)).first->second;
      }
    private:
      std::map<std::u16string, fs::path> m_Recovered{};
      std::map<fs::path, std::vector<fs::path>> m_Listings{};

      [[nodiscard]]
      fs::path recover(const std::u16string& spelling)
      {
        const fs::path asSpelled{spelling};
        std::error_code error{};
        const auto canonical{fs::weakly_canonical(asSpelled, error)};
        const auto& candidate{error ? asSpelled : canonical};

        return fs::exists(candidate) ? candidate : walk(candidate);
      }

      /// The names in `dir`, listed once
      [[nodiscard]]
      const std::vector<fs::path>& listing_of(const fs::path& dir)
      {
        if(const auto found{m_Listings.find(dir)}; found != m_Listings.end())
          return found->second;

        std::error_code error{};
        auto names{
            fs::directory_iterator{dir, error}
          | std::views::transform([](const fs::directory_entry& entry){ return entry.path().filename(); })
          | std::ranges::to<std::vector>()
        };

        return m_Listings.try_emplace(dir, std::move(names)).first->second;
      }

      /// The entry of `dir` spelled as `name` but for case, if there is one
      [[nodiscard]]
      std::optional<fs::path> entry_ignoring_case(const fs::path& dir, const fs::path& name)
      {
        const auto& listing{listing_of(dir)};

        const auto spelled{name.string()};
        auto sameLetter{[](unsigned char l, unsigned char r){ return std::tolower(l) == std::tolower(r); }};
        auto sameButForCase{
          [&spelled, sameLetter](const fs::path& candidate) {
            return std::ranges::equal(candidate.string(), spelled, sameLetter);
          }
        };
        const auto match{std::ranges::find_if(listing, sameButForCase)};

        return (match != listing.end()) ? std::optional{*match} : std::nullopt;
      }

      /// Components are matched against their directories until one is not found; the components beyond that one keep their spelling
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

    /// The object among the files the compilation of `source` wrote; none, if the compilation wrote no object
    [[nodiscard]]
    std::optional<fs::path> object_of(const fs::path& tlogDir,
                                      const fs::path& source,
                                      std::span<const std::u16string> written,
                                      path_spelling_recoverer& recover)
    {
      const auto objects{
          written
        | std::views::transform([&recover](const std::u16string& w){ return recover(w); })
        | std::views::filter([](const fs::path& path){ return lowercase(path.extension().string()) == ".obj"; })
        | std::ranges::to<std::vector>()
      };

      if(objects.empty())
        return std::nullopt;

      if(objects.size() == 1)
        return objects.front();

      // Sources compiled by one invocation share their writes. Where the object is named after its source
      // it can be picked out by stem, or by the source's whole name where stems collide; where the build
      // names objects by hash (CMAKE_INTERMEDIATE_DIR_STRATEGY) nothing in the log distinguishes them, so
      // such an entry is refused rather than guessed at.
      auto bearsSourcesName{
        [&source](const fs::path& o){ return (o.stem() == source.stem()) || (o.stem() == source.filename()); }
      };

      if(const auto own{std::ranges::find_if(objects, bearsSourcesName)}; own != objects.end())
        return *own;

      throw std::runtime_error{
        std::format("The tracker's log in {} does not say which of {} objects {} produced",
                    tlogDir.generic_string(),
                    objects.size(),
                    source.filename().generic_string())
      };
    }

    /// `source`, then the other files the compilation of `source` read, sorted, each once
    [[nodiscard]]
    std::vector<fs::path> inputs_of(const fs::path& source,
                                    std::span<const std::u16string> read,
                                    path_spelling_recoverer& recover)
    {
      auto inputs{
          read
        | std::views::transform([&recover](const std::u16string& r){ return recover(r); })
        | std::views::filter([&source](const fs::path& path){ return path != source; })
        | std::ranges::to<std::vector>()
      };

      std::ranges::sort(inputs);
      const auto [duplicates, end]{std::ranges::unique(inputs)};
      inputs.erase(duplicates, end);
      inputs.insert(inputs.begin(), source);

      return inputs;
    }

    /** The records of one `.tlog` directory, its files numbered into `files`.

        The tracker's logs, as MSBuild writes them:
        -# `CL.read.*.tlog` lists, under each source, every file the compiler read; `CL.write.*.tlog`
           lists what the compiler wrote, which is where the object file is named.
        -# A source is a line beginning `^`. Sources compiled by one invocation share a line, separated
           by `|`, and so share what is listed beneath the line.
        -# Both are UTF-16 with a byte order mark, and spell paths in upper case, so each path is put
           through the filesystem to recover its case.

        Hence, where sources share their writes, each object file is given to the source whose stem or
        name the object file bears; what cannot be told apart is refused rather than guessed.
     */
    [[nodiscard]]
    std::vector<compilations::record> read_tlogs(path_table& files, const fs::path& tlogDir)
    {
      const auto reads{read_tlog_entries(tlogDir, "read")};
      const auto writes{read_tlog_entries(tlogDir, "write")};
      path_spelling_recoverer recover{};

      auto indexOf{[&files](const fs::path& p){ return files.insert(p); }};

      auto writesOf{
        [&writes](const std::u16string& source) -> std::span<const std::u16string> {
          const auto found{writes.find(source)};
          return (found == writes.end()) ? std::span<const std::u16string>{} : found->second;
        }
      };

      // A source which wrote no object is not a compilation, so each source yields at most one
      auto recordOf{
        [&](const tlog_entries::value_type& entry) -> std::vector<compilations::record> {
          const auto& [source, read]{entry};
          const auto& sourcePath{recover(source)};
          const auto object{object_of(tlogDir, sourcePath, writesOf(source), recover)};
          if(!object)
            return {};

          const auto inputs{inputs_of(sourcePath, read, recover)};
          return {compilations::record{.object_index{indexOf(*object)}, .input_indices{inputs | std::views::transform(indexOf) | std::ranges::to<std::vector>()}}};
        }
      };

      auto records{
          reads
        | std::views::transform(recordOf)
        | std::views::join
        | std::ranges::to<std::vector>()
      };

      auto objectPath{[&files](const compilations::record& record) -> const fs::path& { return files[record.object_index]; }};
      std::ranges::sort(records, {}, objectPath);
      if(const auto twice{std::ranges::adjacent_find(records, {}, objectPath)}; twice != records.end())
        throw std::runtime_error{
          std::format("The tracker's log in {} attributes {} to more than one source",
                      tlogDir.generic_string(),
                      files[twice->object_index].filename().generic_string())
        };

      return records;
    }
  }

  namespace
  {
    /// The value of `set(NAME "value")` in a CMake script
    [[nodiscard]]
    std::optional<std::string> cmake_script_value(std::string_view text, std::string_view name)
    {
      const auto [first, last]{find_sandwiched_text(text, std::format("set({} \"", name), "\"")};
      if(first < last)
        return std::string{text.substr(first, last - first)};

      return std::nullopt;
    }

    /// The generator a CMake cache names
    [[nodiscard]]
    std::string generator_of(const fs::path& cacheFile)
    {
      const auto generator{cmake_cache{cacheFile}.variable("CMAKE_GENERATOR")};
      if(!generator)
        throw std::runtime_error{malformed_error(cacheFile, "no generator is named")};

      return *generator;
    }

    /** The compiler's built-in include directories, canonical.

        CMake records them as `CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES` in
        `<buildDirectory>/CMakeFiles/<version>/CMakeCXXCompiler.cmake`; a tree without that file
        yields none.
     */
    [[nodiscard]]
    std::vector<fs::path> implicit_include_directories_of(const fs::path& buildDirectory)
    {
      // Canonical, so that a directory reached through a symlink - macOS's SDK is one - compares equal to the files beneath it
      auto canonical{
        [](std::string_view dir) {
          std::error_code error{};
          const auto canonicalDir{fs::weakly_canonical(fs::path{dir}, error)};
          return error ? fs::path{dir}.lexically_normal() : canonicalDir;
        }
      };

      auto directoriesIn{
        [canonical](const fs::path& info) {
          const auto directories{cmake_script_value(read(info), "CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES").value_or("")};
          return std::views::split(directories, ';')
               | std::views::transform([](auto dirRange){ return std::string_view{dirRange}; })
               | std::views::filter([](std::string_view dir){ return !dir.empty(); })
               | std::views::transform(canonical)
               | std::ranges::to<std::vector>();
        }
      };

      const auto cmakeFiles{buildDirectory / "CMakeFiles"};
      if(!fs::is_directory(cmakeFiles))
        return {};

      auto compilerInfo{[](const fs::directory_entry& entry){ return entry.path() / "CMakeCXXCompiler.cmake"; }};

      return fs::directory_iterator{cmakeFiles}
           | std::views::filter([](const fs::directory_entry& entry){ return entry.is_directory(); })
           | std::views::transform(compilerInfo)
           | std::views::filter([](const fs::path& info){ return fs::exists(info); })
           | std::views::transform(directoriesIn)
           | std::views::join
           | std::ranges::to<std::vector>();
    }
  }

  [[nodiscard]]
  build_tree read_build_tree(const fs::path& cacheFile)
  {
    const auto buildDirectory{cacheFile.parent_path()};

    return build_tree{
      .build_directory{buildDirectory},
      .generator{generator_of(cacheFile)},
      .implicit_include_directories{implicit_include_directories_of(buildDirectory)}
    };
  }

  namespace
  {
    /** The compilations of a Ninja build, from the log `.ninja_deps` and the statements in `build.ninja`.

        The log gives every compilation ninja has ever recorded, and is trimmed to the object files the
        build currently has, which the statements name. Each record then has its source put first among
        the inputs: gcc and clang report the source among a compilation's inputs, MSVC reports the headers
        alone, and the statement names the source in either case.

        \throws std::runtime_error if
        -# There is no log, nothing having been built;
        -# No record's object file is named by any statement: the log and the statements then spell one
           tree two ways, and nothing would ever be selected.
     */
    [[nodiscard]]
    compilations ninja_compilations(const build_tree& tree)
    {
      const auto log{tree.build_directory / ".ninja_deps"};
      if(!fs::exists(log))
        throw std::runtime_error{std::format("{} has no dependency log; has anything been built?", tree.build_directory.generic_string())};

      const auto sourcesByObjectFile{read_ninja_sources(tree.build_directory / "build.ninja")};
      auto [loggedFiles, loggedRecords]{read_ninja_deps(log)};
      path_table files{std::move(loggedFiles)};

      auto buildHasObjectFile{
        [&sourcesByObjectFile, &files](const compilations::record& record) {
          return sourcesByObjectFile.contains(files[record.object_index].generic_string());
        }
      };

      /* A compiler that reports the source has named the source among the inputs, and the source moves to the
         front; MSVC reports the headers alone, and the source is inserted - into the table, which the log never
         gave the source, and at the front of the inputs.
       */
      auto withSourceFirst{
        [&](compilations::record& record) {
          const auto sourceIndex{files.insert(sourcesByObjectFile.at(files[record.object_index].generic_string()))};
          auto& inputIndices{record.input_indices};
          if(const auto found{std::ranges::find(inputIndices, sourceIndex)}; found != inputIndices.end())
            std::ranges::rotate(inputIndices.begin(), found, found + 1);
          else
            inputIndices.insert(inputIndices.begin(), sourceIndex);

          return compilations::record{.object_index{record.object_index}, .input_indices{std::move(inputIndices)}};
        }
      };

      auto records{
          loggedRecords
        | std::views::filter(buildHasObjectFile)
        | std::views::transform(withSourceFirst)
        | std::ranges::to<std::vector>()
      };

      if(records.empty() && !loggedRecords.empty())
        throw std::runtime_error{
          std::format("None of the objects {} records is named by build.ninja; are the two spelled differently?", log.generic_string())
        };

      return compilations{.files{std::move(files).release_files()}, .records{std::move(records)}};
    }

    /** The compilations of a Visual Studio build: those of every target's tracker logs in the
        executable's configuration, which is the name of the directory holding the executable.

        Every target's files are numbered into one table, so that a file two targets both read is one file.
     */
    [[nodiscard]]
    compilations visual_studio_compilations(const build_tree& tree, const fs::path& executable)
    {
      const auto configuration{executable.parent_path().filename()};
      auto isTlogOfConfiguration{
        [&configuration](const fs::directory_entry& entry) {
          return entry.is_directory()
              && (entry.path().extension() == ".tlog")
              && (entry.path().parent_path().filename() == configuration);
        }
      };

      path_table files{};
      std::vector<compilations::record> records{};
      for(const auto& entry : fs::recursive_directory_iterator{tree.build_directory} | std::views::filter(isTlogOfConfiguration))
      {
        records.append_range(read_tlogs(files, entry.path()));
      }

      return compilations{.files{std::move(files).release_files()}, .records{std::move(records)}};
    }
  }

  /// `Ninja Multi-Config` keeps its statements elsewhere and is not understood; nor is any generator but the two
  [[nodiscard]]
  compilations read_compilations(const build_tree& tree, const fs::path& executable)
  {
    if(tree.generator == "Ninja")
      return ninja_compilations(tree);

    if(tree.generator.starts_with("Visual Studio"))
      return visual_studio_compilations(tree, executable);

    throw std::runtime_error{
      std::format("The build in {} was written by the {} generator, whose record of dependencies is not understood; "
                  "Ninja's and Visual Studio's are",
                  tree.build_directory.generic_string(),
                  tree.generator)
    };
  }
}
