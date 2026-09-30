////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/TestRemover.hpp"

#include "sequoia/TestFramework/FileEditors.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"
#include "sequoia/TestFramework/IndividualTestPaths.hpp"
#include "sequoia/TestFramework/TestCreator.hpp"

#include "sequoia/FileSystem/FileSystem.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <iterator>
#include <ostream>
#include <ranges>
#include <stdexcept>
#include <tuple>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    /** \brief A source file, beneath the tests repository, and the names of the tests it defines. */
    struct source_of_tests
    {
      fs::path file{};
      std::vector<std::string> tests{};
    };

    constexpr auto name_of{[](const test_registration& reg) { return reg.name; }};

    constexpr std::string_view cmakeEntryPrefix{"${TestDir}/"};

    [[nodiscard]]
    fs::path within_tests_repo(const fs::path& source, const project_paths& projPaths)
    {
      const auto& repo{projPaths.tests().repo()};
      return (repo / rebase_from(source, repo)).lexically_normal();
    }

    /** \brief Whether the last components of `path` are those of `suffix`.

        Not `std::ranges::ends_with`: libstdc++ 15 and the MS STL do not admit `fs::path` to it.
     */
    [[nodiscard]]
    bool ends_with_components(const fs::path& path, const fs::path& suffix)
    {
      const auto pathEnd{std::make_reverse_iterator(path.begin())}, suffixEnd{std::make_reverse_iterator(suffix.begin())};
      return std::mismatch(std::make_reverse_iterator(suffix.end()), suffixEnd,
                           std::make_reverse_iterator(path.end()),   pathEnd).first == suffixEnd;
    }

    [[nodiscard]]
    std::string relative_to_root(const fs::path& path, const project_paths& projPaths)
    {
      return path.lexically_relative(projPaths.project_root()).generic_string();
    }

    /** \brief The registered tests `request` names: by class, if it has no directory separator or extension; else by
               source file, for each test whose source's path ends in the request's.

        \throws std::runtime_error if the request names no registered test, or names a source file ambiguously.
     */
    [[nodiscard]]
    std::vector<test_registration> tests_named_by(const project_paths& projPaths,
                                                  const std::string& request,
                                                  const std::vector<test_registration>& registered)
    {
      const bool namesClass{request.find_first_of("/\\.") == std::string::npos};
      const fs::path requestPath{fs::path{request}.lexically_normal()};

      auto isNamed{
        [&](const test_registration& reg) {
          return namesClass ? (reg.name == request)
                            : ends_with_components(within_tests_repo(reg.source, projPaths), requestPath);
        }
      };

      const auto named{registered | std::views::filter(isNamed) | std::ranges::to<std::vector>()};
      if(named.empty())
        throw std::runtime_error{std::format("remove-test: {} names no test registered with this runner", request)};

      auto sourceOf{[&projPaths](const test_registration& reg) { return within_tests_repo(reg.source, projPaths); }};
      auto sources{named | std::views::transform(sourceOf) | std::ranges::to<std::vector>()};
      std::ranges::sort(sources);
      sources.erase(std::ranges::unique(sources).begin(), sources.end());

      if(sources.size() > 1)
      {
        auto relativeToRoot{[&projPaths](const fs::path& source) { return relative_to_root(source, projPaths); }};
        const auto candidates{
            sources
          | std::views::transform(relativeToRoot)
          | std::views::join_with('\n')
          | std::ranges::to<std::string>()
        };

        throw std::runtime_error{std::format("remove-test: {} may name any of\n{}", request, candidates)};
      }

      return named;
    }

    /** \brief Each source file holding a test to remove, with those tests, in order of the file's path.

        \throws std::runtime_error if a source file lies outside the tests repository, or defines a registered test
        which is not to be removed.
     */
    [[nodiscard]]
    std::vector<source_of_tests> group_by_source(const project_paths& projPaths,
                                                 const std::vector<test_registration>& testsToRemove,
                                                 const std::vector<test_registration>& registered)
    {
      auto locate{
        [&projPaths](const test_registration& reg) {
          return test_registration{reg.name, within_tests_repo(reg.source, projPaths)};
        }
      };

      auto located{testsToRemove | std::views::transform(locate) | std::ranges::to<std::vector>()};

      auto liesOutside{
        [&projPaths](const test_registration& reg) {
          const auto relative{reg.source.lexically_relative(projPaths.tests().repo())};
          return relative.empty() || (*relative.begin() == "..");
        }
      };

      if(const auto outside{std::ranges::find_if(located, liesOutside)}; outside != located.end())
        throw std::runtime_error{
          std::format("remove-test: the source file of {}, {}, lies outside the tests repository {}",
                      outside->name,
                      outside->source.generic_string(),
                      projPaths.tests().repo().generic_string())
        };
      std::ranges::sort(located, {}, [](const test_registration& reg) { return std::tie(reg.source, reg.name); });

      // A test may be named twice, by its class and by its source file.
      located.erase(std::ranges::unique(located).begin(), located.end());

      auto sameSource{
        [](const test_registration& lhs, const test_registration& rhs) { return lhs.source == rhs.source; }
      };

      auto toSource{
        [&](auto&& group) {
          const source_of_tests source{
            .file{std::ranges::begin(group)->source},
            .tests{group | std::views::transform(name_of) | std::ranges::to<std::vector>()}
          };

          auto remainsBehind{
            [&](const test_registration& reg) {
              return (within_tests_repo(reg.source, projPaths) == source.file)
                  && !std::ranges::contains(source.tests, reg.name);
            }
          };

          if(const auto kept{std::ranges::find_if(registered, remainsBehind)}; kept != registered.end())
            throw std::runtime_error{
              std::format("remove-test: {} defines {} as well as {}; name the source file to remove both",
                          relative_to_root(source.file, projPaths),
                          kept->name,
                          source.tests.front())
            };

          return source;
        }
      };

      return located
           | std::views::chunk_by(sameSource)
           | std::views::transform(toSource)
           | std::ranges::to<std::vector>();
    }

    /** \brief The files which may name a test: C++ sources and headers, and `CMakeLists.txt`, beyond the project's
               sources, tests, materials, build trees, build system, output, dependencies and templates; and, wherever
               they lie, the runner's own mains, their `CMakeLists.txt` and the common includes.
     */
    [[nodiscard]]
    std::vector<fs::path> files_which_may_name_tests(const project_paths& projPaths)
    {
      const std::array<fs::path, 8> excluded{
        projPaths.source().repo(),
        projPaths.tests().repo(),
        projPaths.test_materials().repo(),
        projPaths.build().dir(),
        projPaths.output().dir(),
        projPaths.dependencies().repo(),
        projPaths.aux_paths().repo(),
        projPaths.build_system().repo()
      };

      auto isExcluded{
        [&excluded](const fs::path& dir) {
          return back(dir).string().starts_with('.') || std::ranges::contains(excluded, dir.lexically_normal());
        }
      };

      auto mayNameTests{
        [](const fs::path& file) {
          return (file.filename() == "CMakeLists.txt")
              || (file.extension() == ".cpp")
              || std::ranges::contains(header_extensions, file.extension().string());
        }
      };

      // A loop rather than a view, since an excluded directory must not be entered.
      std::vector<fs::path> files{};
      for(fs::recursive_directory_iterator i{projPaths.project_root()}, end{}; i != end; ++i)
      {
        if(i->is_directory())
        {
          if(isExcluded(i->path()))
            i.disable_recursion_pending();
        }
        else if(i->is_regular_file() && mayNameTests(i->path()))
        {
          files.push_back(i->path());
        }
      }

      auto withItsCMakeLists{
        [](const main_paths& main) { return std::array{main.file(), main.cmake_lists(), main.common_includes()}; }
      };

      files.append_range(withItsCMakeLists(projPaths.main()));
      files.append_range(projPaths.ancillary_main_cpps() | std::views::transform(withItsCMakeLists) | std::views::join);

      std::ranges::sort(files);
      files.erase(std::ranges::unique(files).begin(), files.end());
      return files;
    }

    /** \brief Removes `dir`, and then each enclosing directory beneath `root`, for as long as each is empty. */
    void remove_empty_directories(fs::path dir, const fs::path& root)
    {
      auto liesBeneathRoot{
        [&root](const fs::path& p) {
          const auto relative{p.lexically_relative(root)};
          return !relative.empty() && (relative != ".") && (*relative.begin() != "..");
        }
      };

      while(liesBeneathRoot(dir) && fs::is_directory(dir) && fs::is_empty(dir))
      {
        fs::remove(dir);
        dir = dir.parent_path();
      }
    }

    /** \brief Whether a file of versioned output with this stem is the test's, among `rivals`: the other tests whose
               versioned output shares its directory.

        A test's versioned output is named for it: by its name alone, or by its name followed by an underscore and a
        qualifier, such as a configuration. The name of one test may begin that of another, so a file belongs to the
        test with the longest name the stem begins with.
     */
    [[nodiscard]]
    bool is_output_of(std::string_view stem, std::string_view test, const std::vector<std::string>& rivals)
    {
      auto isNamedFor{
        [stem](std::string_view name) {
          return (stem == name) || stem.starts_with(std::format("{}_", name));
        }
      };

      auto claimsIt{
        [&isNamedFor, test](const std::string& rival) { return (rival.size() > test.size()) && isNamedFor(rival); }
      };

      return isNamedFor(test) && std::ranges::none_of(rivals, claimsIt);
    }

    /** \brief The files of versioned output in `dir` which are the test's. */
    [[nodiscard]]
    std::vector<fs::path> output_of(const fs::path& dir, std::string_view test, const std::vector<std::string>& rivals)
    {
      if(!fs::is_directory(dir))
        return {};

      auto isTestsOutput{
        [test, &rivals](const fs::directory_entry& entry) {
          return entry.is_regular_file() && is_output_of(entry.path().stem().string(), test, rivals);
        }
      };

      auto files{
          fs::directory_iterator{dir}
        | std::views::filter(isTestsOutput)
        | std::views::transform([](const fs::directory_entry& entry) { return entry.path(); })
        | std::ranges::to<std::vector>()
      };

      std::ranges::sort(files);
      return files;
    }

    /** \brief Removes from `file` each line naming a test defined in `source`, and says whether there was one.

        `headerIncludes` are the paths by which the headers of `source` are included.
     */
    bool remove_lines_naming(const fs::path& file,
                             const source_of_tests& source,
                             const std::vector<std::string>& headerIncludes,
                             const fs::path& testsRepo)
    {
      if(file.filename() == "CMakeLists.txt")
        return remove_from_cmake(file, testsRepo, source.file, cmakeEntryPrefix);

      bool removed{remove_test_registrations(file, source.tests)};
      for(const auto& include : headerIncludes)
      {
        removed = remove_include(file, include) || removed;
      }

      return removed;
    }

    void report_path(std::ostream& stream, const fs::path& path, const project_paths& projPaths)
    {
      stream << std::format("\"{}\"\n", relative_to_root(path, projPaths));
    }

    /** \brief Removes the test's materials, and its versioned output for every configuration, reporting what goes.

        `rivals` are the other tests whose versioned output shares the test's directories.
     */
    void remove_materials_and_output(const project_paths& projPaths,
                                     const fs::path& source,
                                     std::string_view test,
                                     const std::vector<std::string>& rivals,
                                     std::ostream& stream)
    {
      const individual_materials_paths materials{source, test, projPaths, std::nullopt};
      if(fs::remove_all(materials.original_test_root()) > 0)
        report_path(stream, materials.original_test_root(), projPaths);

      remove_empty_directories(materials.original_test_root().parent_path(), projPaths.test_materials().repo());

      fs::remove_all(materials.temporary_materials_root());
      remove_empty_directories(materials.temporary_materials_root().parent_path(),
                               projPaths.output().tests_temporary_data());

      const auto summaryDir{test_summary_path{source, test, projPaths, std::nullopt}.file_path().parent_path()};
      const auto diagnosticsDir{
        individual_diagnostics_paths{projPaths, test, source, test_mode::standard, std::nullopt}
          .caught_exceptions_file_path()
          .parent_path()
      };

      for(const auto& [dir, root] : std::array{std::pair{summaryDir,     projPaths.output().test_summaries()},
                                               std::pair{diagnosticsDir, projPaths.output().diagnostics()}})
      {
        for(const auto& file : output_of(dir, test, rivals))
        {
          fs::remove(file);
          report_path(stream, file, projPaths);
        }

        remove_empty_directories(dir, root);
      }
    }

    void remove_source(const project_paths& projPaths,
                       const source_of_tests& source,
                       const std::vector<test_registration>& registered,
                       const std::vector<fs::path>& filesWhichMayNameTests,
                       std::ostream& stream)
    {
      const auto& testsRepo{projPaths.tests().repo()};

      auto withExtension{
        [&source](std::string_view extension) { return fs::path{source.file}.replace_extension(extension); }
      };

      auto exists{[](const fs::path& file) { return fs::exists(file); }};

      const auto headers{
          header_extensions
        | std::views::transform(withExtension)
        | std::views::filter(exists)
        | std::ranges::to<std::vector>()
      };

      const auto testList{
          source.tests
        | std::views::join_with(std::string_view{", "})
        | std::ranges::to<std::string>()
      };

      // The lines naming the files go before the files, so that a failure part-way leaves no line naming a file
      // which is gone.
      stream << std::format("Removing {}\nEditing:\n", testList);

      auto includePath{
        [&testsRepo](const fs::path& header) { return header.lexically_relative(testsRepo).generic_string(); }
      };

      const auto headerIncludes{headers | std::views::transform(includePath) | std::ranges::to<std::vector>()};

      for(const auto& file : filesWhichMayNameTests)
      {
        if(remove_lines_naming(file, source, headerIncludes, testsRepo))
          report_path(stream, file, projPaths);
      }

      stream << "Deleting:\n";

      for(const auto& file : headers)
      {
        if(fs::remove(file))
          report_path(stream, file, projPaths);
      }

      if(fs::remove(source.file))
        report_path(stream, source.file, projPaths);

      remove_empty_directories(source.file.parent_path(), testsRepo);

      for(const auto& test : source.tests)
      {
        // The tests whose sources share the directory, and so share its versioned output.
        auto isRival{
          [&](const test_registration& reg) {
            return (reg.name != test)
                && (within_tests_repo(reg.source, projPaths).parent_path() == source.file.parent_path());
          }
        };

        const auto rivals{
            registered
          | std::views::filter(isRival)
          | std::views::transform(name_of)
          | std::ranges::to<std::vector>()
        };

        remove_materials_and_output(projPaths, source.file, test, rivals, stream);
      }

      stream << '\n';
    }

    /** \brief Checks that the runner's own main registers each test in `source` as `create` writes the registration,
               and that the `CMakeLists.txt` beside it lists the source as `create` writes the entry.

        What the running executable registers, its main must: so a registration or an entry spelt otherwise shows
        that the project names the test in ways removal would miss.

        \throws std::runtime_error if either is missing.
     */
    void check_found_as_created(const project_paths& projPaths, const source_of_tests& source)
    {
      const auto& main{projPaths.main()};
      for(const auto& test : source.tests)
      {
        if(!registers_test(main.file(), test))
          throw std::runtime_error{
            std::format("remove-test: {} has no line registering {} as create writes it",
                        relative_to_root(main.file(), projPaths),
                        test)
          };
      }

      if(!names_in_cmake(main.cmake_lists(), projPaths.tests().repo(), source.file, cmakeEntryPrefix))
        throw std::runtime_error{
          std::format("remove-test: {} does not list {} as create writes it",
                      relative_to_root(main.cmake_lists(), projPaths),
                      relative_to_root(source.file, projPaths))
        };
    }
  }

  void remove_tests(const project_paths& projPaths,
                    const std::vector<std::string>& requests,
                    const std::vector<test_registration>& registered,
                    std::ostream& stream)
  {
    // Every refusal comes before anything is removed.
    auto testsNamed{
      [&projPaths, &registered](const std::string& request) { return tests_named_by(projPaths, request, registered); }
    };

    const auto testsToRemove{
        requests
      | std::views::transform(testsNamed)
      | std::views::join
      | std::ranges::to<std::vector>()
    };
    const auto sources{group_by_source(projPaths, testsToRemove, registered)};
    for(const auto& source : sources)
    {
      check_found_as_created(projPaths, source);
    }

    const auto filesWhichMayNameTests{files_which_may_name_tests(projPaths)};

    for(const auto& source : sources)
    {
      remove_source(projPaths, source, registered, filesWhichMayNameTests, stream);
    }
  }
}
