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

    [[nodiscard]]
    fs::path within_tests_repo(const fs::path& source, const project_paths& projPaths)
    {
      const auto& repo{projPaths.tests().repo()};
      return (repo / rebase_from(source, repo)).lexically_normal();
    }

    [[nodiscard]]
    std::string relative_to_root(const fs::path& path, const project_paths& projPaths)
    {
      return path.lexically_relative(projPaths.project_root()).generic_string();
    }

    /** \brief Each source file holding a test to remove, with those tests, in order of the file's path.

        \throws std::runtime_error if a source file defines a registered test which is not to be removed.
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
      std::ranges::sort(located, {}, [](const test_registration& reg) { return std::tie(reg.source, reg.name); });

      // A test may be named twice, by its class and by its source file.
      located.erase(std::ranges::unique(located).begin(), located.end());

      auto sameSource{
        [](const test_registration& lhs, const test_registration& rhs) { return lhs.source == rhs.source; }
      };

      auto nameOf{[](const test_registration& reg) { return reg.name; }};

      auto toSource{
        [&](auto&& group) {
          const source_of_tests source{
            .file{std::ranges::begin(group)->source},
            .tests{group | std::views::transform(nameOf) | std::ranges::to<std::vector>()}
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

    /** \brief The files beyond the project's sources, tests, materials, build trees, build system, output, dependencies
               and templates which may name a test: C++ sources and headers, and `CMakeLists.txt`.
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

      std::ranges::sort(files);
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
        return remove_from_cmake(file, testsRepo, source.file, "target_sources(", ")\n", "${TestDir}/");

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
      if(fs::remove_all(materials.original_test_root()))
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

      stream << std::format("Removing {}:\n", testList);

      for(const auto& file : headers)
      {
        fs::remove(file);
        report_path(stream, file, projPaths);
      }

      fs::remove(source.file);
      report_path(stream, source.file, projPaths);
      remove_empty_directories(source.file.parent_path(), testsRepo);

      auto nameOf{[](const test_registration& reg) { return reg.name; }};

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
          | std::views::transform(nameOf)
          | std::ranges::to<std::vector>()
        };

        remove_materials_and_output(projPaths, source.file, test, rivals, stream);
      }

      stream << "Editing:\n";

      auto includePath{
        [&testsRepo](const fs::path& header) { return header.lexically_relative(testsRepo).generic_string(); }
      };

      const auto headerIncludes{headers | std::views::transform(includePath) | std::ranges::to<std::vector>()};

      for(const auto& file : filesWhichMayNameTests)
      {
        if(remove_lines_naming(file, source, headerIncludes, testsRepo))
          report_path(stream, file, projPaths);
      }

      stream << '\n';
    }
  }

  void remove_tests(const project_paths& projPaths,
                    const std::vector<test_registration>& testsToRemove,
                    const std::vector<test_registration>& registered,
                    std::ostream& stream)
  {
    // Every refusal comes before anything is removed.
    const auto sources{group_by_source(projPaths, testsToRemove, registered)};
    const auto filesWhichMayNameTests{files_which_may_name_tests(projPaths)};

    for(const auto& source : sources)
    {
      remove_source(projPaths, source, registered, filesWhichMayNameTests, stream);
    }
  }
}
