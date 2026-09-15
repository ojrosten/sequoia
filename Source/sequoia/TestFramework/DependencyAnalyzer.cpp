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
#include <cstdint>
#include <format>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <optional>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>

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
      if(!std::getline(s, line))
        throw std::runtime_error{std::format("Expected a line beginning '{}' but found the end of the file", key)};

      if(!line.starts_with(key))
        throw std::runtime_error{std::format("Expected a line beginning '{}' but found '{}'", key, line)};

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
      return std::ranges::starts_with(file, repo);
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

    /// The files outside both the project and the toolchain which the tests were built from: the third parties relied on
    void write_external_dependencies(const fs::path& file, const std::set<fs::path>& dependencies)
    {
      if(std::ofstream ostream{file})
      {
        std::ranges::copy(dependencies | std::views::transform([](const fs::path& p){ return p.generic_string(); }),
                          std::ostream_iterator<std::string>{ostream, "\n"});
      }
    }


    [[nodiscard]]
    bool materials_modified(const fs::path& relFilePath,
                            const fs::path& materialsRepo,
                            const fs::file_time_type stalenessThreshold)
    {
      const auto materials{materialsRepo / fs::path{relFilePath}.replace_extension("")};
      if(!fs::exists(materials))
        return false;

      auto modifiedSince{[stalenessThreshold](const fs::directory_entry& entry){ return fs::last_write_time(entry) > stalenessThreshold; }};

      return std::ranges::any_of(fs::recursive_directory_iterator(materials), modifiedSince);
    }

    [[nodiscard]]
    std::optional<fs::file_time_type> materials_max_write_time(const fs::path& relFilePath, const fs::path& materialsRepo)
    {
      const auto materials{materialsRepo / fs::path{relFilePath}.replace_extension("")};
      if(!fs::exists(materials))
        return std::nullopt;

      auto writeTimes{
          fs::recursive_directory_iterator(materials)
        | std::views::transform([](const fs::directory_entry& entry){ return fs::last_write_time(entry); })
      };

      return std::ranges::fold_left(writeTimes, fs::last_write_time(materials), std::ranges::max);
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
      if(fs::exists(file))
        return fs::last_write_time(file);

      return std::nullopt;
    }

    /** \brief The dependency graph of the executable, as the build which produced it recorded it.

        The build gives each object the files the compiler read to produce it - see
        BuildArtefacts.hpp for where. One convention is layered on that, saying that a definition
        matters to whoever sees its declaration: an object whose source shares its stem with a
        header it includes furnishes that header with its own dependencies, as `Foo.cpp` does
        `Foo.hpp`.
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
              const auto asRecorded{(p.is_absolute() ? p : tree.build_directory / p).lexically_normal()};
              std::error_code error{};
              auto canonical{fs::weakly_canonical(asRecorded, error)};
              m_Files.push_back(error ? asRecorded : std::move(canonical));
              m_Objects.emplace_back();
            }

            return iter->second;
          }
        };

        auto isSource{[this](index_type i){ return is_cpp(m_Files[i]); }};

        // Interning may grow m_Objects, so no reference into it is held across a call to indexOf
        for(const auto& record : read_compilations(tree, executable))
        {
          const auto output{indexOf(record.object)};
          auto inputs{record.inputs | std::views::transform(indexOf) | std::ranges::to<std::vector>()};

          auto& object{m_Objects[output]};
          if(const auto source{std::ranges::find_if(inputs, isSource)}; source != inputs.end())
            object.source = *source;

          object.inputs = std::move(inputs);
        }

        // The convention: a header is furnished by each object whose source shares its stem and which includes it
        const auto stems{
            m_Files
          | std::views::transform([](const fs::path& p){ return p.stem().string(); })
          | std::ranges::to<std::vector>()
        };
        const auto headers{m_Files | std::views::transform(is_header) | std::ranges::to<std::vector<bool>>()};

        for(const auto& [i, object] : std::views::enumerate(m_Objects))
        {
          if(!object.source)
            continue;

          auto sameStemHeader{[&](index_type input){ return headers[input] && (stems[input] == stems[*object.source]); }};
          for(const auto input : object.inputs | std::views::filter(sameStemHeader))
          {
            m_Objects[input].furnished_by.push_back(static_cast<index_type>(i));
          }
        }
      }

      /// The objects whose source lies in `repo`, each with its source
      [[nodiscard]]
      std::vector<std::pair<index_type, fs::path>> sources_in(const fs::path& repo) const
      {
        auto hasSourceIn{
          [this, &repo](const auto& indexed) {
            const auto& source{std::get<1>(indexed).source};
            return source && in_repo(m_Files[*source], repo);
          }
        };

        auto indexAndSource{
          [this](const auto& indexed) {
            return std::pair{static_cast<index_type>(std::get<0>(indexed)), m_Files[*std::get<1>(indexed).source]};
          }
        };

        return std::views::enumerate(m_Objects)
             | std::views::filter(hasSourceIn)
             | std::views::transform(indexAndSource)
             | std::ranges::to<std::vector>();
      }

      /// Every file on which the object depends: its inputs, and those of the objects furnishing its headers
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
            for(const auto furnisher : m_Objects[input].furnished_by) visit(furnisher);
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
      std::size_t file_count() const noexcept { return m_Files.size(); }

      [[nodiscard]]
      bool empty() const noexcept { return m_Objects.empty(); }
    private:
      /* Every path the build mentions gets an index, and every index an entry here, so an entry
         is an object where it has inputs and a plain input otherwise. `furnished_by` holds the
         objects the convention attaches to a header: the same-stem objects including it.
       */
      struct object_info
      {
        std::vector<index_type> inputs{}, furnished_by{};
        std::optional<index_type> source{};
      };

      std::vector<fs::path> m_Files{};
      std::vector<object_info> m_Objects{};
    };

    /// The modification times of the graph's files, each read once, and each checked against the executable's
    class modification_times
    {
    public:
      modification_times(const build_graph& graph, const std::optional<fs::file_time_type>& exeTimeStamp)
        : m_Graph{graph}
        , m_ExeTimeStamp{exeTimeStamp}
        , m_Times(graph.file_count())
      {}

      /// Throws if the file post-dates the executable or cannot be found, either meaning the executable is out of date
      [[nodiscard]]
      fs::file_time_type operator()(build_graph::index_type i)
      {
        const auto& file{m_Graph.file(i)};
        auto& cached{m_Times[i]};
        if(!cached)
        {
          std::error_code error{};
          cached = fs::last_write_time(file, error);
          if(error)
            throw std::runtime_error{
              std::format("Executable is out of date; please build it!\n{} was read by the build but cannot be found\n",
                          file.generic_string())
            };
        }

        if(m_ExeTimeStamp.has_value() && (*cached >= m_ExeTimeStamp.value()))
          throw std::runtime_error{
                  std::format(
                    "Executable is out of date; please build it!\nExecutable time stamp: {}\n{} time stamp: {}\n",
                    m_ExeTimeStamp.value(),
                    file.generic_string(),
                    *cached
                  )
                };

        return *cached;
      }
    private:
      const build_graph& m_Graph;
      std::optional<fs::file_time_type> m_ExeTimeStamp;
      std::vector<std::optional<fs::file_time_type>> m_Times;
    };

    /// The newest modification among `files`
    [[nodiscard]]
    fs::file_time_type newest_modification(std::span<const build_graph::index_type> files, modification_times& modificationTime)
    {
      return std::ranges::fold_left(files | std::views::transform(std::ref(modificationTime)), fs::file_time_type::min(), std::ranges::max);
    }

    /// Whether a test has passed since everything it depends on last changed, its materials included
    class passing_tests
    {
    public:
      passing_tests(const project_paths& projPaths, fs::file_time_type stalenessThreshold)
        : m_MaterialsRepo{projPaths.test_materials().repo()}
        , m_StalenessThreshold{stalenessThreshold}
        , m_Passes{read_tests(projPaths.prune().selected_passes(std::nullopt))}
        , m_PassesStamp{get_stamp(projPaths.prune().selected_passes(std::nullopt))}
      {}

      /** A test is stale if its dependencies or its materials moved after the threshold; a record
          of it passing since then makes it current again - see the comment on the comparison.
       */
      [[nodiscard]]
      bool is_stale(const fs::path& relPath, fs::file_time_type implicitModificationTime) const
      {
        bool stale{implicitModificationTime > m_StalenessThreshold};

        if(!m_PassesStamp || !std::ranges::binary_search(m_Passes, relPath, {}, path_projector{}))
          return stale || materials_modified(relPath, m_MaterialsRepo, m_StalenessThreshold);

        const auto materialsWriteTime{materials_max_write_time(relPath, m_MaterialsRepo)};
        if(!stale && (materialsWriteTime > m_StalenessThreshold))
          stale = true;

        const auto maxModificationTime{
          materialsWriteTime ? std::ranges::max(materialsWriteTime.value(), implicitModificationTime) : implicitModificationTime
        };

        /* `>=`, not `>`: the question is whether the record of this test passing is older
           than the newest modification. Both sides come from `last_write_time`, and the record
           is written after the modification it supersedes, so wherever the filesystem can
           represent that difference the two spellings agree. Where it cannot - libstdc++
           truncates to whole seconds on macOS - "after" collapses onto "equal", and `>` then
           rejects a record which does post-date the change, so the test is selected again by
           every later run and never settles.
        */
        if(stale && (m_PassesStamp.value() >= maxModificationTime))
          consider_passing_tests(stale, relPath, m_Passes, maxModificationTime);

        return stale;
      }
    private:
      fs::path m_MaterialsRepo;
      fs::file_time_type m_StalenessThreshold;
      std::vector<prune_record> m_Passes;
      std::optional<fs::file_time_type> m_PassesStamp;
    };

    [[nodiscard]]
    std::vector<fs::path> find_stale_tests(fs::file_time_type stalenessThreshold, const project_paths& projPaths)
    {
      const auto tree{read_build_tree(projPaths.discovered().cmake_cache())};
      const build_graph graph{tree, projPaths.executable()};

      const auto testSources{graph.sources_in(projPaths.tests().repo())};
      /* An empty selection is what a prune with nothing changed returns, so a build whose record
         names no test at all must not pass for one: it means the spellings could not be matched,
         and nothing would ever be selected again.
       */
      if(testSources.empty() && !graph.empty())
        throw std::runtime_error{
          std::format("The build's record names no source under {}; "
                      "was the tree configured with a different spelling of the project's path?",
                      projPaths.tests().repo().generic_string())
        };

      modification_times modificationTime{graph, get_stamp(projPaths.executable())};
      const passing_tests passes{projPaths, stalenessThreshold};

      std::set<build_graph::index_type> dependedOn{};
      std::vector<fs::path> staleTests{};

      for(const auto& [output, source] : testSources | std::views::filter([](const auto& indexed){ return is_cpp(indexed.second); }))
      {
        const auto relPath{fs::relative(source, projPaths.tests().repo())};
        const auto dependencies{graph.dependencies_of(output)};
        dependedOn.insert(dependencies.begin(), dependencies.end());

        if(passes.is_stale(relPath, newest_modification(dependencies, modificationTime)))
          staleTests.push_back(relPath);
      }

      // What is neither the project's nor the toolchain's: the third parties relied on
      auto isToolchains{
        [&tree](const fs::path& file) {
          return std::ranges::any_of(tree.implicit_include_directories, [&file](const fs::path& dir){ return in_repo(file, dir); });
        }
      };

      auto isExternal{
        [&](build_graph::index_type i) {
          const auto& file{graph.file(i)};
          return !in_repo(file, projPaths.project_root()) && !isToolchains(file);
        }
      };

      const auto externalDependencies{
          dependedOn
        | std::views::filter(isExternal)
        | std::views::transform([&graph](build_graph::index_type i){ return graph.file(i); })
        | std::ranges::to<std::set>()
      };

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

    struct least_path_most_recent
    {
      [[nodiscard]]
      bool operator()(const prune_record& lhs, const prune_record& rhs) const
      {
        const auto comp{lhs.test_path <=> rhs.test_path};
        return comp == 0 ? lhs.time_stamp > rhs.time_stamp : comp < 0;
      }
    };

    std::vector<prune_record>& to_unique_range(std::vector<prune_record>& r)
    {
      const auto erased{std::ranges::unique(r, {}, path_projector{})};
      r.erase(erased.begin(), erased.end());
      return r;
    }

    [[nodiscard]]
    std::vector<prune_record> aggregate_failures(const prune_paths& prunePaths, const std::size_t numReps)
    {
      auto allTests{
          std::views::iota(0uz, numReps)
        | std::views::transform([&prunePaths](std::size_t i){ return read_tests(prunePaths.failures(i)); })
        | std::views::join
        | std::ranges::to<std::vector>()
      };

      return to_unique_range(allTests);
    }

    [[nodiscard]]
    std::optional<std::vector<prune_record>> aggregate_passes(const prune_paths& prunePaths, const std::size_t numReps)
    {
      const auto files{
          std::views::iota(0uz, numReps)
        | std::views::transform([&prunePaths](std::size_t i){ return prunePaths.selected_passes(i); })
        | std::ranges::to<std::vector>()
      };

      if(!std::ranges::all_of(files, [](const fs::path& file){ return fs::exists(file); }))
        return std::nullopt;

      auto intersect{
        [](std::vector<prune_record> lhs, const std::vector<prune_record>& rhs) {
          std::vector<prune_record> common{};
          std::ranges::set_intersection(lhs, rhs, std::back_inserter(common));
          return common;
        }
      };

      return std::ranges::fold_left_first(files | std::views::transform(read_tests), intersect);
    }
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
      auto rebased{
        [&projPaths](const prune_record& test) {
          return prune_record{rebase_from(test.test_path, projPaths.tests().repo()), test.time_stamp};
        }
      };

      std::ranges::copy(tests | std::views::transform(rebased), std::ostream_iterator<prune_record>{ostream, "\n"});
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
    std::vector<prune_record> build_prune_records(std::span<const fs::path> tests, fs::file_time_type updateTime)
    {
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

    if(!pruneTimeStamp)
      return std::nullopt;

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
