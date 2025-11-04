#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import sys, errno  # noqa: I201,I001; pylint: disable=C0410

if __name__ == '__main__':
	print(__file__ + ': This module is not meant to be invoked directly.',  # noqa: T001
	      file=sys.stderr)
	sys.exit(errno.ELIBEXEC)

import logging  # noqa: I100,I202
import re
from concurrent.futures import Executor
from enum import Enum
from os import path
from threading import Lock
from typing import Any, Generic, Optional, TypeVar, Union, overload

from compat_shims import Iterable, Iterator, MutableSequence, MutableSet, NoReturn, OrderedDict
from compat_shims import Reversible, Sequence, Tuple
from compat_shims import ItemsView, KeysView, Mapping, MutableMapping, ValuesView
from context import PackagerContext, PackagerOptionsBase
from script_common import FrozenList
from options import RunArgOption
from utilbase import PackagerOperationToolState, PackagerToolState, PackagerUtilBase, ToolStateT

__all__ = [
	'ElfInfoUtil',
	'ElfInfo',
	'ElfVersionRefGroup',
	'ElfInfoUtilOptionsBase',
	'TableValueT',
	'LddLibPaths',
	'ElfInfoSource',
	'DynamicSectionTableBase',
	'DynamicSectionKeysView',
	'DynamicSectionValuesView',
	'DynamicSectionIndivValuesView',
	'DynamicSectionItemsView',
	'DynamicSectionIndivItemsView',
	'DynamicSectionTable',
	'ElfInfoToolStateBase',
	'ElfInfoToolState',
	'ElfInfoOperationState'
]


class ElfInfoUtilOptionsBase(PackagerOptionsBase):  # pylint: disable=W0223
	ldd_path: Optional[str] = RunArgOption(
		'--ldd-path', metavar='TOOLPATH',
		group=PackagerOptionsBase.tool_args,
		action='store', opt_type=str, default=None,
		help='Path to ldd tool to use'
	)
	readelf_path: Optional[str] = RunArgOption(
		'--readelf-path', metavar='TOOLPATH',
		group=PackagerOptionsBase.tool_args,
		action='store', opt_type=str, default=None,
		help='Path to readelf tool to use'
	)
	objdump_path: Optional[str] = RunArgOption(
		'--objdump-path', metavar='TOOLPATH',
		group=PackagerOptionsBase.tool_args,
		action='store', opt_type=str, default=None,
		help='Path to objdump tool to use'
	)


class ElfInfoToolStateBase(Generic[ToolStateT]):

	def __init__(
		self,
		ldd_tool: ToolStateT,
		lief_tool: ToolStateT,
		readelf_tool: ToolStateT,
		objdump_tool: ToolStateT
	):
		self._ldd_tool = ldd_tool
		self._lief_tool = lief_tool
		self._readelf_tool = readelf_tool
		self._objdump_tool = objdump_tool

	@property
	def ldd_tool(self) -> ToolStateT:
		return self._ldd_tool

	@property
	def ldd(self) -> ToolStateT:
		return self.ldd_tool

	@property
	def lief_tool(self) -> ToolStateT:
		return self._lief_tool

	@property
	def lief(self) -> ToolStateT:
		return self.lief_tool

	@property
	def readelf_tool(self) -> ToolStateT:
		return self._readelf_tool

	@property
	def readelf(self) -> ToolStateT:
		return self.readelf_tool

	@property
	def objdump_tool(self) -> ToolStateT:
		return self._objdump_tool

	@property
	def objdump(self) -> ToolStateT:
		return self.objdump_tool

	@property
	def elftools_usable(self) -> bool:
		return self.lief_tool or self.readelf_tool #or self.objdump_tool


class ElfInfoToolState(ElfInfoToolStateBase[PackagerToolState]):
	pass


class ElfInfoOperationState(ElfInfoToolStateBase[PackagerOperationToolState]):

	def __init__(self, tool_state: ElfInfoToolState):
		ldd_tool = PackagerOperationToolState(tool_state.ldd_tool)
		lief_tool = PackagerOperationToolState(tool_state.lief_tool)
		readelf_tool = PackagerOperationToolState(tool_state.readelf_tool)
		objdump_tool = PackagerOperationToolState(tool_state.objdump_tool)
		super().__init__(ldd_tool, lief_tool, readelf_tool, objdump_tool)


class LddLibPaths():

	def __init__(
		self,
		lib_paths: MutableMapping[str, str],
		not_found: MutableSet[str],
		other_libs: MutableSet[str]
	):
		super().__init__()
		self._lib_paths = lib_paths
		self._not_found = not_found
		self._other_libs = other_libs
		self._repr_format: str = (
			self.__class__.__name__ + '(lib_paths=%r, not_found=%r, other_libs=%r)'
		)

	@property
	def lib_paths(self) -> MutableMapping[str, str]:
		return self._lib_paths

	@property
	def not_found(self) -> MutableSet[str]:
		return self._not_found

	@property
	def other_libs(self) -> MutableSet[str]:
		return self._other_libs

	def __repr__(self) -> str:
		if not self:
			return self.__class__.__name__ + '()'
		return self._repr_format % (self.lib_paths, self.not_found, self.other_libs)


class ElfInfoSource(Enum):
	READELF = 1
	OBJDUMP = 2
	LIEF = 3


TableValueT = TypeVar('TableValueT')


class DynamicSectionTableBase(Generic[TableValueT]):

	def __init__(self, table: OrderedDict[str, MutableSequence[Any]]):
		self._table: OrderedDict[str, MutableSequence[TableValueT]] = table

	def _len_group(self) -> int:
		sz = 0
		for seq in self._table.values():
			if len(seq) > 0:
				sz += 1
		return sz

	def _len_indiv(self) -> int:
		sz = 0
		for seq in self._table.values():
			sz += len(seq)
		return sz


class DynamicSectionKeysView(DynamicSectionTableBase[TableValueT], KeysView[str], Reversible[str]):

	def __init__(self, table: OrderedDict[str, MutableSequence[TableValueT]]):
		KeysView.__init__(self, table)  # pylint: disable=W0233
		DynamicSectionTableBase.__init__(self, table)  # pylint: disable=W0233

	def __contains__(self, key: str) -> bool:
		try:
			return len(self._mapping[key]) > 0
		except KeyError:
			return False

	def __len__(self) -> int:
		return self._len_group()

	def _iter(  # pylint: disable=R0201
		self,
		iterator: Iterator[Tuple[str, Sequence[TableValueT]]]
	) -> Iterator[str]:
		for tag, seq in iterator:
			if len(seq) > 0:
				yield tag

	def __iter__(self) -> Iterator[str]:
		return self._iter(self._table.items())

	def __reversed__(self) -> Iterator[str]:
		return self._iter(reversed(self._table.items()))


class DynamicSectionItemsView(
	DynamicSectionTableBase[TableValueT],
	ItemsView[str, MutableSequence[TableValueT]],
	Reversible[Tuple[str, MutableSequence[TableValueT]]]
):

	def __init__(self, table: OrderedDict[str, MutableSequence[TableValueT]]):
		ItemsView.__init__(self, table)  # pylint: disable=W0233
		DynamicSectionTableBase.__init__(self, table)  # pylint: disable=W0233

	def __contains__(self, item: Tuple[str, Sequence[TableValueT]]) -> bool:
		tag, iseq = item
		if iseq is None or len(iseq) < 1:
			return False
		try:
			tseq = self._table[tag]
		except KeyError:
			return False
		else:
			if tseq is None or len(tseq) < 1:
				return False
			return tseq is iseq or tseq == iseq

	def __len__(self) -> int:
		return self._len_group()

	def _iter(  # pylint: disable=R0201
		self,
		iterator: Iterator[Tuple[str, MutableSequence[TableValueT]]]
	) -> Iterator[Tuple[str, MutableSequence[TableValueT]]]:
		for tag, seq in iterator:
			if len(seq) > 0:
				yield (tag, seq)

	def __iter__(self) -> Iterator[Tuple[str, MutableSequence[TableValueT]]]:
		return self._iter(self._table.items())

	def __reversed__(self) -> Iterator[Tuple[str, MutableSequence[TableValueT]]]:
		return self._iter(reversed(self._table.items()))


class DynamicSectionIndivItemsView(
	DynamicSectionTableBase[TableValueT],
	ItemsView[str, TableValueT],
	Reversible[Tuple[str, TableValueT]]
):

	def __init__(self, table: OrderedDict[str, Sequence[TableValueT]]):
		ItemsView.__init__(self, table)  # pylint: disable=W0233
		DynamicSectionTableBase.__init__(self, table)  # pylint: disable=W0233

	def __contains__(self, item: Tuple[str, TableValueT]) -> bool:
		tag, ival = item
		try:
			seq = self._table[tag]
		except KeyError:
			return False
		else:
			return any(sval is ival or sval == ival for sval in seq)

	def __len__(self) -> int:
		return self._len_indiv()

	def __iter__(self) -> Iterator[Tuple[str, TableValueT]]:
		for tag, seq in self._table.items():
			for value in seq:
				yield (tag, value)

	def __reversed__(self) -> Iterator[Tuple[str, TableValueT]]:
		for tag, seq in reversed(self._table.items()):
			for value in reversed(seq):
				yield (tag, value)


class DynamicSectionValuesView(
	DynamicSectionTableBase[TableValueT],
	ValuesView[MutableSequence[TableValueT]],
	Reversible[MutableSequence[TableValueT]]
):

	def __init__(self, table: OrderedDict[str, MutableSequence[TableValueT]]):
		ValuesView.__init__(self, table)  # pylint: disable=W0233
		DynamicSectionTableBase.__init__(self, table)  # pylint: disable=W0233

	def __contains__(self, value: Sequence[TableValueT]) -> bool:
		if value is None or len(value) < 1:
			return False
		return any(seq is value or seq == value for seq in self._table.values())

	def __len__(self) -> int:
		return self._len_group()

	def _iter(  # pylint: disable=R0201
		self,
		iterator: Iterator[MutableSequence[TableValueT]]
	) -> Iterator[MutableSequence[TableValueT]]:
		for seq in iterator:
			if len(seq) > 0:
				yield seq

	def __iter__(self) -> Iterator[MutableSequence[TableValueT]]:
		return self._iter(self._table.values())

	def __reversed__(self) -> Iterator[MutableSequence[TableValueT]]:
		return self._iter(reversed(self._table.values()))


class DynamicSectionIndivValuesView(
	DynamicSectionTableBase[TableValueT],
	ValuesView[TableValueT],
	Reversible[TableValueT]
):

	def __init__(self, table: OrderedDict[str, Sequence[TableValueT]]):
		ValuesView.__init__(self, table)  # pylint: disable=W0233
		DynamicSectionTableBase.__init__(self, table)  # pylint: disable=W0233

	def __contains__(self, value: TableValueT) -> bool:
		return any(value in seq for seq in self._table.values())

	def __len__(self) -> int:
		return self._len_indiv()

	def __iter__(self) -> Iterator[Tuple[str, TableValueT]]:
		for seq in self._table.values():
			yield from seq

	def __reversed__(self) -> Iterator[Tuple[str, TableValueT]]:
		for seq in reversed(self._table.values()):
			yield from reversed(seq)


# TODO: This could serve as the basis for a generic multi-value map class
class DynamicSectionTable(
	DynamicSectionTableBase[TableValueT],
	MutableMapping[str, MutableSequence[TableValueT]],
	Reversible[str]
):

	@overload
	def __init__(
		self,
		source: Optional[ElfInfoSource] = None
	):
		pass

	@overload
	def __init__(
		self,
		mapping: Mapping[str, Iterable[TableValueT]],
		source: Optional[ElfInfoSource] = None
	):
		pass

	@overload
	def __init__(
		self,
		iterable: Iterable[Tuple[str, Iterable[TableValueT]]],
		source: Optional[ElfInfoSource] = None
	):
		pass

	def __init__(
		self,
		*args,
		source: Optional[ElfInfoSource] = None
	):
		if len(args) > 1:
			raise ValueError('Too many positional arguments')

		try:
			MutableMapping.__init__(self)  # pylint: disable=W0233
		except TypeError:
			pass
		DynamicSectionTableBase.__init__(self, OrderedDict(*args))  # pylint: disable=W0233

		self._source = source
		self._lock: Lock = Lock()

	@property
	def source(self) -> ElfInfoSource:
		return self._source

	def setdefault(
		self,
		key: str,
		default: Optional[Sequence[TableValueT]] = None
	) -> MutableSequence[TableValueT]:
		if not isinstance(key, str):
			raise TypeError('key must be string')
		with self._lock:
			if key in self._table:
				seq = self._table[key]
				if len(seq) < 0:
					return seq
				else:
					if default is not None:
						seq = list(default)
						self._table[key] = seq
					self._table.move_to_end(key)
					return seq
			if default:
				seq = list(default)
			else:
				seq = []
			self._table[key] = seq
			self._table.move_to_end(key)
			return seq

	def __getitem__(self, key: str) -> MutableSequence[TableValueT]:
		return self.setdefault(key)

	def __len__(self) -> int:
		return self._len_indiv()

	def __contains__(self, key: str) -> bool:
		try:
			return len(self._table[key]) > 0
		except KeyError:
			return False

	def __iter__(self) -> Iterator[str]:
		return iter(self.keys())

	def __reversed__(self) -> Iterator[str]:
		return iter(reversed(self.keys()))

	def __setitem__(self, key: str, value: MutableSequence[TableValueT]) -> NoReturn:
		with self._lock:
			if value is None and value in self._table:
				del self._table
			else:
				self._table[key] = value
				if len(value) < 1:
					self._table.move_to_end(key)

	def __delitem__(self, key: str) -> NoReturn:
		try:
			del self._table[key]
		except (KeyError, IndexError):
			if not isinstance(key, str):
				raise TypeError('key must be string')  # pylint: disable=W0707

	# NOTE: while __getitem__() will create entries in the underlying map, get() is a read-only
	# operation. If default is returned, updating the returned sequence will not update the data in
	# the table.
	# see also: setdefault
	def get(
		self,
		key: str,
		default: Optional[Sequence[TableValueT]] = None
	) -> Optional[Union[MutableSequence[TableValueT], Sequence[TableValueT]]]:
		with self._lock:
			if key in self._table:
				seq = self._table[key]
				if len(seq) > 0:
					return seq
			return default

	def add(self, key: str, value: TableValueT) -> NoReturn:
		self[key].append(value)

	# TODO: implement DynamicSectionTable.pop
	def pop(self, key: str, default=None):
		raise NotImplementedError

	# TODO: implement DynamicSectionTable.popitem
	def popitem(self, last: bool = True) -> Tuple[str, TableValueT]:  # noqa: W0221
		raise NotImplementedError

	def popentry(
		self,
		key: str,
		last: bool = True,
		default: Optional[TableValueT] = None
	) -> Optional[TableValueT]:
		with self._lock:
			if key in self._table:
				seq = self._table[key]
				if len(seq) < 1:
					self._table.move_to_end(key)
					return default
				popidx = -1 if last else 0
				return seq.pop(index=popidx)
			return default

	def popgroup(
		self,
		key: str,
		default: Sequence[TableValueT] = None
	) -> Union[MutableSequence[TableValueT], Sequence[TableValueT]]:
		with self._lock:
			if key in self._table:
				seq = self._table[key]
				if len(seq) < 1:
					if default:
						seq = default
					else:
						seq = []
				del self._table[key]
				return seq
			if default:
				return default
			return []

	def clear(self) -> NoReturn:
		with self._lock:
			for seq in self._table.values():
				seq.clear()

	# TODO: implement DynamicSectionTable.update
	def update(self, *args, **kwargs):
		raise NotImplementedError

	def keys(self) -> DynamicSectionKeysView:
		return DynamicSectionKeysView(self._table)

	def items(self) -> DynamicSectionIndivItemsView[TableValueT]:
		return DynamicSectionIndivItemsView[TableValueT](self._table)

	def values(self) -> DynamicSectionIndivValuesView[TableValueT]:
		return DynamicSectionIndivValuesView[TableValueT](self._table)

	def itemgroups(self) -> DynamicSectionItemsView[TableValueT]:
		return DynamicSectionItemsView[TableValueT](self._table)

	def valuegroups(self) -> DynamicSectionValuesView[TableValueT]:
		return DynamicSectionValuesView[TableValueT](self._table)


class ElfVersionRefGroup(FrozenList[str]):

	@overload
	def __init__(self, soname: str):
		pass

	@overload
	def __init__(self, soname: str, entries: Iterable[str]):
		pass

	def __init__(self, soname: str, *args: Iterable[str]):
		if len(args) > 1:
			raise ValueError('Too many positional arguments')
		elif len(args) == 1 and args[0] is not None:
			super().__init__(args[0])
		else:
			super().__init__()

		self._soname = soname

	@property
	def soname(self) -> str:
		return self._soname


class ElfInfo():

	def __init__(
		self,
		*,
		source: Optional[ElfInfoSource] = None,
		rpath: Optional[Sequence[str]] = None,
		runpath: Optional[Sequence[str]] = None,
		imported_libs: Optional[Sequence[str]] = None,
		has_origin_flag: bool = False,
		has_origin_flag_1: bool = False,
		version_refs: Optional[Sequence[ElfVersionRefGroup]] = None
	):
		self._source = source
		self._rpath = None if rpath is None else FrozenList(rpath)
		self._runpath = None if runpath is None else FrozenList(runpath)
		self._imported_libs = FrozenList(imported_libs) if imported_libs else FrozenList()
		self._has_origin_flag = has_origin_flag
		self._has_origin_flag_1 = has_origin_flag_1
		self._version_refs = FrozenList(version_refs) if version_refs else FrozenList()

	@property
	def source(self) -> Optional[ElfInfoSource]:
		return self._source

	@property
	def rpath(self) -> Optional[Sequence[str]]:
		return self._rpath

	@property
	def runpath(self) -> Optional[Sequence[str]]:
		return self._runpath

	@property
	def imported_libs(self) -> Sequence[str]:
		return self._imported_libs

	@property
	def has_origin_flag(self) -> bool:
		return self._has_origin_flag

	@property
	def has_origin_flag_1(self) -> bool:
		return self._has_origin_flag_1

	@property
	def version_refs(self) -> Sequence[ElfVersionRefGroup]:
		return self._version_refs


# TODO: Implement fetching DT_NEEDED entries using objdump.
# TODO: Implement fetching DF_ORIGIN status using objdump.
# TODO: Implement fetching rpath and runpath using readelf and objdump.
# TODO: (maybe?) Implement fetching of symbol versioning info using readelf and objdump.
# A lot of the regex for all three is already done at the bottom of this class.
# TODO: Concurrency, baby!
# TODO: Identify earliest version of llvm-objdump to support display of dynamic section
# TODO: Identify earliest version of llvm-readelf to support GNU output for -V
# TODO: Use elf info to make strip util and runpath util smarter
class ElfInfoUtil(PackagerUtilBase[ElfInfoUtilOptionsBase]):

	# readelf and objdump are not used currently, but in the future, we may need to support
	# getting the soname from a library, and these would come in handy.
	def __init__(
		self,
		context: PackagerContext[ElfInfoUtilOptionsBase],
		executor: Optional[Executor] = None,
		raise_if_ldd_not_found: bool = False,
		raise_if_elftools_not_found: bool = False
	):
		super().__init__(context, executor)

		env = self.fill_out_envdict()

		ldd_path = self.find_tool(
			['ldd'],
			self.options.ldd_path,
			raise_if_not_found=raise_if_ldd_not_found,
			env=env
		)
		readelf_path = self.find_tool(
			['readelf', 'eu-readelf', 'llvm-readelf'],
			self.options.readelf_path,
			raise_if_not_found=False,
			prettyname='readelf',
			env=env
		)
		objdump_path = self.find_tool(
			['objdump', 'llvm-objdump'],
			self.options.objdump_path,
			raise_if_not_found=False,
			env=env
		)

		self._tool_state = ElfInfoToolState(
			PackagerToolState(is_usable=bool(ldd_path), path=ldd_path),
			PackagerToolState(is_usable=self.lief_info.lief_usable),
			PackagerToolState(is_usable=bool(readelf_path), path=readelf_path),
			PackagerToolState(is_usable=bool(objdump_path), path=objdump_path)
		)

		if raise_if_elftools_not_found and not self.tool_state.elftools_usable:
			raise OSError(
				errno.ENOPKG,
				'lief functionality not available and neither readelf nor objdump could be found'
			)

	@property
	def tool_state(self) -> ElfInfoToolState:
		return self._tool_state

	_re_ldd_unused_dep = re.compile(r'^[ \t]+(.+)$', re.MULTILINE)

	def get_unused_dependencies(
		self,
		bin_path: str,
		return_full_paths: bool = False
	) -> MutableSequence[str]:
		ilib = self.ilib

		env = self.fill_out_envdict()
		env['LANG'] = 'C'

		# FIXME: this doesn't work with musl ldd
		ldd_args = [self.tool_state.ldd.path, '-r', '-u', bin_path]

		ldd_out, ldd_err, ldd_retcode = ilib.execute_command_permissive(ldd_args, env=env)

		# ldd returns 1 if unneeded libraries are found
		if ldd_retcode == 1:
			ldd_retcode = 0
		ilib.check_command_return(ldd_args, ldd_out, ldd_err, ldd_retcode, env=env)

		unused_deps = self._re_ldd_unused_dep.findall(ldd_out)

		if return_full_paths:
			return unused_deps

		return [path.basename(ulibpath) for ulibpath in unused_deps]

	# if the output format of ldd/ld.so ever changes, this will probably break
	_rp_ldd_soname = r'^[ \t]+(?P<soname>[^><\n]+)'
	_rp_ldd_address = r'\(0x[0-9a-f]+(, 0x[0-9a-f]+)?\)$'
	_re_ldd_lib_path = re.compile(
		_rp_ldd_soname + r'[ \t]+=>[ \t]+(?P<path>[^><\n]*)[ \t]+' + _rp_ldd_address,
		re.MULTILINE
	)
	_re_ldd_virt_result = re.compile(
		_rp_ldd_soname + r'[ \t]+' + _rp_ldd_address,
		re.MULTILINE
	)
	_re_ldd_not_found = re.compile(
		_rp_ldd_soname + r'[ \t]+=>[ \t]+not found$' + _rp_ldd_address,
		re.MULTILINE
	)

	def get_lib_paths(self, bin_path: str) -> LddLibPaths:
		l = logging.getLogger(__name__)  # noqa: VNE001,E741
		ilib = self.ilib

		env = self.fill_out_envdict()
		env['LANG'] = 'C'

		lib_paths = {}
		not_found = set()
		other_libs = set()

		# FIXME: musl ldd behaves differently for missing libs
		ldd_out, ldd_err = ilib.execute_command(  # pylint: disable=W0612
			[self.tool_state.ldd.path, bin_path],
			env=env
		)

		for ldd_out_l in ldd_out.splitlines():
			ldd_out_l_re = self._re_ldd_lib_path.match(ldd_out_l)

			if not ldd_out_l_re:
				ldd_out_l_re = self._re_ldd_virt_result.match(ldd_out_l)
				if not ldd_out_l_re:
					ldd_out_l_re = self._re_ldd_not_found.match(ldd_out_l)
					if ldd_out_l_re:
						not_found.add(ldd_out_l_re.groupdict()['soname'])
					continue

			ldd_out_l_re_g = ldd_out_l_re.groupdict()
			soname = ldd_out_l_re_g['soname']
			libpath = ldd_out_l_re_g.get('path', None)

			if path.isabs(soname):
				other_libs.add(path.basename(soname))
				continue
			elif not libpath:
				other_libs.add(soname)
				continue
			elif not path.exists(libpath):
				l.error('path %s from ldd does not exist', libpath)
				continue

			lib_paths[soname] = libpath

		return LddLibPaths(lib_paths, not_found, other_libs)

	_rp_readelf_d_tag_int = r'(?P<tag_int>0x[0-9a-f]+)?(?(tag_int)[ \t]+)'
	_rp_readelf_d_tag_name = r'(?P<tagn_lparen>\()?(?P<tag_name>[A-Z0-9_]+)(?(tagn_lparen)(?P<tagn_rparen>\)))'
	_rp_readelf_d_value = r'[ \t]+(?P<value>[^\n]*)$'
	_re_readelf_dynsect = re.compile(
		r'^[ \t]+' + _rp_readelf_d_tag_int + _rp_readelf_d_tag_name + _rp_readelf_d_value,
		re.MULTILINE
	)

	def _get_dynamic_section_readelf(self, bin_path: str) -> DynamicSectionTable[str]:
		ilib = self.ilib

		dynsect = DynamicSectionTable[str](source=ElfInfoSource.READELF)

		env = self.fill_out_envdict()
		env['LANG'] = 'C'

		readelf_out, readelf_err = ilib.execute_command(  # pylint: disable=W0612
			[self.tool_state.readelf.path, '-W', '-d', bin_path],
			env=env
		)

		for readelf_out_l in readelf_out.splitlines():
			readelf_out_l_re = self._re_readelf_dynsect.match(readelf_out_l)
			if not readelf_out_l_re:
				continue
			readelf_out_l_re_g = readelf_out_l_re.groupdict()
			dynsect.add(readelf_out_l_re_g['tag_name'], readelf_out_l_re_g.get('value'))

		return dynsect

	_re_readelf_d_sharedlib = re.compile(
		r'^Shared library:\s+\[(?P<soname>[^><\n\]]+)\]$',
		re.MULTILINE
	)

	def _get_elfinfo_readelf(self, bin_path: str) -> MutableSequence[str]:
		dynsect = self._get_dynamic_section_readelf(bin_path)

		# TODO: readelf rpath/runpath extraction
		# looks like most readelf impls do 'Library runpath: []' or 'Library rpath: []'.
		# old llvm-readelf just does the string

		imported_libs = []
		d_neededs = dynsect['NEEDED']
		for needed in d_neededs:
			needed_re = self._re_readelf_d_sharedlib.match(needed)
			needed_re_g = needed_re.groupdict()
			imported_libs.append(needed_re_g['soname'])

		has_origin_flag = False
		if 'FLAGS' in dynsect and len(dynsect['FLAGS']) > 0:
			# Assume there is only one FLAGS entry
			has_origin_flag = 'ORIGIN' in dynsect['FLAGS'][0]

		has_origin_flag_1 = False
		if 'FLAGS_1' in dynsect and len(dynsect['FLAGS_1']) > 0:
			# Assume there is only one FLAGS entry
			has_origin_flag_1 = 'ORIGIN' in dynsect['FLAGS_1'][0]

		# TODO: readelf version ref extraction
		# regex is at the bottom of the class

		return ElfInfo(
			source=ElfInfoSource.READELF,
			imported_libs=imported_libs,
			has_origin_flag=has_origin_flag,
			has_origin_flag_1=has_origin_flag_1
		)

	def _get_elfinfo_objdump(self, bin_path: str) -> ElfInfo:
		raise NotImplementedError

	def _get_elfinfo_lief(self, bin_path: str) -> ElfInfo:
		lib = self.lief.parse(bin_path)

		rpath = None
		if lib.has(self.lief.ELF.DYNAMIC_TAGS.RPATH):
			# Assume there is only one RPATH entry
			rentry = lib.get(self.lief.ELF.DYNAMIC_TAGS.RPATH)
			rpath = rentry.paths

		runpath = None
		if lib.has(self.lief.ELF.DYNAMIC_TAGS.RUNPATH):
			# Assume there is only one RUNPATH entry
			rentry = lib.get(self.lief.ELF.DYNAMIC_TAGS.RUNPATH)
			runpath = rentry.paths

		imported_libs = list(lib.libraries)

		has_origin_flag = False
		if lib.has(self.lief.ELF.DYNAMIC_TAGS.FLAGS):
			# Assume there is only one FLAGS entry
			fentry = lib.get(self.lief.ELF.DYNAMIC_TAGS.FLAGS)
			has_origin_flag = int(self.lief.ELF.DYNAMIC_FLAGS.ORIGIN) in fentry.flags

		has_origin_flag_1 = False
		if lib.has(self.lief.ELF.DYNAMIC_TAGS.FLAGS_1):
			# Assume there is only one FLAGS_1 entry
			fentry = lib.get(self.lief.ELF.DYNAMIC_TAGS.FLAGS_1)
			has_origin_flag_1 = int(self.lief.ELF.DYNAMIC_FLAGS_1.ORIGIN) in fentry.flags

		version_refs = []
		for verreq in lib.symbols_version_requirement:
			reqentries = [e.name for e in verreq.get_auxiliary_symbols()]
			version_refs.append(ElfVersionRefGroup(verreq.name, reqentries))

		return ElfInfo(
			source=ElfInfoSource.LIEF,
			rpath=rpath,
			runpath=runpath,
			imported_libs=imported_libs,
			has_origin_flag=has_origin_flag,
			has_origin_flag_1=has_origin_flag_1,
			version_refs=version_refs
		)

	def get_elfinfo(self, bin_path: str) -> ElfInfo:
		if self.tool_state.readelf:
			return self._get_elfinfo_readelf(bin_path)
		elif self.tool_state.lief:
			return self._get_elfinfo_lief(bin_path)
		elif self.tool_state.objdump:
			raise NotImplementedError('fetching elf info with objdump is not implemented')
		else:
			raise RuntimeError('get_elfinfo called but no tool available')

	# readelf -W -V [lib]
	# TODO: old llvm-readelf doesn't output GNU format for -V
	_rp_readelf_v_line_ind = r'^(?<indent>[ \t]+)'

	_rp_readelf_v_offset = r'(?P<offset>0x[0-9a-f]+):'
	# eu-readelf does not add additional padding to verentries
	_rp_readelf_v_pad = r'(?<spacing>[ \t]+)'
	_rp_readelf_v_structver = r'Version:[ \t]+(?P<structver>\d+)'
	_rp_readelf_v_implib = r'[ \t]+File:[ \t]+(?P<soname>[^><\s:]+)'
	_rp_readelf_v_entqty = r'[ \t]+Cnt:[ \t]+(?P<entqty>\d+)'

	_rp_readelf_v_name = r'Name:[ \t]+(?P<name>[^\s]+)'
	# TODO: we don't actually know what flags look like when not none
	# TODO: assume flags can have spaces in the value, but value will be followed by two spaces
	_rp_readelf_v_flags = r'[ \t]+Flags:[ \t]+(?P<flags>[^\s]+)'
	_rp_readelf_v_index = r'[ \t]+Version:[ \t]+(?P<index>\d+)'

	# objdump -w -p [lib]
	_rp_objdump_p_ind = r'^(?<indent>[ \t]*)'
	_rp_objdump_p_line = r'(?P<line>[^\n]*)$'

	# 'Dynamic Section:'
	_rp_objdump_dynsect_tag_name = r'(?P<tag_name>[A-Z0-9_]+)'
	_rp_objdump_dynsect_value = r'[ \t]+(?P<value>[^\n]*)$'

	# 'Version References:'
	_rp_objdump_verref_implib = r'required from (?P<soname>[^><\n:]+):$'
	_rp_od_vr_ent_hash = r'(?P<hash>0x[0-9a-f]+)'
	_rp_od_vr_ent_flags = r'[ \t]+(?P<flags>0x[0-9a-f]+)'
	_rp_od_vr_ent_index = r'[ \t]+(?P<index>\d+)'
	_rp_od_vr_ent_name = r'[ \t]+(?P<name>[^\s]+)'
