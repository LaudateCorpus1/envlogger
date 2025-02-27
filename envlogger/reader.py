# coding=utf-8
# Copyright 2021 DeepMind Technologies Limited..
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Reader of EnvironmentLogger data."""

from typing import Any, Optional, Sequence, Union

from absl import logging
from envlogger import step_data
from envlogger.backends import backend_reader
from envlogger.backends import backend_type
from envlogger.backends import in_memory_backend
from envlogger.backends import riegeli_backend_reader
from envlogger.converters import spec_codec


class Reader:
  """Reader of trajectories generated by EnvLogger."""

  def __init__(self,
               *backend_args,
               backend: Union[
                   backend_reader.BackendReader,
                   backend_type.BackendType] = backend_type.BackendType.RIEGELI,
               **backend_kwargs):
    logging.info('backend: %r', backend)
    logging.info('backend_args: %r', backend_args)
    logging.info('backend_kwargs: %r', backend_kwargs)
    # Set backend.
    if isinstance(backend, backend_reader.BackendReader):
      self._backend = backend
    elif isinstance(backend, backend_type.BackendType):
      self._backend = {
          backend_type.BackendType.RIEGELI:
              riegeli_backend_reader.RiegeliBackendReader,
          backend_type.BackendType.IN_MEMORY:
              in_memory_backend.InMemoryBackendReader,
      }[backend](*backend_args, **backend_kwargs)
    else:
      raise TypeError(f'Unsupported backend: {backend}')
    self._set_specs()

  def __enter__(self):
    return self

  def __exit__(self, exc_type, exc_value, tb):
    self._backend.close()

  def __del__(self):
    self._backend.close()

  def metadata(self):
    return self._backend.metadata()

  @property
  def steps(self) -> Sequence[step_data.StepData]:
    return self._backend.steps

  @property
  def episodes(self) -> Sequence[Sequence[step_data.StepData]]:
    return self._backend.episodes

  def episode_metadata(self) -> Sequence[Optional[Any]]:
    return self._backend.episode_metadata()

  def observation_spec(self):
    return self._observation_spec

  def action_spec(self):
    return self._action_spec

  def reward_spec(self):
    return self._reward_spec

  def discount_spec(self):
    return self._discount_spec

  def _set_specs(self) -> None:
    """Extracts and decodes environment specs from the logged data."""
    metadata = self._backend.metadata() or {}
    env_specs = spec_codec.decode_environment_specs(
        metadata.get('environment_specs', {}))
    self._observation_spec = env_specs['observation_spec']
    self._action_spec = env_specs['action_spec']
    self._reward_spec = env_specs['reward_spec']
    self._discount_spec = env_specs['discount_spec']
