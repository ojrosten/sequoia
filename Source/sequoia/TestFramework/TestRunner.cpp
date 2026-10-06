////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/TestRunner.hpp"

#include "sequoia/TestFramework/DependencyAnalyzer.hpp"
#include "sequoia/TestFramework/DumpComparison.hpp"
#include "sequoia/TestFramework/MaterialsUpdater.hpp"
#include "sequoia/TestFramework/ProjectCreator.hpp"
#include "sequoia/TestFramework/Summary.hpp"
#include "sequoia/TestFramework/TestCreator.hpp"

#include "sequoia/Core/Concurrency/ConcurrencyModels.hpp"
#include "sequoia/Parsing/CommandLineArguments.hpp"
#include "sequoia/PlatformSpecific/Helpers.hpp"
#include "sequoia/PlatformSpecific/Preprocessor.hpp"
#include "sequoia/Runtime/ShellCommands.hpp"
#include "sequoia/TextProcessing/Characters.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"
#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <set>
#include <ranges>
#include <span>
#include <system_error>
#include <format>
#include <functional>
#include <mutex>
#include <utility>
#include <variant>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    const auto entry_time_stamp{std::chrono::file_clock::now()};

    [[nodiscard]]
    std::string started_at(std::chrono::system_clock::time_point start)
    {
      return std::format("started {:%FT%TZ}\n", std::chrono::floor<std::chrono::milliseconds>(start));
    }

    /** \brief How long a sleep of `target` typically lasts on this machine. */
    [[nodiscard]]
    std::chrono::duration<double> typical_sleep_duration(std::chrono::milliseconds target)
    {
      auto sleepForTarget{[target]() { std::this_thread::sleep_for(target); }};

      std::array<std::chrono::duration<double>, 3> durations{};
      std::ranges::generate(durations, [&sleepForTarget]() { return profile(sleepForTarget); });
      std::ranges::sort(durations);

      // The fastest sleep is not typical: the first sleep can end within the timer tick it starts in, even
      // when every later sleep is rounded up to a whole tick.
      return durations[1];
    }

    void overwrite_quietly(const fs::path& file, std::string_view text)
    {
      std::error_code selectsTheNonThrowingOverload{};
      fs::create_directories(file.parent_path(), selectsTheNonThrowingOverload);

      replace_contents_quietly(file, text, write_mode::binary);
    }

    [[nodiscard]]
    std::string running_tests_message(concurrency_mode mode)
    {
      std::string mess{"\nRunning tests"};
      if(mode == concurrency_mode::serial) mess.append(", synchronously");

      return mess.append("...\n\n");
    }

    struct thread_pool_policy
    {
      std::size_t num{8};
    };

    template<class Weight, class UnaryFn>
    void accelerate(thread_pool_policy p, std::span<Weight> weights, UnaryFn fn)
    {
      if(const auto num{weights.size()}; num > 1)
      {
        concurrency::thread_pool<void> pool{std::ranges::min(num, p.num)};
        auto futures{
              std::views::transform(weights, [&pool, fn](auto& wt){ return pool.push([&wt, fn](){ return fn(wt); }); })
            | std::ranges::to<std::vector>()
        };

        for(auto& f : futures) f.get();
      }
      else if(num > 0)
      {
        fn(weights.front());
      }
    }

    template<class ExecutionPolicy, class Weight, class UnaryFn>
    void accelerate(ExecutionPolicy&& policy, std::span<Weight> weights, UnaryFn f)
    {
      if constexpr(has_parallel_algorithms_v)
      {
        std::for_each(std::forward<ExecutionPolicy>(policy), weights.begin(), weights.end(), std::move(f));
      }
      else
      {
        accelerate(thread_pool_policy{.num{8}}, weights, std::move(f));
      }
    }

    struct execution_info
    {
      std::thread::id       thread_id{};
      log_summary::duration duration{};
    };

    [[nodiscard]]
    log_summary::duration summed_duration(std::span<const execution_info> executions)
    {
      return std::ranges::fold_left(
               executions | std::views::transform(&execution_info::duration),
               log_summary::duration{},
               std::plus{}
             );
    }

    /** Groups the executions by thread, and returns the largest of the groups' summed durations. */
    [[nodiscard]]
    log_summary::duration busiest_thread_execution_duration(std::vector<execution_info> executions)
    {
      std::ranges::sort(executions, std::ranges::less{}, &execution_info::thread_id);

      auto sameThread{
        [](const execution_info& lhs, const execution_info& rhs){
          return lhs.thread_id == rhs.thread_id;
        }
      };

      auto threadTotals{
          executions
        | std::views::chunk_by(sameThread)
        | std::views::transform(summed_duration)
      };

      return std::ranges::fold_left(threadTotals, log_summary::duration{}, std::ranges::max);
    }

    constexpr std::array<std::string_view, 3> materials_kinds{"WorkingCopy", "Prediction", "Auxiliary"};

    constexpr auto filename_of{
      [](const fs::directory_entry& entry) { return entry.path().filename().generic_string(); }
    };

    [[nodiscard]]
    bool is_placeholder(std::string_view name)
    {
      return (name == ".keep") || (name == ".DS_Store");
    }

    /// Whether `name` is one of the kinds of material, ignoring case
    [[nodiscard]]
    bool is_materials_kind(std::string_view name)
    {
      auto isName{[name](std::string_view kind){ return ascii::same_ignoring_case(name, kind); }};
      return std::ranges::any_of(materials_kinds, isName);
    }

    /** An original materials root holds `WorkingCopy`, `Prediction` and `Auxiliary`, besides a
        `.keep` or Finder's `.DS_Store`. Anything more was committed for an older layout, and preparing
        the materials would ignore it without a word.
     */
    void throw_if_stray_materials(const individual_materials_paths& materials)
    {
      auto isStray{
        [](const std::string& name) { return !std::ranges::contains(materials_kinds, name) && !is_placeholder(name); }
      };

      const auto root{materials.original_materials_root()};
      auto strays{
          fs::directory_iterator{root}
        | std::views::transform(filename_of)
        | std::views::filter(isStray)
        | std::ranges::to<std::vector>()
      };

      if(!strays.empty())
      {
        std::ranges::sort(strays);
        throw std::runtime_error{
          std::format("The materials in {} hold {}, which would be ignored: "
                      "only WorkingCopy, Prediction and Auxiliary are used",
                      root.generic_string(),
                      strays | std::views::join_with(std::string_view{", "}) | std::ranges::to<std::string>())
        };
      }
    }

    /** Whether `name` is a name Windows reserves for a device, alone or followed immediately by an
        extension, ignoring case. The reserved names are those listed in Microsoft's "Naming Files,
        Paths, and Namespaces".
     */
    [[nodiscard]]
    bool is_windows_device_name(std::string_view name)
    {
      auto stem{name.substr(0, name.find('.'))};

      // The superscript digits U+00B9, U+00B2 and U+00B3 are matched in their UTF-8 encoding
      constexpr auto portNumbers{
        std::to_array<std::string_view>({
          "1", "2", "3", "4", "5", "6", "7", "8", "9", "\xC2\xB9", "\xC2\xB2", "\xC2\xB3"
        })
      };

      constexpr std::size_t portNameLength{3};
      const auto portName{stem.substr(0, portNameLength)};
      const bool numberedDevice{
           (stem.size() > portNameLength)
        && (ascii::same_ignoring_case(portName, "COM") || ascii::same_ignoring_case(portName, "LPT"))
        && std::ranges::contains(portNumbers, stem.substr(portNameLength))
      };

      constexpr auto devices{std::to_array<std::string_view>({"CON", "PRN", "AUX", "NUL"})};
      auto isStem{[stem](std::string_view device){ return ascii::same_ignoring_case(stem, device); }};
      return numberedDevice || std::ranges::any_of(devices, isStem);
    }

    /** \throws std::runtime_error if one of these holds:
                 -# The materials discriminator is not a portable name for one directory;
                 -# The discriminator names a kind of material, ignoring case;
                 -# The discriminator differs only in case from the name of an entry in the test's own
                    directory.
     */
    void throw_if_bad_materials_discriminator(const individual_materials_paths& materials)
    {
      const auto& name{materials.materials_discriminator().value()};

      auto failureMessage{
        [&name](std::string_view restriction) {
          return std::format("The materials discriminator \"{}\" must {}", name, restriction);
        }
      };

      if(name.empty())
        throw std::runtime_error{failureMessage("not be empty")};

      if((name == ".") || (name == ".."))
        throw std::runtime_error{failureMessage("name a directory of its own")};

      constexpr std::string_view forbidden{"/\\:*?\"<>|"};
      if(const auto pos{name.find_first_of(forbidden)}; pos != std::string::npos)
        throw std::runtime_error{failureMessage(std::format("not contain '{}'", name[pos]))};

      if(std::ranges::any_of(name, [](unsigned char c){ return c < 0x20; }))
        throw std::runtime_error{failureMessage("not contain a control character")};

      // Windows strips a trailing dot or space, so two discriminators could share one directory
      if((name.back() == '.') || (name.back() == ' '))
        throw std::runtime_error{failureMessage("not end in a dot or a space")};

      if(is_windows_device_name(name))
        throw std::runtime_error{failureMessage("not be a device name on Windows")};

      if(is_materials_kind(name))
        throw std::runtime_error{failureMessage("not name a kind of material")};

      const auto& root{materials.original_test_root()};
      if(!fs::exists(root))
        return;

      // A case-insensitive filesystem takes names differing only in case for the same entry
      auto differsOnlyInCase{
        [&name](const std::string& sibling) { return (sibling != name) && ascii::same_ignoring_case(sibling, name); }
      };

      auto caseVariants{
          fs::directory_iterator{root}
        | std::views::transform(filename_of)
        | std::views::filter(differsOnlyInCase)
        | std::ranges::to<std::vector>()
      };

      if(!caseVariants.empty())
      {
        std::ranges::sort(caseVariants);
        const auto caseVariantList{
            caseVariants
          | std::views::join_with(std::string_view{", "})
          | std::ranges::to<std::string>()
        };

        throw std::runtime_error{failureMessage(std::format("not differ only in case from {}", caseVariantList))};
      }
    }

    void throw_if_materials_beside_discriminated_directories(const individual_materials_paths& materials)
    {
      const auto& root{materials.original_test_root()};
      auto isIgnored{
        [](const fs::directory_entry& entry) {
          const auto name{filename_of(entry)};
          return !is_placeholder(name) && (!entry.is_directory() || is_materials_kind(name));
        }
      };

      auto ignored{
          fs::directory_iterator{root}
        | std::views::filter(isIgnored)
        | std::views::transform(filename_of)
        | std::ranges::to<std::vector>()
      };

      if(!ignored.empty())
      {
        std::ranges::sort(ignored);
        throw std::runtime_error{
          std::format("The materials in {} hold {} beside the discriminated directories, which would be ignored: "
                      "only {} is read",
                      root.generic_string(),
                      ignored | std::views::join_with(std::string_view{", "}) | std::ranges::to<std::string>(),
                      materials.materials_discriminator().value())
        };
      }
    }

    [[nodiscard]]
    std::optional<removal_failure> try_remove_all(const fs::path& dir)
    {
      std::error_code error{};
      fs::remove_all(dir, error);
      if(error)
        return removal_failure{dir, error.message()};

      return std::nullopt;
    }

    [[nodiscard]]
    bool try_rename(const fs::path& from, const fs::path& to)
    {
      std::error_code error{};
      fs::rename(from, to, error);
      return !error;
    }

    // If the leftover could not be removed either, the error names both, since
    // the two failures usually share a cause
    void remove_temporary_root_in_place(const fs::path& temporaryRoot,
                                        const std::optional<removal_failure>& leftoverFailure)
    {
      try
      {
        fs::remove_all(temporaryRoot);
      }
      catch(const fs::filesystem_error& e)
      {
        if(!leftoverFailure)
          throw;

        throw std::runtime_error{
          std::format("Unable to remove the temporary root {} in place, "
                      "after its discarded root {} could not be removed\n"
                      "The temporary root: {}\n"
                      "The discarded root: {}",
                      temporaryRoot.generic_string(),
                      leftoverFailure->dir.generic_string(),
                      e.code().message(),
                      leftoverFailure->error_message)
        };
      }
    }

    [[nodiscard]]
    discarded_materials_remover::future_type ready_future_of(std::optional<removal_failure> failure)
    {
      std::promise<std::optional<removal_failure>> promise{};
      promise.set_value(std::move(failure));
      return promise.get_future();
    }

    [[nodiscard]]
    std::string removal_failure_message(const removal_failure& failure, const fs::path& projectRoot)
    {
      return std::format("Discarded materials not removed from {}:\n{}",
                         failure.dir.lexically_relative(projectRoot).generic_string(),
                         failure.error_message);
    }

    [[nodiscard]]
    std::vector<std::string> post_run_failure_messages(std::span<const std::string> trackerFailures,
                                                       std::span<const removal_failure> removalFailures,
                                                       const fs::path& projectRoot)
    {
      auto removalMessage{
        [&projectRoot](const removal_failure& failure) { return removal_failure_message(failure, projectRoot); }
      };

      // TO DO: std::views::concat | std::ranges::to<std::vector>, once the
      // MS STL has concat (P2542)

      std::vector<std::string> messages{trackerFailures.begin(), trackerFailures.end()};
      messages.append_range(removalFailures | std::views::transform(removalMessage));
      return messages;
    }

    void copy_original_materials(const individual_materials_paths& materials)
    {
      if(materials.materials_discriminator())
      {
        throw_if_bad_materials_discriminator(materials);
        if(fs::exists(materials.original_test_root()))
          throw_if_materials_beside_discriminated_directories(materials);
      }

      if(!fs::exists(materials.original_materials_root()))
        return;

      throw_if_stray_materials(materials);

      if(const auto originalWorking{materials.original_working()}; fs::exists(originalWorking))
      {
        fs::copy(originalWorking, materials.working(), fs::copy_options::recursive);
      }
      else
      {
        fs::create_directory(materials.working());
      }

      if(const auto originalAuxiliary{materials.original_auxiliary()}; fs::exists(originalAuxiliary))
      {
        fs::copy(originalAuxiliary, materials.auxiliary(), fs::copy_options::recursive);
      }
    }

    struct test_paths
    {
      test_paths(const fs::path& sourceFile,
                 const test_summary_path& summaryFile,
                 const individual_materials_paths& materialsPaths,
                 const project_paths& projPaths)
        : summary{summaryFile}
        , test_file{rebase_from(sourceFile, projPaths.tests().repo())}
        , materials{materialsPaths}
      {}

      test_summary_path summary;
      fs::path test_file;
      individual_materials_paths materials;
    };

    struct paths_comparator
    {
      [[nodiscard]]
      bool operator()(const test_paths& lhs, const test_paths& rhs) const
      {
        return lhs.materials.working() < rhs.materials.working();
      }
    };

    struct nascent_test_data
    {
      nascent_test_data(std::string type, std::string subType, test_runner& r, std::vector<nascent_test_vessel>& nascentTests);

      void operator()(const parsing::commandline::arg_list& args);

      std::string genus, species;
      test_runner& runner;
      std::vector<nascent_test_vessel>& nascent_tests;
    };


    nascent_test_data::nascent_test_data(std::string type, std::string subType, test_runner& r, std::vector<nascent_test_vessel>& nascentTests)
      : genus{std::move(type)}
      , species{std::move(subType)}
      , runner{r}
      , nascent_tests{nascentTests}
    {}

    void nascent_test_data::operator()(const parsing::commandline::arg_list& args)
    {
      nascent_test_factory factory{"semantic", "allocation", "behavioural"};
      auto nascentTest{factory.make(genus, runner.proj_paths(), runner.copyright(), runner.code_indent(), runner.stream())};

      std::visit(
        overloaded{
          [&args,&species = species](nascent_semantics_test& nascent) {
            if(ascii::is_empty_or_whitespace(args[1]))
              throw std::runtime_error{
                std::format("{}_test {} '{}': the equivalent_type names no type", species, args[0], args[1])
              };

            nascent.test_type(species);
            nascent.qualified_name(args[0]);
            nascent.equivalent_type(args[1]);
          },
          [&args,&species = species](nascent_allocation_test& nascent) {
            nascent.test_type(species);
            nascent.forename(args[0]);
          },
          [&args,&species = species](nascent_behavioural_test& nascent) {
            nascent.test_type(species);
            nascent.header(args[0]);
          }
        },
        nascentTest);

      nascent_tests.emplace_back(std::move(nascentTest));
    }

    constexpr auto return_code_names{
      std::to_array<std::pair<return_code, std::string_view>>({
        {return_code::versioned_output_diffs, "versioned_output_diffs"},
        {return_code::soft_failures,          "soft_failures"         },
        {return_code::critical_failures,      "critical_failures"     },
        {return_code::incomplete_run,         "incomplete_run"        },
        {return_code::post_run_failures,      "post_run_failures"     }
      })
    };

    constexpr return_code dirty_return_codes{
      std::ranges::fold_left(
        return_code_names,
        return_code::success,
        [](return_code acc, const auto& entry) { return acc | entry.first; }
      )
    };

    static_assert(std::has_single_bit(std::to_underlying(dirty_return_codes) + 1),
                  "The flags with rows in return_code_names do not occupy consecutive bits from the lowest; "
                  "give every return_code flag a row, and keep the flags consecutive");

    /** \brief The number a runner adds to its `return_code` to give its exit status, unless the
               code is success.

        A runner therefore exits either with 0 or with a status from `runner_exit_offset + 1` to
        `max_runner_exit_status`. That range is clear of these statuses, which a process gives when
        it fails for reasons of its own:
        -# 1 and 2, generically;
        -# 23, from LeakSanitizer;
        -# 64 to 78, from BSD's sysexits;
        -# 66 and 77, from ThreadSanitizer and MemorySanitizer;
        -# 126 and above, from a shell.

        TestRunner.hpp states this value in the contract of `to_exit_code`.
     */
    constexpr int runner_exit_offset{80};

    /** \brief The highest exit status a runner gives: the one which carries every flag. */
    constexpr int max_runner_exit_status{
      runner_exit_offset + static_cast<int>(std::to_underlying(dirty_return_codes))
    };

    static_assert(runner_exit_offset > 78, "The runner's exit statuses would overlap BSD's sysexits");

    // A POSIX exit status is 8 bits, and a shell's own statuses begin at 126.
    static_assert(max_runner_exit_status <= 125,
                  "The runner's exit statuses no longer fit below the shell's; report the return_code "
                  "to a parent process through a file it names, rather than in the exit status");

    [[nodiscard]]
    std::string to_async_option(concurrency_mode mode, std::size_t threadPoolSize)
    {
      switch(mode)
      {
      case concurrency_mode::serial:
        return " --serial";
      case concurrency_mode::dynamic:
        return "";
      case concurrency_mode::fixed:
        return std::format(" --thread-pool {}", threadPoolSize);
      }

      throw std::logic_error{"Illegal option for concurrency_mode"};
    }

    const std::string& convert(const std::string& s) { return s; }
    std::string convert(const fs::path& p) { return p.generic_string(); }

    [[nodiscard]]
    std::vector<fs::path> concatenate(std::span<const fs::path> first, std::span<const fs::path> second)
    {
      // TO DO: std::views::concat | std::ranges::to<std::vector>, once the
      // MS STL has concat (P2542)
      std::vector<fs::path> both{first.begin(), first.end()};
      both.append_range(second);
      return both;
    }

    /** \brief Warns of each request from the command line - a suite or a source, selected or
        excluded - which matched no registered test, with the hint the request's key suggests.
     */
    template<class Key, invocable_exact_r<std::string, const Key&> Hint>
    void report_unmatched(std::ostream& stream, std::span<const std::pair<Key, bool>> requests, std::string_view kind, Hint hint)
    {
      for(const auto& [key, matched] : requests)
      {
        if(!matched)
        {
          using namespace parsing::commandline;
          stream << warning(std::format("{} '{}' not found\n{}", kind, convert(key), hint(key)));
        }
      }
    }

    enum class is_filtered { no, yes };

    class test_tracker
    {
    public:
      explicit test_tracker(const project_paths& projPaths,
                            std::optional<std::size_t> id,
                            is_filtered isFiltered,
                            std::vector<fs::path> testsLeftOut)
        : m_ProjPaths{projPaths}
        , m_Id{id}
        , m_Filtered{isFiltered}
        , m_TestsLeftOut{std::move(testsLeftOut)}
      {}

      void update_materials_and_prune_info()
      {
        for(const auto& update : m_Updateables)
        {
          const auto deleted{
            [&]() -> std::vector<fs::path> {
              try
              {
                return soft_update(update.materials.working(), update.materials.prediction());
              }
              catch(const incomplete_update& e)
              {
                record_materials_update_failure(update.test_file, e.what());
                return std::ranges::to<std::vector>(e.deleted());
              }
              catch(const std::exception& e)
              {
                record_materials_update_failure(update.test_file, e.what());
              }
              catch(...)
              {
                record_materials_update_failure(update.test_file, unrecognized);
              }

              return {};
            }()
          };

          record_deleted_predictions(update.test_file, deleted);
        }

        try
        {
          update_prune_info();
        }
        catch(const std::exception& e)
        {
          record_prune_update_failure(e.what());
        }
        catch(...)
        {
          record_prune_update_failure(unrecognized);
        }
      }

      [[nodiscard]]
      std::span<const std::string> post_run_failures() const noexcept { return m_PostRunFailures; }

      [[nodiscard]]
      std::span<const std::string> materials_update_report() const noexcept { return m_MaterialsUpdateReport; }

      void process_test(const test_paths& files, const log_summary& summary, update_mode updateMode)
      {
        m_ExecutedTests.push_back(files.test_file);

        if(summary.soft_failures() || summary.critical_failures())
          m_FailedTests.push_back(files.test_file);

        to_file(files.summary, summary);

        if(updateMode != update_mode::none)
        {
          const auto& materials{files.materials};
          if(summary.soft_failures() && fs::exists(materials.working()) && fs::exists(materials.prediction()))
          {
            if(summary.critical_failures())
              record_update_withheld(files.test_file);
            else
              m_Updateables.insert(files);
          }
        }
      }
    private:
      constexpr static std::string_view unrecognized{"Unrecognized exception"};

      project_paths m_ProjPaths;
      std::optional<std::size_t> m_Id{};
      is_filtered m_Filtered{};

      std::vector<fs::path> m_FailedTests{}, m_ExecutedTests{}, m_TestsLeftOut{};
      std::vector<std::string> m_PostRunFailures{}, m_MaterialsUpdateReport{};
      std::set<test_paths, paths_comparator> m_Updateables{};

      void to_file(const test_summary_path& summaryFile, const log_summary& summary)
      {
        const auto& filename{summaryFile.file_path()};
        if(filename.empty()) return;

        fs::create_directories(filename.parent_path());

        write_to_file(filename,
                      summarize(summary, "", summary_detail::failure_messages, no_indent, no_indent),
                      std::ios_base::out | std::ios_base::binary);
      }

      void record_materials_update_failure(const fs::path& testFile, std::string_view what)
      {
        m_PostRunFailures.push_back(
          std::format("Update of materials for {} did not complete:\n{}", testFile.generic_string(), what)
        );
      }

      void record_deleted_predictions(const fs::path& testFile, std::span<const fs::path> deleted)
      {
        if(deleted.empty())
          return;

        const auto relativeToProjectRoot{
          [&root = m_ProjPaths.project_root()](const fs::path& path) {
            return path.lexically_relative(root).generic_string();
          }
        };

        const auto listing{
            deleted
          | std::views::transform(relativeToProjectRoot)
          | std::views::join_with('\n')
          | std::ranges::to<std::string>()
        };

        m_MaterialsUpdateReport.push_back(
          std::format("Predictions for {} deleted by the update:\n{}", testFile.generic_string(), listing)
        );
      }

      void record_update_withheld(const fs::path& testFile)
      {
        m_MaterialsUpdateReport.push_back(
          std::format("Materials for {} not updated, since the test had critical failures", testFile.generic_string())
        );
      }

      void record_prune_update_failure(std::string_view what)
      {
        m_PostRunFailures.push_back(std::format("Prune information not written:\n{}", what));
      }

      void update_prune_info() const
      {
        if(m_Filtered == is_filtered::yes)
        {
          update_prune_files(m_ProjPaths, m_ExecutedTests, m_FailedTests, entry_time_stamp, m_Id);
        }
        else
        {
          const auto testsToRerun{concatenate(m_FailedTests, m_TestsLeftOut)};
          update_prune_files(m_ProjPaths, testsToRerun, entry_time_stamp, m_Id);
        }
      }
    };

    template<class Act, class Vessel>
    inline constexpr bool acts_on_every_alternative_v{false};

    template<class Act, class... Nascents>
    inline constexpr bool acts_on_every_alternative_v<Act, std::variant<Nascents...>>{
      (std::invocable<const Act&, Nascents&, const parsing::commandline::arg_list&> && ...)
    };
  }

  std::string to_string(const return_code code)
  {
    if((code & ~dirty_return_codes) != return_code::success)
      throw std::logic_error{std::format("Unrecognized bits in return_code: {}", std::to_underlying(code))};

    if(code == return_code::success) return "success";

    return std::ranges::fold_left(
             return_code_names,
             std::string{},
             [code](std::string name, const auto& entry) {
               const auto& [bit, text] {entry};

               if((code & bit) == bit)
               {
                 if(!name.empty()) name.append("|");
                 name.append(text);
               }

               return name;
             }
           );
  }

  [[nodiscard]]
  return_code child_return_code(const int exitStatus, std::string_view childDescription)
  {
    if(exitStatus == 0)
      return return_code::success;

    // The range is checked before the offset is subtracted. A Windows status of 0x80000000 or more
    // is negative, and near INT_MIN the subtraction would overflow.
    if((exitStatus <= runner_exit_offset) || (exitStatus > max_runner_exit_status))
      throw std::runtime_error{
        std::format("{} {}.\nThat is not one of a test runner's exit statuses, which are 0 and {} to {}, so "
                    "it did not complete a test run: it may not have been built, may be misconfigured, "
                    "or may have crashed.\n",
                    childDescription,
                    runtime::describe_exit_status(exitStatus),
                    runner_exit_offset + 1,
                    max_runner_exit_status)
      };

    return static_cast<return_code>(exitStatus - runner_exit_offset);
  }

  [[nodiscard]]
  return_code to_return_code(const log_summary& summary) noexcept
  {
    auto code{return_code::success};

    if(summary.soft_failures())     code |= return_code::soft_failures;
    if(summary.critical_failures()) code |= return_code::critical_failures;

    return code;
  }

  [[nodiscard]]
  int to_exit_code(const return_code code) noexcept
  {
    if(code == return_code::success)
      return 0;

    const auto carried{(code & ~dirty_return_codes) == return_code::success
                         ? code
                         : (code & dirty_return_codes) | return_code::incomplete_run};

    return runner_exit_offset + static_cast<int>(std::to_underlying(carried));
  }

  [[nodiscard]]
  discarded_materials_remover::future_type discarded_materials_remover::enqueue_removal(fs::path discardedRoot)
  {
    return m_Pool.push([discardedRoot{std::move(discardedRoot)}](){ return try_remove_all(discardedRoot); });
  }

  void discarded_materials_remover::join()
  {
    m_Pool.join();
  }

  [[nodiscard]]
  discarded_materials_remover::future_type prepare_materials(const individual_materials_paths& materials,
                                                             discarded_materials_remover& remover)
  {
    const auto& temporaryRoot{materials.temporary_materials_root()};
    if(temporaryRoot.empty())
      throw std::logic_error{"Unable to prepare materials whose paths name no test"};

    // Moving or removing the whole of this test's temporary tree, or of its
    // discarded root, is safe because `test_runner::register_test` admits each
    // name once, ignoring case, and no source whose materials prefix nests
    // with another's.
    const auto discardedRoot{materials.discarded_materials_root()};

    // A run which died, or which could not remove it, leaves the discarded
    // root behind, and the move cannot replace it
    const auto leftoverFailure{try_remove_all(discardedRoot)};

    // The move is one metadata operation, whereas a removal visits every
    // entry. The move fails if the temporary root does not exist and, under
    // Windows, if a file within the tree is open.
    const bool moved{!leftoverFailure && try_rename(temporaryRoot, discardedRoot)};
    if(!moved)
      remove_temporary_root_in_place(temporaryRoot, leftoverFailure);

    fs::create_directories(temporaryRoot);
    copy_original_materials(materials);

    // If copying throws, the discarded root stays until the test's next
    // preparation removes it
    if(moved)
      return remover.enqueue_removal(discardedRoot);

    return ready_future_of(leftoverFailure);
  }

  void test_to_run::versioned_write(const fs::path& file, std::string_view text)
  {
    if(!text.empty() || fs::exists(file))
    {
      // An empty directory cannot be committed, so this one is made only when a file goes into it.
      fs::create_directories(file.parent_path());

      write_to_file(file, text, std::ios_base::out | std::ios_base::binary);
    }
  }

  test_to_run::scoped_execution_record::scoped_execution_record(std::filesystem::path file,
                                                                const execution_timer& executionTimer)
    : m_File{std::move(file)}
    , m_Start{std::chrono::system_clock::now()}
    , m_ExecutionTimer{executionTimer}
  {
    overwrite_quietly(m_File, started_at(m_Start));
  }

  test_to_run::scoped_execution_record::~scoped_execution_record()
  {
    using std::chrono::microseconds, std::chrono::duration_cast;
    overwrite_quietly(m_File,
                      std::format("{}execution duration {}us\nrunner overhead {}us\n",
                                  started_at(m_Start),
                                  duration_cast<microseconds>(m_ExecutionTimer.execution_duration()).count(),
                                  duration_cast<microseconds>(m_ExecutionTimer.runner_overhead()).count()));
  }

  [[nodiscard]]
  log_summary test_to_run::execute(std::optional<std::size_t> index, discarded_materials_remover& remover)
  {
    execution_timer executionTimer{};
    auto summary{execute_and_record(index, executionTimer, remover)};
    summary.runner_overhead(executionTimer.runner_overhead());

    return summary;
  }

  [[nodiscard]]
  std::optional<removal_failure> test_to_run::extract_discarded_materials_removal_failure()
  {
    if(!m_DiscardedMaterialsRemovalFailureFuture.valid())
      return std::nullopt;

    try
    {
      return m_DiscardedMaterialsRemovalFailureFuture.get();
    }
    catch(const std::exception& e)
    {
      return removal_failure{m_Vessel.materials_paths().discarded_materials_root(), e.what()};
    }
    catch(...)
    {
      return removal_failure{m_Vessel.materials_paths().discarded_materials_root(), "Unknown exception"};
    }
  }

  /** Returns the test's summary but for the runner's overhead. The overhead
      includes finishing the record, and the record finishes only after this
      function has made the summary.
   */
  [[nodiscard]]
  log_summary test_to_run::execute_and_record(std::optional<std::size_t> index,
                                              execution_timer& executionTimer,
                                              discarded_materials_remover& remover)
  {
    // Also installed per test, since under MSVC each thread has its own
    // terminate handler
    const scoped_terminate_handler terminationReported{report_termination};
    const scoped_execution_record record{m_ExecutionRecord.file_path(), executionTimer};

    if(try_prepare_materials(remover))
      executionTimer.time_execution([this](){ try_run_tests(); });

    return write_output(executionTimer.execution_duration(), index);
  }

  void test_to_run::try_run_tests()
  {
    try
    {
      m_Vessel.run_tests();
    }
    catch(const std::exception& e)
    {
      m_Vessel.log_critical_failure("Unexpected", e.what());
    }
    catch(...)
    {
      m_Vessel.log_critical_failure("Unknown", "");
    }
  }

  [[nodiscard]]
  log_summary test_to_run::write_output(const log_summary::duration executionDuration,
                                        std::optional<std::size_t> index)
  {
    try
    {
      m_Vessel.write_instability_analysis_output(index);
      return write_versioned_output(executionDuration);
    }
    catch(const std::exception& e)
    {
      m_Vessel.log_critical_failure("Output Writing", e.what());
    }
    catch(...)
    {
      m_Vessel.log_critical_failure("Output Writing", "Unknown exception");
    }

    return m_Vessel.summarize(executionDuration);
  }

  [[nodiscard]]
  bool test_to_run::try_prepare_materials(discarded_materials_remover& remover)
  {
    try
    {
      m_DiscardedMaterialsRemovalFailureFuture = prepare_materials(m_Vessel.materials_paths(), remover);
      return true;
    }
    catch(const std::exception& e)
    {
      m_DiscardedMaterialsRemovalFailureFuture = {};
      m_Vessel.log_critical_failure("Materials Preparation", e.what());
      return false;
    }
  }

  [[nodiscard]]
  log_summary test_to_run::write_versioned_output(const log_summary::duration executionDuration) const
  {
    auto summary{m_Vessel.summarize(executionDuration)};

    if(!m_Vessel.has_critical_failures())
    {
      const auto& diagnostics{m_Vessel.diagnostics_file_paths()};
      versioned_write(diagnostics.false_positive_or_negative_file_path(), summary.diagnostics_output());
      versioned_write(diagnostics.caught_exceptions_file_path(),          summary.caught_exceptions_output());
    }

    return summary;
  }

  //=========================================== test_runner ===========================================//

  //===================================== test_filter =====================================//

  [[nodiscard]]
  bool test_runner::test_filter::operator()(const normal_path& source,
                                            std::span<const std::string> suites,
                                            is_performance_test isPerformanceTest)
  {
    auto sameSource{[this, &source](const normal_path& listed){ return m_Equivalent(listed, source); }};
    auto amongSuites{[suites](const std::string& selected){ return std::ranges::contains(suites, selected); }};

    const bool excluded{mark_found(m_ExcludedItems, sameSource)};
    const bool selectedBySource{m_SelectedItems  && mark_found(*m_SelectedItems,  sameSource)};
    const bool selectedBySuite {m_SelectedSuites && mark_found(*m_SelectedSuites, amongSuites)};

    const bool leftOut{excluded || ((isPerformanceTest == is_performance_test::yes) && excludes_performance_tests())};
    if(leftOut)
      m_TestsLeftOut.push_back(source.path());

    return !leftOut && (!selects() || selectedBySource || selectedBySuite);
  }

  //===================================== path_equivalence =====================================//

  [[nodiscard]]
  bool test_runner::path_equivalence::operator()(const normal_path& selectedSource, const normal_path& filepath) const
  {
    if(filepath.path().empty() || selectedSource.path().empty() || (back(selectedSource) != back(filepath)))
      return false;

    if(filepath == selectedSource) return true;

    // A selection is typed at the command line, from a directory this code cannot know.
    // Rebasing both paths onto the test repository compares them on the assumption that
    // the selection names a file beneath the repository. Failing that, a selection which
    // is a bare filename is looked up in the tree.

    if(auto repo{*m_Repo}; !repo.empty())
    {
      if(rebase_from(selectedSource, repo) == rebase_from(filepath, repo))
        return true;

      if(const auto path{find_in_tree(repo, selectedSource)}; !path.empty())
      {
        if(rebase_from(path, repo) == rebase_from(filepath, repo))
          return true;
      }
    }

    return false;
  }

  test_runner::test_runner(int argc,
                           char** argv,
                           std::string copyright,
                           std::string codeIndent,
                           const project_paths::customizer& projectPathsCustomization,
                           std::ostream& stream)
    : m_Copyright{std::move(copyright)}
    , m_ProjPaths{project_paths{argc, argv, projectPathsCustomization}}
    , m_CMakeCache{m_ProjPaths.build()}
    , m_CodeIndent{std::move(codeIndent)}
    , m_Stream{&stream}
  {
    check_indent(m_CodeIndent);

    process_args(argc, argv);

    fs::create_directory(proj_paths().output().dir());
    fs::create_directory(proj_paths().output().diagnostics());
    fs::create_directory(proj_paths().output().test_summaries());
  }

  void test_runner::process_args(int argc, char** argv)
  {
    using namespace parsing::commandline;

    std::vector<nascent_test_vessel> nascentTests{};
    std::vector<project_data> nascentProjects{};

    auto updateCurrentNascentTest{
      [&nascentTests]<class Act>(Act act) requires acts_on_every_alternative_v<Act, nascent_test_vessel> {
        return [&nascentTests, act](const arg_list& args) {
          if(nascentTests.empty())
            throw std::logic_error{"Unable to find nascent test"};

          std::visit([&act, &args](auto& nascent) { act(nascent, args); }, nascentTests.back());
        };
      }
    };

    const option diagnosticsOption{"--framework-diagnostics", {"--diagnostics"}, {},
      updateCurrentNascentTest(
        [](auto& nascent, const arg_list&) {
          nascent.flavour(nascent_test_flavour::framework_diagnostics);
        }
      ),
      {},
      "Make the test one of the framework's own diagnostics"
    };

    const option headerOption{"--header", {}, {"header"},
      updateCurrentNascentTest([](auto& nascent, const arg_list& args) { nascent.header(args[0]); }),
      {},
      "Name the header declaring the class under test"
    };

    const option forenameOption{"--test-class-forename", {"--forename"}, {"forename"},
      updateCurrentNascentTest([](auto& nascent, const arg_list& args) { nascent.forename(args[0]); }),
      {},
      "Name the test class <forename>_test rather than after the header"
    };

    const option fullnameOption{"--fullname", {}, {"name"},
      updateCurrentNascentTest([](auto& nascent, const arg_list& args) { nascent.full_name(args[0]); }),
      {},
      "Name the test class <name> exactly, and its files after it"
    };

    const option testingUtilitiesOption{"--testing-utilities", {}, {"header"},
      updateCurrentNascentTest([](auto& nascent, const arg_list& args) { nascent.testing_utilities(args[0]); }),
      {},
      "Take the value_tester from an existing header within Tests; a regular or move-only test then "
      "generates neither testing utilities nor false-negative diagnostics"
    };

    using src_opt = nascent_test_base::gen_source_option;

    const option genFreeSourceOption{"--gen-source", {"-g"}, {"namespace"},
      updateCurrentNascentTest(
        overloaded{
          [](nascent_behavioural_test& nascent, const arg_list& args) {
            nascent.generate_source_files(src_opt::yes);
            if(args[0] != "::")
              nascent.set_namespace(args[0]);
          },
          [](auto&, const arg_list&) {}
        }
      ),
      {},
      "Generate a source file too, in <namespace> (:: for the global one)"
    };

    const option genClassSourceOption{"--gen-source", {"-g"}, {"dir"},
      updateCurrentNascentTest(
        overloaded{
          [](std::derived_from<nascent_class_test_base> auto& nascent, const arg_list& args) {
            nascent.generate_source_files(src_opt::yes);
            nascent.source_dir(args[0]);
          },
          [](auto&, const arg_list&) {}
        }
      ),
      {},
      "Generate the class's header and source too, under Source/<project>/<dir>"
    };

    const std::initializer_list<maths::tree_initializer<option>>
      classOptions      {{headerOption}, {genClassSourceOption}, {fullnameOption}, {testingUtilitiesOption}},
      performanceOptions{{fullnameOption}},
      freeOptions       {{forenameOption}, {fullnameOption}, {genFreeSourceOption}, {diagnosticsOption}};

    const auto help{
      parse_invoke_depth_first(argc, argv,
                { {{{"test", {"t"}, {"suite"},
                    [this](const arg_list& args) {
                      m_RunnerMode |= runner_mode::test;

                      m_Filter.add_selected_suite(args.front());
                    },
                    {},
                    "Run the tests of a suite: a directory beneath Tests"}
                  }},
                  {{{"select", {"s"}, {"source"},
                    [this](const arg_list& args) {
                      m_RunnerMode |= runner_mode::test;

                      m_Filter.add_selected_item(fs::path{args.front()});
                    },
                    {},
                    "Run the test defined in a source file"}
                  }},
                  {{{"prune", {"p"}, {},
                    [this](const arg_list&) {
                      m_RunnerMode |= runner_mode::test;
                      m_PruneMode = prune_mode::active;
                    },
                    {},
                    "Run the tests affected by changes since the previous run"}
                  }},
                  {{{"create", {"c"}, {},
                        [](const arg_list&) {},
                        [this,&nascentTests](const arg_list&) {
                          if(!nascentTests.empty())
                          {
                            // The first `create` checks that sequoia has not changed since the build. The
                            // executable writes registrations in the form compiled into it.
                            if(!in_mode(runner_mode::create))
                            {
                              throw_if_sequoia_changed_since_build(proj_paths(),
                                                                   sequoia_sources(),
                                                                   stream());
                            }

                            m_RunnerMode |= runner_mode::create;
                            overloaded visitor{ [](auto& nascent) { nascent.finalize(); } };
                            std::visit(visitor, nascentTests.back());
                          }
                        },
                        "Create a test; the command names its kind\n"
                        "A class is spelt with its namespace and template parameters, as in foo::bar<class T>; "
                        "an allocation test takes its bare name"},
                    { {{"regular_test", {"regular"}, {"class", "equivalent_type"},
                        nascent_test_data{"semantic", "regular", *this, nascentTests}, {},
                        "A regular test of the class against an equivalent type"}, classOptions
                      },
                      {{"move_only_test", {"move_only"}, {"class", "equivalent_type"},
                        nascent_test_data{"semantic", "move_only", *this, nascentTests}, {},
                        "A move-only test of the class against an equivalent type"}, classOptions
                      },
                      {{"regular_allocation_test", {"regular_allocation", "allocation_test"}, {"class"},
                        nascent_test_data{"allocation", "regular_allocation", *this, nascentTests}, {},
                        "An allocation test of the regular class"}, classOptions
                      },
                      {{"move_only_allocation_test", {"move_only_allocation"}, {"class"},
                        nascent_test_data{"allocation", "move_only_allocation", *this, nascentTests}, {},
                        "An allocation test of the move-only class"}, classOptions
                      },
                      {{"free_test", {"free"}, {"header"},
                        nascent_test_data{"behavioural", "free", *this, nascentTests}, {},
                        "A test of the free functions the header declares"}, freeOptions
                      },
                      {{"performance_test", {"performance"}, {"header"},
                         nascent_test_data{"behavioural", "performance", *this, nascentTests}, {},
                         "A performance test of what the header declares"}, performanceOptions
                      }
                    }
                  }},
                  {{{"init", {"i"}, {"owner", "path", "indent"},
                    [this,&nascentProjects](const arg_list& args) {
                      m_RunnerMode |= runner_mode::init;

                      const auto ind{
                        [](std::string arg) {

                          replace_all(arg, "\\t", "\t");
                          return indentation{arg};
                        }
                      };

                      nascentProjects.push_back(project_data{args[0], args[1], ind(args[2])});
                    },
                    {},
                    "Create a project at the path, named after its last directory"},
                    { {{"--no-build", {}, {},
                        [&nascentProjects](const arg_list&) { nascentProjects.back().do_build = build_invocation::no; },
                        {},
                        "Do not build the new project"}},
                      {{"--no-git", {}, {},
                        [&nascentProjects](const arg_list&) { nascentProjects.back().use_git = git_invocation::no; },
                        {},
                        "Do not put the new project under git"}},
                      {{"--to-files",  {}, {"path"},
                        [&nascentProjects](const arg_list& args) { nascentProjects.back().output = args[0]; },
                        {},
                        "Send the output of git and the build to the file at the path"}},
                      {{"--no-ide", {}, {},
                        [&nascentProjects](const arg_list&) {
                          auto& build{nascentProjects.back().do_build};
                          if(build == build_invocation::launch_ide) build = build_invocation::yes;
                        },
                        {},
                        "Build the new project without opening it in an IDE"}}
                    }
                  }},
                  {{{"update-materials", {"u"}, {},
                    [this](const arg_list&) {
                      m_RunnerMode |= runner_mode::test;
                      m_UpdateMode = update_mode::soft;
                    },
                    {},
                    "Run the tests, updating the predictions of tests that fail without throwing"
                  }}},
                  {{{"locate-instabilities", {"locate"}, {"repetitions"},
                    [this](const arg_list& args) {
                      using parsing::commandline::error;
                      const int i{
                        [arg{args.front()}] (){
                          try
                          {
                            return std::stoi(arg);
                          }
                          catch(const std::exception&)
                          {
                            throw std::runtime_error{std::format("locate-instabilities: unable to interpret '{}' as an integer number of repetitions", arg)};
                          }
                        }()
                      };

                      if(i < 2)
                        throw std::runtime_error{error("Number of repetitions, must be >= 2")};

                      m_InstabilityMode = instability_mode::single_instance;
                      m_NumReps = i;
                    },
                    {},
                    "Run the tests repeatedly, reporting the checks whose outcome varies"},
                    { {{"--sandbox", {}, {},
                        [this](const arg_list&) {
                          m_InstabilityMode = instability_mode::coordinator;
                        },
                        {},
                        "Run each repetition in a process of its own"}},
                      {{"--runner-id", {}, {"id"},
                        [this](const arg_list& args) {
                          m_RunnerID = std::stoi(args.front());
                          m_InstabilityMode = instability_mode::sandbox;
                        },
                        {},
                        "Mark this run as a sandboxed repetition; not for use by hand"}}
                    }
                  }},
                  {{{"recover", {}, {},
                    [this, recovery{proj_paths().output().recovery()}](const arg_list&) {
                      if(!fs::create_directories(recovery.dir()))
                      {
                        fs::remove(recovery.recovery_file());
                      }
                      m_RecoveryMode |= recovery_mode::recovery;
                      if(m_ConcurrencyMode == concurrency_mode::dynamic)
                        m_ConcurrencyMode = concurrency_mode::serial;
                    },
                    {},
                    "Run serially, recording each check before it runs, to find a crash"
                  }}},
                  {{{"dump", {}, {},
                    [this, recovery{proj_paths().output().recovery()}](const arg_list&) {
                      fs::create_directories(recovery.dir());
                      write_to_file(recovery.dump_file(), "", std::ios_base::out);
                      m_RecoveryMode |= recovery_mode::dump;
                      if(m_ConcurrencyMode == concurrency_mode::dynamic)
                        m_ConcurrencyMode = concurrency_mode::serial;
                    },
                    {},
                    "Run serially, recording every check, for comparing two runs"},
                    { {{"--as", {}, {"name"},
                        [this](const arg_list& args) { m_KeepDumpAs = dump_name(args.front()); },
                        {},
                        "Keep the dump under a name, to compare a later run against"}},
                      {{"--against", {}, {"name"},
                        [this](const arg_list& args) { m_CompareDumpAgainst = dump_name(args.front()); },
                        {},
                        "Compare the run's checks with the dump kept under the name"}}
                    }
                  }},
                  {{{"--check-versioned-output", {}, {},
                    [this, drift{proj_paths().output().drift()}](const arg_list&) {
                      fs::create_directories(drift.dir());
                      fs::remove(drift.patch_file());
                      m_VersionedOutputMode = versioned_output_mode::checked;
                    },
                    {},
                    "Fail the run if it changes anything versioned under output"
                  }}},
                  {{{"--exclude-performance", {}, {},
                    [this](const arg_list&) { m_Filter.exclude_performance_tests(); },
                    {},
                    "Leave out the performance tests"
                  }}},
                  {{{"exclude", {"e"}, {"source"},
                    [this](const arg_list& args) { m_Filter.exclude_item(normal_path{args.front()}); },
                    {},
                    "Leave out the test defined in a source file"
                  }}},
                  {{{"--serial",  {}, {},
                    [this](const arg_list&) { m_ConcurrencyMode = concurrency_mode::serial; },
                    {},
                    "Run the tests on one thread"
                  }}},
                  {{{"--thread-pool", {}, {"threads"},
                    [this](const arg_list& args) {
                      if(const auto num{std::stoi(args.front())}; num > 0)
                      {
                        m_ConcurrencyMode = concurrency_mode::fixed;
                        m_PoolSize = num;
                      }
                      else
                      {
                        stream() << warning("Thread pool size must be positive\n");
                      }
                    },
                    {},
                    "Run the tests on a pool of the given number of threads"
                  }}},
                  {{{"--verbose",  {"-v"}, {},
                    [this](const arg_list&) { m_Verbosity = verbosity::verbose; },
                    {},
                    "Print the suites and their tests as a tree"
                  }}}
                },
                [](std::string_view){})
        };

    // A help request is not a mode: nothing was asked for but the text, and execute() has nothing to do
    if(!help.empty())
    {
      stream() << help;
    }
    else
    {
      if(m_RunnerMode == runner_mode::none)
        m_RunnerMode = runner_mode::test;

      check_argument_consistency();

      if(in_mode(runner_mode::create))
        stream() << '\n' << cmake_nascent_tests(proj_paths());
  
      if(in_mode(runner_mode::init))
        init_projects(proj_paths(), nascentProjects, stream());

      if(in_mode(runner_mode::test))
      {
        if((m_InstabilityMode == instability_mode::single_instance) || (m_InstabilityMode == instability_mode::coordinator))
        {
          setup_instability_analysis_prune_folder(proj_paths());
        }

        // Note: this needs to be done here, before test families are added
        prune();
      }
    }
  }

  void test_runner::check_argument_consistency()
  {
    using parsing::commandline::warning;
    using parsing::commandline::error;

    if((m_InstabilityMode != instability_mode::none) && (m_UpdateMode == update_mode::soft))
    {
      m_UpdateMode = update_mode::none;
      stream() << warning("Update of materials suppressed when checking for instabilities\n");
    }

    if((m_ConcurrencyMode != concurrency_mode::serial) && (m_RecoveryMode != recovery_mode::none))
      throw std::runtime_error{error("Can't run asynchronously in recovery/dump mode\n")};

    if((m_PruneMode == prune_mode::active) && m_Filter.selects())
    {
      m_PruneMode = prune_mode::passive;
      stream() << warning("'prune' ignored when tests are selected\n");
    }
  }

  void test_runner::check_for_coarse_sleeps()
  {
    auto warnIfCoarse{
      [this]() {
        constexpr std::chrono::milliseconds target{5};
        if(const auto typicalSleep{typical_sleep_duration(target)}; is_coarse_sleep(typicalSleep, target))
          stream() << coarse_sleep_message(typicalSleep, target) << std::flush;
      }
    };

    // The timer resolution belongs to the process, so the first runner's check serves every later runner in it
    static std::once_flag checked{};
    std::call_once(checked, warnIfCoarse);
  }

  void test_runner::check_for_missing_tests()
  {
    if(m_PruneMode == prune_mode::passive)
    {
      if(const auto suites{m_Filter.selected_suites()})
      {
        auto hint{
          [](const std::string& name) -> std::string {
            return (name.rfind('.') < std::string::npos) ? "    If trying to select a source file use 'select' rather than 'test'\n" : "";
          }
        };

        report_unmatched(stream(), std::span{*suites}, "Test Suite", hint);
      }

      if(const auto sources{m_Filter.selected_items()})
      {
        auto hint{
          [](const fs::path& p) -> std::string {
            return p.has_extension() ? "" : "    If trying to test a suite use 'test' rather than 'select'\n";
          }
        };

        report_unmatched(stream(), std::span{*sources}, "Test File", hint);
      }
    }

    auto hint{
      [](const fs::path& p) -> std::string {
        return p.has_extension() ? "" : "    'exclude' takes the source file of a test\n";
      }
    };

    const auto excluded{m_Filter.excluded_items()};
    report_unmatched(stream(), std::span{excluded}, "Excluded Test File", hint);
  }

  return_code test_runner::execute()
  {
    if(!in_mode(runner_mode::test))
      return return_code::success;

    const scoped_terminate_handler terminationReported{report_termination};
    const debug_report_redirector debugReportRedirector{};
    const windows_crash_report_enabler windowsCrashReportEnabler{};
    set_finest_windows_timer_resolution();

    fs::create_directories(proj_paths().prune().dir());
    build_suite_tree();
    check_for_missing_tests();

    // A run with nothing to do still has a dump, an empty one, to compare or keep
    const auto code{nothing_to_do() ? return_code::success : run()};

    compare_dump();
    keep_dump();

    return code;
  }

  return_code test_runner::run()
  {
    if(m_InstabilityMode != instability_mode::sandbox)
    {
      fs::remove_all(proj_paths().output().instability_analysis());
      overwrite_quietly(proj_paths().execution_records().stamp(), started_at(std::chrono::system_clock::now()));
      check_for_coarse_sleeps();
    }

    const auto baseline{versioned_output_baseline()};
    const auto code{  m_InstabilityMode == instability_mode::coordinator
                    ? run_tests_in_sandboxes()
                    : run_tests_in_this_process()};

    if(   (m_InstabilityMode == instability_mode::single_instance)
       || (m_InstabilityMode == instability_mode::coordinator))
    {
      aggregate_instability_analysis_prune_files(proj_paths(), m_PruneMode, entry_time_stamp, m_NumReps);
      stream() << instability_analysis(proj_paths().output().instability_analysis(), m_NumReps);
    }

    return code | report_versioned_output_changes(baseline);
  }

  [[nodiscard]]
  std::string test_runner::dump_name(std::string name)
  {
    if(name.empty())
      throw std::runtime_error{parsing::commandline::error("a dump is kept under, and compared against, a name; none was given")};

    return name;
  }

  // Comparison comes before keeping, so that one run may compare against a name and then take it
  void test_runner::compare_dump()
  {
    if(m_CompareDumpAgainst.empty())
      return;

    const auto recovery{proj_paths().output().recovery()};
    const auto kept{recovery.kept_dump(m_CompareDumpAgainst)};
    if(!fs::exists(kept))
    {
      using parsing::commandline::error;
      throw std::runtime_error{
        error(std::format("no dump has been kept as '{}': expected {}", m_CompareDumpAgainst, kept.generic_string()))
      };
    }

    stream() << '\n' << to_string(compare_dumps(kept, recovery.dump_file()), m_CompareDumpAgainst);
  }

  void test_runner::keep_dump()
  {
    if(m_KeepDumpAs.empty())
      return;

    const auto recovery{proj_paths().output().recovery()};
    const auto kept{recovery.kept_dump(m_KeepDumpAs)};
    fs::create_directories(kept.parent_path());
    fs::copy_file(recovery.dump_file(), kept, fs::copy_options::overwrite_existing);
  }

  [[nodiscard]]
  std::string test_runner::selection_options() const
  {
    std::string options{};

    if(auto items{m_Filter.selected_items()})
    {
      for(const auto& [file, found] : *items)
      {
        if(found)
          options += std::format(" select {}", runtime::quote_for_shell(file.path().generic_string()));
      }
    }

    if(auto suites{m_Filter.selected_suites()})
    {
      for(const auto& [name, found] : *suites)
      {
        if(found)
          options += std::format(" test {}", runtime::quote_for_shell(name));
      }
    }

    for(const auto& [file, found] : m_Filter.excluded_items())
    {
      if(found)
        options += std::format(" exclude {}", runtime::quote_for_shell(file.path().generic_string()));
    }

    if(m_Filter.excludes_performance_tests())
      options += " --exclude-performance";

    return options;
  }

  [[nodiscard]]
  return_code test_runner::run_tests_in_sandboxes()
  {
    if(proj_paths().executable().empty())
      throw std::runtime_error{"Unable to run in sandbox mode, as executable cannot be found"};

    const auto selection{selection_options()},
               async{to_async_option(m_ConcurrencyMode, m_PoolSize)};

    auto code{return_code::success};
    for(std::size_t i{}; i < m_NumReps; ++i)
    {
      const auto command{std::format("{} locate {} --runner-id {}{}{}",
                                     runtime::quote_for_shell(proj_paths().executable().string()),
                                     m_NumReps,
                                     i,
                                     selection,
                                     async)};

      code |= child_return_code(invoke(runtime::shell_command{command}),
                                std::format("Sandbox run {}, {},", i, command));
    }

    return code;
  }

  [[nodiscard]]
  return_code test_runner::run_tests_in_this_process()
  {
    if(concurrent_execution()) sort_tests();

    if(m_InstabilityMode == instability_mode::sandbox)
      return run_tests(m_RunnerID);

    auto code{return_code::success};
    for(std::size_t i{}; i < m_NumReps; ++i)
    {
      if(i) reset_tests();

      const auto optIndex{m_NumReps > 1 ? std::optional<std::size_t>{i} : std::nullopt};
      code |= run_tests(optIndex);
    }

    return code;
  }

  [[nodiscard]]
  std::optional<versioned_output_snapshot> test_runner::versioned_output_baseline() const
  {
    if(m_VersionedOutputMode != versioned_output_mode::checked) return std::nullopt;

    return take_versioned_output_snapshot(proj_paths().output());
  }

  [[nodiscard]]
  return_code test_runner::report_versioned_output_changes(const std::optional<versioned_output_snapshot>& baseline)
  {
    if(!baseline) return return_code::success;

    const auto after{take_versioned_output_snapshot(proj_paths().output())};
    const auto differences{compare_versioned_output(*baseline, after)};

    if(differences.empty())
    {
      stream() << "\nVersioned output written by this run matches what was on disk.\n";
      return return_code::success;
    }

    // The patch is to be applied from the project root, so its paths are relative to there
    const auto outputDir{fs::relative(proj_paths().output().dir(), proj_paths().project_root())};
    write_to_file(proj_paths().output().drift().patch_file(), unified_diff(*baseline, after, outputDir), std::ios_base::out | std::ios_base::binary);

    stream() << "\nVersioned output written by this run differs from what was on disk:\n" << to_string(differences)
             << "A patch from what was on disk to what this run wrote: " << drift_paths{outputDir}.patch_file().generic_string() << "\n";

    return return_code::versioned_output_diffs;
  }

  [[nodiscard]]
  const log_summary& test_runner::root_summary() const
  {
    return m_Suites.cbegin_node_weights()->summary;
  }

  void test_runner::sort_tests()
  {
    // TO DO: replace this with a stable_sort
    m_Suites.sort_nodes(1, m_Suites.order(), [&s = m_Suites](auto i, auto j) {
      auto& lhs{s.cbegin_node_weights()[i]};
      auto& rhs{s.cbegin_node_weights()[j]};

      if(!lhs.optTest && !rhs.optTest) return i < j;

      if(!lhs.optTest && rhs.optTest) return true;
      if(lhs.optTest && !rhs.optTest) return false;

      if(!lhs.optTest->parallelizable() && rhs.optTest->parallelizable()) return true;
      if(lhs.optTest->parallelizable() && !rhs.optTest->parallelizable()) return false;

      return i < j;
      });
  }

  return_code test_runner::run_tests(const std::optional<std::size_t> id)
  {
    const timer t{};

    // Without the flush, a run killed while the tests are silent would never show that they had begun
    stream() << running_tests_message(m_ConcurrencyMode) << std::flush;

    const auto concurrentDurations{execute_tests(id)};
    const auto removalFailures{extract_discarded_materials_removal_failures()};

    test_tracker tracker{proj_paths(), id, m_Filter.selects() ? is_filtered::yes : is_filtered::no, m_Filter.tests_left_out()};

    using namespace maths;
    auto summarizeAndTrack{
      [this,&tracker](auto n) {
        m_Suites.mutate_node_weight(
          std::ranges::next(m_Suites.cbegin_node_weights(), n),
          [this, &tracker,n](suite_node& wt) {
            for(const auto& edge : m_Suites.cedges(n))
              wt.summary += std::ranges::next(m_Suites.cbegin_node_weights(), edge.target_node())->summary;

            if(wt.optTest)
            {
              auto pathsMaker{
                  [this](const test_to_run& test) -> test_paths {
                    return {test.source_file(),
                            test.summary_file_path(),
                            test.materials_paths(),
                            proj_paths()};
                  }
              };

              tracker.process_test(pathsMaker(wt.optTest.value()), wt.summary, m_UpdateMode);
            }
          }
        );
      }
    };

    traverse(depth_first, m_Suites, find_disconnected_t{}, null_func_obj{}, summarizeAndTrack, null_func_obj{});
    tracker.update_materials_and_prune_info();

    report_results();

    if(concurrentDurations)
    {
      auto& rootSummary{m_Suites.begin_node_weights()->summary};
      rootSummary.execution_duration(concurrentDurations->execution_duration);
      rootSummary.runner_overhead(concurrentDurations->runner_overhead);
    }

    stream() << "\n-----------Grand Totals-----------\n";
    stream() << summarize(root_summary(), "", t.time_elapsed(), summary_detail::absent_checks | summary_detail::timings, indentation{"\t"}, no_indent);

    if(const auto materialsUpdateReport{tracker.materials_update_report()}; !materialsUpdateReport.empty())
    {
      stream() << "\n-----------Materials Update-----------\n";
      for(const auto& entry : materialsUpdateReport)
      {
        stream() << sequoia::indent(entry, indentation{"\t"}) << "\n\n";
      }
    }

    // Not folded into the totals, which count what the tests found: these are failures of the run itself
    const auto postRunFailures{
      post_run_failure_messages(tracker.post_run_failures(),
                                removalFailures,
                                proj_paths().project_root())
    };
    if(!postRunFailures.empty())
    {
      stream() << "\n-----------Post-Run Failures-----------\n";
      for(const auto& failure : postRunFailures)
      {
        stream() << sequoia::indent(failure, indentation{"\t"}) << "\n\n";
      }
    }

    return to_return_code(root_summary()) | (postRunFailures.empty() ? return_code::success : return_code::post_run_failures);
  }

  [[nodiscard]]
  std::optional<test_runner::run_durations> test_runner::execute_tests(const std::optional<std::size_t> id)
  {
    discarded_materials_remover remover{};
    if(concurrent_execution())
      return execute_concurrently(id, remover);

    execute_serially(id, remover);
    return std::nullopt;
  }

  [[nodiscard]]
  test_runner::run_durations test_runner::execute_concurrently(const std::optional<std::size_t> id,
                                                               discarded_materials_remover& remover)
  {
    auto first{std::ranges::find_if(m_Suites.begin_node_weights(), m_Suites.end_node_weights(), [](const auto& wt) -> bool { return wt.optTest != std::nullopt; })};
    auto next{std::ranges::find_if(first, m_Suites.end_node_weights(), [](const auto& wt) -> bool { return wt.optTest->parallelizable(); })};

    auto executor{
      [id, &remover](suite_node& wt){
        wt.summary             = wt.optTest->execute(id, remover);
        wt.executing_thread_id = std::this_thread::get_id();
      }
    };

    std::span nonParallelizable{first, next}, parallelizable{next, m_Suites.end_node_weights()};

    const timer asyncTimer{};
    std::ranges::for_each(nonParallelizable, executor);

    switch(m_ConcurrencyMode)
    {
      using enum concurrency_mode;
    case dynamic:
      accelerate(sequoia::execution::par, parallelizable, executor);
      break;
    case fixed:
      accelerate(thread_pool_policy{.num{m_PoolSize}}, parallelizable, executor);
      break;
    default:
      throw std::logic_error{"Unexpected concurrency_mode"};
    }

    const auto wallClock{asyncTimer.time_elapsed()};

    auto executionInfoOf{
      [](const suite_node& wt){
        return execution_info{.thread_id{wt.executing_thread_id}, .duration{wt.summary.execution_duration()}};
      }
    };

    auto executionInfos{
      [executionInfoOf](std::span<const suite_node> nodes){
        return nodes | std::views::transform(executionInfoOf) | std::ranges::to<std::vector>();
      }
    };

    // The longest the tests ran one after another: those which are not parallelizable, then the busiest thread's
    const auto executionDuration{
        summed_duration(executionInfos(nonParallelizable))
      + busiest_thread_execution_duration(executionInfos(parallelizable))
    };
    return run_durations{
      .execution_duration{executionDuration},
      .runner_overhead{wallClock - executionDuration}
    };
  }

  void test_runner::execute_serially(const std::optional<std::size_t> id, discarded_materials_remover& remover)
  {
    using namespace maths;
    auto execute{
      [&s = m_Suites, id, &remover](auto n) {
        auto& wt{s.begin_node_weights()[n]};
        if(wt.optTest)
          wt.summary = wt.optTest->execute(id, remover);
      }
    };

    traverse(depth_first, m_Suites, find_disconnected_t{}, execute, null_func_obj{}, null_func_obj{});
  }

  void test_runner::report_results()
  {
    using namespace maths;
    if(m_Verbosity == verbosity::verbose)
    {
      // One step per level of the tree, a test's report being a level below the test. Spaces
      // rather than a tab: a tab advances to the next tab stop instead of by a fixed amount, so
      // following the spaces of the levels above it, the gap between a test's name and its report
      // would vary both with depth and with whatever renders the output.
      const indentation step{"    "};
      indentation suiteIndent{no_indent};
      auto printNode{
        [&s = m_Suites, &suiteIndent, &step, &stream = stream(), serial{!concurrent_execution()}](auto n) {
          if(n)
          {
            const auto& wt{s.cbegin_node_weights()[n]};
            if(wt.optTest)
            {
              stream << summarize(wt.summary,
                                  ":",
                                  summary_detail::failure_messages | summary_detail::timings,
                                  suiteIndent,
                                  step);
            }
            else
            {
              const auto heading{sequoia::indent(wt.summary.name() + ":", suiteIndent)};
              stream << (serial ? append_indented(heading, report_time(wt.summary, std::nullopt), suiteIndent)
                                : heading + '\n');
            }

            suiteIndent.append(std::string{step});
          }
        }
      };

      auto decreaseIndent{
        [&suiteIndent, &step](auto n) {
          if(n)
            suiteIndent.trim(std::string_view{step}.size());
        }
      };

      traverse(depth_first, m_Suites, find_disconnected_t{}, printNode, decreaseIndent, null_func_obj{});
    }
    else
    {
      const auto detail{!concurrent_execution() ? summary_detail::failure_messages | summary_detail::timings : summary_detail::failure_messages};

      // Depth-first, so the tests are reported in the order they sit in the tree rather than in
      // whichever order the concurrency sort left them.
      auto printTest{
        [&s = m_Suites, detail, &stream = stream()](auto n) {
          if(const auto& wt{s.cbegin_node_weights()[n]}; wt.optTest)
            stream << summarize(wt.summary, ":", detail, no_indent, tab);
        }
      };

      traverse(depth_first, m_Suites, find_disconnected_t{}, printTest, null_func_obj{}, null_func_obj{});
    }
  }

  [[nodiscard]]
  std::vector<removal_failure> test_runner::extract_discarded_materials_removal_failures()
  {
    auto holdsTest{[](const suite_node& node) { return node.optTest.has_value(); }};

    auto failureOf{
      [](suite_node& node) -> std::vector<removal_failure> {
        if(auto failure{node.optTest->extract_discarded_materials_removal_failure()})
          return {std::move(*failure)};

        return {};
      }
    };

    const auto failures{
        m_Suites.node_weights()
      | std::views::filter(holdsTest)
      | std::views::transform(failureOf)
      | std::views::join
      | std::ranges::to<std::vector>()
    };

    return failures;
  }

  [[nodiscard]]
  bool test_runner::nothing_to_do()
  {
    if(!m_Registered)
    {
      stream() << "Nothing to do: try creating some tests!\nRun with --help to see options\n";
      return true;
    }
    else if(m_Suites.order() <= 1)
    {
      if(m_PruneMode == prune_mode::active)
        stream() << "Nothing to do: no changes since the last run, therefore 'prune' has pruned all tests\n";

      return true;
    }

    return false;
  }

  void test_runner::reset_tests()
  {
    using namespace maths;

    std::string suiteName{};

    auto resetFn{
      [&,this](auto n){
        m_Suites.mutate_node_weight(std::ranges::next(m_Suites.cbegin_node_weights(), n),
          [&,this](auto& wt){
            if(wt.optTest)
            {
              wt.optTest->reset_results();
            }
            else
            {
              suiteName = wt.summary.name();
            }

            wt.summary = log_summary{wt.summary.name()};
          }
        );
      }
    };

    traverse(depth_first, m_Suites, find_disconnected_t{}, resetFn, null_func_obj{}, null_func_obj{});
  }

  void test_runner::prune()
  {
    if(m_PruneMode == prune_mode::passive) return;

    stream() << "\nAnalyzing dependencies...\n" << std::flush;
    const timer t{};

    if(const auto fallback{do_prune()})
    {
      using parsing::commandline::warning;
      switch(*fallback)
      {
      case prune_fallback_reason::no_previous_stamp:
        stream() << warning({"Time stamp of previous run does not exist, so unable to prune.",
                            "This should be automatically rectified for the next successful run.",
                            "No action required."});
        break;
      case prune_fallback_reason::toolchain_changed:
        stream() << warning({"The toolchain has changed since the previous run, so prune is ignored and every test runs.",
                            "No action required."});
        break;
      }
    }
    else
    {
      const auto [dur, unit] {testing::stringify_duration(t.time_elapsed())};
      stream() << "[" << dur << unit << "]\n\n";
    }
  }

  [[nodiscard]]
  std::optional<prune_fallback_reason> test_runner::do_prune()
  {
    const auto selection{tests_to_run(proj_paths())};
    if(const auto* tests{std::get_if<std::vector<fs::path>>(&selection)})
    {
      for(const auto& src : *tests)
      {
        m_Filter.add_selected_item(src);
      }

      if(!m_Filter.selects())
        m_Filter.select_nothing();

      return std::nullopt;
    }

    m_PruneMode = prune_mode::passive;

    return std::get<prune_fallback_reason>(selection);
  }

  [[nodiscard]]
  std::vector<std::string> test_runner::enclosing_suites(const fs::path& source) const
  {
    return rebase_from(source, proj_paths().tests().repo()).parent_path()
         | std::views::drop_while([](const fs::path& component){ return component == ".."; })
         | std::views::transform([](const fs::path& component){ return component.generic_string(); })
         | std::ranges::to<std::vector>();
  }

  [[nodiscard]]
  std::string test_runner::duplication_message(std::string_view testName, const fs::path& source)
  {
    using namespace parsing::commandline;

    return error({std::format("Test: \"{}\"", testName),
                  std::format("Source file: \"{}\"", source.generic_string()),
                  "A test's name is that of its class, and determines where its output is written,"
                  " so no two tests may share a name, ignoring case."});
  }

  namespace
  {
    [[nodiscard]]
    bool contains_non_ascii(std::string_view text)
    {
      return !std::ranges::all_of(text, is_ascii);
    }
  }

  /** A test's materials are wiped and synchronized as a whole, so no two tests' materials may nest.
      Each test's materials are its source's materials prefix with the test's name appended. The names
      are distinct single components, ignoring case, so refusing nested prefixes suffices, whatever is
      appended to the prefixes. Case is ignored because the filesystems of macOS and Windows ignore it.
      Beyond ASCII they equate names by rules of their own - case folding, and on macOS Unicode
      normalization - so a name or prefix containing anything non-ASCII is refused.
   */
  void test_runner::throw_if_name_refused(std::string_view name, const fs::path& source) const
  {
    if(contains_non_ascii(name))
      throw std::logic_error{non_ascii_name_message(source)};

    if(m_LowerCaseTestNames.contains(ascii::to_lowercase(name)))
      throw std::logic_error{duplication_message(name, source)};
  }

  void test_runner::throw_if_source_refused(const fs::path& source) const
  {
    const auto prefix{lower_case_materials_prefix(source)};
    if(prefix.empty())
      throw std::logic_error{unplaceable_source_message(source)};

    if(contains_non_ascii(prefix))
      throw std::logic_error{non_ascii_source_message(source)};

    auto isDescendantOf{
      [](std::string_view path, std::string_view ancestor) {
        return (path.size() > ancestor.size()) && path.starts_with(ancestor) && (path[ancestor.size()] == '/');
      }
    };

    auto nestsWithPrefix{
      [&prefix, &isDescendantOf](const auto& admitted) {
        return isDescendantOf(prefix, admitted.first) || isDescendantOf(admitted.first, prefix);
      }
    };

    const auto nestedWith{std::ranges::find_if(m_SourcesByLowerCasePrefix, nestsWithPrefix)};

    if(nestedWith != m_SourcesByLowerCasePrefix.end())
      throw std::logic_error{nesting_message(source, nestedWith->second)};
  }

  void test_runner::throw_if_summary_refused(std::string_view name, const test_summary_path& summary) const
  {
    const auto& file{summary.file_path()};
    const auto registered{m_TestNamesByLowerCaseSummary.find(ascii::to_lowercase(file.generic_string()))};

    if(registered != m_TestNamesByLowerCaseSummary.end())
      throw std::runtime_error{summary_collision_message(registered->second, name, file)};
  }

  void test_runner::register_name(std::string_view name)
  {
    m_LowerCaseTestNames.insert(ascii::to_lowercase(name));
  }

  void test_runner::register_source(const fs::path& source)
  {
    m_SourcesByLowerCasePrefix.try_emplace(lower_case_materials_prefix(source), source);
  }

  void test_runner::register_summary(std::string_view name, const test_summary_path& summary)
  {
    m_TestNamesByLowerCaseSummary.try_emplace(ascii::to_lowercase(summary.file_path().generic_string()), name);
  }

  [[nodiscard]]
  std::string test_runner::lower_case_materials_prefix(const fs::path& source) const
  {
    return ascii::to_lowercase(materials_prefix(source, proj_paths()).lexically_normal().generic_string());
  }

  [[nodiscard]]
  std::string test_runner::nesting_message(const fs::path& source, const fs::path& nestedWith)
  {
    using namespace parsing::commandline;

    return error(std::format("Source file: \"{}\"\n"
                             "Nests with:  \"{}\"\n"
                             "Each test's materials are kept beneath the path of its source file less the extension,"
                             " so, ignoring case, that path must not name a directory"
                             " holding another test's source file.\n",
                             source.generic_string(),
                             nestedWith.generic_string()));
  }

  [[nodiscard]]
  std::string test_runner::non_ascii_name_message(const fs::path& source)
  {
    using namespace parsing::commandline;

    return error(std::format("Source file: \"{}\"\n"
                             "A test's name is that of its class, and the names of the tests in this source file"
                             " must be ASCII.\n",
                             source.generic_string()));
  }

  [[nodiscard]]
  std::string test_runner::non_ascii_source_message(const fs::path& source)
  {
    using namespace parsing::commandline;

    return error(std::format("Source file: \"{}\"\n"
                             "The path of this source file relative to the tests repository must be ASCII.\n",
                             source.generic_string()));
  }

  [[nodiscard]]
  std::string test_runner::unplaceable_source_message(const fs::path& source)
  {
    using namespace parsing::commandline;

    return error(std::format("Source file: \"{}\"\n"
                             "Each test's materials are kept beneath the path of its source file relative to the tests"
                             " repository, and this source file has no such path.\n",
                             source.generic_string()));
  }

  std::string test_runner::summary_collision_message(std::string_view firstTest,
                                                     std::string_view secondTest,
                                                     const fs::path& summaryFile)
  {
    using namespace parsing::commandline;

    return error(std::format("Tests \"{}\" and \"{}\" would write their summaries to one file, ignoring case:\n\"{}\"\n"
                             "Rename one, or change its summary discriminator.\n",
                             firstTest,
                             secondTest,
                             summaryFile.generic_string()));
  }

  void test_runner::build_suite_tree()
  {
    // A runner may be executed more than once. Each execution runs the tests
    // registered since the previous one.
    auto tests{std::exchange(m_Tests, {})};
    m_Suites = suite_type{};
    const auto root{m_Suites.add_node(suite_type::npos)};

    // By name, so that where a registration sits in a main does not decide what the output says.
    std::ranges::sort(tests, {}, [](const test_to_run& t){ return t.name(); });

    const auto findOrAddSuite{
      [this](const suite_node_index enclosingSuiteNode, const std::string& suiteName) {
        const auto children{m_Suites.cedges(enclosingSuiteNode)};

        // A test is a node beside the suites and may share a name with a sibling directory, so
        // only a suite may be descended into.
        const auto existing{
          std::ranges::find_if(children,
                               [this, &suiteName](const auto& edge) {
                                 const auto& weight{m_Suites.cbegin_node_weights()[edge.target_node()]};
                                 return !weight.optTest && (weight.summary.name() == suiteName);
                               })
        };

        return existing != children.end()
             ? existing->target_node()
             : m_Suites.add_node(enclosingSuiteNode, suite_node{.summary{log_summary{suiteName}}});
      }
    };

    for(auto& test : tests)
    {
      const auto enclosingSuiteNode{
        std::ranges::fold_left(enclosing_suites(test.source_file()), root, findOrAddSuite)
      };

      m_Suites.add_node(enclosingSuiteNode,
                        suite_node{.summary{log_summary{test.name()}}, .optTest{std::move(test)}});
    }
  }

  [[nodiscard]]
  active_recovery_files make_active_recovery_paths(recovery_mode mode, const project_paths& projPaths)
  {
    active_recovery_files paths{};
    if((mode & recovery_mode::recovery) == recovery_mode::recovery)
      paths.recovery_file = projPaths.output().recovery().recovery_file();

    if((mode & recovery_mode::dump) == recovery_mode::dump)
      paths.dump_file = projPaths.output().recovery().dump_file();

    return paths;
  }
}
