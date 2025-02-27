// Copyright 2021 DeepMind Technologies Limited..
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef THIRD_PARTY_PY_ENVLOGGER_CONVERTERS_MAKE_VISITOR_H_
#define THIRD_PARTY_PY_ENVLOGGER_CONVERTERS_MAKE_VISITOR_H_

#include <tuple>
#include <type_traits>
#include <utility>

#include "absl/types/variant.h"
#include "absl/utility/utility.h"

// Template magic for more easily creating overloaded visitors for use with
// absl::visit (and eventually std::visit with C++17).
//
// Example:
//
// const absl::variant<Foo, Bar> my_variant = PopulateVariant();
// const auto visitor = envlogger::MakeVisitor(
//     [](const Foo& foo) { DoFooStuff(foo); },
//     [](const Bar& bar) { DoBarStuff(bar); }
// );
// absl::visit(visitor, my_variant);

namespace envlogger {

// This uses C++17 type expansion to inherit from each of the lambda types
// passed in to the constructor and inherit all of their operator()s.
template <class... Visitors>
struct Visitor : Visitors... {
  explicit Visitor(const Visitors&... v) : Visitors(v)... {}
  using Visitors::operator()...;
};

template <class... Visitors>
Visitor<Visitors...> MakeVisitor(Visitors... visitors) {
  return Visitor<Visitors...>(visitors...);
}

}  // namespace envlogger

#endif  // LEARNING_DEEPMIND_DATASETS_ENVIRONMENT_LOGGER_CONVERTERS_MAKE_VISITOR_H_
