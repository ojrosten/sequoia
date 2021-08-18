////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/MaterialsUpdater.hpp"

#include "sequoia/TestFramework/CoreInfrastructure.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"
#include "sequoia/TestFramework/FileSystem.hpp"

namespace sequoia::testing
{
  namespace fs = std::filesystem;


  void copy_special_files_to_working_copy(const fs::path& predictions, const fs::path& working)
  {
    for(auto& p : fs::recursive_directory_iterator(predictions))
    {
      if(fs::is_regular_file(p) && ((p.path().extension() == seqpat) || (p.path().filename() == ".keep")))
      {
        const auto predRelDir{fs::relative(p.path().parent_path(), predictions)};
        const auto workingSubdir{working / predRelDir};

        if(p.path().extension() == seqpat)
        {
          for(auto& w : fs::directory_iterator(workingSubdir))
          {
            if((w.path().stem() == p.path().stem()) && (w.path().extension() != seqpat))
            {
              fs::copy(p, workingSubdir, fs::copy_options::overwrite_existing);
              break;
            }
          }
        }
        else if(p.path().filename() == ".keep")
        {
          fs::copy(p, workingSubdir, fs::copy_options::overwrite_existing);
        }
      }
    }
  }

  namespace
  {
    [[nodiscard]]
    std::vector<fs::path> sort_dir_entries(const fs::path& dir)
    {
      std::vector<fs::path> paths{};
      for(auto& entry : fs::directory_iterator(dir))
      {
        paths.push_back(entry.path());
      }

      std::sort(paths.begin(), paths.end());
      return paths;
    }

    using paths_iter = std::vector<fs::path>::const_iterator;

    void soft_update(paths_iter workingBegin, paths_iter workingEnd, paths_iter predictionBegin, paths_iter predictionEnd, const fs::path& predictionDir)
    {
      auto compare{[](const fs::path& lhs, const fs::path& rhs) {
        using type = std::underlying_type_t<fs::file_type>;

        return (lhs == rhs) ? static_cast<type>(fs::status(lhs).type()) < static_cast<type>(fs::status(rhs).type()) : lhs < rhs;
      }};

      auto iters{std::mismatch(workingBegin, workingEnd, predictionBegin, predictionBegin, compare)};
      for(auto wi{workingBegin}, pi{predictionBegin}; wi != iters.first; ++wi, ++pi)
      {
        const auto pathType{fs::status(*wi).type()};

        switch(pathType)
        {
        case fs::file_type::regular:
        {
          const auto [working, prediction] {get_reduced_file_content(*wi, *pi)};
          if(working && prediction)
          {
            if(working.value() != prediction.value())
            {
              fs::copy_file(working.value(), prediction.value(), fs::copy_options::overwrite_existing);
            }
          }
          break;
        }
        case fs::file_type::directory:
        {
          testing::soft_update(*wi, *pi);
          break;
        }
        default:
          throw std::logic_error{std::string{"Detailed equivalance check for paths of type '"}
            .append(serializer<fs::file_type>::make(pathType)).append("' not currently implemented")};
        }
      }


      if((iters.first != workingEnd) && (iters.second != predictionEnd))
      {
        fs::remove_all(*iters.second);
        soft_update(iters.first, workingEnd, ++iters.second, predictionEnd, predictionDir);
      }
      else if(iters.first != workingEnd)
      {
        for(; iters.first != workingEnd; ++iters.first)
        {
          fs::copy(*iters.first, predictionDir, fs::copy_options::recursive);
        }
      }
      else if(iters.second != predictionEnd)
      {
        for(; iters.second != predictionEnd; ++iters.second)
        {
          fs::remove_all(*iters.second);
        }
      }
    }
  }

  void soft_update(const fs::path& working, const fs::path& prediction)
  {
    const std::vector<fs::path>
      sortedWorkingEntries{sort_dir_entries(working)},
      sortedPredictionEntries{sort_dir_entries(prediction)};

    soft_update(sortedWorkingEntries.begin(), sortedWorkingEntries.end(), sortedPredictionEntries.begin(), sortedPredictionEntries.end(), prediction);
  }

}