////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/DependencyAnalyzer.hpp"
#include "sequoia/TestFramework/BuildArtefacts.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"

#include "sequoia/Maths/Arithmetic/ArithmeticCasts.hpp"
#include "sequoia/Streaming/Streaming.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <format>
#include <fstream>
#include <functional>
#include <map>
#include <optional>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  using maths::checked_conversion_to;

  namespace
  {
    using duration_t   = prune_record::stamp_type::duration;
    using stream_rep_t = std::int64_t;

    template<std::invocable<std::string> Parser>
    [[nodiscard]]
    std::remove_cvref_t<std::invoke_result_t<Parser, std::string>> extract_field(std::istream& s, std::string_view key, Parser parse)
    {
      std::string line{};
      if(!std::getline(s, line))   throw std::runtime_error{std::format("Expected a line beginning '{}' but found the end of the file", key)};
      if(!line.starts_with(key))   throw std::runtime_error{std::format("Expected a line beginning '{}' but found '{}'", key, line)};

      return parse(line.substr(key.size()));
    }

    [[nodiscard]]
    prune_record::stamp_type to_stamp(const std::string& text)
    {
      const auto last{text.data() + text.size()};
      if(stream_rep_t count{}; std::from_chars(text.data(), last, count) == std::from_chars_result{last, std::errc{}})
        return prune_record::stamp_type{duration_t{checked_conversion_to<duration_t::rep>(count)}};

      throw std::runtime_error{std::format("'{}' is not a time stamp", text)};
    }
  }

  std::ostream& operator<<(std::ostream& s, const prune_record& record)
  {
    return s << "path: "      << record.test_path.generic_string() << '\n'
             << "timestamp: " << std::format("{}", checked_conversion_to<stream_rep_t>(record.time_stamp.time_since_epoch().count()));
  }

  std::istream& operator>>(std::istream& s, prune_record& record)
  {
    if(s.peek() == std::char_traits<char>::eof())
    {
      s.setstate(std::ios::failbit);
      return s;
    }

    record = prune_record{extract_field(s, "path: ", std::identity{}), extract_field(s, "timestamp: ", to_stamp)};

    return s;
  }

  namespace
  {
    struct path_projector
    {
      const fs::path& operator()(const prune_record& record) const { return record.test_path; }
    };
    


    [[nodiscard]]
    bool in_repo(const fs::path& file, const fs::path& repo)
    {
      auto zipped{std::views::zip(file, repo)};
      return std::ranges::find_if(zipped, [](const auto& e) { return std::get<0>(e) != std::get<1>(e); }) == zipped.end();
    }

    [[nodiscard]]
    bool is_cpp(const fs::path& file)
    {
      const auto ext{file.extension()};
      return (ext == ".cpp") || (ext == ".cc") || (ext == ".cxx");
    }

    [[nodiscard]]
    bool is_header(const fs::path& file)
    {
      const auto ext{file.extension()};
      return (ext == ".hpp") || (ext == ".h") || (ext == ".hxx");
    }

    /// Accumulates characters while `pred` holds, leaving the first which fails it unconsumed.
    template<std::predicate<char> Pred>
    std::string read_while(std::istream& istr, Pred pred)
    {
      constexpr auto eof{std::ifstream::traits_type::eof()};
      using int_type = std::ifstream::int_type;

      std::string str{};

      int_type c{};
      while((c = istr.get()) != eof)
      {
        if(!pred(static_cast<char>(c)))
        {
          istr.unget();
          break;
        }

        str.push_back(static_cast<char>(c));
      }

      return str;
    }

    /** \brief Everything [cpp.pre] admits between a directive's tokens: any whitespace but the newline ending it.

        The same set is all that may precede a directive on its line, and `module` and `import`
        declarations are directives in that grammar too ([cpp.pre]/1), so this serves them as it
        serves `#`.
     */
    [[nodiscard]]
    bool is_directive_space(char c) noexcept
    {
      // The carriage return is not [cpp.pre]'s, but a CRLF file read in binary puts one before every newline
      return (c == ' ') || (c == '\t') || (c == '\v') || (c == '\f') || (c == '\r');
    }

    /// The characters which may appear in an identifier; ASCII, and so independent of the locale.
    [[nodiscard]]
    bool is_identifier_char(char c) noexcept
    {
      return    ((c >= 'a') && (c <= 'z'))
             || ((c >= 'A') && (c <= 'Z'))
             || ((c >= '0') && (c <= '9'))
             || (c == '_');
    }

    void skip_directive_space(std::istream& istr)
    {
      read_while(istr, [](char c){ return is_directive_space(c); });
    }

    /// Reads an identifier, leaving the first character which cannot extend one unconsumed.
    [[nodiscard]]
    std::string read_identifier(std::istream& istr)
    {
      return read_while(istr, [](char c){ return is_identifier_char(c); });
    }

    /** \brief Reads to the end of a declaration, returning its subject with all whitespace removed.

        A module name may be written `M : P` as readily as `M:P`, so the spaces cannot be kept if
        two spellings of one name are to compare equal.
     */
    [[nodiscard]]
    std::string declaration_subject(std::istream& istr)
    {
      auto subject{read_while(istr, [](char c){ return (c != ';') && (c != '\n'); })};
      if(istr.peek() == ';') istr.get();

      std::erase_if(subject, [](char c){ return is_directive_space(c) || (c == '\r'); });

      return subject;
    }

    /** \brief Whether a string is spelled as a module name.

        `module` is not a reserved word, so a line beginning with it may be ordinary code. Requiring
        what follows to look like a module name - identifier characters and dots, with at most one
        colon introducing a partition - is what keeps `module = 3;` from declaring a module.
     */
    [[nodiscard]]
    bool is_module_name(std::string_view name)
    {
      const auto isIdentifier{
        [](std::string_view part) {
          return    !part.empty()
                 && !((part.front() >= '0') && (part.front() <= '9'))
                 && std::ranges::all_of(part, [](char c){ return is_identifier_char(c); });
        }
      };

      const auto isDottedName{
        [isIdentifier](std::string_view dotted) {
          return    !dotted.empty()
                 && std::ranges::all_of(std::views::split(dotted, '.'), [isIdentifier](const auto& part){ return isIdentifier(std::string_view{part}); });
        }
      };

      const auto colon{name.find(':')};
      if(colon == std::string_view::npos) return isDottedName(name);

      if(name.find(':', colon + 1) != std::string_view::npos) return false;

      return isDottedName(name.substr(0, colon)) && isDottedName(name.substr(colon + 1));
    }

    /// The extensions under which a module unit is conventionally written; what it declares is the lexer's to say.
    [[nodiscard]]
    bool has_module_extension(const fs::path& file)
    {
      const auto ext{file.extension()};
      return (ext == ".cppm") || (ext == ".ixx") || (ext == ".cxxm") || (ext == ".ccm") || (ext == ".c++m") || (ext == ".mpp");
    }


    /// The files outside both the project and the toolchain which the tests were built from: the third parties relied on
    void write_external_dependencies(const fs::path& file, const std::set<fs::path>& dependencies)
    {
      if(std::ofstream ostream{file})
      {
        for(const auto& dependency : dependencies) ostream << dependency.generic_string() << '\n';
      }
    }


    [[nodiscard]]
    bool materials_modified(const fs::path& relFilePath,
                            const fs::path& materialsRepo,
                            const fs::file_time_type stalenessThreshold)
    {
      const auto materials{materialsRepo / fs::path{relFilePath}.replace_extension("")};
      if(fs::exists(materials))
      {
        for(const auto& entry : fs::recursive_directory_iterator(materials))
        {
          if(fs::last_write_time(entry) > stalenessThreshold) return true;
        }
      }

      return false;
    }

    [[nodiscard]]
    std::optional<fs::file_time_type> materials_max_write_time(const fs::path& relFilePath, const fs::path& materialsRepo)
    {
      const auto materials{materialsRepo / fs::path{relFilePath}.replace_extension("")};
      if(fs::exists(materials))
      {
        fs::file_time_type maxTime{fs::last_write_time(materials)};

        for(const auto& entry : fs::recursive_directory_iterator(materials))
        {
          maxTime = std::ranges::max(maxTime, fs::last_write_time(entry));
        }

        return maxTime;
      }

      return std::nullopt;
    }

    void consider_passing_tests(bool& stale,
                                const fs::path& relFilePath,
                                std::span<const prune_record> passingTests,
                                fs::file_time_type maxModificationTime)
    {
      auto iter{std::ranges::lower_bound(passingTests, relFilePath, {}, path_projector{})};
      if((iter != passingTests.end()) && (iter->test_path == relFilePath) && (iter->time_stamp > maxModificationTime))
      {
        stale = false;
      }
    }

    [[nodiscard]]
    std::optional<fs::file_time_type> get_stamp(const fs::path& file)
    {
      if(fs::exists(file)) return fs::last_write_time(file);

      return std::nullopt;
    }

    /** \brief The dependency graph of the executable, as the build which produced it recorded it.

        The build gives each object the files the compiler read to produce it, the module interface
        files it needed and the one it produced - see BuildArtefacts.hpp for where. Two conventions
        are layered on that, both saying that a definition matters to whoever sees its declaration:
        an object whose source shares its stem with a header it includes furnishes that header with
        its own dependencies, as `Foo.cpp` does `Foo.hpp`; and an implementation unit, which nothing
        can name, furnishes the interface of the module it declares. The second is the one fact no
        artefact carries, since `module M;` and `import M;` look alike to a scan, so it is read from
        the source.
     */
    class build_graph
    {
    public:
      using index_type = std::size_t;

      build_graph(const build_tree& tree, const fs::path& executable)
      {
        /* A record spells a path identically each time it mentions it, so interning is keyed on the
           spelling and the filesystem is asked once per spelling for the canonical path - which is
           what project_paths holds, the build having recorded whatever spelling it was configured
           with, through whatever symlink and in whatever case.
         */
        std::map<std::string, index_type> indices{};
        auto indexOf{
          [this, &tree, &indices](const fs::path& p) {
            const auto [iter, inserted]{indices.try_emplace(p.string(), m_Files.size())};
            if(inserted)
            {
              const auto asRecorded{(p.is_absolute() ? p : tree.root / p).lexically_normal()};
              std::error_code error{};
              auto canonical{fs::weakly_canonical(asRecorded, error)};
              m_Files.push_back(error ? asRecorded : std::move(canonical));
              m_Objects.emplace_back();
            }

            return iter->second;
          }
        };

        // Interning may grow m_Objects, so no reference into it is held across a call to indexOf
        for(const auto& record : read_compilations(tree, executable))
        {
          const auto output{indexOf(record.output)};
          auto inputs{record.inputs | std::views::transform(indexOf) | std::ranges::to<std::vector>()};
          auto required{record.requiredModules | std::views::transform(indexOf) | std::ranges::to<std::vector>()};
          const auto provided{record.providedModule ? std::optional{indexOf(*record.providedModule)} : std::nullopt};

          auto& object{m_Objects[output]};
          if(auto source{std::ranges::find_if(inputs, [this](index_type i){ return is_source(m_Files[i]); })}; source != inputs.end())
            object.source = *source;

          object.inputs          = std::move(inputs);
          object.requiredModules = std::move(required);
          if(provided) m_Objects[*provided].provider = output;
        }

        // The conventions: which objects furnish a header, and which furnish a module's primary interface
        const auto stems{m_Files | std::views::transform([](const fs::path& p){ return p.stem().string(); }) | std::ranges::to<std::vector>()};
        const auto headers{m_Files | std::views::transform(is_header) | std::ranges::to<std::vector<bool>>()};

        std::vector<bool> providesModule(m_Objects.size(), false);
        for(const auto& object : m_Objects)
        {
          if(object.provider) providesModule[*object.provider] = true;
        }

        for(index_type i{}; i < m_Objects.size(); ++i)
        {
          auto& object{m_Objects[i]};
          if(!object.source) continue;

          for(const auto input : object.inputs)
          {
            if(headers[input] && (stems[input] == stems[*object.source])) m_Objects[input].furnishedBy.push_back(i);
          }

          // A unit which needs a module and provides none is an importer or an implementation unit; only its declaration says which
          if(is_cpp(m_Files[*object.source]) && !object.requiredModules.empty() && !providesModule[i])
          {
            if(const auto declaration{module_declaration_of(m_Files[*object.source])}; declaration && (declaration->role == module_role::implementation_unit))
            {
              // CMake names a primary interface's file after its module, `M.pcm` or `M.ifc`, and a partition's `M-P`
              const auto primary{declaration->primary_name()};
              for(const auto required : object.requiredModules)
              {
                if(stems[required] == primary) m_Objects[required].furnishedBy.push_back(i);
              }
            }
          }
        }
      }

      /// The objects whose source lies in `repo`, each with its source
      [[nodiscard]]
      std::vector<std::pair<index_type, fs::path>> sources_in(const fs::path& repo) const
      {
        std::vector<std::pair<index_type, fs::path>> found{};
        for(index_type i{}; i < m_Objects.size(); ++i)
        {
          if(const auto& source{m_Objects[i].source}; source && in_repo(m_Files[*source], repo)) found.emplace_back(i, m_Files[*source]);
        }

        return found;
      }

      /// Every file on which the object depends: its inputs, those of the objects providing what it needs, and those the conventions add
      [[nodiscard]]
      std::vector<index_type> dependencies_of(index_type output) const
      {
        std::vector<index_type> files{}, pending{output};
        std::vector<bool> visited(m_Objects.size(), false);

        auto visit{
          [&](index_type i) {
            if(!visited[i])
            {
              visited[i] = true;
              pending.push_back(i);
            }
          }
        };

        visited[output] = true;
        while(!pending.empty())
        {
          const auto current{pending.back()};
          pending.pop_back();

          const auto& object{m_Objects[current]};
          for(const auto input : object.inputs)
          {
            files.push_back(input);
            for(const auto furnisher : m_Objects[input].furnishedBy) visit(furnisher);
          }

          for(const auto required : object.requiredModules)
          {
            if(const auto& provider{m_Objects[required].provider}) visit(*provider);
            for(const auto furnisher : m_Objects[required].furnishedBy) visit(furnisher);
          }
        }

        std::ranges::sort(files);
        const auto [first, last]{std::ranges::unique(files)};
        files.erase(first, last);

        return files;
      }

      [[nodiscard]]
      const fs::path& file(index_type i) const noexcept { return m_Files[i]; }

      [[nodiscard]]
      bool empty() const noexcept { return m_Objects.empty(); }
    private:
      /* Every path the build mentions gets an index, and every index an entry here, so an entry
         is an object where it has inputs, a module file where it has a provider, and a plain
         input otherwise. `furnishedBy` holds the objects the conventions attach: for a header, the
         same-stem objects including it; for a primary interface file, the module's
         implementation units.
       */
      struct object_info
      {
        std::vector<index_type> inputs{}, requiredModules{}, furnishedBy{};
        std::optional<index_type> source{}, provider{};
      };

      std::vector<fs::path> m_Files{};
      std::vector<object_info> m_Objects{};

      [[nodiscard]]
      static bool is_source(const fs::path& file) { return is_cpp(file) || has_module_extension(file); }

      [[nodiscard]]
      static std::optional<module_declaration> module_declaration_of(const fs::path& source)
      {
        std::ifstream in{source, std::ios_base::binary};
        if(!in) return std::nullopt;

        return scan_module_declaration(in);
      }
    };

    /// The newest modification among `files`, throwing if one of them post-dates the executable or no longer exists
    [[nodiscard]]
    fs::file_time_type newest_modification(const build_graph& graph,
                                          std::span<const build_graph::index_type> files,
                                          const std::optional<fs::file_time_type>& exeTimeStamp,
                                          std::vector<std::optional<fs::file_time_type>>& modificationTimes)
    {
      fs::file_time_type newest{fs::file_time_type::min()};
      for(const auto i : files)
      {
        const auto& file{graph.file(i)};
        auto& cached{modificationTimes[i]};
        if(!cached)
        {
          std::error_code error{};
          cached = fs::last_write_time(file, error);
          if(error)
            throw std::runtime_error{std::format("Executable is out of date; please build it!\n{} was read by the build but cannot be found\n", file.generic_string())};
        }

        if(exeTimeStamp.has_value() && (*cached >= exeTimeStamp.value()))
          throw std::runtime_error{
                  std::format(
                    "Executable is out of date; please build it!\nExecutable time stamp: {}\n{} time stamp: {}\n",
                    exeTimeStamp.value(),
                    file.generic_string(),
                    *cached
                  )
                };

        newest = std::ranges::max(newest, *cached);
      }

      return newest;
    }

    [[nodiscard]]
    std::vector<fs::path> find_stale_tests(fs::file_time_type stalenessThreshold, const project_paths& projPaths)
    {
      const auto tree{read_build_tree(projPaths.discovered().cmake_cache())};
      const build_graph graph{tree, projPaths.executable()};

      const auto exeTimeStamp{get_stamp(projPaths.executable())};
      const auto passesFile{projPaths.prune().selected_passes(std::nullopt)};
      const auto passingTestsFromFile{read_tests(passesFile)};
      const auto passesStamp{get_stamp(passesFile)};

      const auto testSources{graph.sources_in(projPaths.tests().repo())};
      /* An empty selection is what a prune with nothing changed returns, so a build whose record
         names no test at all must not pass for one: it means the spellings could not be matched,
         and nothing would ever be selected again.
       */
      if(testSources.empty() && !graph.empty())
        throw std::runtime_error{std::format("The build's record names no source under {}; was the tree configured with a different spelling of the project's path?", projPaths.tests().repo().generic_string())};
      std::vector<std::optional<fs::file_time_type>> modificationTimes{};
      std::vector<bool> external{}; // the files any test depends on, filtered to those outside the project once all are known
      std::vector<fs::path> staleTests{};

      for(const auto& [output, source] : testSources)
      {
        if(!is_cpp(source)) continue;

        const auto relPath{fs::relative(source, projPaths.tests().repo())};
        const auto dependencies{graph.dependencies_of(output)};
        if(!dependencies.empty())
        {
          modificationTimes.resize(std::ranges::max(modificationTimes.size(), dependencies.back() + 1));
          external.resize(modificationTimes.size(), false);
        }

        const auto implicitModificationTime{newest_modification(graph, dependencies, exeTimeStamp, modificationTimes)};
        bool stale{implicitModificationTime > stalenessThreshold};

        for(const auto i : dependencies) external[i] = true;

        if(passesStamp && std::ranges::binary_search(passingTestsFromFile, relPath, {}, path_projector{}))
        {
          const auto materialsWriteTime{materials_max_write_time(relPath, projPaths.test_materials().repo())};
          if(!stale && (materialsWriteTime > stalenessThreshold))
            stale = true;

          const auto maxModificationTime{materialsWriteTime ? std::ranges::max(materialsWriteTime.value(), implicitModificationTime) : implicitModificationTime};

          /* `>=`, not `>`: the question is whether the record of this test passing is older
             than the newest modification. Both sides come from `last_write_time`, and the record
             is written after the modification it supersedes, so wherever the filesystem can
             represent that difference the two spellings agree. Where it cannot - libstdc++
             truncates to whole seconds on macOS - "after" collapses onto "equal", and `>` then
             rejects a record which does post-date the change, so the test is selected again by
             every later run and never settles.
          */
          if(stale && (passesStamp.value() >= maxModificationTime))
            consider_passing_tests(stale, relPath, passingTestsFromFile, maxModificationTime);
        }
        else if(!stale)
        {
          if(materials_modified(relPath, projPaths.test_materials().repo(), stalenessThreshold))
          {
            stale = true;
          }
        }

        if(stale) staleTests.push_back(relPath);
      }

      // What is neither the project's nor the toolchain's: the third parties relied on
      auto isToolchains{
        [&tree](const fs::path& file) { return std::ranges::any_of(tree.implicitIncludeDirs, [&file](const fs::path& dir){ return in_repo(file, dir); }); }
      };

      std::set<fs::path> externalDependencies{};
      for(std::size_t i{}; i < external.size(); ++i)
      {
        if(const auto& file{graph.file(i)}; external[i] && !in_repo(file, projPaths.project_root()) && !isToolchains(file)) externalDependencies.insert(file);
      }

      write_external_dependencies(projPaths.prune().external_dependencies(), externalDependencies);

      std::ranges::sort(staleTests);

      return staleTests;
    }

    void update_prune_stamp_on_disk(const prune_paths& prunePaths, fs::file_time_type time)
    {
      const auto stamp{prunePaths.stamp()};
      if(!fs::exists(stamp))
      {
        std::ofstream{stamp};
      }
      fs::last_write_time(stamp, time);
    }

    std::vector<prune_record>& read_tests_to(const fs::path& file, std::vector<prune_record>& tests)
    {
      
      if(std::ifstream ifile{file})
      {
        try
        {
          // A call rather than a pipe, to stay identical to `modules-native`. The pipe is
          // fine here and is rejected there: under `import std`, g++ 15.2 reports
          // "use of operator| ... before deduction of 'auto'" whenever the adaptor carries a
          // lambda - a named predicate pipes fine. See gcc-bugs/E in the sequoia-LLM
          // repository, and PR 120318; fixed in gcc 16.1.
          
          using iter_t = std::istream_iterator<prune_record>;
          tests.append_range(
            std::views::filter(
              std::ranges::subrange{iter_t{ifile}, iter_t{}},
              [](const prune_record& record) { return !record.test_path.empty(); }
            )
          );
        }
        catch(const std::exception& e)
        {
          throw
            std::runtime_error{
              std::format(
                "Unable to read the prune records in {}: {}\nTry deleting the parent directory and starting afresh",
                file.generic_string(),
                e.what()
              )
            };
        }
      }

      return tests;
    }

    struct least_path_most_recent{
      [[nodiscard]]
      bool operator()(const prune_record& lhs, const prune_record& rhs) const {
        auto comp{lhs.test_path <=> rhs.test_path};
        return comp == 0 ? lhs.time_stamp > rhs.time_stamp : comp < 0;
      }
    };

    std::vector<prune_record>& to_unique_range(std::vector<prune_record>& r) {
      auto erased{std::ranges::unique(r, {}, path_projector{})};
      r.erase(erased.begin(), erased.end());
      return r;
    }
    
    [[nodiscard]]
    std::vector<prune_record> aggregate_failures(const prune_paths& prunePaths, const std::size_t numReps)
    {
      std::vector<prune_record> allTests{};
      for(auto i : std::views::iota(0uz, numReps))
      {
        read_tests_to(prunePaths.failures(i), allTests);
      }

      return to_unique_range(allTests);
    }

    [[nodiscard]]
    std::optional<std::vector<prune_record>> aggregate_passes(const prune_paths& prunePaths, const std::size_t numReps)
    {
      std::vector<prune_record> intersection{};
      for(auto i : std::views::iota(0uz, numReps))
      {
        const auto file{prunePaths.selected_passes(i)};
        if(!fs::exists(file)) return std::nullopt;

        std::vector<prune_record> tests{testing::read_tests(file)};
        if(i)
        {
          std::vector<prune_record> currentIntersection{};
          std::ranges::set_intersection(tests, intersection, std::back_inserter(currentIntersection));
          intersection = std::move(currentIntersection);
        }
        else
        {
          intersection = std::move(tests);
        }
      }

      return intersection;
    }
  }

  [[nodiscard]]
  std::string_view module_declaration::primary_name() const noexcept
  {
    return std::string_view{name}.substr(0, name.find(':'));
  }

  [[nodiscard]]
  std::optional<module_declaration> scan_module_declaration(std::istream& source)
  {
    constexpr auto eof{std::ifstream::traits_type::eof()};
    using int_type = std::ifstream::int_type;

    /* A declaration is recognized only at the start of a line, which is what keeps `module` - not
       a reserved word - from being found in the middle of ordinary code. Being a directive, only
       directive space may precede one, so the flag survives that and nothing else.
    */
    bool atLineStart{true};

    // A byte order mark, which Visual Studio writes by default, precedes the first line without being part of it
    for(const char byte : {'\xEF', '\xBB', '\xBF'})
    {
      if(source.peek() != std::ifstream::traits_type::to_int_type(byte))
      {
        source.clear();
        source.seekg(0);
        break;
      }

      source.get();
    }

    int_type c{};
    while((c = source.get()) != eof)
    {
      const bool lineStart{std::exchange(atLineStart, false)};

      if(lineStart && ((c == 'e') || (c == 'm')))
      {
        // Only these two characters can begin `export` or `module`; a line beginning with either which proves to be something else is ordinary code, and so ends the preamble
        source.unget();

        auto keyword{read_identifier(source)};
        auto role{module_role::implementation_unit};

        if(keyword == "export")
        {
          role = module_role::interface_unit;
          skip_directive_space(source);
          keyword = read_identifier(source);
        }

        if(keyword != "module") return std::nullopt;

        auto name{declaration_subject(source)};

        // `module;` introduces the global module fragment and declares nothing; `export module;` is not a thing
        if(name.empty())
        {
          if(role == module_role::interface_unit) return std::nullopt;

          continue;
        }

        // A partition names the module it belongs to when it declares itself; only an import may abbreviate
        if(name.starts_with(':') || !is_module_name(name)) return std::nullopt;

        return module_declaration{std::move(name), role};
      }

      if(is_directive_space(static_cast<char>(c)))
      {
        atLineStart = lineStart;
      }
      else if(c == '\n')
      {
        atLineStart = true;
      }
      else if(c == '/')
      {
        if(source.peek() == '/')
        {
          source.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
          atLineStart = true;
        }
        else if(source.peek() == '*')
        {
          source.get();
          while(source)
          {
            source.ignore(std::numeric_limits<std::streamsize>::max(), '*');
            if(source.peek() == '/')
            {
              source.get();
              break;
            }
          }

          // A comment is whitespace [lex.phases], so what follows one is as much at the start of the line as the comment was
          atLineStart = lineStart;
        }
        else
        {
          return std::nullopt;
        }
      }
      else if(c == '#')
      {
        // A directive runs to the end of its line, or beyond it where a backslash precedes the newline - or the CRLF
        int_type previous{};
        while(((c = source.get()) != eof) && ((c != '\n') || (previous == '\\')))
        {
          if(c != '\r') previous = c;
        }

        atLineStart = true;
      }
      else if(lineStart)
      {
        return std::nullopt;
      }
    }

    return std::nullopt;
  }

  [[nodiscard]]
  fs::file_time_type staleness_threshold(const fs::file_time_type stamp)
  {
    using namespace std::chrono;
    const auto subSecond{stamp.time_since_epoch() - floor<seconds>(stamp.time_since_epoch())};

    return (subSecond == fs::file_time_type::duration::zero())
      ? stamp - duration_cast<fs::file_time_type::duration>(seconds{1})
      : stamp;
  }

  [[nodiscard]]
  std::vector<prune_record> read_tests(const fs::path& file)
  {
    std::vector<prune_record> tests{};
    return read_tests_to(file, tests);
  }

  void write_tests(const project_paths& projPaths, const fs::path& file, std::span<const prune_record> tests)
  {
    if(std::ofstream ostream{file})
    {
      for(const auto& test : tests)
      {
        ostream << prune_record{rebase_from(test.test_path, projPaths.tests().repo()), test.time_stamp} << "\n";
      }
    }
  }

  namespace
  {
    void do_update_prune_files(const project_paths& projPaths,
                          std::vector<prune_record> failedTests,
                          fs::file_time_type updateTime,
                          std::optional<std::size_t> id)
    {
      std::ranges::sort(failedTests, least_path_most_recent{});
      
      const auto prunePaths{projPaths.prune()};
      write_tests(projPaths, prunePaths.failures(id), failedTests);
      fs::remove(prunePaths.selected_passes(id));
      update_prune_stamp_on_disk(prunePaths, updateTime);
    }

    void do_update_prune_files(const project_paths& projPaths,
                               std::vector<prune_record> executedTests,
                               std::vector<prune_record> failedTests,
                               std::optional<std::size_t> id)
    {
      std::ranges::sort(executedTests, least_path_most_recent{});
      std::ranges::sort(failedTests, least_path_most_recent{});
      to_unique_range(executedTests);

      auto unionize{
        [](std::span<const prune_record> a, std::span<const prune_record> b){
          std::vector<prune_record> tests{};
          std::ranges::set_union(a, b, std::back_inserter(tests), least_path_most_recent{});
          to_unique_range(tests);
          return tests;
        }
      };

      auto difference{
        [](std::span<const prune_record> a, std::span<const prune_record> b){
          std::vector<prune_record> tests{};
          std::ranges::set_difference(a, b, std::back_inserter(tests), {}, path_projector{}, path_projector{});
          return tests;
        }
      };

      const auto prunePaths{projPaths.prune()};
      const auto passesFile{prunePaths.selected_passes(id)},
                 failuresFile{prunePaths.failures(id)};
      
      const std::vector<prune_record> trialPasses{unionize(executedTests, read_tests(passesFile))};      
      const std::vector<prune_record> passingTests{difference(trialPasses, failedTests)};
      const std::vector<prune_record> remainingPreviousFailures{difference(read_tests(failuresFile), passingTests)};
      const std::vector<prune_record> allFailures{unionize(remainingPreviousFailures, failedTests)};

      write_tests(projPaths, failuresFile, allFailures);
      write_tests(projPaths, passesFile, passingTests);
    }

    [[nodiscard]]
    std::vector<prune_record> build_prune_records(std::span<const fs::path> tests, fs::file_time_type updateTime) {
      return   std::views::transform(tests, [updateTime](const fs::path& p){ return prune_record{p, updateTime}; })
             | std::ranges::to<std::vector>();
    }
  }

  [[nodiscard]]
  std::optional<std::vector<fs::path>>
  tests_to_run(const project_paths& projPaths)
  {
    const auto prunePaths{projPaths.prune()};
    const auto pruneTimeStamp{get_stamp(prunePaths.stamp())};

    if(!pruneTimeStamp) return std::nullopt;

    const auto staleTests{find_stale_tests(staleness_threshold(pruneTimeStamp.value()), projPaths)};

    const std::vector<fs::path> failingTests{
      std::views::transform(read_tests(prunePaths.failures(std::nullopt)), path_projector{}) | std::ranges::to<std::vector>()
    };

    std::vector<fs::path> testsToRun{};
    std::ranges::set_union(staleTests, failingTests, std::back_inserter(testsToRun));

    return testsToRun;
  }

  void update_prune_files(const project_paths& projPaths,
                          std::span<const fs::path> failedTests,
                          fs::file_time_type updateTime,
                          std::optional<std::size_t> id)
  {
    do_update_prune_files(
      projPaths,
      build_prune_records(failedTests, updateTime),
      updateTime,
      id
    );
  }

  void update_prune_files(const project_paths& projPaths,
                          std::span<const fs::path> executedTests,
                          std::span<const fs::path> failedTests,
                          fs::file_time_type updateTime,
                          std::optional<std::size_t> id)
  {    
    do_update_prune_files(
      projPaths,
      build_prune_records(executedTests, updateTime),
      build_prune_records(failedTests, updateTime),
      id
    );
  }

  void setup_instability_analysis_prune_folder(const project_paths& projPaths)
  {
    const auto dir{projPaths.prune().instability_analysis()};
    fs::remove_all(dir);
    fs::create_directories(dir);
  }

  void aggregate_instability_analysis_prune_files(const project_paths& projPaths, prune_mode mode, std::filesystem::file_time_type timeStamp, std::size_t numReps)
  {
    const auto prunePaths{projPaths.prune()};
    auto failingCases{aggregate_failures(prunePaths, numReps)};

    switch(mode)
    {
    case prune_mode::passive:
    {
      if(auto optPasses{aggregate_passes(prunePaths, numReps)})
      {
        auto& executedCases{optPasses.value()};
        executedCases.insert(executedCases.end(), failingCases.begin(), failingCases.end());

        do_update_prune_files(projPaths, std::move(executedCases), std::move(failingCases), std::nullopt);
      }
      else
      {
        do_update_prune_files(projPaths, std::move(failingCases), timeStamp, std::nullopt);
      }

      break;
    }
    case prune_mode::active:
    {
      do_update_prune_files(projPaths, std::move(failingCases), timeStamp, std::nullopt);
      break;
    }
    }

    fs::remove_all(prunePaths.instability_analysis());
  }
}
