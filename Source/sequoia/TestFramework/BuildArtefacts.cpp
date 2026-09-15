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
#include <cstdint>
#include <cstring>
#include <format>
#include <fstream>
#include <iterator>
#include <map>
#include <ranges>
#include <set>
#include <stdexcept>
#include <string_view>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    /* The log's layout, as ninja's deps_log.cc writes it. A signature line and a version word, then
       records, each led by a size word whose high bit says which kind it is. A path record is the
       path, NUL-padded to a multiple of four bytes, then a checksum which is the bitwise complement
       of the id the path is given by its position. A deps record is the id of the output, an
       eight-byte time stamp and then the ids of the inputs. Words are in the host's byte order,
       since the log is only ever read on the machine which wrote it.
     */
    constexpr std::string_view signature{"# ninjadeps\n"};
    constexpr std::uint32_t depsFlag{0x80000000u}, maxRecordSize{(1u << 19) - 1};

    [[nodiscard]]
    std::uint32_t read_word(std::string_view bytes, std::size_t at)
    {
      std::uint32_t word{};
      std::memcpy(&word, bytes.data() + at, sizeof(word));
      return word;
    }

    void write_word(std::ostream& out, std::uint32_t word)
    {
      out.write(reinterpret_cast<const char*>(&word), sizeof(word));
    }

    [[nodiscard]]
    std::runtime_error malformed(const fs::path& file, std::string_view what)
    {
      return std::runtime_error{std::format("Unable to read {}: {}", file.generic_string(), what)};
    }

    [[nodiscard]]
    std::string read_whole(const fs::path& file)
    {
      auto text{read_to_string(file, std::ios_base::binary)};
      if(!text) throw std::runtime_error{"Unable to open " + file.generic_string()};

      return std::move(*text);
    }

    /// Undoes ninja's lexical escapes: `$ `, `$:` and `$$` stand for a space, a colon and a dollar.
    [[nodiscard]]
    std::string unescape(std::string_view token)
    {
      std::string s{};
      for(std::size_t i{}; i < token.size(); ++i)
      {
        if((token[i] == '$') && (i + 1 < token.size()))
          ++i;

        s.push_back(token[i]);
      }

      return s;
    }

    [[nodiscard]]
    std::string escape(const fs::path& p)
    {
      std::string s{};
      for(const char c : p.generic_string())
      {
        if((c == ' ') || (c == ':') || (c == '$')) s.push_back('$');
        s.push_back(c);
      }

      return s;
    }

    /// Splits on ninja's unescaped spaces, so that `a$ b` is one token.
    [[nodiscard]]
    std::vector<std::string> tokens(std::string_view text)
    {
      std::vector<std::string> found{};
      std::string current{};
      for(std::size_t i{}; i < text.size(); ++i)
      {
        if(text[i] == '$' && (i + 1 < text.size()))
        {
          current.push_back(text[i]);
          current.push_back(text[++i]);
        }
        else if(text[i] == ' ')
        {
          if(!current.empty()) found.push_back(unescape(std::exchange(current, {})));
        }
        else
        {
          current.push_back(text[i]);
        }
      }

      if(!current.empty()) found.push_back(unescape(current));

      return found;
    }
  }

  [[nodiscard]]
  std::vector<compilation_record> read_ninja_deps(const fs::path& log)
  {
    const std::string bytes{read_whole(log)};
    if(!bytes.starts_with(signature)) throw malformed(log, "not a ninja dependency log");

    std::size_t pos{signature.size()};
    if(pos + 4 > bytes.size()) throw malformed(log, "no version");

    const auto version{read_word(bytes, pos)};
    pos += 4;
    if(version != 4) throw malformed(log, std::format("version {} is not understood", version));

    constexpr std::size_t stampWidth{8};

    std::vector<fs::path> paths{};
    std::map<std::uint32_t, std::vector<std::uint32_t>> deps{};

    while(pos + 4 <= bytes.size())
    {
      const auto sizeWord{read_word(bytes, pos)};
      const bool isDeps{(sizeWord & depsFlag) != 0};
      const std::size_t size{sizeWord & ~depsFlag};
      if(size > maxRecordSize) throw malformed(log, "a record exceeds ninja's maximum size");

      pos += 4;
      // A truncated final record is what an interrupted ninja leaves, and ninja discards it
      if(pos + size > bytes.size()) break;

      if(isDeps)
      {
        if((size < 4 + stampWidth) || ((size - 4 - stampWidth) % 4)) throw malformed(log, "a deps record of impossible size");

        const auto outputId{read_word(bytes, pos)};
        if(outputId >= paths.size()) throw malformed(log, "a deps record names an output not yet seen");

        std::vector<std::uint32_t> inputIds{};
        for(std::size_t at{pos + 4 + stampWidth}; at < pos + size; at += 4)
        {
          const auto id{read_word(bytes, at)};
          if(id >= paths.size()) throw malformed(log, "a deps record names an input not yet seen");
          inputIds.push_back(id);
        }

        deps[outputId] = std::move(inputIds);
      }
      else
      {
        if(size < 4) throw malformed(log, "a path record of impossible size");

        std::string_view path{bytes.data() + pos, size - 4};
        while(!path.empty() && (path.back() == '\0')) path.remove_suffix(1);

        if(read_word(bytes, pos + size - 4) != ~static_cast<std::uint32_t>(paths.size()))
          throw malformed(log, "a path record's checksum does not match its position");

        paths.emplace_back(path);
      }

      pos += size;
    }

    std::vector<compilation_record> records{};
    for(const auto& [outputId, inputIds] : deps)
    {
      records.push_back({.output{paths[outputId]}, .inputs{inputIds | std::views::transform([&paths](std::uint32_t id){ return paths[id]; }) | std::ranges::to<std::vector>()}});
    }

    return records;
  }

  void write_ninja_deps(const fs::path& log, std::span<const compilation_record> records)
  {
    std::ofstream out{log, std::ios_base::binary};
    if(!out) throw std::runtime_error{"Unable to open " + log.generic_string()};

    out.write(signature.data(), static_cast<std::streamsize>(signature.size()));
    write_word(out, 4);

    std::map<std::string, std::uint32_t> ids{};
    auto idOf{
      [&](const fs::path& p) {
        const auto key{p.generic_string()};
        if(const auto found{ids.find(key)}; found != ids.end()) return found->second;

        const auto id{static_cast<std::uint32_t>(ids.size())};
        ids.emplace(key, id);

        const std::size_t padding{(4 - key.size() % 4) % 4};
        write_word(out, static_cast<std::uint32_t>(key.size() + padding + 4));
        out.write(key.data(), static_cast<std::streamsize>(key.size()));
        for(std::size_t i{}; i < padding; ++i) out.put('\0');
        write_word(out, ~id);

        return id;
      }
    };

    for(const auto& record : records)
    {
      const auto outputId{idOf(record.output)};
      const auto inputIds{record.inputs | std::views::transform(idOf) | std::ranges::to<std::vector>()};

      write_word(out, depsFlag | static_cast<std::uint32_t>(4 * (1 + 2 + inputIds.size())));
      write_word(out, outputId);
      write_word(out, 0);
      write_word(out, 0);
      for(const auto id : inputIds) write_word(out, id);
    }
  }

  [[nodiscard]]
  std::vector<compilation_record> read_dyndep(const fs::path& file)
  {
    std::ifstream in{file};
    if(!in) throw std::runtime_error{"Unable to open " + file.generic_string()};

    std::vector<compilation_record> records{};
    std::string line{};
    while(std::getline(in, line))
    {
      if(!line.starts_with("build ")) continue;

      const auto colon{line.find(": dyndep")};
      if(colon == std::string::npos) throw malformed(file, "a build statement which is not a dyndep");

      compilation_record record{};

      const auto outputs{tokens(std::string_view{line}.substr(6, colon - 6))};
      if(outputs.empty()) throw malformed(file, "a build statement with no output");

      record.output = outputs.front();
      if(outputs.size() > 1)
      {
        if((outputs[1] != "|") || (outputs.size() != 3)) throw malformed(file, "an object providing other than one module file");
        record.providedModule = outputs[2];
      }

      auto inputs{std::string_view{line}.substr(colon + 8)};
      if(const auto orderOnly{inputs.find("||")}; orderOnly != std::string_view::npos) inputs = inputs.substr(0, orderOnly);

      auto required{tokens(inputs)};
      if(!required.empty())
      {
        if(required.front() != "|") throw malformed(file, "a dyndep with explicit inputs");
        required.erase(required.begin());
      }

      record.requiredModules = std::move(required) | std::views::transform([](std::string& s){ return fs::path{std::move(s)}; }) | std::ranges::to<std::vector>();

      records.push_back(std::move(record));
    }

    return records;
  }

  void write_dyndep(const fs::path& file, std::span<const compilation_record> records)
  {
    std::ofstream out{file};
    if(!out) throw std::runtime_error{"Unable to open " + file.generic_string()};

    out << "ninja_dyndep_version = 1.0\n";
    for(const auto& record : records)
    {
      out << "build " << escape(record.output);
      if(record.providedModule) out << " | " << escape(*record.providedModule);
      out << ": dyndep";
      if(!record.requiredModules.empty())
      {
        out << " |";
        for(const auto& m : record.requiredModules) out << ' ' << escape(m);
      }
      out << '\n';
    }
  }

  std::ostream& operator<<(std::ostream& s, const compilation_record& record)
  {
    s << record.output.generic_string();
    if(record.providedModule) s << " provides " << record.providedModule->generic_string();
    for(const auto& input : record.inputs) s << "\n  " << input.generic_string();
    for(const auto& m : record.requiredModules) s << "\n  requires " << m.generic_string();

    return s;
  }

  namespace
  {
    /// The tracker writes UTF-16, little-endian, with a byte order mark; a path is built from the code units themselves, so nothing is lost in a narrow encoding
    [[nodiscard]]
    std::u16string decode_utf16le(std::string_view bytes)
    {
      std::u16string text{};
      for(std::size_t i{}; i + 1 < bytes.size(); i += 2)
      {
        text.push_back(static_cast<char16_t>(static_cast<unsigned char>(bytes[i]) | (static_cast<unsigned char>(bytes[i + 1]) << 8)));
      }

      return text;
    }

    void write_utf16le(std::ostream& out, std::u16string_view text)
    {
      for(const char16_t unit : text)
      {
        out.put(static_cast<char>(unit & 0xFF));
        out.put(static_cast<char>(unit >> 8));
      }
    }

    /// `CL.read.1.tlog`, or `CL.11932.write.1.tlog`: a target's tracker logs may carry a number, which persists across its builds; both forms were met in one tree
    [[nodiscard]]
    bool is_tlog(const fs::path& file, std::string_view kind)
    {
      auto name{file.filename().string()};
      std::ranges::transform(name, name.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
      if(!name.starts_with("cl.") || !name.ends_with(".tlog")) return false;

      std::string_view rest{std::string_view{name}.substr(3)};
      if(const auto number{rest.find_first_not_of("0123456789")}; (number > 0) && (number != std::string_view::npos) && (rest[number] == '.')) rest.remove_prefix(number + 1);

      return rest.starts_with(std::string{kind}.append("."));
    }

    /// The tracker's logs: under each `^`-led line naming one or more sources, the files that compilation touched
    [[nodiscard]]
    std::map<std::u16string, std::vector<std::u16string>> read_tlog_entries(const fs::path& tlogDir, std::string_view kind)
    {
      std::map<std::u16string, std::vector<std::u16string>> entries{};
      for(const auto& entry : fs::directory_iterator(tlogDir))
      {
        if(!is_tlog(entry.path(), kind)) continue;

        const std::string bytes{read_whole(entry.path())};
        if(!bytes.starts_with("\xFF\xFE")) throw malformed(entry.path(), "not the tracker's UTF-16");

        std::vector<std::u16string> sources{};
        for(const auto lineRange : std::views::split(decode_utf16le(std::string_view{bytes}.substr(2)), u'\n'))
        {
          std::u16string_view line{lineRange};
          if(line.ends_with(u'\r')) line.remove_suffix(1);
          if(line.empty()) continue;

          if(line.starts_with(u'^'))
          {
            sources.clear();
            for(const auto sourceRange : std::views::split(line.substr(1), u'|'))
            {
              // A source is an entry even where it touched nothing else, as a module unit with no imports does
              entries.try_emplace(sources.emplace_back(std::u16string_view{sourceRange}));
            }
          }
          else
          {
            for(const auto& source : sources) entries[source].emplace_back(line);
          }
        }
      }

      return entries;
    }

    /** The tracker spells paths in upper case; the directories know how they are really spelled, and
        are asked once each. A filesystem which finds a file whatever its case answers through
        `weakly_canonical`; one which does not - ext4, where the fixtures also run - is walked a
        component at a time, each matched against its directory's entries without regard to case.
        What no directory holds keeps its spelling.
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
          if(error) canonical = asSpelled;

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

      [[nodiscard]]
      fs::path walk(const fs::path& p)
      {
        fs::path recovered{p.root_path()};
        bool matching{true};
        for(const auto& component : p.relative_path())
        {
          if(matching)
          {
            if(fs::exists(recovered / component))
            {
              recovered /= component;
              continue;
            }

            auto listing{m_Listings.find(recovered)};
            if(listing == m_Listings.end())
            {
              std::vector<fs::path> names{};
              std::error_code error{};
              for(const auto& entry : fs::directory_iterator{recovered, error}) names.push_back(entry.path().filename());
              listing = m_Listings.emplace(recovered, std::move(names)).first;
            }

            const auto spelled{component.string()};
            if(const auto match{std::ranges::find_if(listing->second, [&spelled](const fs::path& name){ return equal_ignoring_case(name.string(), spelled); })}; match != listing->second.end())
            {
              recovered /= *match;
              continue;
            }

            matching = false;
          }

          recovered /= component;
        }

        return recovered;
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
      if(first < last) return std::string{text.substr(first, last - first)};

      return std::nullopt;
    }

    /// The value of `NAME:TYPE=value` in a CMake cache
    [[nodiscard]]
    std::optional<std::string> cmake_cache_value(std::string_view text, std::string_view name)
    {
      const auto [first, last]{find_sandwiched_text(text, std::string{"\n"}.append(name).append(":"), "\n")};
      if(first >= last) return std::nullopt;

      auto entry{text.substr(first, last - first)};
      if(entry.ends_with('\r')) entry.remove_suffix(1);

      const auto equals{entry.find('=')};
      if(equals == std::string_view::npos) return std::nullopt;

      return std::string{entry.substr(equals + 1)};
    }

    /// A line of a ninja file with its trailing whitespace and carriage return removed
    [[nodiscard]]
    std::string_view trimmed(std::string_view line)
    {
      while(!line.empty() && ((line.back() == ' ') || (line.back() == '\r') || (line.back() == '\t'))) line.remove_suffix(1);
      return line;
    }

    /// The first unescaped occurrence of `c`, `$` escaping what follows it
    [[nodiscard]]
    std::size_t find_unescaped(std::string_view text, char c)
    {
      for(std::size_t i{}; i < text.size(); ++i)
      {
        if(text[i] == '$')      ++i;
        else if(text[i] == c) return i;
      }

      return std::string_view::npos;
    }

    /// What `build.ninja` says of the current build: each object's source, keyed by the object's generic spelling, and the dyndep file, if any, its statement names
    struct ninja_statements
    {
      std::map<std::string, std::string> sourceOf{};
      std::set<fs::path> dyndepFiles{};
    };

    /** A build statement is `build <outputs> [| <implicit outputs>]: <rule> <inputs> [| <implicit>] [|| <order-only>]`,
        with indented `name = value` lines beneath it binding variables, of which `dyndep` names a dyndep file.
     */
    [[nodiscard]]
    ninja_statements read_ninja_statements(const fs::path& buildFile, const fs::path& root)
    {
      ninja_statements statements{};
      std::vector<std::string> currentOutputs{};

      for(const auto lineRange : std::views::split(read_whole(buildFile), '\n'))
      {
        const auto line{trimmed(std::string_view{lineRange})};
        if(line.starts_with("build "))
        {
          currentOutputs.clear();

          const auto colon{find_unescaped(line, ':')};
          if(colon == std::string_view::npos) throw malformed(buildFile, "a build statement with no rule");

          auto outputs{tokens(line.substr(6, colon - 6))};
          if(const auto implicit{std::ranges::find(outputs, std::string_view{"|"})}; implicit != outputs.end()) outputs.erase(implicit, outputs.end());

          auto inputs{tokens(line.substr(colon + 1))};
          // The rule comes first; a `|` or `||` ends the explicit inputs
          if(inputs.size() > 1)
          {
            auto explicitEnd{std::ranges::find_if(inputs.begin() + 1, inputs.end(), [](const std::string& t){ return (t == "|") || (t == "||"); })};
            if(explicitEnd != inputs.begin() + 1)
            {
              // On Windows the generator writes `CMakeFiles\Foo.dir\Bar.cpp.obj` where the log, canonicalized by ninja, has `CMakeFiles/Foo.dir/Bar.cpp.obj`
              for(const auto& output : outputs) statements.sourceOf.emplace(fs::path{output}.generic_string(), inputs[1]);
            }
          }

          currentOutputs = std::move(outputs);
        }
        else if(!currentOutputs.empty() && (line.find_first_not_of(" \t") != std::string_view::npos))
        {
          const auto content{line.substr(line.find_first_not_of(" \t"))};
          if(content.starts_with("dyndep = ")) statements.dyndepFiles.insert(root / unescape(trimmed(content.substr(9))));
        }
      }

      return statements;
    }
  }

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
        std::vector<fs::path> objects{}, interfaces{};
        for(const auto& w : written->second)
        {
          const auto& path{recover(w)};
          if(has_extension(path, ".obj"))      objects.push_back(path);
          else if(has_extension(path, ".ifc")) interfaces.push_back(path);
        }

        // Sources compiled by one invocation share their writes; each object bears its source's stem, or its whole name where stems collide
        auto own{std::ranges::find_if(objects, [&sourcePath](const fs::path& o){ return (o.stem() == sourcePath.stem()) || (o.stem() == sourcePath.filename()); })};
        if((own == objects.end()) && (objects.size() == 1)) own = objects.begin();

        if(own != objects.end())
          record.output = *own;
        else if(!objects.empty())
          throw std::runtime_error{std::format("The tracker's log in {} does not say which of {} objects {} produced", tlogDir.generic_string(), objects.size(), sourcePath.filename().generic_string())};

        if(interfaces.size() == 1)
          record.providedModule = interfaces.front();
        else if(interfaces.size() > 1)
          throw std::runtime_error{std::format("The tracker's log in {} does not say which of {} module interfaces {} produced", tlogDir.generic_string(), interfaces.size(), sourcePath.filename().generic_string())};
      }

      if(record.output.empty()) continue;

      // The source first, then what was read in the order the tracker recorded it, the source itself among them or not
      std::set<fs::path> seen{sourcePath};
      record.inputs.push_back(sourcePath);
      for(const auto& r : read)
      {
        const auto& path{recover(r)};
        if(has_extension(path, ".ifc"))
        {
          record.requiredModules.push_back(path);
        }
        else if(seen.insert(path).second)
        {
          record.inputs.push_back(path);
        }
      }

      records.push_back(std::move(record));
    }

    return records;
  }

  void write_tlogs(const fs::path& tlogDir, std::span<const compilation_record> records)
  {
    fs::create_directories(tlogDir);
    std::ofstream read{tlogDir / "CL.read.1.tlog", std::ios_base::binary}, write{tlogDir / "CL.write.1.tlog", std::ios_base::binary};
    if(!read || !write) throw std::runtime_error{"Unable to write tracking logs in " + tlogDir.generic_string()};

    // The tracker upper-cases, which for the ASCII the test fixtures use is what std::toupper does
    auto upper{
      [](const fs::path& p) {
        auto s{p.u16string()};
        std::ranges::transform(s, s.begin(), [](char16_t c){ return (c < 0x80) ? static_cast<char16_t>(std::toupper(static_cast<unsigned char>(c))) : c; });
        return s;
      }
    };

    for(auto& out : {&read, &write}) out->write("\xFF\xFE", 2);

    for(const auto& record : records)
    {
      if(record.inputs.empty()) continue;

      const auto source{u"^" + upper(record.inputs.front()) + u"\r\n"};

      write_utf16le(read, source);
      for(const auto& input : record.inputs | std::views::drop(1)) write_utf16le(read, upper(input) + u"\r\n");
      for(const auto& m : record.requiredModules) write_utf16le(read, upper(m) + u"\r\n");

      write_utf16le(write, source);
      write_utf16le(write, upper(record.output) + u"\r\n");
      if(record.providedModule) write_utf16le(write, upper(*record.providedModule) + u"\r\n");
    }
  }

  [[nodiscard]]
  build_tree read_build_tree(const fs::path& cacheFile)
  {
    build_tree tree{.root{cacheFile.parent_path()}};

    tree.generator = cmake_cache_value(read_whole(cacheFile), "CMAKE_GENERATOR").value_or("");

    if(const auto cmakeFiles{tree.root / "CMakeFiles"}; fs::is_directory(cmakeFiles))
    {
      for(const auto& entry : fs::directory_iterator(cmakeFiles))
      {
        if(const auto info{entry.path() / "CMakeCXXCompiler.cmake"}; entry.is_directory() && fs::exists(info))
        {
          for(const auto dirRange : std::views::split(cmake_script_value(read_whole(info), "CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES").value_or(""), ';'))
          {
            // Canonical, so that a directory reached through a symlink - macOS's SDK is one - compares equal to the files beneath it
            if(std::string_view d{dirRange}; !d.empty())
            {
              std::error_code error{};
              auto canonical{fs::weakly_canonical(fs::path{d}, error)};
              tree.implicitIncludeDirs.push_back(error ? fs::path{d}.lexically_normal() : std::move(canonical));
            }
          }
        }
      }
    }

    return tree;
  }

  [[nodiscard]]
  std::vector<compilation_record> read_compilations(const build_tree& tree, const fs::path& executable)
  {
    std::vector<compilation_record> records{};

    if(tree.generator == "Ninja")
    {
      const auto log{tree.root / ".ninja_deps"};
      if(!fs::exists(log)) throw std::runtime_error{std::format("{} has no dependency log; has anything been built?", tree.root.generic_string())};

      const auto statements{read_ninja_statements(tree.root / "build.ninja", tree.root)};

      std::map<std::string, std::size_t> positions{};
      auto logged{read_ninja_deps(log)};
      for(auto& record : logged)
      {
        const auto source{statements.sourceOf.find(record.output.generic_string())};
        if(source == statements.sourceOf.end()) continue;

        // MSVC reports the headers alone, so the source is added from the statement where the log lacks it
        if(const fs::path sourcePath{source->second}; !std::ranges::contains(record.inputs, sourcePath))
          record.inputs.insert(record.inputs.begin(), sourcePath);

        positions.emplace(record.output.generic_string(), records.size());
        records.push_back(std::move(record));
      }

      /* A log every one of whose records fails to match a statement is not an empty build: it is two
         spellings of one tree, and what follows would select nothing, forever, without a word.
       */
      if(records.empty() && !logged.empty())
        throw std::runtime_error{std::format("{} records {} objects, none of which build.ninja names; are the two spelled differently?", log.generic_string(), logged.size())};

      for(const auto& dyndepFile : statements.dyndepFiles)
      {
        if(!fs::exists(dyndepFile)) continue;

        for(auto& dyndep : read_dyndep(dyndepFile))
        {
          if(const auto found{positions.find(dyndep.output.generic_string())}; found != positions.end())
          {
            records[found->second].providedModule  = std::move(dyndep.providedModule);
            records[found->second].requiredModules = std::move(dyndep.requiredModules);
          }
        }
      }
    }
    else if(tree.generator.starts_with("Visual Studio"))
    {
      const auto configuration{executable.parent_path().filename()};
      for(const auto& entry : fs::recursive_directory_iterator(tree.root))
      {
        if(!entry.is_directory() || (entry.path().extension() != ".tlog") || (entry.path().parent_path().filename() != configuration)) continue;

        records.append_range(read_tlogs(entry.path()));
      }
    }
    else
    {
      throw std::runtime_error{std::format("The build in {} was written by the {} generator, whose record of dependencies is not understood; Ninja's and Visual Studio's are", tree.root.generic_string(), tree.generator)};
    }

    return records;
  }
}
