#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import sys, errno  # noqa: I201,I001; pylint: disable=C0410

if __name__ == '__main__':
	print(__file__ + ': This module is not meant to be invoked directly.',  # noqa: T001
	      file=sys.stderr)
	sys.exit(errno.ELIBEXEC)

import os  # noqa: I100,I202
import itertools
from concurrent.futures import Future
from concurrent.futures import FIRST_EXCEPTION as F_FIRST_EXCEPTION
from concurrent.futures import wait as f_wait
from typing import Any, Optional, TypeVar, overload

from compat_shims import Callable, Collection, Iterable, Iterator, MutableMapping, MutableSequence
from compat_shims import Sequence

__all__ = [
	'irods_scripts_dir',
	'futures_wait',
	'IteratorFactory',
	'CollectionCompoundView',
	'FrozenList'
]

T_co = TypeVar('T_co', covariant=True)  # pylint: disable=C0103

irods_scripts_dir = '/var/lib/irods/scripts'  # pylint: disable=C0103
if 'IRODS_SCRIPTS_DIR' in os.environ:
	irods_scripts_dir = os.environ['IRODS_SCRIPTS_DIR']


def futures_wait(futures: Iterable[Future]) -> MutableSequence:
	fs_done, fs_notdone = f_wait(futures, return_when=F_FIRST_EXCEPTION)  # pylint: disable=W0612
	res = [f.result() for f in fs_done]
	return res


class IteratorFactory(Iterable[T_co]):

	def __init__(self, fn: Callable, *args: Any, **kwargs: Any):
		self.fn: Callable = fn
		self.args: MutableSequence[Any] = args
		self.kwargs: MutableMapping[str, Any] = kwargs

	def __iter__(self) -> Iterator[T_co]:
		return self.fn(*self.args, **self.kwargs)


class CollectionCompoundView(Collection[T_co]):

	def __init__(self, collections: Iterable[Collection[T_co]]):
		super().__init__()
		self._collections = collections

	@property
	def collections(self) -> Iterable[Collection[T_co]]:
		return self._collections

	def __contains__(self, obj: T_co) -> bool:
		return any(obj in coll for coll in self.collections)

	def __len__(self) -> int:
		sz = 0
		for coll in self.collections:
			sz = sz + len(coll)
		return sz

	def __iter__(self) -> Iterator[T_co]:
		return itertools.chain.from_iterable(self.collections)


class FrozenList(Sequence[T_co]):

	@overload
	def __init__(self):  # noqa: ANN204
		pass

	@overload
	def __init__(self, iterable: Iterable[T_co]):
		pass

	def __init__(self, *args: Iterable[T_co]):
		super().__init__()
		if len(args) > 1:
			raise ValueError('Too many positional arguments')
		elif len(args) == 1 and args[0] is not None:
			self._list = list(args[0])
		else:
			self._list = []

	def __add__(self, other: Sequence[T_co]) -> Sequence[T_co]:
		if isinstance(other, FrozenList):
			return FrozenList(self._list + other._list)  # pylint: disable=W0212
		return FrozenList(self._list + other)

	def __contains__(self, value: T_co) -> bool:
		return value in self._list

	def __eq__(self, other: Sequence[T_co]) -> bool:
		if self is other:
			return True
		if isinstance(other, MutableSequence):
			return False
		if isinstance(other, FrozenList):
			return self._list == other._list  # pylint: disable=W0212
		if isinstance(other, Sequence):
			return tuple(self._list) == tuple(other)

		return False

	def __format__(self, format_spec: str) -> str:
		return self._list.__format__(format_spec)

	def __getitem__(self, index: int) -> T_co:
		return self._list[index]

	def __iter__(self) -> Iterator[T_co]:
		return iter(self._list)

	def __len__(self) -> int:
		return len(self._list)

	def __repr__(self) -> str:
		return self._list.__repr__()

	def __reversed__(self) -> Iterator[T_co]:
		return reversed(self._list)

	def __str__(self) -> str:
		return self._list.__str__()

	def copy(self) -> Sequence[T_co]:
		return FrozenList(self._list.copy())

	def count(self, value: T_co) -> int:
		return self._list.count(value)

	def index(self, value: T_co, start: int = 0, stop: Optional[int] = None) -> int:
		if stop is None:
			return self._list.index(value, start)
		else:
			return self._list.index(value, start, stop)
