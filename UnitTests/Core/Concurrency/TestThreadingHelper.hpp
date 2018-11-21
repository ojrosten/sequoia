#pragma once

namespace sequoia
{
  namespace unit_testing
  {
    // Deliberately ridiculous way of
    // computing triangular number!
    class TriangularNumbers
    {
    public:
      TriangularNumbers(const std::size_t n) : m_Upper(n)
      {
      }

      std::size_t operator()() const
      {
        std::size_t answer{0};
        for(std::size_t i=1; i <= m_Upper; ++i)
        {
          answer += i;
        }

        return answer;
      }

    private:
      std::size_t m_Upper;
    };

    class UpdatableFunctor
    {
    public:
      void operator()(const int x)
      {
	m_Data.push_back(x);
      }

      const auto& get_data() const { return m_Data; }
    private:
      std::vector<int> m_Data;
    };
  }
}
