#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import sys, errno  # noqa: I201,I001; pylint: disable=C0410

if __name__ == '__main__':
	print(__file__ + ': This module is not meant to be invoked directly.',  # noqa: T001
	      file=sys.stderr)
	sys.exit(errno.ELIBEXEC)

import logging  # noqa: I100,I202
from abc import abstractmethod
from contextlib import suppress as suppress_ex
from types import ModuleType
from typing import Union

import pkg_resources

from compat_shims import NoReturn, Protocol
from compat_shims import Version as PkgVersion

try:
	from lief import Binary as LiefBinT
	from lief.ELF import Binary as LiefElfT
except ImportError:
	from typing import Any as LiefBinT
	from typing import Any as LiefElfT


__all__ = [
	'lief_elf_module_proto',
	'lief_module_proto',
	'LiefElfModuleT',
	'LiefModuleT',
	'lief_ver_info'
]


class lief_elf_module_proto(Protocol):

	@staticmethod
	@abstractmethod
	def parse(*args, **kwargs) -> LiefElfT:
		raise NotImplementedError()


LiefElfModuleT = Union[lief_elf_module_proto, ModuleType]


class lief_module_proto(Protocol):

	@staticmethod
	@abstractmethod
	def parse(*args, **kwargs) -> LiefBinT:
		raise NotImplementedError()

	@property
	@staticmethod
	@abstractmethod
	def ELF() -> LiefElfModuleT:  # noqa: N802; pylint: disable=C0103
		raise NotImplementedError


LiefModuleT = Union[lief_module_proto, ModuleType]


class lief_ver_info():
	_min_req_ver = PkgVersion('0.10.0')
	_min_rec_ver = PkgVersion('0.12.0')

	_std_rebuild_size_warn_threshold = (48 * 1024 * 1024)
	_slow_rebuild_size_warn_threshold = (36 * 1024 * 1024)
	_reallyslow_rebuild_size_warn_threshold = (16 * 1024 * 1024)

	#@classmethod
	@property
	def minimum_required_version(cls) -> PkgVersion:
		return cls._min_req_ver

	#@classmethod
	@property
	def minimum_recommended_version(cls) -> PkgVersion:
		return cls._min_rec_ver

	@classmethod
	def _get_lief_version(cls, lief: LiefElfModuleT) -> PkgVersion:
		lief_ver_str_meta = None
		lief_ver_str_tag = None
		lief_ver_str_dunder = None

		with suppress_ex(Exception):
			return pkg_resources.get_distribution('lief').parsed_version

		if hasattr(lief, '__tag__'):
			with suppress_ex(Exception):
				lief_ver_str_tag = lief.__tag__
				return PkgVersion(lief_ver_str_tag)

		if hasattr(lief, '__version__'):
			with suppress_ex(Exception):
				lief_ver_str_dunder = lief.__version__
				return PkgVersion(lief_ver_str_tag)

		if lief_ver_str_dunder:
			with suppress_ex(Exception):
				lief_ver_str_dunder = lief_ver_str_dunder.partition('-')[0]
				return PkgVersion(lief_ver_str_dunder)

		if lief_ver_str_meta:
			with suppress_ex(Exception):
				lief_ver_str_meta = lief_ver_str_meta.partition('-')[0]
				return PkgVersion(lief_ver_str_meta)

		return None

	def __init__(self, lief: LiefElfModuleT):
		self._lief: LiefElfModuleT = lief
		self._lief_imported = False
		self._lief_version = None
		self._lief_usable = False
		self._rebuild_is_reallyslow = None
		self._rebuild_is_slow = None
		self._old_logger_api = None
		self._not_threadsafe = None

		if lief:
			self._lief_imported = True
			self._lief_version = self._get_lief_version(lief)
			if self._lief_version is None:
				self._lief_usable = True
			elif self._lief_version < self.minimum_required_version:
				self._lief_usable = False
			else:
				self._lief_usable = True

				older_than_0_11_0 = self._lief_version < PkgVersion('0.11.0')
				older_than_0_12_0 = self._lief_version < PkgVersion('0.12.0')

				self._rebuild_is_slow = older_than_0_12_0
				self._rebuild_is_reallyslow = older_than_0_11_0
				self._old_logger_api = older_than_0_11_0
				self._not_threadsafe = older_than_0_11_0

	@property
	def lief_imported(self) -> bool:
		return self._lief_imported

	@property
	def lief_version(self) -> PkgVersion:
		return self._lief_version

	@property
	def lief_usable(self) -> bool:
		return self._lief_usable

	@property
	def rebuild_is_slow(self) -> bool:
		return self._rebuild_is_slow

	@property
	def rebuild_is_reallyslow(self) -> bool:
		return self._rebuild_is_reallyslow

	@property
	def old_logger_api(self) -> bool:
		return self._old_logger_api

	@property
	def not_threadsafe(self) -> bool:
		return self._not_threadsafe

	# Settable instance property because we might want to allow the value to be
	# tweaked for specific operations
	@property
	def std_rebuild_size_warn_threshold(self) -> int:
		return self._std_rebuild_size_warn_threshold
	@std_rebuild_size_warn_threshold.setter  # noqa: E301
	def std_rebuild_size_warn_threshold(self, value: int) -> NoReturn:
		self._std_rebuild_size_warn_threshold = value

	# Settable instance property because we might want to allow the value to be
	# tweaked for specific operations
	@property
	def slow_rebuild_size_warn_threshold(self) -> int:
		return self._slow_rebuild_size_warn_threshold
	@slow_rebuild_size_warn_threshold.setter  # noqa: E301
	def slow_rebuild_size_warn_threshold(self, value: int) -> NoReturn:
		self._slow_rebuild_size_warn_threshold = value

	# Settable instance property because we might want to allow the value to be
	# tweaked for specific operations
	@property
	def reallyslow_rebuild_size_warn_threshold(self) -> int:
		return self._reallyslow_rebuild_size_warn_threshold
	@slow_rebuild_size_warn_threshold.setter  # noqa: E301
	def reallyslow_rebuild_size_warn_threshold(self, value: int) -> NoReturn:
		self._reallyslow_rebuild_size_warn_threshold = value

	@property
	def rebuild_size_warn_threshold(self) -> int:
		if self.rebuild_is_reallyslow:
			return self.reallyslow_rebuild_size_warn_threshold
		elif self.rebuild_is_slow:
			return self.slow_rebuild_size_warn_threshold
		return self.std_rebuild_size_warn_threshold

	def check_and_whine(self) -> NoReturn:
		l = logging.getLogger(__name__)  # noqa: VNE001,E741

		if self.lief_version is None:
			l.warning('lief version could not be detected. Caveat emptor.')
		elif self.lief_version < self.minimum_required_version:
			l.warning('lief version %s detected. Version %s or greater is required to enable '
			          'lief functionality. Version %s or greater is strongly recommended.',
			          str(self.lief_version), str(self.minimum_required_version),
			          str(self.minimum_recommended_version))
		elif self.lief_version < self.minimum_recommended_version:
			l.warning('lief version %s detected. Version %s or greater is strongly recommended. '
			          'Caveat emptor.', str(self.lief_version), str(self.minimum_recommended_version))

	def init_logging(self, verbosity: int) -> NoReturn:
		if not self.lief_usable:
			return

		lief = self._lief
		lief_log_level_offset = -2

		if self.old_logger_api:
			verbosity = verbosity + lief_log_level_offset

			lief.Logger.disable()
			lief.Logger.enable()

			# The old logging API makes absolutely no sense.
			#lief.Logger.set_verbose_level(1)
			lief.Logger.set_level(lief.LOGGING_LEVEL.UNKNOWN)

			if verbosity == 0:
				lief.Logger.set_level(lief.LOGGING_LEVEL.INFO)
			elif verbosity == 1:
				lief.Logger.set_level(lief.LOGGING_LEVEL.WARNING)
			elif verbosity == 2:
				lief.Logger.set_level(lief.LOGGING_LEVEL.ERROR)
			elif verbosity >= 3:
				lief.Logger.set_level(lief.LOGGING_LEVEL.DEBUG)
		else:
			verbosity = verbosity + lief_log_level_offset

			lief.logging.disable()
			lief.logging.enable()

			if verbosity == -1:
				lief.logging.set_level(lief.logging.LOGGING_LEVEL.WARNING)
			elif verbosity == 0:
				lief.logging.set_level(lief.logging.LOGGING_LEVEL.INFO)
			elif verbosity == 1:
				lief.logging.set_level(lief.logging.LOGGING_LEVEL.DEBUG)
			elif verbosity >= 2:
				lief.logging.set_level(lief.logging.LOGGING_LEVEL.TRACE)
			else:
				lief.logging.set_level(lief.logging.LOGGING_LEVEL.ERROR)
