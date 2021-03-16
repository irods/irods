#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import sys, errno  # noqa: I201,I001; pylint: disable=C0410

if __name__ == '__main__':
	print(__file__ + ': This module is not meant to be invoked directly.',  # noqa: T001
	      file=sys.stderr)
	sys.exit(errno.ELIBEXEC)

import os, shutil  # noqa: I100,I202; pylint: disable=C0410
import ctypes  # noqa: I100
import io
import logging
import multiprocessing
import signal, traceback  # pylint: disable=C0410
import textwrap
from pprint import PrettyPrinter as _pprinter  # noqa: N813
from typing import Any, Optional

from compat_shims import NoReturn, Sequence

__all__ = [
	'TRACE',
	'MESSAGE',
	'STATUS',
	'trace',
	'message',
	'status',
	'LoggingColorFormatter',
	'get_term_sz',
	'PrettyPrinter',
	'pformat',
	'pprint',
	'pformat_captioned',
	'pprint_captioned',
	'sizeof_fmt',
	'dump_stacktrace',
	'setup_stacktrace_signal',
	'init_logging',
]

_logging_initialized = False  # pylint: disable=C0103

TRACE   = logging.DEBUG - 5
MESSAGE = logging.INFO  + 2
STATUS  = logging.INFO  + 5


def trace(logger: logging.Logger, msg: str, *args: Any, **kwargs: Any) -> NoReturn:
	logger.log(TRACE, msg, *args, **kwargs)


def message(logger: logging.Logger, msg: str, *args: Any, **kwargs: Any) -> NoReturn:
	logger.log(MESSAGE, msg, *args, **kwargs)


def status(logger: logging.Logger, msg: str, *args: Any, **kwargs: Any) -> NoReturn:
	logger.log(STATUS, msg, *args, **kwargs)


class LoggingColorFormatter(logging.Formatter):
	STYLE_RESET_OFFSET = 20
	ALL_RESET = 0

	STYLE_BRIGHT    = 1
	STYLE_DIM       = 2
	STYLE_BLINK     = 4
	STYLE_INVERT    = 7
	STYLE_HIDE      = 8

	STYLE_BRIGHT_RESET  = STYLE_RESET_OFFSET + STYLE_BRIGHT
	STYLE_DIM_RESET     = STYLE_RESET_OFFSET + STYLE_DIM
	STYLE_BLINK_RESET   = STYLE_RESET_OFFSET + STYLE_BLINK
	STYLE_INVERT_RESET  = STYLE_RESET_OFFSET + STYLE_INVERT
	STYLE_HIDE_RESET    = STYLE_RESET_OFFSET + STYLE_HIDE

	FG_COLOR_START      = 30
	BG_COLOR_START      = FG_COLOR_START + 10
	COLOR_ALT_OFFSET    = 60

	COLOR_IDX_RESET     = 9

	COLOR_IDX_BLACK     = 0
	COLOR_IDX_RED       = 1
	COLOR_IDX_GREEN     = 2
	COLOR_IDX_YELLOW    = 3
	COLOR_IDX_BLUE      = 4
	COLOR_IDX_MAGENTA   = 5
	COLOR_IDX_CYAN      = 6
	COLOR_IDX_LIGHTGRAY = 7

	COLOR_IDX_DARKGRAY      = COLOR_ALT_OFFSET + COLOR_IDX_BLACK
	COLOR_IDX_LIGHTRED      = COLOR_ALT_OFFSET + COLOR_IDX_RED
	COLOR_IDX_LIGHTGREEN    = COLOR_ALT_OFFSET + COLOR_IDX_GREEN
	COLOR_IDX_LIGHTYELLOW   = COLOR_ALT_OFFSET + COLOR_IDX_YELLOW
	COLOR_IDX_LIGHTBLUE     = COLOR_ALT_OFFSET + COLOR_IDX_BLUE
	COLOR_IDX_LIGHTMAGENTA  = COLOR_ALT_OFFSET + COLOR_IDX_MAGENTA
	COLOR_IDX_LIGHTCYAN     = COLOR_ALT_OFFSET + COLOR_IDX_CYAN
	COLOR_IDX_WHITE         = COLOR_ALT_OFFSET + COLOR_IDX_LIGHTGRAY

	FG_RESET = FG_COLOR_START + COLOR_IDX_RESET

	FG_BLACK        = FG_COLOR_START + COLOR_IDX_BLACK
	FG_RED          = FG_COLOR_START + COLOR_IDX_RED
	FG_GREEN        = FG_COLOR_START + COLOR_IDX_GREEN
	FG_YELLOW       = FG_COLOR_START + COLOR_IDX_YELLOW
	FG_BLUE         = FG_COLOR_START + COLOR_IDX_BLUE
	FG_MAGENTA      = FG_COLOR_START + COLOR_IDX_MAGENTA
	FG_CYAN         = FG_COLOR_START + COLOR_IDX_CYAN
	FG_LIGHTGRAY    = FG_COLOR_START + COLOR_IDX_LIGHTGRAY
	FG_DARKGRAY     = FG_COLOR_START + COLOR_IDX_DARKGRAY
	FG_LIGHTRED     = FG_COLOR_START + COLOR_IDX_LIGHTRED
	FG_LIGHTGREEN   = FG_COLOR_START + COLOR_IDX_LIGHTGREEN
	FG_LIGHTYELLOW  = FG_COLOR_START + COLOR_IDX_LIGHTYELLOW
	FG_LIGHTBLUE    = FG_COLOR_START + COLOR_IDX_LIGHTBLUE
	FG_LIGHTMAGENTA = FG_COLOR_START + COLOR_IDX_LIGHTMAGENTA
	FG_LIGHTCYAN    = FG_COLOR_START + COLOR_IDX_LIGHTCYAN
	FG_WHITE        = FG_COLOR_START + COLOR_IDX_WHITE

	BG_RESET = BG_COLOR_START + COLOR_IDX_RESET

	BG_BLACK        = BG_COLOR_START + COLOR_IDX_BLACK
	BG_RED          = BG_COLOR_START + COLOR_IDX_RED
	BG_GREEN        = BG_COLOR_START + COLOR_IDX_GREEN
	BG_YELLOW       = BG_COLOR_START + COLOR_IDX_YELLOW
	BG_BLUE         = BG_COLOR_START + COLOR_IDX_BLUE
	BG_MAGENTA      = BG_COLOR_START + COLOR_IDX_MAGENTA
	BG_CYAN         = BG_COLOR_START + COLOR_IDX_CYAN
	BG_LIGHTGRAY    = BG_COLOR_START + COLOR_IDX_LIGHTGRAY
	BG_DARKGRAY     = BG_COLOR_START + COLOR_IDX_DARKGRAY
	BG_LIGHTRED     = BG_COLOR_START + COLOR_IDX_LIGHTRED
	BG_LIGHTGREEN   = BG_COLOR_START + COLOR_IDX_LIGHTGREEN
	BG_LIGHTYELLOW  = BG_COLOR_START + COLOR_IDX_LIGHTYELLOW
	BG_LIGHTBLUE    = BG_COLOR_START + COLOR_IDX_LIGHTBLUE
	BG_LIGHTMAGENTA = BG_COLOR_START + COLOR_IDX_LIGHTMAGENTA
	BG_LIGHTCYAN    = BG_COLOR_START + COLOR_IDX_LIGHTCYAN
	BG_WHITE        = BG_COLOR_START + COLOR_IDX_WHITE

	@classmethod
	def get_sequence(cls, *args: Any) -> str:
		if len(args) < 1:
			return None
		return '\033[' + ';'.join([str(arg) for arg in args]) + 'm'

	def __init__(self, *args, **kwargs):  # noqa: ANN002,ANN003,ANN204
		super().__init__(*args, **kwargs)
		self.enabled = sys.stdout.isatty()
		self.reset_all_seq = self.get_sequence(self.ALL_RESET)
		self.critical_seq = self.get_sequence(self.STYLE_BRIGHT, self.FG_LIGHTRED, self.BG_DARKGRAY)
		self.error_seq = self.get_sequence(self.STYLE_BRIGHT, self.FG_RED)
		self.warning_seq = self.get_sequence(self.STYLE_BRIGHT, self.FG_YELLOW)
		self.status_seq = self.get_sequence(self.STYLE_BRIGHT, self.FG_BLUE)
		self.message_seq = self.get_sequence(self.STYLE_BRIGHT, self.FG_LIGHTGRAY)

	def format(self, record: logging.LogRecord) -> str:  # noqa: A003
		msg = super().format(record)

		if not self.enabled:
			return msg

		if record.levelno >= logging.CRITICAL:
			return self.critical_seq + msg + self.reset_all_seq
		elif record.levelno >= logging.ERROR:
			return self.error_seq + msg + self.reset_all_seq
		elif record.levelno >= logging.WARNING:
			return self.warning_seq + msg + self.reset_all_seq
		elif record.levelno >= STATUS:
			return self.status_seq + msg + self.reset_all_seq
		elif record.levelno >= MESSAGE:
			return self.message_seq + msg + self.reset_all_seq

		return msg


_last_term_sz = os.terminal_size((80, 24))


def get_term_sz() -> os.terminal_size:
	global _last_term_sz  # pylint: disable=C0103,W0603
	_last_term_sz = shutil.get_terminal_size(_last_term_sz)
	return _last_term_sz


class PrettyPrinter(_pprinter):

	def __init__(
		self,
		*args,  # noqa: ANN002
		indent: int = 2,
		width: Optional[int] = None,
		caption_separator: str = ': ',
		**kwargs  # noqa: ANN003
	):
		if width is None:
			width = get_term_sz().columns

		super().__init__(indent=indent, width=width, *args, **kwargs)

		self._caption_separator = caption_separator

	def pprint(self, obj: Any, *, flush: bool = False) -> NoReturn:  # noqa: T004; pylint: disable=W0221
		super().pprint(obj)
		if flush:
			self._stream.flush()

	def _pformat_captioned(self, cap: str, obj: Any) -> Sequence[str]:
		cap = cap + self._caption_separator
		indent_sz = len(cap)

		prevwidth = self._width
		try:
			self._width = self._width - (indent_sz + 1)
			if not self._width:
				raise ValueError('_width == 0')
			pfstrp = list(self.pformat(obj).rstrip().partition('\n'))
		finally:
			self._width = prevwidth

		pfstrp[0] = cap + pfstrp[0]
		if pfstrp[2]:
			pfstrp[2] = textwrap.indent(pfstrp[2], ' ' * indent_sz)

		return pfstrp

	def pformat_captioned(self, caption: str, obj: Any) -> str:
		pfstrp = self._pformat_captioned(caption, obj)
		return ''.join(pfstrp)

	def pprint_captioned(self, caption: str, obj: Any, flush: bool = False) -> NoReturn:
		pfstrp = self._pformat_captioned(caption, obj)
		for pfstr in pfstrp:
			self._stream.write(pfstr)
		self._stream.write('\n')
		if flush:
			self._stream.flush()


def pformat(obj: Any, *args, **kwargs) -> str:  # noqa: ANN002,ANN003
	pp = PrettyPrinter(*args, **kwargs)
	return pp.pformat(obj)


def pprint(obj: Any, *args, **kwargs) -> NoReturn:  # noqa: T004,ANN002,ANN003
	pp = PrettyPrinter(*args, **kwargs)
	pp.pprint(obj)


def pformat_captioned(caption: str, obj: Any, *args, **kwargs) -> str:  # noqa: ANN002,ANN003
	pp = PrettyPrinter(*args, **kwargs)
	return pp.pformat_captioned(caption, obj)


def pprint_captioned(caption: str, obj: Any, *args, **kwargs) -> NoReturn:  # noqa: ANN002,ANN003
	pp = PrettyPrinter(*args, **kwargs)
	pp.pprint_captioned(caption, obj)


def sizeof_fmt(num: int, suffix: str = 'B') -> str:
	for unit in ['', 'Ki', 'Mi', 'Gi', 'Ti', 'Pi', 'Ei', 'Zi']:
		if abs(num) < 1024.0:
			return '%3.1f%s%s' % (num, unit, suffix)
		num /= 1024.0
	return '%.1f%s%s' % (num, 'Yi', suffix)


def dump_stacktrace(sig, frame) -> NoReturn:  # noqa: ANN001
	msgb = io.StringIO()
	tid = ctypes.CDLL('libc.so.6').syscall(186)
	msgb.write('Signal {0} received.\n'.format(sig))
	msgb.write('PID: {0}; TID: {1}\n'.format(os.getpid(), tid))
	msgb.write('Multiprocessing start method: {0}\n'.format(multiprocessing.get_start_method()))
	msgb.write('Stacktrace:\n')
	for fmt_stk in traceback.format_stack(frame):
		msgb.write(fmt_stk)
	print(msgb.getvalue(), file=sys.stderr)  # noqa: T001


def setup_stacktrace_signal() -> NoReturn:
	signal.signal(signal.SIGQUIT, dump_stacktrace)
	#print('PID:', os.getpid(), file=sys.stderr)  # noqa: T001


def init_logging(verbosity: int) -> NoReturn:
	# we don't want redundant output, so make sure we don't initialize more than once
	global _logging_initialized  # pylint: disable=C0103,W0603
	if _logging_initialized:
		return

	log_level_offset = 0
	verbosity = verbosity + log_level_offset

	l = logging.getLogger()  # noqa: VNE001,E741

	logging.addLevelName(STATUS, 'STATUS')
	logging.addLevelName(MESSAGE, 'MESSAGE')
	logging.addLevelName(TRACE, 'TRACE')

	l.setLevel(logging.NOTSET)

	formatter = LoggingColorFormatter()

	err_handler = logging.StreamHandler(sys.stderr)
	err_handler.setFormatter(formatter)

	info_handler = logging.StreamHandler(sys.stdout)
	info_handler.setFormatter(formatter)
	info_handler.addFilter(lambda rec: 1 if rec.levelno < logging.WARNING else 0)
	info_handler.setLevel(logging.CRITICAL)

	trace_handler = logging.StreamHandler(sys.stderr)
	trace_handler.setFormatter(formatter)
	trace_handler.addFilter(lambda rec: 1 if rec.levelno < logging.INFO else 0)
	trace_handler.setLevel(logging.CRITICAL)

	err_handler.setLevel(logging.ERROR)
	if verbosity >= -2:
		err_handler.setLevel(logging.WARNING)
	if verbosity == -1:
		info_handler.setLevel(STATUS)
	elif verbosity == 0:
		info_handler.setLevel(MESSAGE)
	elif verbosity >= 1:
		info_handler.setLevel(logging.INFO)
	if verbosity == 2:
		trace_handler.setLevel(logging.DEBUG)
	elif verbosity == 3:
		trace_handler.setLevel(TRACE)
	elif verbosity >= 4:
		trace_handler.setLevel(logging.NOTSET)
	if verbosity == 5:
		import multiprocessing.util as multiproc_util
		multiproc_util.log_to_stderr(logging.NOTSET)

	l.addHandler(err_handler)
	l.addHandler(info_handler)
	l.addHandler(trace_handler)

	_logging_initialized = True
