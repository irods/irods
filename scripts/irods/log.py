import logging
import sys
import time

class ColorFormatter(logging.Formatter):
    BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE = range(8)

    RESET_SEQ = '\033[0m'
    COLOR_SEQ_TEMPLATE = '\033[1;{fore_color_int}m'

    LEVELNO_TO_COLOR_INT_DICT = {
            logging.WARNING: YELLOW,
            logging.ERROR: RED,
        }

    def format(self, record):
        message = logging.Formatter.format(self, record)
        color_seq = ''
        if record.levelno in self.LEVELNO_TO_COLOR_INT_DICT:
            fore_color_int = 30 + self.LEVELNO_TO_COLOR_INT_DICT[record.levelno]
            color_seq = self.COLOR_SEQ_TEMPLATE.format(fore_color_int=fore_color_int)
        return '{0}{1}{2}'.format(color_seq, message, self.RESET_SEQ)

class NullHandler(logging.Handler):
    def emit(self, record):
        pass

class LessThanFilter(logging.Filter, object):
    def __init__(self, exclusive_maximum, name=""):
        super(LessThanFilter, self).__init__(name)
        self.max_level = exclusive_maximum

    def filter(self, record):
        return 1 if record.levelno < self.max_level else 0

class DeferInfoToDebugFilter(logging.Filter, object):
    def __init__(self, name=""):
        super(DeferInfoToDebugFilter, self).__init__(name)

    def filter(self, record):
        if record.levelno == logging.INFO:
            record.levelno = logging.DEBUG
            record.levelname = 'DEBUG'
        return 1

def register_tty_handler(stream, minlevel, maxlevel):
    logging_handler = logging.StreamHandler(stream)
    logging_handler.setFormatter(ColorFormatter('%(message)s'))
    if minlevel is not None:
        logging_handler.setLevel(minlevel)
    else:
        logging_handler.setLevel(logging.NOTSET)
    if maxlevel is not None:
        logging_handler.addFilter(LessThanFilter(maxlevel))
    logging.getLogger().addHandler(logging_handler)

def register_file_handler(log_file_path, level=logging.DEBUG):
    logging_handler = logging.FileHandler(log_file_path)
    logging_handler.setFormatter(logging.Formatter('%(asctime)s.%(msecs)03dZ - %(levelname)7s - %(filename)30s:%(lineno)4d - %(message)s', '%Y-%m-%dT%H:%M:%S'))
    logging_handler.formatter.converter = time.gmtime
    logging_handler.setLevel(logging.DEBUG)
    logging.getLogger().addHandler(logging_handler)
