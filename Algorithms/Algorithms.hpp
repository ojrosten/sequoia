#pragma once

#include <functional>

namespace sequoia
{
  template<class Iter> constexpr void iter_swap(Iter a, Iter b)
  {
    auto tmp{std::move(*a)};
    *a = std::move(*b);
    *b = std::move(tmp);
  }
  
  template<class FwdIter, class Comparer=std::less<std::decay_t<decltype(*FwdIter())>>>
  constexpr void bubble_up(FwdIter begin, FwdIter current, Comparer comp = Comparer{})
  {
    if(current == begin) return;

    using namespace std;
    auto parent{begin + (distance(begin, current) - 1) / 2};
    if(comp(*parent, *current))
    {
      iter_swap(parent, current);
      bubble_up(begin, parent, comp);
    }
  }
  
  template<class FwdIter, class Comparer=std::less<std::decay_t<decltype(*FwdIter())>>>
  constexpr void bubble_down(FwdIter begin, FwdIter current, FwdIter end, Comparer comp = Comparer{})
  {
    using namespace std;
    if(distance(begin, end) <= 1) return;

    if(2*(distance(begin, current) + 1) < distance(begin, end))
    {    
      auto rightChild{begin + 2*(distance(begin, current) + 1)};
      auto leftChild{rightChild - 1};

      auto dominantChild{comp(*rightChild, *leftChild) ? leftChild : rightChild};
      if(comp(*current, *dominantChild))
      {
        iter_swap(dominantChild, current);
        bubble_down(begin, dominantChild, end, comp);
      }
    }
    else if(2*distance(begin, current) + 1 < distance(begin, end))
    {
      auto leftChild{begin + 2*distance(begin, current) + 1};
      if(comp(*current, *leftChild)) iter_swap(leftChild, current);
    }
  }

  template<class FwdIter, class Comparer=std::less<std::decay_t<decltype(*FwdIter())>>>
  constexpr void make_heap(FwdIter begin, FwdIter end, Comparer comp = Comparer{})
  {
    using namespace std;
    if(distance(begin, end) <= 1) return;
    
    auto current{begin+1};
    while(current != end)
    {
      bubble_up(begin, current, comp);        
      ++current;
    }
  }

  template<class FwdIter, class Comparer=std::less<std::decay_t<decltype(*FwdIter())>>>
  constexpr void sort(FwdIter begin, FwdIter end, Comparer comp = Comparer{})
  {
    using namespace std;
    if(distance(begin, end) <= 1) return;
    
    sequoia::make_heap(begin, end, comp);
    while(end != begin)
    {
      iter_swap(begin, end-1);
      bubble_down(begin, begin, end-1, comp);
      --end;
    }
  }

  /*template<class FwdIter> constexpr void rotate(FwdIter begin, FwdIter end)
  {
    using namespace std;
    if(distance(begin, end) > 1)
    {
      --end;
      auto last{*end};
      while(end != begin)
      {
        *end = std::move(*(end-1));
        --end;
      }
      *end = last;
    }
    }*/

  template<class FwdIter, class Comparer=std::equal_to<std::decay_t<decltype(*FwdIter())>>>
  constexpr void cluster(FwdIter begin, FwdIter end, Comparer comp = Comparer{})
  {
    if(begin == end) return;
    
    auto current{begin};
    while((current != end) && comp(*current, *begin)) ++current;

    auto endOfCluster{current};
    
    while(current != end)
    {
      if(comp(*current, *begin))
      {
        iter_swap(endOfCluster, current);
        ++endOfCluster;
      }
      
      ++current;
    }

    cluster(endOfCluster, end, comp);
  }

  template<class FwdIter, class T, class Comparer=std::less<std::decay_t<decltype(*FwdIter())>>>
  constexpr FwdIter lower_bound(FwdIter begin, FwdIter end, const T& val, Comparer comp = Comparer{})
  {
    while(begin != end)
    {        
      using namespace std;
      auto partition{begin + distance(begin, end)/2};
      if(comp(*partition, val))
      {
        begin = ++partition;
      }
      else
      {
        end = partition;
      }
    }

    return end;
  }

  template<class FwdIter, class T, class Comparer=std::less<std::decay_t<decltype(*FwdIter())>>>
  constexpr FwdIter upper_bound(FwdIter begin, FwdIter end, const T& val, Comparer comp = Comparer{})
  {
    auto notComp{
      [comp](const auto& lhs, const auto& rhs){
        return !comp(rhs, lhs);
      }
    };

    return sequoia::lower_bound(begin, end, val, notComp);
  }

  template<class FwdIter, class T, class Comparer=std::less<std::decay_t<decltype(*FwdIter())>>>
  constexpr std::pair<FwdIter, FwdIter> equal_range(FwdIter begin, FwdIter end, const T& val, Comparer comp = Comparer{})
  {
    return {sequoia::lower_bound(begin, end, val, comp), sequoia::upper_bound(begin, end, val, comp)};
  }
}
