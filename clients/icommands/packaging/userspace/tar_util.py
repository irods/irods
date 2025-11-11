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
from enum import Enum
from typing import Optional

from compat_shims import NoReturn
from context import PackagerContext, PackagerOptionsBase
from options import RunArgOption
from utilbase import PackagerUtilBase

__all__ = ['TarUtil', 'TarUtilOptionsBase', 'TarToolFlavor']


class TarUtilOptionsBase(PackagerOptionsBase):  # pylint: disable=W0223
	tar_path: Optional[str] = RunArgOption(
		'--tar-path', metavar='TOOLPATH',
		group=PackagerOptionsBase.tool_args,
		action='store', opt_type=str, default=None,
		help='Path to tar tool to use'
	)


class TarToolFlavor(Enum):
	BSD = 1
	GNU = 2


class TarUtil(PackagerUtilBase[TarUtilOptionsBase]):

	def __init__(
		self,
		context: PackagerContext[TarUtilOptionsBase],
		raise_if_tool_flavor_unrecognized: bool = True
	):
		super().__init__(context, None)
		ilib = self.ilib

		env = self.fill_out_envdict()
		env['LANG'] = 'C'

		self.tar_path: str = self.find_tool(
			['bsdtar', 'tar'],
			self.options.tar_path,
			prettyname='tar',
			env=env
		)

		# figure out tool flavor
		tar_flavor = None
		tar_ver_out, tar_ver_err = ilib.execute_command(  # pylint: disable=W0612
			[self.tar_path, '--version'],
			env=env
		)
		tar_ver_out = tar_ver_out.partition('\n')[0]
		if 'bsdtar' in tar_ver_out:
			tar_flavor = TarToolFlavor.BSD
		elif 'GNU tar' in tar_ver_out:
			tar_flavor = TarToolFlavor.GNU
		elif raise_if_tool_flavor_unrecognized:
			raise OSError(
				errno.ENOPKG,
				'tar tool incompatible (only GNU tar and bsdtar are supported)'
			)

		self.tar_flavor: Optional[TarToolFlavor] = tar_flavor

	def create_tarball(
		self,
		source_dir: str,
		output_path: str
	) -> NoReturn:
		l = logging.getLogger(__name__)  # noqa: VNE001,E741
		ilib = self.ilib

		l.info('Packing: %s', output_path)

		env = self.fill_out_envdict()
		env['LANG'] = 'C'

		tar_args = [self.tar_path, '-c', '-a', '-f', output_path]

		if self.tar_flavor == TarToolFlavor.BSD:
			tar_args.append('--no-fflags')
			# bsdtar will derive format from filename
			tar_args.extend(['--numeric-owner', '--uid', '0', '--gid', '0'])
		elif self.tar_flavor == TarToolFlavor.GNU:
			# GNU tar has no equivalent for --no-fflags
			tar_args.extend(['-H', 'posix'])
			tar_args.extend(['--numeric-owner', '--owner=0', '--group=0'])

		tar_args.extend(['-C', source_dir])
		tar_args.extend(os.listdir(source_dir))

		ilib.execute_command(tar_args, env=env)
