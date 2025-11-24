#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import shutil, sys, os, stat  # pylint: disable=C0410
import argparse  # noqa: I100
import errno
import itertools
import logging
import tempfile
from concurrent.futures import Executor, Future
from os import path
from typing import Generic, Optional, Union, overload

import log_instrumentation as logging_ex
from compat_shims import Iterable, Mapping, MutableSequence, Tuple
from context import PackagerContext, PackagerOptionsBase, augment_module_search_paths
from options import RunArgOption
from utilbase import PackagerOperationToolState, PackagerToolState, PackagerUtilBase, ToolStateT

__all__ = [
	'RunpathUtil',
	'RunpathUtilOptionsBase',
	'RunpathToolStateBase',
	'RunpathToolState',
	'RunpathOperationState'
]


class RunpathUtilOptionsBase(PackagerOptionsBase):  # pylint: disable=W0223
	chrpath_path: Optional[str] = RunArgOption(
		'--chrpath-path', metavar='TOOLPATH',
		group=PackagerOptionsBase.tool_args,
		action='store', opt_type=str, default=None,
		help='Path to chrpath tool to use'
	)
	cmake_path: Optional[str] = RunArgOption(
		'--cmake-path', metavar='TOOLPATH',
		group=PackagerOptionsBase.tool_args,
		action='store', opt_type=str, default=None,
		help='Path to cmake tool to use'
	)

	set_origin_flag: Optional[bool] = RunArgOption(
		'--set-origin-flag', metavar='BOOL',
		action='store', opt_type=bool, default=None,
		help='Whether or not to set DF_ORIGIN flag'
	)


class RunpathUtilOptions(RunpathUtilOptionsBase):
	output_path: Optional[str] = RunArgOption(
		'-o', '--output', metavar='PATH',
		action='store', default=None,
		help='Place stripped output into PATH'
	)

	runpath: str = RunArgOption(
		metavar='RUNPATH',
		action='store', default=None
	)
	libraries: MutableSequence[str] = RunArgOption(
		metavar='FILE', nargs='+',
		action='store', opt_type=str, default=None
	)

	def create_empty_argparser(self) -> argparse.ArgumentParser:
		return argparse.ArgumentParser(
			description='Set RUNPATH and set DF_ORIGIN if needed',
			allow_abbrev=False
		)


class RunpathToolStateBase(Generic[ToolStateT]):

	def __init__(
		self,
		lief_tool: ToolStateT,
		chrpath_tool: ToolStateT,
		cmake_tool: ToolStateT
	):
		self._lief_tool = lief_tool
		self._chrpath_tool = chrpath_tool
		self._cmake_tool = cmake_tool

	@property
	def lief_tool(self) -> ToolStateT:
		return self._lief_tool

	@property
	def lief(self) -> ToolStateT:
		return self.lief_tool

	@property
	def chrpath_tool(self) -> ToolStateT:
		return self._chrpath_tool

	@property
	def chrpath(self) -> ToolStateT:
		return self.chrpath_tool

	@property
	def cmake_tool(self) -> ToolStateT:
		return self._cmake_tool

	@property
	def cmake(self) -> ToolStateT:
		return self.cmake_tool

	def __bool__(self) -> bool:
		return bool(self.lief_tool or self.chrpath_tool or self.cmake_tool)


class RunpathToolState(RunpathToolStateBase[PackagerToolState]):
	pass


class RunpathOperationState(RunpathToolStateBase[PackagerOperationToolState]):

	def __init__(self, tool_state: RunpathToolState):
		lief_tool = PackagerOperationToolState(tool_state.lief_tool)
		chrpath_tool = PackagerOperationToolState(tool_state.chrpath_tool)
		cmake_tool = PackagerOperationToolState(tool_state.cmake_tool)
		super().__init__(lief_tool, chrpath_tool, cmake_tool)


# TODO: skip runpath setting if no direct dependencies are in tarball?
class RunpathUtil(PackagerUtilBase[RunpathUtilOptionsBase]):

	def __init__(
		self,
		context: PackagerContext[RunpathUtilOptionsBase],
		executor: Optional[Executor] = None
	):
		super().__init__(context, executor)

		env = self.fill_out_envdict()

		chrpath_path = self.find_tool(
			['chrpath'],
			self.options.chrpath_path,
			raise_if_not_found=False,
			env=env
		)
		cmake_path = self.find_tool(
			['cmake'],
			self.options.cmake_path,
			raise_if_not_found=False,
			env=env
		)

		if self.options.set_origin_flag is None:
			self.options.set_origin_flag = True

		self._tool_state = RunpathToolState(
			PackagerToolState(is_usable=self.lief_info.lief_usable),
			PackagerToolState(is_usable=bool(chrpath_path), path=chrpath_path),
			PackagerToolState(is_usable=bool(cmake_path), path=cmake_path)
		)

	@property
	def tool_state(self) -> RunpathToolState:
		return self._tool_state

	def _set_rpath_lief(
		self,
		bin_path: str,
		runpath: Optional[str],
		set_origin: bool,
		output_path: str,
		inplace: bool,
		op_state: RunpathOperationState
	) -> str:
		l = logging.getLogger(__name__)  # noqa: VNE001,E741
		lief = self.lief
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
			lib = lief.parse(bin_path)
			if set_origin:
				if lib.has(lief.ELF.DYNAMIC_TAGS.FLAGS):
					ent_flags = lib.get(lief.ELF.DYNAMIC_TAGS.FLAGS)
					ent_flags.add(lief.ELF.DYNAMIC_FLAGS.ORIGIN)
				else:
					ent_flags = lief.ELF.DynamicEntryFlags(
						lief.ELF.DYNAMIC_TAGS.FLAGS,
						int(lief.ELF.DYNAMIC_FLAGS.ORIGIN)
					)
					lib.add(ent_flags)

				if lib.has(lief.ELF.DYNAMIC_TAGS.FLAGS_1):
					ent_flags_1 = lib.get(lief.ELF.DYNAMIC_TAGS.FLAGS_1)
					ent_flags_1.add(lief.ELF.DYNAMIC_FLAGS_1.ORIGIN)
				else:
					ent_flags_1 = lief.ELF.DynamicEntryFlags(
						lief.ELF.DYNAMIC_TAGS.FLAGS_1,
						int(lief.ELF.DYNAMIC_FLAGS_1.ORIGIN)
					)
					lib.add(ent_flags_1)

			# TODO: If library has rpath but no runpath, and another tool is available, only set flag
			runpath_is_rpath = False
			if lib.has(lief.ELF.DYNAMIC_TAGS.RUNPATH):
				ent_runpath = lib.get(lief.ELF.DYNAMIC_TAGS.RUNPATH)
				ent_runpath.paths = [runpath]
			elif lib.has(lief.ELF.DYNAMIC_TAGS.RPATH):
				ent_runpath = lib.get(lief.ELF.DYNAMIC_TAGS.RPATH)
				ent_runpath.paths = [runpath]
				if self.lief_info.fragile_builder:
					# Changing the tag here causes a segfault when writing
					runpath_is_rpath = True
				else:
					ent_runpath.tag = lief.ELF.DYNAMIC_TAGS.RUNPATH
			else:
				ent_runpath = lief.ELF.DynamicEntryRunPath(runpath)
				lib.add(ent_runpath)

			l.info('Writing modified elf to %s', tmp_out)
			# for large files, warn that this might take a sec
			if lib.virtual_size > self.lief_info.rebuild_size_warn_threshold:
				l.info('This may take a while for large files (this one is ~%s)',
				       logging_ex.sizeof_fmt(lib.virtual_size))

			lib.write(tmp_out)

			if runpath_is_rpath:
				l.info('Re-parsing modified elf %s to patch rpath tag value', tmp_out)
				lib = lief.parse(tmp_out)
				sect_dyn = lib.get(lief.ELF.SECTION_TYPES.DYNAMIC)
				ent_runpath_idx = 0
				for ent in lib.dynamic_entries:
					if ent.tag == lief.ELF.DYNAMIC_TAGS.RPATH:
						ent_runpath = ent
						break
					ent_runpath_idx = ent_runpath_idx + 1
				else:
					raise RuntimeError('LIEF internal error')

				sect_off = ent_runpath_idx * sect_dyn.entry_size
				tag_off = sect_dyn.file_offset + sect_off

				# assume 64-bit
				tag_sz = 8
				if lib.header.identity_class == lief.ELF.ELF_CLASS.CLASS32:
					tag_sz = 4
				# assume little endian
				tag_end = 'little'
				if lib.header.identity_data == lief.ELF.ELF_DATA.MSB:
					tag_end = 'big'

				tag_b = int(lief.ELF.DYNAMIC_TAGS.RUNPATH).to_bytes(tag_sz, tag_end)

				with open(tmp_out, 'rb+') as tmp_out_f:
					tmp_out_f.seek(tag_off)
					tmp_out_f.write(tag_b)

			shutil.move(tmp_out, output_path)
			return output_path
		finally:
			if path.exists(tmp_out):
				os.unlink(tmp_out)
			op_state.lief.tool_tried = True

	def _set_rpath_chrpath(
		self,
		bin_path: str,
		runpath: Optional[str],
		output_path: str,
		inplace: bool,
		env: Mapping[str, str],
		op_state: RunpathOperationState
	) -> str:
		ilib = self.ilib
		fname = path.basename(bin_path)

		# a little insurance, helps if operating inplace
		tmp_out_fd, tmp_out = tempfile.mkstemp(
			suffix=self.options.sharedlib_suffixes[0],
			prefix=fname+'.',  # noqa: E226
			dir=self.options.cmake_tmpdir
		)
		os.close(tmp_out_fd)
		if path.exists(tmp_out):
			os.unlink(tmp_out)
		try:
			shutil.copyfile(bin_path, tmp_out)
			chrpath_args = [op_state.chrpath.path, '-c', '-r', runpath, tmp_out]
			ilib.execute_command(chrpath_args, env=env)

			shutil.move(tmp_out, output_path)
			return output_path
		finally:
			if path.exists(tmp_out):
				os.unlink(tmp_out)
			op_state.chrpath.tool_tried = True

	# FIXME: runpath setting with cmake fails silently on centos 7
	def _set_rpath_cmake(
		self,
		bin_path: str,
		runpath: Optional[str],
		output_path: str,
		inplace: bool,
		env: Mapping[str, str],
		op_state: RunpathOperationState
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
		try:
			shutil.copyfile(bin_path, tmp_out)
			cmake_args = [
				op_state.cmake.path,
				'-DCHRPATH_TARGETS=' + tmp_out,
				'-DNEW_CHRPATH=' + runpath,
				'-P', path.join(self.context.packager_dir, 'chrpath.cmake')
			]
			ilib.execute_command(cmake_args, env=env)

			shutil.move(tmp_out, output_path)
			return output_path
		finally:
			if path.exists(tmp_out):
				os.unlink(tmp_out)
			op_state.cmake.tool_tried = True

	def _set_rpath(
		self,
		bin_path: str,
		runpath: Optional[str],
		set_origin: bool,
		output_path: str,
		env: Mapping[str, str]
	) -> str:
		l = logging.getLogger(__name__)  # noqa: VNE001,E741
		fname = path.basename(bin_path)

		op_state = RunpathOperationState(self.tool_state)

		if output_path is not None and path.isdir(output_path):
			output_path = path.join(output_path, path.basename(bin_path))
			# if output_path is still a directory, panic
			if path.isdir(output_path):
				raise OSError(errno.EISDIR, 'destination path is a directory.', output_path)

		inplace = output_path is None or path.abspath(bin_path) == path.abspath(output_path)

		if output_path is None:
			output_path = bin_path

		# TODO: isolate lief so that segfaults can be handled gracefully.
		# lief tends to segfault with libc++. mark it as already tried
		if self.lief_info.fragile_builder and (fname.startswith('libc++.') or fname.startswith('libc++abi.')):
			op_state.lief.tool_tried = True

		done = False

		l.info('%s: Setting runpath', bin_path)

		# if we are setting the origin flag, we try lief first
		if set_origin and not op_state.lief.tool_tried:
			try:
				ret = self._set_rpath_lief(bin_path, runpath, set_origin, output_path, inplace,
				                           op_state)
				done = True
			except Exception as ex:  # noqa: B902; pylint: disable=W0703
				op_state.lief.tool_tried = True
				l.debug(
					'%s: failed setting rpath with lief. %s: %s',
					bin_path, type(ex).__name__, str(ex),
					exc_info=True
				)

		if not done and not op_state.chrpath.tool_tried:
			try:
				ret = self._set_rpath_chrpath(bin_path, runpath, output_path, inplace, env,
				                              op_state)
				done = True
			except Exception as ex:  # noqa: B902; pylint: disable=W0703
				op_state.lief.tool_tried = True
				l.debug(
					'%s: failed setting rpath with chrpath. %s: %s',
					bin_path, type(ex).__name__, str(ex),
					exc_info=True
				)

		if not done and not op_state.cmake.tool_tried:
			try:
				ret = self._set_rpath_cmake(bin_path, runpath, output_path, inplace, env, op_state)
				done = True
			except Exception as ex:  # noqa: B902; pylint: disable=W0703
				op_state.lief.tool_tried = True
				l.debug(
					'%s: failed setting rpath with cmake. %s: %s',
					bin_path, type(ex).__name__, str(ex),
					exc_info=True
				)

		if not done and not op_state.lief.tool_tried:
			try:
				ret = self._set_rpath_lief(bin_path, runpath, set_origin, output_path, inplace,
				                           op_state)
				done = True
			except Exception as ex:  # noqa: B902; pylint: disable=W0703
				op_state.lief.tool_tried = True
				l.debug(
					'%s: failed setting rpath with lief. %s: %s',
					bin_path, type(ex).__name__, str(ex),
					exc_info=True
				)

		if not done:
			l.warning('%s: could not set RUNPATH', bin_path)
			if not inplace:
				shutil.copyfile(bin_path, output_path)
			ret = output_path

		os.chmod(ret, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR |  # noqa: E222,W504
		              stat.S_IRGRP |                stat.S_IXGRP |  # noqa: E127,E222,W504
		              stat.S_IROTH |                stat.S_IXOTH)  # noqa: E127,E222

		return ret

	def set_rpath(
		self,
		bin_path: str,
		runpath: Optional[str],
		set_origin: Optional[bool] = None,
		output_path: Optional[str] = None
	) -> str:
		if not self.tool_state:
			raise OSError(errno.ENOPKG, 'Cannot set RUNPATH. lief functionality unavailable '
			                            'and neither cmake nor chrpath could be found.')

		if set_origin:
			set_origin = self.options.set_origin_flag
		elif set_origin is None:
			set_origin = (
				self.lief_info.lief_usable and  # noqa: W504
				self.options.set_origin_flag and  # noqa: W504
				('$ORIGIN' in runpath or '${ORIGIN}' in runpath)
			)

		env = self.fill_out_envdict()
		env['LANG'] = 'C'

		return self._set_rpath(bin_path, runpath, set_origin, output_path, env)

	@overload
	def set_rpath_dispatch(
		self,
		bin_paths: Iterable[str],
		runpath: Optional[str],
		set_origin: Optional[bool] = None
	) -> MutableSequence[Future]:
		pass

	@overload
	def set_rpath_dispatch(
		self,
		bin_paths: Iterable[Tuple[str, str]],
		runpath: Optional[str],
		set_origin: Optional[bool] = None
	) -> MutableSequence[Future]:
		pass

	def set_rpath_dispatch(
		self,
		bin_paths: Union[Iterable[str], Iterable[Tuple[str, str]]],
		runpath: Optional[str],
		set_origin: Optional[bool] = None
	) -> MutableSequence[Future]:
		results = []

		if not self.tool_state:
			raise OSError(errno.ENOPKG, 'Cannot set RUNPATH. lief functionality unavailable '
			                            'and neither cmake nor chrpath could be found.')

		try:
			tup_len = len(next(iter(bin_paths)))
		except StopIteration:
			return results

		if set_origin:
			set_origin = self.options.set_origin_flag
		elif set_origin is None:
			set_origin = (
				self.lief_info.lief_usable and  # noqa: W504
				self.options.set_origin_flag and  # noqa: W504
				('$ORIGIN' in runpath or '${ORIGIN}' in runpath)
			)

		env = self.fill_out_envdict()
		env['LANG'] = 'C'

		if tup_len != 2:
			bin_paths = zip(bin_paths, itertools.repeat(None))  # pylint: disable=W1637

		for bin_path, output_path in bin_paths:
			results.append(
				self.executor.submit(
					self._set_rpath,
					bin_path,
					runpath,
					set_origin,
					output_path,
					env
				)
			)

		return results


def _main() -> int:
	l = logging.getLogger(__name__)  # noqa: VNE001,E741

	options = RunpathUtilOptions()
	options.parse_args()

	augment_module_search_paths(options=options)

	context = PackagerContext[RunpathUtilOptions](options)
	context.init_logging(options.verbosity)
	logging_ex.setup_stacktrace_signal()

	output_path_is_dir = options.output_path is not None and path.isdir(options.output_path)

	if len(options.libraries) > 1 and options.output_path is not None and not output_path_is_dir:
		l.error('Multiple libraries passed in, but output path %s is not a directory',
		        options.output_path)
		return errno.ENOTDIR

	context.lief_info.check_and_whine()
	if not context.lief_info.lief_usable:
		l.warning('lief functionality not available, cannot set DF_ORIGIN flag.')

	runpath_util = RunpathUtil(context)

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

		runpath_util.set_rpath(libpath, options.runpath, output_path=output_path)

	return 0


if __name__ == '__main__':
	sys.exit(_main())
