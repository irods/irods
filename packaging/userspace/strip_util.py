#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import shutil, sys, os, stat  # pylint: disable=C0410
import argparse  # noqa: I100
import errno
import logging
import tempfile
from concurrent.futures import Executor, Future
from os import path
from typing import Generic, Optional, Union, overload

import log_instrumentation as logging_ex
from compat_shims import Iterable, Mapping, MutableSequence, Tuple
from context import PackagerContext, PackagerOptionsBase, augment_module_search_paths
from options import RunArgOption
from utilbase import PackagerToolState, PackagerUtilBase, ToolStateT
from elfinfo_util import ElfInfoUtil, ElfInfoUtilOptionsBase

__all__ = [
	'StripUtil',
	'StripToolStateBase',
	'StripToolState',
	'StripUtilOptionsBase'
]


class StripUtilOptionsBase(ElfInfoUtilOptionsBase):  # pylint: disable=W0223
	strip_path: Optional[str] = RunArgOption(
		'--strip-path', metavar='TOOLPATH',
		group=PackagerOptionsBase.tool_args,
		action='store', opt_type=str, default=None,
		help='Path to strip tool to use'
	)

	preserve_libs: bool = RunArgOption(
		'--preserve-unneeded-librefs',
		action='store_true', default=False,
		help='Do not remove DT_NEEDED entries for unneeded libraries'
	)


class StripUtilOptions(StripUtilOptionsBase):
	output_path: Optional[str] = RunArgOption(
		'-o', '--output', metavar='PATH',
		action='store', default=None,
		help='Place stripped output into PATH'
	)

	libraries: MutableSequence[str] = RunArgOption(
		metavar='FILE', nargs='+',
		action='store', opt_type=str, default=None,
		help='ELF binaries to strip'
	)

	def create_empty_argparser(self) -> argparse.ArgumentParser:
		return argparse.ArgumentParser(
			description='Strip unneeded symbols and library imports',
			allow_abbrev=False
		)


class StripToolStateBase(Generic[ToolStateT]):

	def __init__(
		self,
		lief_tool: ToolStateT,
		strip_tool: ToolStateT
	):
		self._lief_tool = lief_tool
		self._strip_tool = strip_tool

	@property
	def lief_tool(self) -> ToolStateT:
		return self._lief_tool

	@property
	def lief(self) -> ToolStateT:
		return self.lief_tool

	@property
	def strip_tool(self) -> ToolStateT:
		return self._strip_tool

	@property
	def strip(self) -> ToolStateT:
		return self.strip_tool

	def __bool__(self) -> bool:
		return bool(self.strip_tool)


class StripToolState(StripToolStateBase[PackagerToolState]):
	pass


class StripUtil(PackagerUtilBase[StripUtilOptionsBase]):

	def __init__(
		self,
		context: PackagerContext[StripUtilOptionsBase],
		elfinfo_util: ElfInfoUtil,
		executor: Optional[Executor] = None,
		raise_if_strip_not_found: bool = False
	):
		super().__init__(context, executor)
		self.libinfo_util: ElfInfoUtil = elfinfo_util

		env = self.fill_out_envdict()
		strip_path = self.find_tool(
			['llvm-strip', 'strip'],
			self.options.strip_path,
			raise_if_not_found=raise_if_strip_not_found,
			prettyname='strip',
			env=env
		)

		self._tool_state = StripToolState(
			PackagerToolState(is_usable=self.lief_info.lief_usable),
			PackagerToolState(is_usable=bool(strip_path), path=strip_path)
		)

	@property
	def tool_state(self) -> StripToolState:
		return self._tool_state

	def _strip(
		self,
		bin_path: str,
		strip_debug_symbols: bool,
		output_path: str,
		inplace: bool,
		env: Mapping[str, str]
	) -> str:
		ilib = self.ilib
		fname = path.basename(bin_path)

		# a little insurance, helps if operating inplace
		tmp_out_fd, tmp_out = tempfile.mkstemp(
			suffix=self.options.sharedlib_suffixes[0],
			prefix=fname + '.',
			dir=self.options.cmake_tmpdir
		)
		os.close(tmp_out_fd)
		if path.exists(tmp_out):
			os.unlink(tmp_out)
		try:  # pylint: disable=W0717
			strip_args = [self.tool_state.strip.path, '--strip-unneeded']
			if strip_debug_symbols:
				strip_args.append('--strip-debug')
			strip_args.append(bin_path)
			if not inplace:
				strip_args.extend(['-o', tmp_out])

			ilib.execute_command(strip_args, env=env)

			shutil.move(tmp_out, output_path)
		finally:
			if path.exists(tmp_out):
				os.unlink(tmp_out)

		os.chmod(output_path, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR |  # noqa: E222,W504
		                      stat.S_IRGRP |                stat.S_IXGRP |  # noqa: E127,E222,W504
		                      stat.S_IROTH |                stat.S_IXOTH)  # noqa: E127,E222

		return output_path

	def strip(
		self,
		bin_path: str,
		strip_debug_symbols: bool = False,
		output_path: Optional[str] = None
	) -> str:
		l = logging.getLogger(__name__)  # noqa: VNE001,E741

		if not self.tool_state:
			raise OSError(errno.ENOENT, 'Cannot strip binary. strip tool not found.')

		if output_path is not None and path.isdir(output_path):
			output_path = path.join(output_path, path.basename(bin_path))
			# if output_path is still a directory, panic
			if path.isdir(output_path):
				raise OSError(errno.EISDIR, 'destination path is a directory.', output_path)

		inplace = output_path is None or path.abspath(bin_path) == path.abspath(output_path)

		if output_path is None:
			output_path = bin_path

		l.info('%s: Stripping unneeded symbols', output_path)

		env = self.fill_out_envdict()

		return self._strip(bin_path, strip_debug_symbols, output_path, inplace, env)

	def _clean_unused_dependencies(
		self,
		bin_path: str,
		output_path: str,
		inplace: bool
	) -> str:
		l = logging.getLogger(__name__)  # noqa: VNE001,E741
		fname = path.basename(bin_path)
		lief = self.lief

		# lief tends to segfault with libc++
		if fname.startswith('libc++.') or fname.startswith('libc++abi.'):
			unneeded_libs = []
		else:
			unneeded_libs = self.libinfo_util.get_unused_dependencies(bin_path)
		if len(unneeded_libs) > 0:
			_raw_uneeded_libs = list(unneeded_libs)
			unneeded_libs = set(unneeded_libs)  # pylint: disable=R0204

			# FIXME: Figure out why we have to do this
			for unneeded_lib in _raw_uneeded_libs:
				if unneeded_lib.startswith('libirods_'):
					unneeded_libs.remove(unneeded_lib)

		if len(unneeded_libs) > 0:
			lib = lief.parse(bin_path)

			# TODO: Figure out how to clean these out
			# TODO: Get these from elfinfo util
			versioned_libs = {symver.name for symver in lib.symbols_version_requirement}
			if l.isEnabledFor(logging.DEBUG):
				versioned_libs.intersection_update(unneeded_libs)
				if len(versioned_libs) > 0:
					l.debug(
						'%s: DT_VERNEED subentries found for unused %s, '
						'skipping removal of DT_NEEDED entry(s).',
						output_path, ', '.join(versioned_libs)
					)
			unneeded_libs.difference_update(versioned_libs)

		if len(unneeded_libs) > 0:
			if l.isEnabledFor(logging.INFO):
				l.info(
					'%s: removing DT_NEEDED entry(s) for %s.',
					output_path, ', '.join(unneeded_libs)
				)
			for unneeded_lib in unneeded_libs:
				if lib.has_library(unneeded_lib):
					lib.remove_library(unneeded_lib)
				else:
					l.warning(
						'%s: ldd reports %s as unneeded import, but %s not found in imports.',
						output_path, unneeded_lib, unneeded_lib
					)

			# a little insurance, helps if operating inplace
			tmp_out_fd, tmp_out = tempfile.mkstemp(
				suffix=self.options.sharedlib_suffixes[0],
				prefix=fname + '.',
				dir=self.options.cmake_tmpdir
			)
			os.close(tmp_out_fd)
			if path.exists(tmp_out):
				os.unlink(tmp_out)
			try:
				l.info('Writing cleaned elf to %s', tmp_out)
				# for large files, warn that this might take a sec
				if lib.virtual_size > self.lief_info.rebuild_size_warn_threshold and l.isEnabledFor(logging.INFO):
					l.info(
						'This may take a while for large files (this one is ~%s)',
						logging_ex.sizeof_fmt(lib.virtual_size)
					)

				lib.write(tmp_out)
				shutil.move(tmp_out, output_path)
			finally:
				if path.exists(tmp_out):
					os.unlink(tmp_out)

		elif not inplace:
			shutil.copyfile(bin_path, output_path)

		os.chmod(output_path, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR |  # noqa: E222,W504
		                      stat.S_IRGRP |                stat.S_IXGRP |  # noqa: E127,E222,W504
		                      stat.S_IROTH |                stat.S_IXOTH)  # noqa: E127,E222

		return output_path

	def clean_unused_dependencies(
		self,
		bin_path: str,
		output_path: Optional[str] = None
	) -> str:
		l = logging.getLogger(__name__)  # noqa: VNE001,E741

		if not self.lief_info.lief_usable:
			raise OSError(errno.ENOPKG, 'lief functionality unavailable')

		if output_path is not None and path.isdir(output_path):
			output_path = path.join(output_path, path.basename(bin_path))
			# if output_path is still a directory, panic
			if path.isdir(output_path):
				raise OSError(errno.EISDIR, 'destination path is a directory.', output_path)

		inplace = output_path is None or path.abspath(bin_path) == path.abspath(output_path)

		if output_path is None:
			output_path = bin_path

		l.info('%s: Cleaning up unneeded library imports', output_path)

		return self._clean_unused_dependencies(bin_path, output_path, inplace)

	def strip_and_clean(
		self,
		bin_path: str,
		strip_debug_symbols: bool = False,
		intermediate_path: Optional[str] = None,
		output_path: Optional[str] = None
	) -> str:
		l = logging.getLogger(__name__)  # noqa: VNE001,E741

		if not self.tool_state:
			raise OSError(errno.ENOENT, 'Cannot strip binary. strip tool not found.')

		if intermediate_path is not None and path.isdir(intermediate_path):
			intermediate_path = path.join(intermediate_path, path.basename(bin_path))
			# if intermediate_path is still a directory, panic
			if path.isdir(intermediate_path):
				raise OSError(errno.EISDIR, 'destination path is a directory.', intermediate_path)

		if output_path is not None and path.isdir(output_path):
			output_path = path.join(output_path, path.basename(bin_path))
			# if output_path is still a directory, panic
			if path.isdir(output_path):
				raise OSError(errno.EISDIR, 'destination path is a directory.', output_path)

		if intermediate_path is None and output_path is None:
			inplace_1 = True
			intermediate_path = bin_path
			output_path = bin_path
		elif output_path and not intermediate_path:
			intermediate_path = output_path
			inplace_1 = path.abspath(bin_path) == path.abspath(output_path)
		elif intermediate_path and not output_path:
			output_path = bin_path
			inplace_1 = path.abspath(bin_path) == path.abspath(intermediate_path)
		else:
			inplace_1 = path.abspath(bin_path) == path.abspath(intermediate_path)

		env = self.fill_out_envdict()

		l.info('%s: Stripping unneeded symbols', output_path)

		intermediate_path = self._strip(
			bin_path,
			strip_debug_symbols,
			intermediate_path,
			inplace_1,
			env
		)

		inplace_2 = path.abspath(intermediate_path) == path.abspath(output_path)

		if self.tool_state.lief.is_usable:
			l.info('%s: Cleaning up unneeded library imports', output_path)
			return self._clean_unused_dependencies(
				intermediate_path,
				output_path,
				inplace_2
			)
		else:
			if not inplace_2:
				shutil.copyfile(intermediate_path, output_path)
			return output_path

	@overload
	def strip_and_clean_dispatch(
		self,
		bin_paths: Iterable[str],
		strip_debug_symbols: bool = False
	) -> MutableSequence[Future]:
		pass

	@overload
	def strip_and_clean_dispatch(
		self,
		bin_paths: Iterable[Tuple[str, str]],
		strip_debug_symbols: bool = False
	) -> MutableSequence[Future]:
		pass

	@overload
	def strip_and_clean_dispatch(
		self,
		bin_paths: Iterable[Tuple[str, str, str]],
		strip_debug_symbols: bool = False
	) -> MutableSequence[Future]:
		pass

	def strip_and_clean_dispatch(
		self,
		bin_paths: Union[Iterable[str], Iterable[Tuple[str, str]], Iterable[Tuple[str, str, str]]],
		strip_debug_symbols: bool = False
	) -> MutableSequence[Future]:
		results = []
		try:
			tup_len = len(next(iter(bin_paths)))
		except StopIteration:
			return results

		if tup_len == 2:
			for bin_path, output_path in bin_paths:
				results.append(
					self.executor.submit(
						self.strip_and_clean,
						bin_path,
						strip_debug_symbols=strip_debug_symbols,
						output_path=output_path
					)
				)
		elif tup_len == 3:
			for bin_path, intermediate_path, output_path in bin_paths:
				results.append(
					self.executor.submit(
						self.strip_and_clean,
						bin_path,
						strip_debug_symbols=strip_debug_symbols,
						intermediate_path=intermediate_path,
						output_path=output_path
					)
				)
		else:
			for bin_path in bin_paths:
				results.append(
					self.executor.submit(
						self.strip_and_clean,
						bin_path,
						strip_debug_symbols=strip_debug_symbols
					)
				)

		return results


def _main() -> int:
	l = logging.getLogger(__name__)  # noqa: VNE001,E741

	options = StripUtilOptions()
	options.parse_args()

	augment_module_search_paths(options=options)

	context = PackagerContext[StripUtilOptions](options)
	context.init_logging(options.verbosity)
	logging_ex.setup_stacktrace_signal()

	output_path_is_dir = options.output_path is not None and path.isdir(options.output_path)

	if len(options.libraries) > 1 and options.output_path is not None and not output_path_is_dir:
		l.error('Multiple libraries passed in, but output path %s is not a directory',
		        options.output_path)
		return errno.ENOTDIR

	context.lief_info.check_and_whine()
	if not context.lief_info.lief_usable:
		l.warning('lief functionality not available, skipping removal of unneeded library imports.')

	libinfo_util = ElfInfoUtil(context)
	strip_util = StripUtil(context, libinfo_util, raise_if_strip_not_found=True)

	for libpath in options.libraries:
		# We don't want to re-evaluate whether output_path is a dir every time, in case it
		# is deleted midway through. We evaluate once, and pre-evaluate full destinations
		# to avoid this potential race condition
		output_path = options.output_path
		if output_path_is_dir:
			output_path = path.join(output_path, path.basename(libpath))
			# if output_path is still a directory, panic
			if path.isdir(output_path):
				l.error('destination path %s is a directory.', output_path)
				return errno.EISDIR

		strip_util.strip_and_clean(libpath, output_path=output_path)

	return 0


if __name__ == '__main__':
	sys.exit(_main())
