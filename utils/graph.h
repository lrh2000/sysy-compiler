#pragma once
#include <cstddef>

class Graph
{
public:
  Graph(size_t num_nodes)
  {
    edges.resize(num_nodes);
  }

  const std::vector<size_t> &adjacent(size_t idx)
  {
    return edges[idx];
  }

  void add_edge(size_t x, size_t y)
  {
    edges[x].emplace_back(y);
    edges[y].emplace_back(x);
  }

private:
  std::vector<std::vector<size_t>> edges;
};
