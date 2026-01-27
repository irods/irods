#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import sys, errno  # noqa: I201,I001; pylint: disable=C0410

if __name__ == '__main__':
	print(__file__ + ': This module is not meant to be invoked directly.',  # noqa: T001
	      file=sys.stderr)
	sys.exit(errno.ELIBEXEC)

import os  # noqa: I100,I202
import logging  # noqa: I100
import tempfile
from contextlib import suppress as suppress_ex
from os import path
from typing import Any, Generic, Optional, TypeVar

import script_common
import log_instrumentation as logging_ext
from compat_shims import MutableMapping, MutableSequence, NoReturn
from lief_instrumentation import LiefModuleT, lief_ver_info
from options import DecrementAction, IncrementAction
from options import RunArgGroup, RunArgOption, RunOption, RunOptsContainer, RunWhininessArg

__all__ = ['PackagerOptionsBase', 'OptionsT', 'PackagerContext', 'augment_module_search_paths']


class PackagerOptionsBase(RunOptsContainer):  # pylint: disable=W0223
	ipath_args = RunArgGroup('iRODS paths')

	irods_install_prefix: str = RunArgOption(
		'--irods-install-prefix', metavar='PATH',
		group=ipath_args,
		action='store', opt_type=str, default='/usr'
	)
	irods_package_prefix: str = RunArgOption(
		'--irods-package-prefix', metavar='PATH',
		group=ipath_args,
		action='store', opt_type=str, default='/usr'
	)
	irods_home_dir: str = RunArgOption(
		'--irods-home-dir', metavar='PATH',
		group=ipath_args,
		action='store', opt_type=str, default='/var/lib/irods',
		help='iRODS home directory'
	)
	irods_scripts_dir: Optional[str] = RunArgOption(
		'--irods-scripts-dir', metavar='PATH',
		group=ipath_args,
		action='store', opt_type=str, default=None,
		help='iRODS scripts directory'
	)

	LD_LIBRARY_PATH: MutableSequence[str] = RunArgOption(
		'--ld_library_path', metavar='PATH',
		action='append', opt_type=str, default=[],
		help='Directory to prepend to LD_LIBRARY_PATH'
	)

	sharedlib_suffixes: MutableSequence[str] = RunArgOption(
		'--sharedlib-suffix', metavar='EXT',
		action='append', opt_type=str, default=['.so']
	)

	cmake_tmpdir: str = RunArgOption(
		'--cmake-tmpdir', metavar='PATH',
		action='store', opt_type=str, default=tempfile.gettempdir()
	)

	gnuinstalldirs_args = RunArgGroup('GNUInstallDirs')

	tool_args = RunArgGroup('Tool paths')

	verbosity_args = RunArgGroup('Output verbosity')
	verbosity: int = RunOption(opt_type=int, default=0)

	_quiet_arg = RunWhininessArg(
		'-q', '--quiet',
		group=verbosity_args,
		action=DecrementAction, dest=verbosity,
		help='Decrease verbosity'
	)
	_verbose_arg = RunWhininessArg(
		'-v', '--verbose',
		group=verbosity_args,
		action=IncrementAction, dest=verbosity,
		help='Increase verbosity'
	)


OptionsT = TypeVar('OptionsT', bound=PackagerOptionsBase)


class PackagerContext(Generic[OptionsT]):
	_packager_dir = path.dirname(path.abspath(path.realpath(__file__)))

	#@classmethod
	@property
	def packager_dir(cls) -> str:
		return cls._packager_dir

	def __new__(cls, options: Optional[OptionsT]):  # noqa: ANN204; pylint: disable=W0613
		obj = super().__new__(cls)

		if not hasattr(cls, '_lief'):
			cls._lief = None
			cls._lief_info = None
			try:
				import lief
			except ImportError:
				cls._lief_info = lief_ver_info(None)
			else:
				cls._lief = lief
				cls._lief_info = lief_ver_info(lief)

		if not hasattr(cls, 'ilib'):
			try:
				from irods import execute as _ilib
			except ImportError:
				from irods import lib as _ilib
			cls.ilib = _ilib

		return obj

	def __init__(self, options: Optional[OptionsT] = None):
		l = logging.getLogger(__name__)  # noqa: VNE001,E741
		l.debug('PackagerContext: __init__ called')

		self._options = options

		self._envdict = {}
		if options and options.LD_LIBRARY_PATH:
			ld_libpath = ':'.join(options.LD_LIBRARY_PATH)
			if 'LD_LIBRARY_PATH' in os.environ:
				self._envdict['LD_LIBRARY_PATH'] = ld_libpath + os.environ['LD_LIBRARY_PATH']
			else:
				self._envdict['LD_LIBRARY_PATH'] = ld_libpath

	#@classmethod
	@property
	def lief(cls) -> Optional[LiefModuleT]:
		return cls._lief

	#@classmethod
	@property
	def lief_info(cls) -> lief_ver_info:
		return cls._lief_info

	@property
	def options(self) -> Optional[OptionsT]:
		return self._options

	@property
	def envdict(self) -> MutableMapping[str, str]:
		return self._envdict

	_logging_initialized = False

	#@classmethod
	@property
	def logging_initialized(cls) -> bool:
		return cls._logging_initialized

	@classmethod
	def _set_logging_initialized(cls, value: bool) -> NoReturn:
		cls._logging_initialized = value

	def init_logging(self, verbosity: int) -> NoReturn:
		# we don't want redundant output, so make sure we don't initialize more than once
		if self.logging_initialized:
			return

		logging_ext.init_logging(verbosity)
		self.lief_info.init_logging(verbosity)

		self._set_logging_initialized(True)

	def trace(  # pylint: disable=R0201
		self,
		logger: logging.Logger,
		msg: str,
		*args: Any,
		**kwargs: Any
	) -> NoReturn:
		logging_ext.trace(logger, msg, *args, **kwargs)

	def message(  # pylint: disable=R0201
		self,
		logger: logging.Logger,
		msg: str,
		*args: Any,
		**kwargs: Any
	) -> NoReturn:
		logging_ext.message(logger, msg, *args, **kwargs)

	def status(  # pylint: disable=R0201
		self,
		logger: logging.Logger,
		msg: str,
		*args: Any,
		**kwargs: Any
	) -> NoReturn:
		logging_ext.status(logger, msg, *args, **kwargs)


def augment_module_search_paths(options: Optional[PackagerOptionsBase] = None) -> NoReturn:
	if not options:
		pass
	elif options.irods_scripts_dir:
		script_common.irods_scripts_dir = options.irods_scripts_dir
	elif options.irods_package_prefix:
		if path.isabs(options.irods_home_dir):
			script_common.irods_scripts_dir = path.join(
				options.irods_package_prefix,
				options.irods_home_dir[1:],
				'scripts'
			)
		else:
			script_common.irods_scripts_dir = path.join(
				options.irods_package_prefix,
				options.irods_home_dir,
				'scripts'
			)
	else:
		script_common.irods_scripts_dir = path.join(
			options.irods_install_prefix,
			options.irods_home_dir,
			'scripts'
		)

	if not path.isabs(script_common.irods_scripts_dir):
		raise OSError(
			errno.EINVAL,
			'irods_scripts_dir is not an absolute path',
			script_common.irods_scripts_dir
		)
	if not path.exists(script_common.irods_scripts_dir):
		raise OSError(
			errno.ENOENT,
			'irods_scripts_dir not found',
			script_common.irods_scripts_dir
		)
	if not path.isdir(script_common.irods_scripts_dir):
		raise OSError(
			errno.ENOTDIR,
			'irods_scripts_dir is not a directory',
			script_common.irods_scripts_dir
		)

	# If the irods script dir is already in the search paths, do nothing
	for searchpath in sys.path:
		with suppress_ex(OSError):
			if path.samefile(searchpath, script_common.irods_scripts_dir):
				return

	sys.path.insert(1, script_common.irods_scripts_dir)
