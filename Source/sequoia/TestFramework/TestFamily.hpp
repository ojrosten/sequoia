////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities for grouping tests into families.

 */

#include "sequoia/TestFramework/FreeTestCore.hpp"
#include "sequoia/TestFramework/FileSystem.hpp"
#include "sequoia/TestFramework/Summary.hpp"

#include <vector>
#include <future>
#include <set>

namespace sequoia::testing
{
  enum class output_mode { standard=0, help=1, verbose=2 };
  enum class update_mode { none=0, soft};

  template<>
  struct as_bitmask<output_mode> : std::true_type
  {};

  /*! \brief Allows tests to be grouped together into a family of related tests

      When tests are executed, it is possible to specify both the concurrency mode
      and whether or not output files (which should generally be version controlled)
      are generated.
   */
  class test_family
  {
  public:
    struct results
    {
      log_summary::duration execution_time{};
      std::vector<log_summary> logs;
    };

    struct summary
    {
      explicit summary(log_summary::duration t) : execution_time{t}
      {}

      log_summary::duration execution_time{};
      log_summary log{};
    };

    template<concrete_test... Tests>
    test_family(std::string_view name,
                std::filesystem::path testRepo,
                std::filesystem::path testMaterialsRepo,
                std::filesystem::path outputDir,
                recovery_paths recovery,
                Tests&&... tests)
      : m_Name{name}
      , m_TestRepo{std::move(testRepo)}
      , m_TestMaterialsRepo{std::move(testMaterialsRepo)}
      , m_OutputDir{std::move(outputDir)}
      , m_Recovery{std::move(recovery)}
    {
      if constexpr(sizeof...(Tests) > 0)
      {
        m_Tests.reserve(sizeof...(tests));
        register_tests(std::forward<Tests>(tests)...);
      }
    }

    test_family(const test_family&)     = delete;
    test_family(test_family&&) noexcept = default;

    test_family& operator=(const test_family&)     = delete;
    test_family& operator=(test_family&&) noexcept = default;

    template<class concrete_test>
    void add_test(concrete_test&& test)
    {
      m_Tests.emplace_back(std::forward<concrete_test>(test));
      set_materials(*m_Tests.back());
    }

    [[nodiscard]]
    auto execute(update_mode updateMode, concurrency_mode concurrenyMode) -> results ;

    [[nodiscard]]
    const std::string& name() const noexcept { return m_Name; }

    [[nodiscard]]
    bool empty() const noexcept { return m_Tests.empty(); }
  private:
    std::vector<test_vessel> m_Tests{};
    std::string m_Name{};
    std::filesystem::path m_TestRepo{}, m_TestMaterialsRepo{}, m_OutputDir{};
    recovery_paths m_Recovery;
    std::vector<std::filesystem::path> m_MaterialsPaths{};

    [[nodiscard]]
    static std::filesystem::path test_summary_filename(const test& t, const std::filesystem::path& outputDir, const std::filesystem::path& testRepo);

    template<class Test, class... Tests>
    void register_tests(Test&& t, Tests&&... tests)
    {
      add_test(std::forward<Test>(t));

      if constexpr (sizeof...(Tests) > 0)
        register_tests(std::forward<Tests>(tests)...);
    }

    void set_materials(test& t);

    class summary_writer
    {
    public:
      void to_file(const std::filesystem::path& filename, const log_summary& summary);
    private:
      std::set<std::filesystem::path> m_Record{};
    };

    struct paths
    {
    #ifdef _MSC_VER
      // TO DO: remove once there's a Workaround for msvc bug which manifests when
      // a type lacking a default constructor is used in a std::future.
      // https://developercommunity.visualstudio.com/content/problem/60897/c-shared-state-futuresstate-default-constructs-the.html
      paths() = default;
    #endif

      paths(const test& t, update_mode updateMode, const std::filesystem::path& outputDir, const std::filesystem::path& testRepo);

      update_mode mode{update_mode::none};
      std::filesystem::path summary, workingMaterials, predictions;
    };
  };

  [[nodiscard]]
  std::string summarize(const test_family::summary& summary, summary_detail suppression, indentation ind_0, indentation ind_1);
}
