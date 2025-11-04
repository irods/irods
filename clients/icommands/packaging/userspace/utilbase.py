#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import sys, errno  # noqa: I201,I001; pylint: disable=C0410

if __name__ == '__main__':
	print(__file__ + ': This module is not meant to be invoked directly.',  # noqa: T001
	      file=sys.stderr)
	sys.exit(errno.ELIBEXEC)

import shutil, os  # noqa: I100,I202; pylint: disable=C0410
from abc import ABCMeta, abstractmethod
from concurrent.futures import Executor, Future
from types import ModuleType
from typing import Generic, Optional, TypeVar

from compat_shims import Callable, Iterable, Mapping, MutableMapping, NoReturn
from context import OptionsT, PackagerContext
from lief_instrumentation import LiefModuleT, lief_ver_info

__all__ = [
	'DummyExecutor',
	'PackagerToolStateBase',
	'PackagerToolState',
	'PackagerOperationToolState',
	'ToolStateT',
	'PackagerUtilBase'
]


class DummyExecutor(Executor):

	def submit(self, fn: Callable, *args, **kwargs) -> Future:  # noqa: ANN002,ANN003; pylint: disable=W0221
		ret = Future()
		try:
			result = fn(*args, **kwargs)
		except BaseException as ex:  # noqa: B902; pylint: disable=W0703
			ret.set_exception(ex)
		else:
			ret.set_result(result)
		return ret


class PackagerToolStateBase(metaclass=ABCMeta):

	@property
	@abstractmethod
	def is_usable(self) -> bool:
		raise NotImplementedError()

	@property
	@abstractmethod
	def path(self) -> Optional[str]:
		raise NotImplementedError()

	def __bool__(self) -> bool:
		return self.is_usable


ToolStateT = TypeVar('ToolStateT', bound=PackagerToolStateBase)


class PackagerToolState(PackagerToolStateBase):

	def __init__(self, is_usable: bool = True, path: Optional[str] = None):
		super().__init__()
		self._is_usable = is_usable
		self._path = path

	@property
	def is_usable(self) -> bool:
		return self._is_usable

	@is_usable.setter
	def is_usable(self, value: bool) -> NoReturn:
		self._is_usable = value

	@property
	def path(self) -> Optional[str]:
		return self._path

	@path.setter
	def path(self, value: Optional[str]) -> NoReturn:
		self._path = value


class PackagerOperationToolState(PackagerToolStateBase):

	def __init__(self, tool_state: PackagerToolStateBase):
		super().__init__()
		self._tool_state = tool_state
		self.tool_tried: bool = not tool_state.is_usable

	@property
	def is_usable(self) -> bool:
		return self._tool_state.is_usable

	@property
	def path(self) -> Optional[str]:
		return self._tool_state.path


class PackagerUtilBase(Generic[OptionsT]):

	def __init__(self, context: PackagerContext[OptionsT], executor: Optional[Executor] = None):
		if executor is None:
			executor = DummyExecutor()
		self._context = context
		self._executor = executor

	@property
	def context(self) -> PackagerContext[OptionsT]:
		return self._context

	@property
	def options(self) -> Optional[OptionsT]:
		return self.context.options

	@property
	def ilib(self) -> ModuleType:
		return self.context.ilib

	@property
	def lief(self) -> Optional[LiefModuleT]:  # noqa: ANN201
		return self.context.lief

	@property
	def lief_info(self) -> lief_ver_info:
		return self.context.lief_info

	@property
	def executor(self) -> Executor:
		return self._executor

	def fill_out_envdict(
		self,
		envdict: Optional[Mapping[str, str]] = None
	) -> MutableMapping[str, str]:
		_env = os.environ.copy()
		if envdict is None:
			envdict = self.context.envdict
		if envdict is not None:
			_env.update(envdict)
		return _env

	def find_tool(
		self, toolnames: Iterable[str],
		*args: str,
		raise_if_not_found: Optional[bool] = True,
		prettyname: Optional[str] = None,
		path: Optional[str] = None,
		env: Optional[Mapping[str, str]] = None
	) -> Optional[str]:
		# if we have an item in *args, it's probably a provided path
		# if it's truthy, return it
		# this is a convenience feature so that options.value None checking can be delegated here
		# if we have more than one item in *args, though, it's an error
		if len(args) == 1 and args[0]:
			return args[0]
		elif len(args) > 1:
			raise ValueError('unknown positional arguments')

		# get PATH from envdict, if it's there
		if env and 'PATH' in env:
			# if path and env were specified, and env contains a 'PATH', panic
			if path:
				raise ValueError('both path and env with PATH specified')
			path = env['PATH']

		# check the context's envdict, too
		if not path:
			path = self.context.envdict.get('PATH', None)

		for toolname in toolnames:
			found_tool = shutil.which(toolname, path=path)
			if found_tool:
				return found_tool

		if raise_if_not_found:
			if not prettyname:
				prettyname = next(iter(prettyname))

			raise OSError(errno.ENOENT, 'Could not find ' + prettyname + ' tool')

		return None
