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
#include "sequoia/Maths/Graph/DynamicGraph.hpp"
#include "sequoia/Maths/Graph/GraphTraversalFunctions.hpp"
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
#include <stdexcept>
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
      // TO DO: ranges::starts_with says the same, from libstdc++ 16
      return std::ranges::mismatch(repo, file).in1 == repo.end();
    }

    [[nodiscard]]
    bool in_toolchain(const fs::path& file, const build_tree& tree)
    {
      return std::ranges::any_of(tree.implicit_include_directories, [&file](const fs::path& dir){ return in_repo(file, dir); });
    }

    /** Every test class may optionally define test materials. For a test class `bar_test`, defined in
        `Tests/Foo/Bar.cpp`, any materials are in the folder `TestMaterials/Foo/Bar/bar_test`.
        For the purposes of pruning, we look for modifications one level up, i.e. within
        `TestMaterials/Foo/Bar/`. This gives pruning the same granularity for both sources and test
        materials. Per-class granularity for materials is possible, but the trade-off does not
        currently seem worthwhile.
     */
    [[nodiscard]]
    std::optional<fs::file_time_type> materials_modification_time(const fs::path& relFilePath, const fs::path& materialsRepo)
    {
      const auto materials{materialsRepo / fs::path{relFilePath}.replace_extension("")};
      if(!fs::exists(materials))
        return std::nullopt;

      auto modificationTimes{
          fs::recursive_directory_iterator(materials)
        | std::views::transform([](const fs::directory_entry& entry){ return fs::last_write_time(entry); })
      };

      return std::ranges::fold_left(modificationTimes, fs::last_write_time(materials), std::ranges::max);
    }

    [[nodiscard]]
    std::optional<fs::file_time_type> get_stamp(const fs::path& file)
    {
      if(fs::exists(file))
        return fs::last_write_time(file);

      return std::nullopt;
    }

    /** \brief The dependency graph of the executable, as recorded by the build which produced the executable.

        The build associates each object file with the files consumed to create it. Typically these will be
        headers. Suppose `Foo.hpp` depends on `Bar.hpp`. This is an explicit dependency. However, it is natural
        that definitions for declarations within `Bar.hpp` are in its associated source file `Bar.cpp`.
        Therefore, in principle, changes to `Bar.cpp` can trigger a change in behaviour of functions declared
        in `Bar.hpp` and consumed by `Foo.hpp`. Therefore there is a dependency of `Foo.hpp` on `Bar.cpp`.

        The build does not record that dependency: how a definition reaches an object is settled at link
        time, and the record holds only what each compilation read. So prune infers the dependency by
        name, and relies on a precondition: a definition is either in a file the consuming compilation
        reads, or in the source named for a header the compilation reads. A definition anywhere else is
        not seen - in a source of a different name, or in a header of a different name which only some
        other compilation reads - although the program links either way.

        The toolchain's own headers are read by every translation unit, so a change to one is a change
        to every test; their newest modification is kept apart, and they are left out of the graph.
     */
    class dependency_graph
    {
    public:
      /** A test's translation unit: its source, relative to the tests repository, and the newest
          modification among the files the unit was built from.
       */
      struct translation_unit
      {
        fs::path source;
        fs::file_time_type newest_modification;
      };

      /** Every file the build read is checked once against the executable's stamp.

          \throws std::runtime_error if a file post-dates the executable, or a file's modification time
          cannot be read.
       */
      dependency_graph(const build_tree& tree, const project_paths& projPaths)
        : dependency_graph{projPaths, get_stamp(projPaths.executable()), read_files(tree, projPaths)}
      {}

      /// The translation units built from sources in the tests repository
      [[nodiscard]]
      std::span<const translation_unit> get_test_translation_units() const noexcept { return m_TestTranslationUnits; }

      /// The newest modification among the toolchain's files the build read; the earliest time, if it read none
      [[nodiscard]]
      fs::file_time_type toolchain_newest_modification() const noexcept { return m_ToolchainNewestModification; }

      /// Whether the build read any file of the project's under `directory`; the object files it wrote there do not count
      [[nodiscard]]
      bool reads_from(const fs::path& directory) const
      {
        auto readFrom{[&directory](const file_info& info){ return !info.is_object_file() && in_repo(info.file, directory); }};

        return std::ranges::any_of(m_Graph.cnode_weights(), readFrom);
      }
    private:
      /// A file the build names; for an object file, the node of the source it was compiled from
      struct file_info
      {
        fs::path file{};
        std::optional<std::size_t> source{};

        [[nodiscard]]
        bool is_object_file() const noexcept { return source.has_value(); }
      };

      using graph_type = maths::directed_graph<maths::null_weight, file_info>;
      using node_index = graph_type::edge_index_type;

      struct files_read_by_build
      {
        graph_type project_graph;
        std::vector<fs::path> toolchain_files;
      };

      fs::path m_TestsRepo;
      graph_type m_Graph;
      fs::file_time_type m_ToolchainNewestModification;
      std::vector<translation_unit> m_TestTranslationUnits;

      dependency_graph(const project_paths& projPaths, std::optional<fs::file_time_type> executableStamp, files_read_by_build&& filesRead)
        : m_TestsRepo{projPaths.tests().repo()}
        , m_Graph{std::move(filesRead.project_graph)}
        , m_ToolchainNewestModification{newest_modification(filesRead.toolchain_files, executableStamp)}
        , m_TestTranslationUnits{read_test_translation_units(executableStamp)}
      {}

      [[nodiscard]]
      const file_info& node(node_index i) const { return node_of(m_Graph, i); }

      [[nodiscard]]
      const fs::path& file(node_index i) const { return node(i).file; }

      [[nodiscard]]
      std::size_t file_count() const noexcept { return m_Graph.order(); }

      /// \throws std::out_of_range for a node the graph does not have
      [[nodiscard]]
      static const file_info& node_of(const graph_type& g, node_index i)
      {
        maths::graph_errors::check_node_index_range("dependency_graph::node", g.order(), i);
        return g.cbegin_node_weights()[i];
      }

      /** The modification time of a file the build read, checked against the executable's stamp.

          \throws std::runtime_error if the file post-dates the executable, or its modification time
          cannot be read.
       */
      [[nodiscard]]
      static fs::file_time_type modification_time(const fs::path& file, std::optional<fs::file_time_type> executableStamp)
      {
        std::error_code error{};
        const auto time{fs::last_write_time(file, error)};
        if(error)
          throw std::runtime_error{
            std::format("{} was read by the build but its modification time cannot be read: {}\n"
                        "Restore the file, or build the executable again\n",
                        file.generic_string(),
                        error.message())
          };

        if(executableStamp && (time >= *executableStamp))
          throw std::runtime_error{
            std::format("Executable is out of date; please build it!\nExecutable time stamp: {}\n{} time stamp: {}\n",
                        *executableStamp,
                        file.generic_string(),
                        time)
          };

        return time;
      }

      /// The newest modification among `files`, each checked; the earliest time, if there are none
      [[nodiscard]]
      static fs::file_time_type newest_modification(std::span<const fs::path> files, std::optional<fs::file_time_type> executableStamp)
      {
        auto times{files | std::views::transform([executableStamp](const fs::path& file){ return modification_time(file, executableStamp); })};

        return std::ranges::fold_left(times, fs::file_time_type::min(), std::ranges::max);
      }

      /** The modification time of every file the build read, by node; an object file, being a product
          rather than something read, holds the earliest time, which no fold for the newest can see.

          Every file is checked, whether or not a test depends on it: an edited `TestMain.cpp` is read by
          no test's translation unit, and is the case which made the executable stale without prune
          noticing.

          \throws std::runtime_error if a file post-dates the executable, or a file's modification time
          cannot be read.
       */
      [[nodiscard]]
      std::vector<fs::file_time_type> modification_times(std::optional<fs::file_time_type> executableStamp) const
      {
        auto modificationTime{
          [executableStamp](const file_info& info) {
            return info.is_object_file() ? fs::file_time_type::min() : modification_time(info.file, executableStamp);
          }
        };

        return m_Graph.cnode_weights() | std::views::transform(modificationTime) | std::ranges::to<std::vector>();
      }

      /// The nodes of the object files whose source lies in the tests repository: the leading nodes, by construction
      [[nodiscard]]
      auto get_test_object_file_nodes() const
      {
        auto isTest{
          [this](node_index i) {
            const auto& info{node(i)};
            return info.is_object_file() && in_repo(file(*info.source), m_TestsRepo);
          }
        };

        return std::views::iota(node_index{}, file_count()) | std::views::take_while(isTest);
      }

      [[nodiscard]]
      std::vector<translation_unit> read_test_translation_units(std::optional<fs::file_time_type> executableStamp) const
      {
        const auto times{modification_times(executableStamp)};

        auto translationUnit{
          [&](node_index object) {
            return
              translation_unit{
                .source{fs::relative(file(*node(object).source), m_TestsRepo)},
                .newest_modification{newest_modification(object, times)}
              };
          }
        };

        return get_test_object_file_nodes() | std::views::transform(translationUnit) | std::ranges::to<std::vector>();
      }

      /// The newest modification among every file on which the object depends
      [[nodiscard]]
      fs::file_time_type newest_modification(node_index object, std::span<const fs::file_time_type> times) const
      {
        auto newest{fs::file_time_type::min()};

        auto takeNewer{
          [this, &times, &newest](graph_type::const_edge_iterator edge) {
            if(const auto target{edge->target_node()}; !node(target).is_object_file())
              newest = std::ranges::max(newest, times[target]);
          }
        };

        maths::traverse(maths::depth_first,
                        m_Graph,
                        maths::ignore_disconnected_t{object},
                        maths::null_func_obj{},
                        maths::null_func_obj{},
                        takeNewer);

        return newest;
      }

      /** A node for every object file and every file of the project's read to produce one - the tests'
          object files first, then in order of first mention - each object file's source on its node; an
          edge from each object file to each such file; and the dependencies the convention adds. The
          toolchain's files are listed apart.

          Each file's path is made canonical - which is what project_paths holds, the build having
          recorded whatever spelling it was configured with, through whatever symlink and in whatever
          case - and each file is classed as the project's or the toolchain's. Both are properties of
          the file's directory, and the filesystem is asked once per directory.
       */
      [[nodiscard]]
      static files_read_by_build read_files(const build_tree& tree, const project_paths& projPaths)
      {
        const auto compilations{read_compilations(tree, projPaths.executable())};

        /* A directory whose existing prefix cannot be resolved - a directory without permission, a
           symlink loop - is kept as recorded, and its files fail with that reason when their modification
           times are read. A file's own name is as the compilation spelled it: a file which is itself a
           symlink keeps its name, which `last_write_time` follows, and on a filesystem which finds a file
           whatever its case, a header included under a case other than its own keeps that case, and so
           does not match the stem of the source named for it.
         */
        struct facts
        {
          fs::path canonical;
          bool toolchain;
        };
        std::map<fs::path, facts> directories{};
        auto directoryFacts{
          [&tree, &directories](const fs::path& dir) -> const facts& {
            if(const auto found{directories.find(dir)}; found != directories.end())
              return found->second;

            std::error_code error{};
            auto canonicalized{fs::weakly_canonical(dir, error)};
            const fs::path& canonical{error ? dir : canonicalized};

            return directories.emplace(dir, facts{.canonical{canonical}, .toolchain{in_toolchain(canonical, tree)}}).first->second;
          }
        };
        auto fileFacts{
          [&tree, &directoryFacts](const fs::path& p) {
            const auto asRecorded{(p.is_absolute() ? p : tree.build_directory / p).lexically_normal()};
            const auto& directory{directoryFacts(asRecorded.parent_path())};

            return facts{.canonical{directory.canonical / asRecorded.filename()}, .toolchain{directory.toolchain}};
          }
        };

        const auto& [files, records]{compilations};

        const auto fileFactsTable{files | std::views::transform(fileFacts) | std::ranges::to<std::vector>()};
        auto isProjectFile{[&fileFactsTable](compilations::file_index fileIndex){ return !fileFactsTable[fileIndex].toolchain; }};

        graph_type g{};
        std::vector<std::optional<node_index>> nodeOfFile(files.size());

        // A node per file the records name, on first sight
        auto nodeOf{
          [&](compilations::file_index fileIndex) {
            auto& node{nodeOfFile[fileIndex]};
            if(!node)
              node = g.add_node(file_info{.file{fileFactsTable[fileIndex].canonical}});

            return *node;
          }
        };

        // read_compilations puts the source first among the inputs
        auto isTest{
          [&fileFactsTable, testsRepo{projPaths.tests().repo()}](const compilations::record& record) {
            return in_repo(fileFactsTable[record.input_indices.front()].canonical, testsRepo);
          }
        };

        // The tests' object files take the leading indices, so that they form a block the graph can walk without a filter
        for(const auto& record : records | std::views::filter(isTest))
        {
          nodeOf(record.object_index);
        }

        for(const auto& record : records)
        {
          const auto objectNode{nodeOf(record.object_index)};
          const auto sourceNode{nodeOf(record.input_indices.front())};

          g.mutate_node_weight(g.cbegin_node_weights() + objectNode, [sourceNode](file_info& info){ info.source = sourceNode; });
          for(const auto inputNode : record.input_indices | std::views::filter(isProjectFile) | std::views::transform(nodeOf))
          {
            g.join(objectNode, inputNode);
          }
        }

        add_dependencies(g);

        auto toolchainFiles{
            fileFactsTable
          | std::views::filter(&facts::toolchain)
          | std::views::transform(&facts::canonical)
          | std::ranges::to<std::vector>()
        };

        return files_read_by_build{.project_graph{std::move(g)}, .toolchain_files{std::move(toolchainFiles)}};
      }

      /** An edge from each header to each object file whose source both shares the header's stem and
          includes the header: the convention that `Foo.cpp` implements `Foo.hpp`.
       */
      static void add_dependencies(graph_type& g)
      {
        auto file{[&g](node_index i) -> const fs::path& { return node_of(g, i).file; }};

        // Per file rather than per edge: a file is the input of many objects
        const auto stems{
            std::views::iota(node_index{}, g.order())
          | std::views::transform([&file](node_index i){ return file(i).stem().string(); })
          | std::ranges::to<std::vector>()
        };

        auto isObjectFile{[&g](node_index i){ return node_of(g, i).is_object_file(); }};
        auto target{[](const auto& edge){ return edge.target_node(); }};

        for(const auto objectNode : std::views::iota(node_index{}, g.order()) | std::views::filter(isObjectFile))
        {
          const auto sourceNode{*node_of(g, objectNode).source};

          auto implements{
            [&](node_index inputNode) {
              return (inputNode != sourceNode) && (stems[inputNode] == stems[sourceNode]);
            }
          };

          // Copied before joining, which may reallocate the edges being read
          const auto inputNodes{g.cedges(objectNode) | std::views::transform(target) | std::ranges::to<std::vector>()};
          for(const auto inputNode : inputNodes | std::views::filter(implements))
          {
            g.join(inputNode, objectNode);
          }
        }
      }
    };

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

      /** A test is stale if its dependencies or its materials moved after the threshold, unless
          it is recorded as passing since then - see the comment on the comparison.
       */
      [[nodiscard]]
      bool is_stale(const fs::path& relPath, fs::file_time_type newestDependencyModification) const
      {
        const auto newestModification{
          std::ranges::max(newestDependencyModification,
                           materials_modification_time(relPath, m_MaterialsRepo).value_or(newestDependencyModification))
        };

        if(newestModification <= m_StalenessThreshold)
          return false;

        /* `>=` rather than `>`. Following a modification to either code or test materials, a build
           may be performed and the tests are run. After the tests are executed, the 'passes' file may
           be written. Therefore, this write happens at a strictly later time than the aforementioned
           modification. However, the filesystem may not represent times with sufficient accuracy to
           be able to resolve this difference. For example, libstdc++ on macOS rounds filesystem times
           down to the nearest second. In this case, a comparison using '>' could return false, in
           the situation where the 'passes' file was actually written after modifications were made.
         */
        const auto passedAt{time_of_recorded_pass(relPath)};
        const bool passedSince{passedAt && (m_PassesStamp.value() >= newestModification) && (passedAt.value() > newestModification)};

        return !passedSince;
      }
    private:
      fs::path m_MaterialsRepo;
      fs::file_time_type m_StalenessThreshold;
      std::vector<prune_record> m_Passes;
      std::optional<fs::file_time_type> m_PassesStamp;

      [[nodiscard]]
      std::optional<fs::file_time_type> time_of_recorded_pass(const fs::path& relPath) const
      {
        if(!m_PassesStamp)
          return std::nullopt;

        const auto record{std::ranges::lower_bound(m_Passes, relPath, {}, path_projector{})};
        if((record == m_Passes.end()) || (record->test_path != relPath))
          return std::nullopt;

        return record->time_stamp;
      }
    };

    /// The stale tests; none at all if the toolchain changed since the threshold, since then every test is stale
    [[nodiscard]]
    std::optional<std::vector<fs::path>> find_stale_tests(fs::file_time_type stalenessThreshold, const project_paths& projPaths)
    {
      const auto tree{read_build_tree(projPaths.discovered().cmake_cache())};
      const dependency_graph graph{tree, projPaths};

      const auto tests{graph.get_test_translation_units()};

      /* With no tests selected, prune runs nothing and reports that nothing has changed. That is the
         truth when the project has no tests, and a permanent silence when it has tests which the
         build's record spells under some other path than project_paths expects - a tree configured
         through a symlink, or in another case - since then nothing would ever be selected again. The
         two are told apart by whether the record names anything of the project's at all.
       */
      if(tests.empty() && !graph.reads_from(projPaths.project_root()))
        throw std::runtime_error{
          std::format("The build's record names nothing under {}; "
                      "was the tree configured with a different spelling of the project's path?",
                      projPaths.project_root().generic_string())
        };

      if(graph.toolchain_newest_modification() > stalenessThreshold)
        return std::nullopt;

      const passing_tests passes{projPaths, stalenessThreshold};

      auto isStale{
        [&passes](const dependency_graph::translation_unit& unit) {
          return passes.is_stale(unit.source, unit.newest_modification);
        }
      };

      /* `copy` rather than `to<vector>`: `to` sizes a filtered range by walking it before copying
         from it, and advancing a `filter` iterator evaluates the predicate, so every unit after the
         first stale one would be judged twice - and the judgement walks that unit's materials.
       */
      std::vector<fs::path> staleTests{};
      std::ranges::copy(
          tests
        | std::views::filter(isStale)
        | std::views::transform(&dependency_graph::translation_unit::source),
        std::back_inserter(staleTests)
      );

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

    struct least_path_most_recent
    {
      [[nodiscard]]
      bool operator()(const prune_record& lhs, const prune_record& rhs) const
      {
        const auto comp{lhs.test_path <=> rhs.test_path};
        return comp == 0 ? lhs.time_stamp > rhs.time_stamp : comp < 0;
      }
    };

    /** Sorted by path; where a path recurs, only its most recent record is kept.

        A path recurs when records of one test from different runs are merged: the repetitions of an
        instability analysis, or a selected run's results with those already on disk.
     */
    [[nodiscard]]
    std::vector<prune_record> unique_by_path(std::vector<prune_record> records)
    {
      std::ranges::sort(records, least_path_most_recent{});
      const auto duplicates{std::ranges::unique(records, {}, path_projector{})};
      records.erase(duplicates.begin(), duplicates.end());
      return records;
    }

    [[nodiscard]]
    std::vector<prune_record> aggregate_failures(const prune_paths& prunePaths, const std::size_t numReps)
    {
      return unique_by_path(
          std::views::iota(0uz, numReps)
        | std::views::transform([&prunePaths](std::size_t i){ return read_tests(prunePaths.failures(i)); })
        | std::views::join
        | std::ranges::to<std::vector>()
      );
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
    std::ifstream ifile{file};
    if(!ifile)
      return {};

    try
    {
      using iter_t = std::istream_iterator<prune_record>;
      auto hasPath{[](const prune_record& record){ return !record.test_path.empty(); }};

      return std::ranges::subrange{iter_t{ifile}, iter_t{}} | std::views::filter(hasPath) | std::ranges::to<std::vector>();
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
      const auto prunePaths{projPaths.prune()};
      write_tests(projPaths, prunePaths.failures(id), unique_by_path(std::move(failedTests)));
      fs::remove(prunePaths.selected_passes(id));
      update_prune_stamp_on_disk(prunePaths, updateTime);
    }

    void do_update_prune_files(const project_paths& projPaths,
                               std::vector<prune_record> executedTests,
                               std::vector<prune_record> failedTests,
                               std::optional<std::size_t> id)
    {
      auto unionize{
        [](std::span<const prune_record> a, std::span<const prune_record> b){
          std::vector<prune_record> tests{};
          std::ranges::set_union(a, b, std::back_inserter(tests), least_path_most_recent{});
          return unique_by_path(std::move(tests));
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

      const auto executed{unique_by_path(std::move(executedTests))};
      const auto failed{unique_by_path(std::move(failedTests))};

      const auto trialPasses{unionize(executed, read_tests(passesFile))};
      const auto passingTests{difference(trialPasses, failed)};
      const auto remainingPreviousFailures{difference(read_tests(failuresFile), passingTests)};
      const auto allFailures{unionize(remainingPreviousFailures, failed)};

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
  std::string to_string(prune_fallback_reason reason)
  {
    switch(reason)
    {
    case prune_fallback_reason::no_previous_stamp: return "no previous stamp";
    case prune_fallback_reason::toolchain_changed: return "toolchain changed";
    }

    throw std::logic_error{"Unhandled prune_fallback_reason"};
  }

  [[nodiscard]]
  std::variant<std::vector<fs::path>, prune_fallback_reason>
  tests_to_run(const project_paths& projPaths)
  {
    const auto prunePaths{projPaths.prune()};
    const auto pruneTimeStamp{get_stamp(prunePaths.stamp())};

    if(!pruneTimeStamp)
      return prune_fallback_reason::no_previous_stamp;

    const auto staleTests{find_stale_tests(staleness_threshold(pruneTimeStamp.value()), projPaths)};
    if(!staleTests)
      return prune_fallback_reason::toolchain_changed;

    const std::vector<fs::path> failingTests{
      std::views::transform(read_tests(prunePaths.failures(std::nullopt)), path_projector{}) | std::ranges::to<std::vector>()
    };

    std::vector<fs::path> testsToRun{};
    std::ranges::set_union(*staleTests, failingTests, std::back_inserter(testsToRun));

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

  void aggregate_instability_analysis_prune_files(const project_paths& projPaths,
                                                  prune_mode mode,
                                                  std::filesystem::file_time_type timeStamp,
                                                  std::size_t numReps)
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
        executedCases.append_range(failingCases);

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
