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

#include "FreeTestCore.hpp"
#include "FileSystem.hpp"
#include "Summary.hpp"

#include <vector>
#include <future>
#include <set>

namespace sequoia::testing
{
  enum class output_mode { none=0, write_files=1, update_materials=2, help=4, verbose=8 };

  [[nodiscard]]
  constexpr output_mode operator|(output_mode lhs, output_mode rhs) noexcept
  {
    return static_cast<output_mode>(static_cast<int>(lhs) | static_cast<int>(rhs));
  }

  constexpr output_mode& operator|=(output_mode& lhs, output_mode rhs) noexcept
  {
    lhs = lhs | rhs;
    return lhs;
  }

  [[nodiscard]]
  constexpr output_mode operator&(output_mode lhs, output_mode rhs) noexcept
  {
    return static_cast<output_mode>(static_cast<int>(lhs) & static_cast<int>(rhs));
  }
  
  constexpr output_mode& operator&=(output_mode& lhs, output_mode rhs) noexcept
  {
    lhs = lhs & rhs;
    return lhs;
  }

  [[nodiscard]]
  constexpr output_mode operator~(output_mode om)
  {
    return static_cast<output_mode>(~static_cast<int>(om));
  }

  [[nodiscard]]
  constexpr output_mode operator^(output_mode lhs, output_mode rhs) noexcept
  {
    return static_cast<output_mode>(static_cast<int>(lhs) ^ static_cast<int>(rhs));
  }

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
                Tests&&... tests)
      : m_Name{name}
      , m_TestRepo{std::move(testRepo)}
      , m_TestMaterialsRepo{std::move(testMaterialsRepo)}
      , m_OutputDir{std::move(outputDir)}
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
    test_family& operator=(test_family&&) noexcept = delete;

    template<class concrete_test>
    void add_test(concrete_test&& test)
    {
      m_Tests.emplace_back(std::make_unique<concrete_test>(std::forward<concrete_test>(test)));
      set_materials(*m_Tests.back());      
    }

    [[nodiscard]]
    auto execute(const output_mode outputMode, const concurrency_mode concurrenyMode) -> results ;

    [[nodiscard]]
    std::string_view name() const noexcept { return m_Name; }

    [[nodiscard]]
    bool empty() const noexcept { return m_Tests.empty(); }
  private:
    std::vector<std::unique_ptr<test>> m_Tests{};
    std::string m_Name{};
    std::filesystem::path m_TestRepo{}, m_TestMaterialsRepo{}, m_OutputDir{};
    std::vector<std::filesystem::path> m_MaterialsPaths{};

    [[nodiscard]]
    std::filesystem::path diagnostics_filename() const;

    [[nodiscard]]
    static std::filesystem::path test_summary_filename(const test& t, output_mode outputMode, const std::filesystem::path& outputDir, const std::filesystem::path& testRepo);

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
      
      paths(const test& t, output_mode outputMode, const std::filesystem::path& outputDir, const std::filesystem::path& testRepo);

      output_mode mode{output_mode::write_files};
      std::filesystem::path summary, workingMaterials, predictions;
    };
  };  

  [[nodiscard]]
  std::string summarize(const test_family::summary& summary, summary_detail suppression, indentation ind_0, indentation ind_1);
}
