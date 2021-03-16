#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Compatibility shims for some python stdlib stuff."""

from __future__ import print_function
import sys, errno  # noqa: I201; pylint: disable=C0410

if __name__ == '__main__':
	print(__file__ + ': This module is not meant to be invoked directly.',  # noqa: T001
	      file=sys.stderr)
	sys.exit(errno.ELIBEXEC)

import platform  # noqa: I100,I202
import site
from threading import Lock

import pkg_resources

site_import_paths = list(site.getsitepackages())
if site.ENABLE_USER_SITE:
	site_import_paths.insert(0, site.getusersitepackages())

Version = type(pkg_resources.Distribution(version='0.0.0').parsed_version)

_str_ver_cache = {}
_str_ver_cache_lock = Lock()
_inf_ver_cache = {}
_inf_ver_cache_lock = Lock()


def parse_version(ver_str: str) -> Version:
	if ver_str in _str_ver_cache:
		return _str_ver_cache[ver_str]
	with _str_ver_cache_lock:
		# check one more time in case it was added while waiting on the lock
		if ver_str in _str_ver_cache:
			return _str_ver_cache[ver_str]
		ver = Version(ver_str)
		_str_ver_cache[ver_str] = ver
		return ver


def version_info_to_version(
	major: int = 0,
	minor: int = 0,
	micro: int = 0,
	releaselevel: str = 'final',
	serial: int = 0
) -> Version:
	version_info = (major, minor, micro, releaselevel, serial)
	if version_info in _inf_ver_cache:
		return _inf_ver_cache[version_info]
	with _inf_ver_cache_lock:
		# check one more time in case it was added while waiting on the lock
		if version_info in _inf_ver_cache:
			return _inf_ver_cache[version_info]
		if releaselevel == 'final':
			pre_fmt = ''
		elif releaselevel == 'alpha':
			pre_fmt = 'a{3}'
		elif releaselevel == 'beta':
			pre_fmt = 'b{3}'
		elif releaselevel == 'candidate':
			pre_fmt = 'rc{3}'
		else:
			raise ValueError('unrecognized releaselevel value')
		ver_fmt = '{0}.{1}.{2}' + pre_fmt
		ver_str = ver_fmt.format(major, minor, micro, serial)
		ver = parse_version(ver_str)
		_inf_ver_cache[version_info] = ver
		return ver


try:
	python_version = version_info_to_version(
		sys.version_info.major,
		sys.version_info.minor,
		sys.version_info.micro,
		sys.version_info.releaselevel,
		sys.version_info.serial
	)
except Exception:  # noqa: B902,S110; pylint: disable=W0703; nosec
	python_version = parse_version(platform.python_version())

# pylint: disable=C0412
import collections  # noqa: I100,I202
import typing
from contextlib import suppress as suppress_ex

if sys.version_info < (3, 9):
	from typing import ByteString, Callable, Collection, Container, Generator, Iterable
	from typing import Iterator, MutableSequence, MutableSet, Reversible, Sequence
	from typing import ItemsView, KeysView, Mapping, MappingView, MutableMapping, ValuesView
else:
	from collections.abc import ByteString, Callable, Collection, Container, Generator, Iterable
	from collections.abc import Iterator, MutableSequence, MutableSet, Reversible, Sequence
	from collections.abc import ItemsView, KeysView, Mapping, MappingView, MutableMapping, ValuesView

# Utility stuff
if sys.version_info < (3, 7):
	try:
		from typing import _generic_new
	except ImportError:
		from typing import _gorg
		def _generic_new(base_cls, cls, *args, **kwds):  # noqa: ANN001,ANN002,ANN003,ANN202
			if cls.__origin__ is None:
				return base_cls.__new__(cls)
			else:
				origin = _gorg(cls)
				obj = base_cls.__new__(origin)
				with suppress_ex(AttributeError):
					obj.__orig_class__ = cls
				obj.__init__(*args, **kwds)
				return obj
	try:
		from typing import _geqv
	except ImportError:
		def _geqv(cls1, cls2):  # noqa: ANN001,ANN202
			return cls1._gorg is cls2  # pylint: disable=W0212
if (3, 7) <= sys.version_info < (3, 9):
	from typing import _alias
if sys.version_info < (3, 9):
	from typing import KT, T, VT

# NoReturn introduced in 3.6.2
try:
	from typing import NoReturn
except ImportError:
	NoReturn = type(None)

# ChainMap and Counter were introduced in 3.6.1 and deprecated in 3.9.
if sys.version_info < (3, 9):
	try:
		from typing import ChainMap
	except ImportError:
		class ChainMap(collections.ChainMap, MutableMapping[KT, VT], extra=collections.ChainMap):
			__slots__ = ()
			def __new__(cls, *args, **kwargs):  # noqa: ANN002,ANN003,ANN204
				if _geqv(cls, ChainMap):
					return collections.ChainMap(*args, **kwargs)
				return _generic_new(collections.ChainMap, cls, *args, **kwargs)
	try:
		from typing import Counter
	except ImportError:
		class Counter(collections.Counter, typing.Dict[T, int], extra=collections.Counter):  # pylint: disable=W0223
			__slots__ = ()
			def __new__(cls, *args, **kwargs):  # noqa: ANN002,ANN003,ANN204
				if _geqv(cls, Counter):
					return collections.Counter(*args, **kwargs)
				return _generic_new(collections.Counter, cls, *args, **kwargs)
else:
	from collections import ChainMap, Counter

# OrderedDict was introduced in 3.7.2 and deprecated in 3.9
if sys.version_info < (3, 7):
	class OrderedDict(collections.OrderedDict, MutableMapping[KT, VT], extra=collections.OrderedDict):
		__slots__ = ()
		def __new__(cls, *args, **kwargs):  # noqa: ANN002,ANN003,ANN204
			if _geqv(cls, OrderedDict):
				return collections.OrderedDict(*args, **kwargs)
			return _generic_new(collections.OrderedDict, cls, *args, **kwargs)
elif sys.version_info < (3, 7, 2):
	# For whatever reason, pylint can crash if we access collections.OrderedDict directly here
	from collections import OrderedDict as OrderedDict_collections
	OrderedDict = _alias(OrderedDict_collections, (KT, VT))
elif sys.version_info < (3, 9):
	# Whatever Wing uses to underline problematic code thinks typing.OrderedDict doesn't exist
	OrderedDict = typing.OrderedDict  # noqa: TYP006
else:
	OrderedDict = collections.OrderedDict

# Protocol was introduced in 3.8.
# _Protocol is an internal type we can use as a shim for < 3.8
if sys.version_info < (3, 8):
	from typing import _Protocol as Protocol  # pylint: disable=E0611
else:
	from typing import Protocol

# AbstractSet -> Set
# defaultdict -> DefaultDict
# tuple -> Tuple
# type -> Type
if sys.version_info < (3, 9):
	from typing import AbstractSet as Set
	from typing import DefaultDict
	from typing import Tuple
	from typing import Type
else:
	from collections.abc import Set
	from collections import defaultdict as DefaultDict
	Tuple = tuple
	Type = type

# platform.linux_distribution was deprecated in 3.5 and removed in 3.8
# the distro package is the recommended replacement
try:
	from distro import linux_distribution
except ImportError:
	if sys.version_info < (3, 8):
		from platform import linux_distribution
	else:
		raise

__all__ = [
	'Version',
	'parse_version',
	'version_info_to_version',
	'python_version',
	'ByteString',
	'Callable',
	'Collection',
	'Container',
	'Generator',
	'Iterable',
	'Iterator',
	'MutableSequence',
	'MutableSet',
	'Sequence',
	'ItemsView',
	'KeysView',
	'Mapping',
	'MappingView',
	'MutableMapping',
	'ValuesView',
	'NoReturn',
	'Reversible',
	'OrderedDict',
	'Protocol',
	'Set',
	'DefaultDict',
	'Tuple',
	'Type',
	'linux_distribution'
]
