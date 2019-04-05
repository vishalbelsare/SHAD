//===------------------------------------------------------------*- C++ -*-===//
//
//                                     SHAD
//
//      The Scalable High-performance Algorithms and Data Structure Library
//
//===----------------------------------------------------------------------===//
//
// Copyright 2018 Battelle Memorial Institute
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy
// of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
//===----------------------------------------------------------------------===/

#include <iostream>
#include <iomanip>
#include <random>

#include "shad/core/array.h"
#include "shad/core/algorithm.h"
#include "shad/core/numeric.h"
#include "shad/core/execution.h"
#include "shad/data_structures/one_per_locality.h"

namespace shad {

int main(int argc, char *argv[]) {

  shad::array<size_t, 128> counters;
  shad::array<size_t, 128> seeds;

  std::mt19937_64 seed_gen;
  shad::generate(
      shad::distributed_sequential_tag{},
      seeds.begin(), seeds.end(),
      [=]() mutable -> size_t {
        return seed_gen();
      });

  size_t numberOfPoints = 1e10;
  size_t numberOfPointsPerSim = numberOfPoints / counters.size();

  shad::transform(
      shad::distributed_parallel_tag{},
      seeds.begin(), seeds.end(),
      counters.begin(),
      [=](const uint64_t & S) -> size_t {
        size_t counter = 0;
        std::knuth_b G(S);
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        for (size_t i = 0; i < numberOfPointsPerSim; ++i) {
          double x = dist(G);
          double y = dist(G);
          if ((x * x + y * y) < 1) {
            ++counter;
          }
        }
        return counter;
      });

  size_t count = shad::reduce(
      shad::distributed_parallel_tag{}, counters.begin(), counters.end());

  std::cout << "Pi is roughly "
            << std::setprecision(20)
            << (4.0 * count) / numberOfPoints << std::endl;

  return 0;
}

}
