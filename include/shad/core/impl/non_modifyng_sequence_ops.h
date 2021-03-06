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
//===----------------------------------------------------------------------===//

#ifndef INCLUDE_SHAD_CORE_IMPL_NON_MODIFYING_SEQUENCE_OPS_H
#define INCLUDE_SHAD_CORE_IMPL_NON_MODIFYING_SEQUENCE_OPS_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>

#include "shad/core/execution.h"
#include "shad/distributed_iterator_traits.h"
#include "shad/runtime/runtime.h"

#include "impl_patterns.h"

namespace shad {
namespace impl {

template <typename ForwardItr, typename UnaryPredicate>
bool all_of(distributed_sequential_tag&& policy, ForwardItr first,
            ForwardItr last, UnaryPredicate p) {
  using itr_traits = distributed_iterator_traits<ForwardItr>;

  return distributed_folding_map_early_termination(
      // range
      first, last,
      // kernel
      [](ForwardItr first, ForwardItr last, const bool partial_solution,
         UnaryPredicate p) {
        // local processing
        auto lrange = itr_traits::local_range(first, last);
        auto local_res = std::all_of(lrange.begin(), lrange.end(), p);
        // update the partial solution
        return local_res;
      },
      // halt condition
      [](const bool x) { return !x; },
      // initial solution
      true,
      // map arguments
      p);
}

template <typename ForwardItr, typename UnaryPredicate>
bool all_of(distributed_parallel_tag&& policy, ForwardItr first,
            ForwardItr last, UnaryPredicate p) {
  using itr_traits = distributed_iterator_traits<ForwardItr>;
  using value_t = typename itr_traits::value_type;

  // distributed map
  auto map_res = distributed_map(
      // range
      first, last,
      // kernel
      [](ForwardItr first, ForwardItr last, UnaryPredicate p) -> uint8_t {
        using local_iterator_t = typename itr_traits::local_iterator_type;

        // local map
        auto lrange = itr_traits::local_range(first, last);

        auto map_res = local_map(
            // range
            lrange.begin(), lrange.end(),
            // kernel
            [&](const local_iterator_t& b, const local_iterator_t& e)
                -> uint8_t { return std::all_of(b, e, p); });

        // local reduce
        return std::all_of(map_res.begin(), map_res.end(),
                           [](bool x) { return x; });
      },
      // map arguments
      p);

  // reduce
  return std::all_of(map_res.begin(), map_res.end(), [](bool x) { return x; });
}

template <typename ForwardItr, typename UnaryPredicate>
bool any_of(distributed_sequential_tag&& policy, ForwardItr first,
            ForwardItr last, UnaryPredicate p) {
  using itr_traits = distributed_iterator_traits<ForwardItr>;

  return distributed_folding_map_early_termination(
      // range
      first, last,
      // kernel
      [](ForwardItr first, ForwardItr last, const bool partial_solution,
         UnaryPredicate p) {
        // local processing
        auto lrange = itr_traits::local_range(first, last);
        auto local_res = std::any_of(lrange.begin(), lrange.end(), p);
        // update the partial solution
        return local_res;
      },
      // halt condition
      [](const bool x) { return x; },
      // initial solution
      false,
      // map arguments
      p);
}

template <typename ForwardItr, typename UnaryPredicate>
bool any_of(distributed_parallel_tag&& policy, ForwardItr first,
            ForwardItr last, UnaryPredicate p) {
  using itr_traits = distributed_iterator_traits<ForwardItr>;
  using value_t = typename itr_traits::value_type;

  // distributed map
  auto map_res = distributed_map(
      // range
      first, last,
      // kernel
      [](ForwardItr first, ForwardItr last, UnaryPredicate p) -> uint8_t {
        using local_iterator_t = typename itr_traits::local_iterator_type;

        // local map
        auto lrange = itr_traits::local_range(first, last);

        auto map_res = local_map(
            // range
            lrange.begin(), lrange.end(),
            // kernel
            [&](local_iterator_t b, local_iterator_t e) -> uint8_t {
              return std::any_of(b, e, p);
            });

        // local reduce
        return std::any_of(map_res.begin(), map_res.end(),
                           [](bool x) { return x; });
      },
      // map arguments
      p);

  // reduce
  return std::any_of(map_res.begin(), map_res.end(), [](bool x) { return x; });
}

template <typename ForwardItr, typename T>
ForwardItr find(distributed_sequential_tag&& policy, ForwardItr first,
                ForwardItr last, const T& value) {
  using itr_traits = distributed_iterator_traits<ForwardItr>;

  return distributed_folding_map_early_termination(
      // range
      first, last,
      // kernel
      [](ForwardItr first, ForwardItr last, const ForwardItr partial_solution,
         const T& value) {
        // local processing
        auto lrange = itr_traits::local_range(first, last);
        auto local_res = std::find(lrange.begin(), lrange.end(), value);
        // update the partial solution
        return (local_res != lrange.end())
                   ? itr_traits::iterator_from_local(first, last, local_res)
                   : partial_solution;
      },
      // halt condition
      [&](const ForwardItr x) { return x != last; },
      // initial solution
      last,
      // map arguments
      value);
}

template <typename ForwardItr, typename T>
ForwardItr find(distributed_parallel_tag&& policy, ForwardItr first,
                ForwardItr last, const T& value) {
  using itr_traits = distributed_iterator_traits<ForwardItr>;
  using value_t = typename itr_traits::value_type;

  // distributed map
  auto map_res = distributed_map(
      // range
      first, last,
      // kernel
      [](ForwardItr first, ForwardItr last, const T& value) {
        using local_iterator_t = typename itr_traits::local_iterator_type;

        // local map
        auto lrange = itr_traits::local_range(first, last);

        auto map_res = local_map(
            // range
            lrange.begin(), lrange.end(),
            // kernel
            [&](local_iterator_t b, local_iterator_t e) {
              auto res = std::find(b, e, value);
              return res != e ? res : lrange.end();
            });

        // local reduce
        auto found = std::find_if(
            map_res.begin(), map_res.end(),
            [&](const local_iterator_t& i) { return i != lrange.end(); });
        return found != map_res.end()
                   ? itr_traits::iterator_from_local(first, last, *found)
                   : last;
      },
      // map arguments
      value);

  // reduce
  auto found = std::find_if(map_res.begin(), map_res.end(),
                            [&](ForwardItr i) { return i != last; });
  return found != map_res.end() ? *found : last;
}

template <typename ForwardItr, typename UnaryPredicate>
ForwardItr find_if(distributed_sequential_tag&& policy, ForwardItr first,
                   ForwardItr last, UnaryPredicate p) {
  using itr_traits = distributed_iterator_traits<ForwardItr>;

  return distributed_folding_map_early_termination(
      // range
      first, last,
      // kernel
      [](ForwardItr first, ForwardItr last, const ForwardItr partial_solution,
         UnaryPredicate p) {
        // local processing
        auto lrange = itr_traits::local_range(first, last);
        auto local_res = std::find_if(lrange.begin(), lrange.end(), p);
        // update the partial solution
        return (local_res != lrange.end())
                   ? itr_traits::iterator_from_local(first, last, local_res)
                   : partial_solution;
      },
      // halt condition
      [&](const ForwardItr x) { return x != last; },
      // initial solution
      last,
      // map arguments
      p);
}

template <typename ForwardItr, typename UnaryPredicate>
ForwardItr find_if(distributed_parallel_tag&& policy, ForwardItr first,
                   ForwardItr last, UnaryPredicate p) {
  using itr_traits = distributed_iterator_traits<ForwardItr>;
  using value_t = typename itr_traits::value_type;

  // distributed map
  auto map_res = distributed_map(
      // range
      first, last,
      // kernel
      [](ForwardItr first, ForwardItr last, UnaryPredicate p) {
        using local_iterator_t = typename itr_traits::local_iterator_type;

        // local map
        auto lrange = itr_traits::local_range(first, last);

        auto map_res = local_map(
            // range
            lrange.begin(), lrange.end(),
            // kernel
            [&](local_iterator_t b, local_iterator_t e) {
              auto res = std::find_if(b, e, p);
              return res != e ? res : lrange.end();
            });

        // local reduce
        auto found = std::find_if(
            map_res.begin(), map_res.end(),
            [&](const local_iterator_t& i) { return i != lrange.end(); });
        return found != map_res.end()
                   ? itr_traits::iterator_from_local(first, last, *found)
                   : last;
      },
      // map arguments
      p);

  // reduce
  auto found = std::find_if(map_res.begin(), map_res.end(),
                            [&](ForwardItr i) { return i != last; });
  return found != map_res.end() ? *found : last;
}

template <typename ForwardItr, typename UnaryPredicate>
ForwardItr find_if_not(distributed_sequential_tag&& policy, ForwardItr first,
                       ForwardItr last, UnaryPredicate p) {
  using itr_traits = distributed_iterator_traits<ForwardItr>;

  return distributed_folding_map_early_termination(
      // range
      first, last,
      // kernel
      [](ForwardItr first, ForwardItr last, const ForwardItr partial_solution,
         UnaryPredicate p) {
        // local processing
        auto lrange = itr_traits::local_range(first, last);
        auto local_res = std::find_if_not(lrange.begin(), lrange.end(), p);
        // update the partial solution
        return (local_res != lrange.end())
                   ? itr_traits::iterator_from_local(first, last, local_res)
                   : partial_solution;
      },
      // halt condition
      [&](const ForwardItr x) { return x != last; },
      // initial solution
      last,
      // map arguments
      p);
}

template <typename ForwardItr, typename UnaryPredicate>
ForwardItr find_if_not(distributed_parallel_tag&& policy, ForwardItr first,
                       ForwardItr last, UnaryPredicate p) {
  using itr_traits = distributed_iterator_traits<ForwardItr>;
  using value_t = typename itr_traits::value_type;

  // distributed map
  auto map_res = distributed_map(
      // range
      first, last,
      // kernel
      [](ForwardItr first, ForwardItr last, UnaryPredicate p) {
        using local_iterator_t = typename itr_traits::local_iterator_type;

        // local map
        auto lrange = itr_traits::local_range(first, last);

        auto map_res = local_map(
            // range
            lrange.begin(), lrange.end(),
            // kernel
            [&](local_iterator_t b, local_iterator_t e) {
              auto res = std::find_if_not(b, e, p);
              return res != e ? res : lrange.end();
            });

        // local reduce
        auto found = std::find_if(
            map_res.begin(), map_res.end(),
            [&](const local_iterator_t& i) { return i != lrange.end(); });
        return found != map_res.end()
                   ? itr_traits::iterator_from_local(first, last, *found)
                   : last;
      },
      // map arguments
      p);

  // reduce
  auto found = std::find_if(map_res.begin(), map_res.end(),
                            [&](ForwardItr i) { return i != last; });
  return found != map_res.end() ? *found : last;
}

template <typename ForwardItr, typename UnaryPredicate>
void for_each(distributed_sequential_tag&& policy, ForwardItr first,
              ForwardItr last, UnaryPredicate p) {
  using itr_traits = distributed_iterator_traits<ForwardItr>;

  distributed_folding_map_void(
      // range
      first, last,
      // kernel
      [](ForwardItr first, ForwardItr last, UnaryPredicate p) {
        // local processing
        auto lrange = itr_traits::local_range(first, last);
        std::for_each(lrange.begin(), lrange.end(), p);
      },
      // map arguments
      p);
}

template <typename ForwardItr, typename UnaryPredicate>
void for_each(distributed_parallel_tag&& policy, ForwardItr first,
              ForwardItr last, UnaryPredicate p) {
  using itr_traits = distributed_iterator_traits<ForwardItr>;

  // distributed map
  distributed_map_void(
      // range
      first, last,
      // kernel
      [](ForwardItr first, ForwardItr last, UnaryPredicate p) {
        using local_iterator_t = typename itr_traits::local_iterator_type;

        // local map
        auto lrange = itr_traits::local_range(first, last);

        local_map_void(
            // range
            lrange.begin(), lrange.end(),
            // kernel
            [&](local_iterator_t b, local_iterator_t e) {
              std::for_each(b, e, p);
            });
      },
      // map arguments
      p);
}

template <typename InputItr, typename T>
typename shad::distributed_iterator_traits<InputItr>::difference_type count(
    distributed_sequential_tag&& policy, InputItr first, InputItr last,
    const T& value) {
  using itr_traits = distributed_iterator_traits<InputItr>;
  using res_t =
      typename shad::distributed_iterator_traits<InputItr>::difference_type;

  return distributed_folding_map(
      // range
      first, last,
      // kernel
      [](InputItr first, InputItr last, res_t cnt, const T& value) {
        // local processing
        auto lrange = itr_traits::local_range(first, last);
        auto local_res = std::count(lrange.begin(), lrange.end(), value);
        // update the partial solution
        return cnt + local_res;
      },
      // initial solution
      res_t{0},
      // map arguments
      value);
}

template <typename InputItr, typename T>
typename shad::distributed_iterator_traits<InputItr>::difference_type count(
    distributed_parallel_tag&& policy, InputItr first, InputItr last,
    const T& value) {
  using itr_traits = distributed_iterator_traits<InputItr>;
  using res_t =
      typename shad::distributed_iterator_traits<InputItr>::difference_type;

  // distributed map
  auto map_res = distributed_map(
      // range
      first, last,
      // kernel
      [](InputItr first, InputItr last, const T& value) {
        using local_iterator_t = typename itr_traits::local_iterator_type;

        // local map
        auto lrange = itr_traits::local_range(first, last);

        auto map_res = local_map(
            // range
            lrange.begin(), lrange.end(),
            // kernel
            [&](local_iterator_t b, local_iterator_t e) {
              return std::count(b, e, value);
            });

        // local reduce
        return std::accumulate(
            map_res.begin(), map_res.end(), res_t{0},
            [](const res_t& acc, const res_t& x) { return acc + x; });
      },
      // map arguments
      value);

  // reduce
  return std::accumulate(
      map_res.begin(), map_res.end(), res_t{0},
      [](const res_t& acc, const res_t& x) { return acc + x; });
}

template <typename InputItr, typename UnaryPredicate>
typename shad::distributed_iterator_traits<InputItr>::difference_type count_if(
    distributed_sequential_tag&& policy, InputItr first, InputItr last,
    UnaryPredicate p) {
  using itr_traits = distributed_iterator_traits<InputItr>;
  using res_t =
      typename shad::distributed_iterator_traits<InputItr>::difference_type;

  return distributed_folding_map(
      // range
      first, last,
      // kernel
      [](InputItr first, InputItr last, res_t cnt, UnaryPredicate p) {
        // local processing
        auto lrange = itr_traits::local_range(first, last);
        auto local_res = std::count_if(lrange.begin(), lrange.end(), p);
        // update the partial solution
        return cnt + local_res;
      },
      // initial solution
      res_t{0},
      // map arguments
      p);
}

template <typename InputItr, typename UnaryPredicate>
typename shad::distributed_iterator_traits<InputItr>::difference_type count_if(
    distributed_parallel_tag&& policy, InputItr first, InputItr last,
    UnaryPredicate p) {
  using itr_traits = distributed_iterator_traits<InputItr>;
  using res_t =
      typename shad::distributed_iterator_traits<InputItr>::difference_type;

  // distributed map
  auto map_res = distributed_map(
      // range
      first, last,
      // kernel
      [](InputItr first, InputItr last, UnaryPredicate p) {
        using local_iterator_t = typename itr_traits::local_iterator_type;

        // local map
        auto lrange = itr_traits::local_range(first, last);

        auto map_res = local_map(
            // range
            lrange.begin(), lrange.end(),
            // kernel
            [&](local_iterator_t b, local_iterator_t e) {
              return std::count_if(b, e, p);
            });

        // local reduce
        return std::accumulate(
            map_res.begin(), map_res.end(), res_t{0},
            [](const res_t& acc, const res_t& x) { return acc + x; });
      },
      // map arguments
      p);

  // reduce
  return std::accumulate(
      map_res.begin(), map_res.end(), res_t{0},
      [](const res_t& acc, const res_t& x) { return acc + x; });
}

}  // namespace impl
}  // namespace shad

#endif /* INCLUDE_SHAD_CORE_IMPL_NON_MODIFYING_SEQUENCE_OPS_H */
