#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import sys, errno  # noqa: I201,I001; pylint: disable=C0410

if __name__ == '__main__':
	print(__file__ + ': This module is not meant to be invoked directly.',  # noqa: T001
	      file=sys.stderr)
	sys.exit(errno.ELIBEXEC)

import argparse  # noqa: I100,I202
from abc import ABCMeta, abstractmethod
from collections import OrderedDict
from typing import Any, Optional, Union

from compat_shims import Mapping, Sequence, Type

__all__ = [
	'_IncrDecrAction',
	'IncrementAction',
	'DecrementAction',
	'ArgparseActor',
	'RunArgGroup',
	'RunOption',
	'RunArgBase',
	'RunArgOption',
	'RunWhininessArg',
	'RunOptsContainer'
]


class _IncrDecrAction(argparse.Action):
	def __init__(
		self,
		option_strings,
		dest,
		nargs=None,
		const=None,
		default=None,
		type=None,  # noqa: A002,VNE003; pylint: disable=W0622
		choices=None,
		required=False,
		metavar=None,
		**kwargs
	):
		if nargs is not None and nargs != 0:
			raise ValueError('nonzero nargs not allowed')
		if const is not None:
			raise ValueError('const not allowed')
		if type is not None and type != int:
			raise ValueError('non-int type not allowed')
		if choices is not None:
			raise ValueError('choices not allowed')
		if required:
			raise ValueError('non-false required not allowed')
		if metavar is not None:
			raise ValueError('metavar not allowed')

		if default is None:
			default = 0

		super().__init__(option_strings, dest, nargs=0, default=0, type=int, **kwargs)

	@abstractmethod
	def __call__(self, parser, namespace, values, option_string=None):
		raise NotImplementedError()


class IncrementAction(_IncrDecrAction):

	def __call__(self, parser, namespace, values, option_string=None):
		setattr(namespace, self.dest, getattr(namespace, self.dest, self.default) + 1)


class DecrementAction(_IncrDecrAction):

	def __call__(self, parser, namespace, values, option_string=None):
		setattr(namespace, self.dest, getattr(namespace, self.dest, self.default) - 1)


class ArgparseActor(metaclass=ABCMeta):

	@abstractmethod
	def add_to_argparser(self, argparser: argparse.ArgumentParser, owner):
		raise NotImplementedError()

	def cleanup_container(self, owner):
		pass


class RunArgGroup(ArgparseActor):
	def __init__(
		self,
		title: Optional[str] = None,
		description: Optional[str] = None
	):
		super().__init__()
		self.title: Optional[str] = title
		self.description: Optional[str] = description

	def __set_name__(self, owner, name: str):
		self.public_name: str = name
		self.private_name: str = '_argparse_group_' + name

	def __get__(self, obj, objtype=None):
		if obj is None:
			return self
		return getattr(obj, self.private_name, None)

	def add_to_argparser(self, argparser: argparse.ArgumentParser, owner):
		group = getattr(owner, self.private_name, None)
		if group and group in argparser._action_groups:  # pylint: disable=W0212
			return group
		group = argparser.add_argument_group(title=self.title, description=self.description)
		setattr(owner, self.private_name, group)
		return group

	def cleanup_container(self, owner):
		if hasattr(owner, self.private_name):
			delattr(owner, self.private_name)


class RunOption(metaclass=ABCMeta):
	def __init__(
		self,
		opt_type: Optional[Type] = None,
		default: Optional[Any] = None
	):
		super().__init__()
		self.opt_type: Optional[Type] = opt_type
		self.default: Optional[Any] = default

	def __set_name__(self, owner, name: str):
		self.public_name: str = name
		self.private_name: str = '_' + name

	def __get__(self, obj, objtype=None):
		if obj is None:
			return self
		return getattr(obj, self.private_name, self.default)

	def __set__(self, obj, value):
		setattr(obj, self.private_name, value)


class RunArgBase(ArgparseActor):
	def __init__(
		self,
		*args: str,
		group: Optional[RunArgGroup] = None,
		action: Optional[Union[str, Type[argparse.Action]]] = None,
		nargs: Optional[Union[str, int]] = None,
		help: Optional[str] = None  # noqa: A002; pylint: disable=W0622
	):
		super().__init__()
		self.args: Sequence[str] = args
		self.group: Optional[RunArgGroup] = group
		self.action: Optional[Union[str, Type[argparse.Action]]] = action
		self.nargs: Optional[Union[str, int]] = nargs
		self.help: Optional[str] = help

	@abstractmethod
	def add_to_argparser(self, argparser: argparse.ArgumentParser, owner):
		raise NotImplementedError()


class RunArgOption(RunOption, RunArgBase):
	def __init__(
		self,
		*args: str,
		metavar: Optional[str] = None,
		group: Optional[RunArgGroup] = None,
		action: Optional[Union[str, Type[argparse.Action]]] = None,
		opt_type: Optional[Type] = None,
		nargs: Optional[Union[str, int]] = None,
		default: Optional[Any] = None,
		help: Optional[str] = None  # noqa: A002; pylint: disable=W0622
	):
		RunOption.__init__(self, opt_type=opt_type, default=default)
		RunArgBase.__init__(self, group=group, action=action, nargs=nargs, help=help, *args)
		self.metavar: Optional[str] = metavar

	def add_to_argparser(self, argparser: argparse.ArgumentParser, owner):
		group = argparser
		if self.group:
			group = self.group.add_to_argparser(argparser, owner)
		args = self.args
		kwargs = {}
		if self.metavar:
			kwargs['metavar'] = self.metavar
		if self.action:
			kwargs['action'] = self.action
		kwargs['dest'] = self.public_name
		if self.opt_type:
			kwargs['type'] = self.opt_type
		if self.nargs:
			kwargs['nargs'] = self.nargs
		if self.default:
			kwargs['default'] = self.default
		if self.help:
			kwargs['help'] = self.help
		return group.add_argument(*args, **kwargs)

	def cleanup_container(self, owner):
		if self.group:
			self.group.cleanup_container(owner)


class RunWhininessArg(RunArgBase):
	def __init__(
		self,
		*args: str,
		group: Optional[RunArgGroup] = None,
		action: Type[_IncrDecrAction] = None,
		dest: Union[str, RunOption] = None,
		help: Optional[str] = None  # noqa: A002; pylint: disable=W0622
	):
		super().__init__(group=group, action=action, help=help, *args)
		self.dest: Union[str, RunOption] = dest

	def add_to_argparser(self, argparser: argparse.ArgumentParser, owner):
		group = argparser
		if self.group:
			group = self.group.add_to_argparser(argparser, owner)
		args = self.args
		kwargs = {}
		kwargs['action'] = self.action
		kwargs['dest'] = self.dest.public_name
		kwargs['type'] = int
		if self.help:
			kwargs['help'] = self.help
		return group.add_argument(*args, **kwargs)

	def cleanup_container(self, owner):
		if self.group:
			self.group.cleanup_container(owner)


class RunOptsContainer(metaclass=ABCMeta):

	def __new__(cls):
		obj = super().__new__(cls)

		_mro = list(cls.mro())
		while _mro[-1] != RunOptsContainer:
			_mro.pop()
		_mro.reverse()

		_groupsdict = OrderedDict()
		_argsdict = OrderedDict()
		_optsdict = OrderedDict()
		_actordict = OrderedDict()
		for scls in _mro:
			for name, attr in scls.__dict__.items():
				if isinstance(attr, RunArgGroup):
					_groupsdict[name] = attr
				if isinstance(attr, RunArgBase):
					_argsdict[name] = attr
				if isinstance(attr, RunOption):
					_optsdict[name] = attr
				if isinstance(attr, ArgparseActor):
					_actordict[name] = attr

		cls._groupsdict: Mapping[str, RunArgGroup] = _groupsdict
		cls._argsdict: Mapping[str, RunArgBase] = _argsdict
		cls._optsdict: Mapping[str, RunOption] = _optsdict
		cls._actordict: Mapping[str, ArgparseActor] = _actordict

		return obj

	def __contains__(self, key: str) -> bool:
		return key in self._optsdict

	def __repr__(self) -> str:
		type_name = type(self).__name__
		arg_strings = []
		star_args = {}
		for name in self._optsdict.keys():
			value = getattr(self, name)
			if name.isidentifier():
				arg_strings.append('%s=%r' % (name, value))
			else:
				star_args[name] = value
		if star_args:
			arg_strings.append('**%s' % repr(star_args))
		return '%s(%s)' % (type_name, ', '.join(arg_strings))

	@abstractmethod
	def create_empty_argparser(self) -> argparse.ArgumentParser:
		raise NotImplementedError

	def parse_args(self, args: Optional[Sequence[str]] = None):
		argparser = self.create_empty_argparser()
		for actor in self._actordict.values():
			actor.add_to_argparser(argparser, self)
		argparser.parse_args(args=args, namespace=self)
		for actor in self._actordict.values():
			actor.cleanup_container(self)
		return self
