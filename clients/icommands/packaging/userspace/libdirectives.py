#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import sys, errno  # noqa: I201,I001; pylint: disable=C0410

if __name__ == '__main__':
	print(__file__ + ': This module is not meant to be invoked directly.',  # noqa: T001
	      file=sys.stderr)
	sys.exit(errno.ELIBEXEC)

import logging  # noqa: I100,I202
from abc import ABCMeta
from collections.abc import Sized
from os import path
from typing import Optional

from compat_shims import Container, Iterable, Mapping, MutableSequence, MutableSet, NoReturn
from context import PackagerContext, PackagerOptionsBase
from options import RunArgOption
from utilbase import PackagerUtilBase

__all__ = ['LibDirectiveOptionsBase', 'LibFsInfo', 'DirectiveLibSets', 'LibDirectives']


class LibDirectiveOptionsBase(PackagerOptionsBase):  # pylint: disable=W0223
	target_platform: Optional[str] = RunArgOption(
		'--target-platform', metavar='PLATFORM',
		action='store', opt_type=str, default=None
	)
	target_platform_base: Optional[str] = RunArgOption(
		'--target-platform-base', metavar='BASE',
		action='store', opt_type=str, default=None
	)
	target_platform_variant: Optional[str] = RunArgOption(
		'--target-platform-variant', metavar='VARIANT',
		action='store', opt_type=str, default=None
	)

	permissive_liballowlist: MutableSequence[str] = RunArgOption(
		'--permissive-lib-directives',
		action='store_true', default=False,
		help='Package libraries for which no directive is specified'
	)


class LibFsInfo(metaclass=ABCMeta):
	"""
	A class containing information about a library on the filesystem.

	This handles a limited amount of symlink chicanery.
	It does not handle linker script files, so libraries that use scripts in lieu
	of symlinks (such as libc++.so) will be a problem.
	"""

	def __init__(self, soname: str, ld_path: str):
		super().__init__()
		self._soname = soname
		self._ld_path = ld_path
		self._links = {}

		# For the time being, we only care about the soname->realpath linkage.
		# We also make the assumption that soname symlinks and the actual
		# libraries reside in the same directory, or that if they do not, it
		# does not matter for our purposes.
		self._real_path = path.realpath(ld_path)
		self._fname = path.basename(self.real_path)
		if soname != self.fname:
			self._links[soname] = self.fname

	@property
	def soname(self) -> str:
		return self._soname

	@property
	def fname(self) -> str:
		return self._fname

	@property
	def real_path(self) -> str:
		return self._real_path

	@property
	def ld_path(self) -> str:
		return self._ld_path

	@property
	def links(self) -> Mapping[str, str]:
		return self._links


class DirectiveLibSets(Container, Sized):

	def __init__(self,
	             context: PackagerContext[LibDirectiveOptionsBase],
	             sonames: Optional[Iterable[str]] = None,
	             libnames: Optional[Iterable[str]] = None):
		super().__init__()
		self._context = context
		self._sonames = set()
		self._libnames = set()

		if sonames:
			self.sonames.update(sonames)
		if libnames:
			self.libnames.update(libnames)

	@property
	def context(self) -> PackagerContext[LibDirectiveOptionsBase]:
		return self._context

	@property
	def sonames(self) -> MutableSet[str]:
		return self._sonames

	@property
	def libnames(self) -> MutableSet[str]:
		return self._libnames

	def __contains__(self, lib: str) -> bool:
		if lib in self.sonames or lib in self.libnames:
			return True

		# don't bother checking if a passed in soname maches a libname if the soname doesn't
		# start with any of the libnames
		skip_libname_check = True
		for libname in self.libnames:
			if lib.startswith(libname):
				skip_libname_check = False
				break
		if skip_libname_check:
			return False

		# check if a passed in soname matches a libname
		suffixes = self.context.options.sharedlib_suffixes
		for suffix in suffixes:
			# match libname.suffix to libname
			if lib.endswith(suffix) and lib[:-len(suffix)] in self.libnames:
				return True

			# check for cases like libname.so.4.9.so.6.so (yes I have really seen this before)
			# in such a case, we would match against libname, libname.so.4.9, & libname.so.4.9.so.6
			suffixsep = suffix + '.'
			splitlib = lib.split(suffixsep)
			if len(splitlib) < 2:
				continue
			potential_libname = splitlib[0]
			for libname_part in splitlib[1:]:
				if potential_libname in self.libnames:
					return True
				potential_libname = potential_libname + suffixsep + libname_part

		return False

	def __len__(self) -> int:
		sz = len(self.sonames) + len(self.libnames)
		return sz


class LibDirectives(PackagerUtilBase[LibDirectiveOptionsBase]):

	def __init__(self, context: PackagerContext[LibDirectiveOptionsBase]):
		super().__init__(context, None)
		self._allowlist = DirectiveLibSets(context)
		self._denylist = DirectiveLibSets(context)

		target_platform = context.options.target_platform
		target_platform_base = context.options.target_platform_base
		target_platform_variant = context.options.target_platform_variant
		if target_platform == 'none':
			target_platform = None
		if target_platform_variant in ('none', 'rolling'):
			target_platform_variant = None
		if target_platform_base in ('none', ''):
			target_platform_base = None
		if target_platform and target_platform_variant:
			target_platform = target_platform + target_platform_variant
		self._target_platform = target_platform
		self._target_platform_base = target_platform_base

	def read_directives_for_target(self, target_name: Optional[str]) -> bool:
		if not target_name:
			return True
		l = logging.getLogger(__name__)  # noqa: VNE001,E741

		libname_dfname = path.join(self.context.packager_dir, 'externs_libs.' + target_name)
		soname_dfname = path.join(self.context.packager_dir, 'known_libs.' + target_name)

		if path.exists(libname_dfname):
			with open(libname_dfname, 'r') as directives_f:
				for directive_l in directives_f:
					if directive_l[0] == '#':
						continue
					directive_l = directive_l.rstrip()
					if directive_l:
						self.allowlist.libnames.add(directive_l)

		if path.exists(soname_dfname):
			with open(soname_dfname, 'r') as directives_f:
				lnum = 0
				for directive_l in directives_f:
					lnum = lnum + 1
					if directive_l[0] == '#':
						continue
					directive_l = directive_l.rstrip()
					if not directive_l:
						continue
					elif directive_l[0] == '+':
						soname = directive_l[1:]
						self.denylist.sonames.discard(soname)
						self.allowlist.sonames.add(soname)
					elif directive_l[0] == '-':
						soname = directive_l[1:]
						self.allowlist.sonames.discard(soname)
						self.denylist.sonames.add(soname)
					elif directive_l[0] == '~':
						soname = directive_l[1:]
						self.allowlist.sonames.discard(soname)
						self.denylist.sonames.discard(soname)
					else:
						l.warning('%s:%s: Syntax error, ignoring line', soname_dfname, str(lnum))
			return True
		return False

	def read_directives(self) -> NoReturn:
		l = logging.getLogger(__name__)  # noqa: VNE001,E741
		l.info('reading known libs data')

		self.read_directives_for_target(target_name='common')

		# FIXME: Account for populated target_platform_base but empty target_platform
		got_base_directives = self.read_directives_for_target(target_name=self.target_platform_base)
		got_target_directives = self.read_directives_for_target(target_name=self.target_platform)
		if self.target_platform_base:
			if not (got_base_directives or got_target_directives):
				l.warning('Could not read soname directives file(s) for %s or %s. '
						  'This will likely result in the exclusion of required libraries.',
						  self.target_platform_base, self.target_platform)
		elif not got_target_directives:
			l.warning('Could not read soname directives file for %s. '
					  'This will likely result in the exclusion of required libraries.',
					  self.target_platform)

	@property
	def target_platform(self) -> Optional[str]:
		return self._target_platform

	@property
	def target_platform_base(self) -> Optional[str]:
		return self._target_platform_base

	@property
	def allowlist(self) -> DirectiveLibSets:
		return self._allowlist

	@property
	def denylist(self) -> DirectiveLibSets:
		return self._denylist
