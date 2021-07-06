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

#ifndef THIRD_PARTY_PY_ENVLOGGER_BACKENDS_CC_RIEGELI_SHARD_READER_H_
#define THIRD_PARTY_PY_ENVLOGGER_BACKENDS_CC_RIEGELI_SHARD_READER_H_

#include <cstdint>
#include <vector>

#include "glog/logging.h"
#include "google/protobuf/message.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "envlogger/backends/cc/episode_info.h"
#include "envlogger/platform/riegeli_file_reader.h"
#include "envlogger/proto/storage.pb.h"
#include "riegeli/records/record_reader.h"

namespace envlogger {

// A RiegeliShardReader refers to a single Riegeli index file (usually named
// "index.riegeli") that's associated with a Riegeli trajectory file (usually
// named "trajectories.riegeli"). The index file is an external index into the
// trajectory so that accesses have minimal latency.
//
// This class reads the data from one of these index files and returns
// information for accessing individual steps and episodes in the trajectories
// file.
//
// Note on nomenclature:
//   - The term "index" when used in an array refers to the position in the
//   array and is always 0-based throughout this library. For example, in the
//   array [13, 17, 24, 173] the element 13 is at index 0, 17 at index 1, 24 at
//   index 2 and 173 at index 3.
//   - The term "offset" refers to a file offset in bytes. This is the unit used
//   by Riegeli to position its reading head to read a particular record.
class RiegeliShardReader {
 public:
  RiegeliShardReader() = default;

  // Dummy copy constructor to allow preallocating vectors of TrajectoryReader.
  RiegeliShardReader(const RiegeliShardReader&) {}

  ~RiegeliShardReader();

  // Reads trajectory `trajectories_filepath` with index `index_filepath`.
  //
  // If `index_filepath` is empty the constructor will return early leaving the
  // object members empty. All methods should fail if called on a object in this
  // state. This is used by clients for efficiently initializing objects to
  // minimize memory allocations by preallocating everything beforehand.
  // DEPRECATED LEGACY format. Please do not use (this will be removed once all
  // existing trajectories have been converted or dealt with).
  absl::Status Init(absl::string_view index_filepath,
                    absl::string_view trajectories_filepath);

  // Reads trajectory data written by TrajectoryWriter.
  //
  // If `step_offsets_filepath` is empty the constructor will return early
  // leaving the object members empty. All methods should fail if called on a
  // object in this state. This is used by clients for efficiently initializing
  // objects to minimize memory allocations by preallocating everything
  // beforehand.
  absl::Status Init(absl::string_view steps_filepath,
                    absl::string_view step_offsets_filepath,
                    absl::string_view episode_metadata_filepath,
                    absl::string_view episode_index_filepath);

  // Returns the number of steps indexed by this TrajectoryReader.
  int64_t NumSteps() const;

  // Returns the number of episodes indexed by this TrajectoryReader.
  int64_t NumEpisodes() const;

  // Returns step data at index `step_index`.
  // Returns nullopt if step_index is not in [0, NumSteps()).
  template <typename T = Data>
  absl::optional<T> Step(int64_t step_index);

  // Returns information for accessing a specific episode.
  // Returns nullopt if `episode_index` is not in [0, NumEpisodes()).
  absl::optional<EpisodeInfo> Episode(int64_t episode_index,
                                      bool include_metadata = false);

  // Releases resources and closes the underlying files.
  void Close();

 private:
  // Riegeli offsets for quickly accessing steps.
  std::vector<int64_t> step_offsets_;
  // Riegeli offsets for quickly accessing episodic metadata.
  std::vector<int64_t> episode_metadata_offsets_;

  // The first steps of each episodes.
  // Episode index -> Step index.
  std::vector<int64_t> episode_starts_;

  riegeli::RecordReader<RiegeliFileReader<>> steps_reader_;
  riegeli::RecordReader<RiegeliFileReader<>> episode_metadata_reader_;
};

////////////////////////////////////////////////////////////////////////////////
// Implementation details of RiegeliShardReader::Step.
////////////////////////////////////////////////////////////////////////////////

template <typename T>
absl::optional<T> RiegeliShardReader::Step(int64_t step_index) {
  if (step_index < 0 ||
      step_index >= static_cast<int64_t>(step_offsets_.size())) {
    return absl::nullopt;
  }

  const int64_t offset = step_offsets_[step_index];
  if (!steps_reader_.Seek(offset)) {
    VLOG(0) << absl::StrCat("Failed to seek to offset ", offset,
                            " status: ", steps_reader_.status().ToString());
    return absl::nullopt;
  }
  T data;
  const bool read_status = steps_reader_.ReadRecord(data);
  if (!read_status) return absl::nullopt;

  return data;
}

}  // namespace envlogger

#endif  // THIRD_PARTY_PY_ENVLOGGER_BACKENDS_CC_RIEGELI_SHARD_READER_H_
