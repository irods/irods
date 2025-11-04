#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import shutil, sys, os, stat  # pylint: disable=C0410
import argparse
import concurrent.futures
import errno
import itertools
import logging
import re
from concurrent.futures import Executor
from os import path
from typing import Optional

import distro

import script_common
import log_instrumentation as logging_ext
from compat_shims import Iterable, NoReturn, Sequence, Set
from context import PackagerContext, PackagerOptionsBase, augment_module_search_paths
from options import RunArgOption
from utilbase import PackagerUtilBase
from elfinfo_util import ElfInfoUtil
from libdirectives import LibDirectiveOptionsBase, LibDirectives, LibFsInfo
from strip_util import StripUtil, StripUtilOptionsBase
from runpath_util import RunpathUtil, RunpathUtilOptionsBase
from tar_util import TarUtil, TarUtilOptionsBase


class PackagerOptions(StripUtilOptionsBase, LibDirectiveOptionsBase, RunpathUtilOptionsBase, TarUtilOptionsBase):
	output_path: Optional[str] = RunArgOption(
		'-o', '--output', metavar='PATH',
		action='store', default=None,
		help='Create tarball at PATH'
	)

	cmake_install_prefix: str = RunArgOption(
		'--cmake-install-prefix', metavar='PATH',
		group=PackagerOptionsBase.gnuinstalldirs_args,
		action='store', opt_type=str, default='/usr',
		help='CMAKE_INSTALL_PREFIX'
	)
	install_bindir: str = RunArgOption(
		'--cmake-install-bindir', metavar='PATH',
		group=PackagerOptionsBase.gnuinstalldirs_args,
		action='store', opt_type=str, default='bin',
		help='CMAKE_INSTALL_BINDIR'
	)
	install_sbindir: str = RunArgOption(
		'--cmake-install-sbindir', metavar='PATH',
		group=PackagerOptionsBase.gnuinstalldirs_args,
		action='store', opt_type=str, default='sbin',
		help='CMAKE_INSTALL_SBINDIR'
	)
	install_libdir: str = RunArgOption(
		'--cmake-install-libdir', metavar='PATH',
		group=PackagerOptionsBase.gnuinstalldirs_args,
		action='store', opt_type=str, default='lib',
		help='CMAKE_INSTALL_LIBDIR'
	)

	irods_bindir: str = RunArgOption(
		'--irods-install-bindir', metavar='PATH',
		group=PackagerOptionsBase.ipath_args,
		action='store', opt_type=str, default='bin'
	)
	irods_sbindir: str = RunArgOption(
		'--irods-install-sbindir', metavar='PATH',
		group=PackagerOptionsBase.ipath_args,
		action='store', opt_type=str, default='sbin'
	)
	irods_libdir: str = RunArgOption(
		'--irods-install-libdir', metavar='PATH',
		group=PackagerOptionsBase.ipath_args,
		action='store', opt_type=str, default='lib'
	)
	irods_plugsdir: str = RunArgOption(
		'--irods-pluginsdir', metavar='PATH',
		group=PackagerOptionsBase.ipath_args,
		action='store', opt_type=str, default='lib/irods/plugins'
	)

	irods_runtime_libs: Sequence[str] = RunArgOption(
		'--irods-runtime-lib', metavar='FILE',
		action='append', opt_type=str, default=[]
	)
	libcxx_libpaths: Sequence[str] = RunArgOption(
		'--libcxx-libpath', metavar='FILE',
		action='append', opt_type=str, default=[]
	)

	build_dir: str = RunArgOption(
		'--build-dir', metavar='PATH',
		action='store', opt_type=str, default=os.getcwd()
	)
	work_dir: Optional[str] = RunArgOption(
		'--work-dir', metavar='PATH',
		action='store', opt_type=str, default=None
	)

	tarball_root_dir_name: Optional[str] = RunArgOption(
		'--tarball-root-dir-name', metavar='DIRNAME',
		action='store', opt_type=str, default=None
	)

	install_components: Sequence[str] = RunArgOption(
		'--install-component', metavar='COMPONENT',
		action='append', opt_type=str, default=[]
	)
	script_icommands: Sequence[str] = RunArgOption(
		'--script-icommand', metavar='NAME',
		action='append', opt_type=str, default=[]
	)

	def create_empty_argparser(self) -> argparse.ArgumentParser:
		return argparse.ArgumentParser(
			description='Package icommands userspace tarball',
			allow_abbrev=False
		)


class Packager(PackagerUtilBase[PackagerOptions]):

	def __init__(
		self,
		context: PackagerContext[PackagerOptions],
		executor: Optional[Executor] = None,
	):
		l = logging.getLogger(__name__)  # noqa: VNE001,E741
		super().__init__(context, executor)

		if not self.options.target_platform:
			plat = distro.linux_distribution(full_distribution_name=False)
			self.options.target_platform = plat[0].lower()
			self.options.target_platform_variant = plat[1].partition('.')[0].lower()
		# This will likely change once we are building on ubi8
		if not self.options.target_platform_base:
			os_plat_id = distro.os_release_attr('platform_id')
			plat_id_re = None
			if os_plat_id is not None:
				plat_id_re = re.match('^platform:(?P<platform>\S+)', os_plat_id)
			if plat_id_re is not None:
				self.options.target_platform_base = plat_id_re.group('platform')
		if self.options.target_platform_base == 'none' or self.options.target_platform_base == '':
			self.options.target_platform_base = None
		if self.options.target_platform_variant == 'none' or self.options.target_platform_variant == 'rolling':
			self.options.target_platform_variant = None
		if self.options.target_platform == 'centos':
			if not self.options.target_platform_base:
				self.options.target_platform_base = 'el' + self.options.target_platform_variant
			if self.options.target_platform_variant == '7' and self.options.set_origin_flag is None:
				self.options.set_origin_flag = not self.lief_info.fragile_builder

		context.lief_info.check_and_whine()
		self.elfinfo_util = ElfInfoUtil(context, executor=executor, raise_if_ldd_not_found=True)
		self.strip_util = StripUtil(context, self.elfinfo_util, executor=executor)
		self.runpath_util = RunpathUtil(context, executor=executor)
		self.tar_util = TarUtil(context)

		if not self.runpath_util.tool_state.cmake:
			raise OSError(errno.ENOENT, 'Could not find cmake tool')

		if not context.lief_info.lief_usable:
			l.warning(
				'lief functionality not available. Will not be able to remove DT_NEEDED '
				'entries for unneeded libraries, set DF_ORIGIN flags, or set RUNPATH in binaries '
				'without pre-existing values. This can result in additional libraries being '
				'included in the package, and may also result in LD_LIBRARY_PATH needing to be '
				'set to run programs from the package.'
			)
			if not self.runpath_util.tool_state:
				l.error(
					'Niehter chrpath nor cmake could be found. Without lief, cmake, or chrpath, '
					'runpath will not be set in package binaries. This will result in bad '
					'runpath/rpath values and LD_LIBRARY_PATH needing to be set to run programs '
					'from the package.'
				)

		self.lib_directives = LibDirectives(context)

		self.work_dir = self.options.work_dir
		self.pkg_dir = path.join(self.work_dir, 'pkg')
		if self.options.tarball_root_dir_name:
			self.pkg_dir_root = path.join(self.pkg_dir, self.options.tarball_root_dir_name)
		else:
			self.pkg_dir_root = self.pkg_dir

		if path.isabs(self.options.install_bindir):
			self.pkg_bin_dir = path.join(self.pkg_dir_root, 'bin')
		else:
			self.pkg_bin_dir = path.join(self.pkg_dir_root, self.options.install_bindir)
		if path.isabs(self.options.irods_libdir):
			l.warning(
				'irods-install-libdir is an absolute path. '
				'If iRODS was built with CMAKE_INSTALL_LIBDIR or IRODS_PLUGINS_DIRECTORY '
				'set to an absolute path, userspace packages will likely not function.'
			)
			self.pkg_lib_dir = path.join(self.pkg_dir_root, 'lib')
		else:
			self.pkg_lib_dir = path.join(self.pkg_dir_root, self.options.irods_libdir)
		if path.isabs(self.options.irods_plugsdir):
			l.warning(
				'irods-pluginsdir is an absolute path. '
				'If iRODS was built with CMAKE_INSTALL_LIBDIR or IRODS_PLUGINS_DIRECTORY '
				'set to an absolute path, userspace packages will likely not function.'
			)
			self.pkg_plugs_dir = path.join(self.pkg_lib_dir, 'irods/plugins')
		else:
			self.pkg_plugs_dir = path.join(self.pkg_dir_root, self.options.irods_plugsdir)

		self.icommand_binaries = {}
		self.icommand_scripts = {}

		self.irods_runtime_libs = {}
		self.irods_runtime_plugs = set()

		# all libs go in here as soon as they've been prepared
		# soname : path
		self.prepared_libs = {}

		# mapping of soname to path for all external libraries we depend on (pre-strip)
		self.ext_lib_paths = {}

		# virtual libs (vdso), ld itself, etc.
		self.ldd_other_libs = set()

		# libraries encountered during dependency resolution that do not have a directive
		self.untracked_libs = {}

	def prepare_workspace(self) -> NoReturn:
		if not path.exists(self.options.cmake_tmpdir):
			os.mkdir(self.options.cmake_tmpdir)
		if path.exists(self.work_dir):
			shutil.rmtree(self.work_dir)
		os.mkdir(self.work_dir)
		os.makedirs(self.pkg_dir_root)
		os.makedirs(self.pkg_bin_dir)
		os.makedirs(self.pkg_lib_dir)
		os.makedirs(self.pkg_plugs_dir)

	def prepare_icommands(self) -> NoReturn:
		l = logging.getLogger(__name__)  # noqa: VNE001,E741
		ilib = self.ilib

		self.context.status(l, '-- Gathering iCommands.')

		cmake_pfx = path.join(self.options.work_dir, 'cmake_pfx')

		if path.exists(cmake_pfx):
			shutil.rmtree(cmake_pfx)
		os.mkdir(cmake_pfx)

		cmake_env = self.fill_out_envdict(self.context.envdict)
		cmake_env.pop('DESTDIR', '')

		for install_component in self.options.install_components:
			# cmake --install not introduced until CMake 3.15
			cmake_args = [
				self.runpath_util.tool_state.cmake.path,
				'-DCMAKE_INSTALL_PREFIX=' + cmake_pfx,
				'-DCMAKE_INSTALL_COMPONENT=' + install_component,
				'-P', path.join(self.options.build_dir, 'cmake_install.cmake')
			]
			ilib.execute_command(cmake_args, env=cmake_env)

		# for now, assume everything we want is in $/bin
		dest_bin = self.pkg_bin_dir

		if path.isabs(self.options.install_bindir):
			cmake_pfx_bin = path.join(cmake_pfx, self.options.install_bindir[1:])
		#elif path.isabs(self.options.cmake_install_prefix):
		#	cmake_pfx_bin = path.join(cmake_pfx, self.options.cmake_install_prefix[1:], self.options.install_bindir)
		#else:
		#	cmake_pfx_bin = path.join(cmake_pfx, self.options.cmake_install_prefix, self.options.install_bindir)
		else:
			cmake_pfx_bin = path.join(cmake_pfx, self.options.install_bindir)

		icommand_names = list(os.listdir(cmake_pfx_bin))

		for icommand_script in self.options.script_icommands:
			if icommand_script in icommand_names:
				icommand_path_in = path.join(cmake_pfx_bin, icommand_script)
				icommand_path_out = path.join(dest_bin, icommand_script)

				shutil.copyfile(icommand_path_in, icommand_path_out)
				os.chmod(icommand_path_out, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR |  # noqa: E222,W504
				                            stat.S_IRGRP |                stat.S_IXGRP |  # noqa: E127,E222,W504
				                            stat.S_IROTH |                stat.S_IXOTH)  # noqa: E127,E222

				icommand_names.remove(icommand_script)
				self.icommand_scripts[icommand_script] = icommand_path_out

		icommand_binaries = {path.join(cmake_pfx_bin, icb): path.join(dest_bin, icb) for icb in icommand_names}

		results = self.strip_util.strip_and_clean_dispatch(icommand_binaries.items())
		script_common.futures_wait(results)

		icommand_binaries = {icb: path.join(dest_bin, icb) for icb in icommand_names}

		self.icommand_binaries.update(icommand_binaries)

	def _should_skip_plugin(self, plugfname: str) -> bool:
		# assume that we can exclude any plugins that end with _server.[ext]
		for suffix in self.options.sharedlib_suffixes:
			if plugfname.endswith(suffix):
				return plugfname.endswith('_server', 0, -len(suffix))
		return False

	def prepare_irods_runtime(self) -> NoReturn:
		l = logging.getLogger(__name__)  # noqa: VNE001,E741

		self.context.status(l, '-- Gathering iRODS runtime libraries and plugins')

		dest_lib = self.pkg_lib_dir
		dest_plugins = self.pkg_plugs_dir

		# for now, assume irods package prefix will always be specified
		if path.isabs(self.options.irods_plugsdir):
			src_plugins = self.options.irods_plugsdir
		else:
			src_plugins = path.join(self.options.irods_package_prefix, self.options.irods_plugsdir)

		# for now, assume all we need are auth and network plugins
		plugsdnames = ['auth', 'network', 'api']
		plugsdirs = []

		for plugsdname in plugsdnames:
			src_plugsdir = path.join(src_plugins, plugsdname)
			dest_plugsdir = path.join(dest_plugins, plugsdname)
			if not path.exists(dest_plugsdir):
				os.mkdir(dest_plugsdir)
			plugsdirs.append((src_plugsdir, dest_plugsdir))

		soname_map = {}
		plugs = []
		runtime_bins = {}

		# for now, assume everything passed through --irods-externals-lib is in $/lib
		# and that the filenames match the sonames
		# and that there's no symlinking shenanigans
		for lib_in in self.options.irods_runtime_libs:
			soname = path.basename(lib_in)
			dest_libpath = path.join(dest_lib, soname)
			soname_map[soname] = dest_libpath
			runtime_bins[lib_in] = dest_libpath

		# continue assuming there's no symlinking shenanigans
		for src_plugsdir, dest_plugsdir in plugsdirs:
			for plugfname in os.listdir(src_plugsdir):
				if self._should_skip_plugin(plugfname):
					continue
				dest_plugpath = path.join(dest_plugsdir, plugfname)
				plugs.append(dest_plugpath)
				runtime_bins[path.join(src_plugsdir, plugfname)] = dest_plugpath

		results = self.strip_util.strip_and_clean_dispatch(runtime_bins.items())
		script_common.futures_wait(results)

		self.irods_runtime_libs.update(soname_map)
		self.irods_runtime_plugs.update(plugs)
		self.prepared_libs.update(soname_map)

	def gather_ext_lib_paths(self) -> NoReturn:
		l = logging.getLogger(__name__)  # noqa: VNE001,E741

		self.context.status(l, '-- Gathering library paths')

		prepped_bins = itertools.chain(self.icommand_binaries.values(),
			self.irods_runtime_libs.values(), self.irods_runtime_plugs)
		ld_paths_res = [
			self.executor.submit(self.elfinfo_util.get_lib_paths, pkg_bin)
			for pkg_bin in prepped_bins
		]

		self.lib_directives.read_directives()

		# As with most libs passed in from a buildsystem, we must assume that we are getting
		# symlinks, and that the filenames do not match the soname
		# For now, assume libc++ doesn't bring in additional deps
		lib_paths_libcxx = {}
		for libcxx_libpath in self.options.libcxx_libpaths:

			# libc++.so is a linker script file. let's avoid having to parse those for now.
			# libc++'s soversion has been '1' for a while now. let's assume that's not changing.
			if libcxx_libpath.endswith('.so'):
				libcxx_libpath = libcxx_libpath + '.1'

			lib_paths_libcxx[path.basename(libcxx_libpath)] = libcxx_libpath

		ld_paths = {}
		ld_not_found = set()

		for ld_r in reversed(ld_paths_res):
			ld_res = ld_r.result()
			ld_paths.update(ld_res.lib_paths)
			ld_not_found.update(ld_res.not_found)
			self.ldd_other_libs.update(ld_res.other_libs)

		for soname, libpath in ld_paths.items():
			if not libpath.startswith(self.pkg_dir_root) and soname not in self.prepared_libs:
				self.ext_lib_paths[soname] = libpath
		self.ext_lib_paths.update(lib_paths_libcxx)

		ld_not_found.difference_update(self.ext_lib_paths)

		if len(ld_not_found) > 0:
			l.debug('ldd could not find some libs:\n%s', logging_ext.pformat(ld_not_found))

		l.debug('other results from ldd:\n%s', logging_ext.pformat(self.ldd_other_libs))

	def _prepare_library(self, lib_in: LibFsInfo) -> LibFsInfo:
		soname = lib_in.soname

		output_path = path.join(self.pkg_lib_dir, lib_in.fname)
		output_ld_path = path.join(self.pkg_lib_dir, lib_in.soname)
		self.strip_util.strip_and_clean(lib_in.real_path, output_path=output_path)

		for linkname, linktgt in lib_in.links.items():
			linkpath = path.join(self.pkg_lib_dir, linkname)
			os.symlink(linktgt, linkpath)

		return LibFsInfo(soname, output_ld_path)

	def _prepare_libraries(self, libs_in: Iterable[LibFsInfo]) -> Sequence[LibFsInfo]:
		prepped_res = [
			self.executor.submit(self._prepare_library, lib_in)
			for lib_in in libs_in
		]

		for lib_info_r in prepped_res:
			lib_info = lib_info_r.result()
			self.prepared_libs[lib_info.soname] = lib_info.real_path

		return [r.result() for r in prepped_res]

	def _filter_ext_libs(self, sonames: Iterable[str]) -> Iterable[LibFsInfo]:
		l = logging.getLogger(__name__)  # noqa: VNE001,E741

		for soname in sonames:
			if soname in self.prepared_libs or soname in self.untracked_libs or soname in self.lib_directives.denylist:
				continue

			libpath = self.ext_lib_paths.get(soname, None)

			if soname not in self.lib_directives.allowlist:
				self.untracked_libs[soname] = libpath
				if self.options.permissive_liballowlist:
					l.info('library %s not in library directives.', soname)
				else:
					l.warning('library %s not in library directives.  Skipping.', soname)
					continue

			if not libpath:
				raise OSError(errno.ENOENT, 'path for ' + soname + ' not found')

			yield LibFsInfo(soname, libpath)

	def _get_needed_libs(self, bin_path: str) -> Sequence[str]:
		elfinfo = self.elfinfo_util.get_elfinfo(bin_path)
		return elfinfo.imported_libs

	def _get_needed_libs_multi(self, bin_paths: Iterable[str]) -> Set[LibFsInfo]:
		libs = set()
		libs_res = [
			self.executor.submit(self._get_needed_libs, bin_path)
			for bin_path in bin_paths
		]
		for libs_r in libs_res:
			libs.update(libs_r.result())
		return libs

	def prepare_ext_libs(self) -> NoReturn:
		l = logging.getLogger(__name__)  # noqa: VNE001,E741

		self.context.status(l, '-- Gathering external libraries')

		if not self.lief_info.lief_usable:
			self._prepare_libraries(self._filter_ext_libs(self.ext_lib_paths.keys()))
			return

		prepped_bins = itertools.chain(self.icommand_binaries.values(),
			self.irods_runtime_libs.values(), self.irods_runtime_plugs)
		current_libs = list(self._filter_ext_libs(self._get_needed_libs_multi(prepped_bins)))
		while len(current_libs) > 0:
			pkg_libs = self._prepare_libraries(current_libs)
			prepped_bins = [pkg_lib.ld_path for pkg_lib in pkg_libs]
			current_libs = list(self._filter_ext_libs(self._get_needed_libs_multi(prepped_bins)))

	def set_runpaths(self) -> NoReturn:
		if not self.runpath_util.tool_state:
			return
		l = logging.getLogger(__name__)  # noqa: VNE001,E741

		self.context.status(l, '-- Setting RUNPATHs')

		libs_res = self.runpath_util.set_rpath_dispatch(self.prepared_libs.values(), '$ORIGIN')

		bins_runpath = os.path.relpath(self.pkg_lib_dir, start=self.pkg_bin_dir)
		if bins_runpath == '.':
			bins_runpath = '$ORIGIN'
		else:
			bins_runpath = path.join('$ORIGIN', bins_runpath)
		bins_res = self.runpath_util.set_rpath_dispatch(self.icommand_binaries.values(), bins_runpath)

		plugs_runpath = os.path.relpath(self.pkg_lib_dir, start=self.pkg_plugs_dir)
		if plugs_runpath == '.':
			plugs_runpath = '$ORIGIN'
		else:
			plugs_runpath = path.join('$ORIGIN', plugs_runpath)
		plugs_res = self.runpath_util.set_rpath_dispatch(self.irods_runtime_plugs, plugs_runpath)

		res_all = script_common.CollectionCompoundView(
			[libs_res, bins_res, plugs_res]
		)
		script_common.futures_wait(res_all)

	def create_tarball(self) -> NoReturn:
		l = logging.getLogger(__name__)  # noqa: VNE001,E741

		self.context.status(l, '-- Creating tarball')

		self.tar_util.create_tarball(self.pkg_dir, self.options.output_path)


def _main() -> Optional[int]:
	options = PackagerOptions()
	options.parse_args()

	augment_module_search_paths(options=options)

	context = PackagerContext(options)
	context.init_logging(options.verbosity)
	logging_ext.setup_stacktrace_signal()

	executor_type = concurrent.futures.ThreadPoolExecutor
	max_workers = None
	if context.lief_info.not_threadsafe:
		# FIXME: ProcessPoolExecutor causes RuntimeError: Queue objects should only be shared...
		#executor_type = concurrent.futures.ProcessPoolExecutor
		max_workers = 1

	with executor_type(max_workers=max_workers) as executor:
		packager = Packager(context, executor)
		packager.prepare_workspace()
		packager.prepare_icommands()
		packager.prepare_irods_runtime()
		packager.gather_ext_lib_paths()
		packager.prepare_ext_libs()
		packager.set_runpaths()
		packager.create_tarball()


if __name__ == '__main__':
	sys.exit(_main())
