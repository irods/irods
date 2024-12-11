#!/usr/bin/env python3

import argparse
import locale
import logging
import os.path
import platform
import re
import subprocess
from collections import namedtuple
from pathlib import Path

import distro

__version__ = '0.1.1'

# Container for caching return values for functions with no args
# @functools.lru_cache is overkill for no arg funcs
# (@functools.cache probably is too), and @functools.cache and
# @functools.cached_property weren't added until Python 3.9 and
# 3.8, respectively. @functools.cached_property only works
# inside classes anyway.
_CachedValue = namedtuple('CachedValue', ['value'])

# Version class based on distutils.version.LooseVersion
class DistroVersion:

	component_re = re.compile(r'(\d+ | [a-z]+ | \.)', re.VERBOSE)

	def __init__(self, vstring=None):
		if vstring:
			self.vstring = vstring
			components = [x for x in self.component_re.split(vstring) if x and x != '.']
			for i, obj in enumerate(components):
				try:
					components[i] = int(obj)
				except ValueError:
					pass

			self.version = components

	def __str__(self):
		return self.vstring

	def __repr__(self):
		return f"DistroVersion ('{self}')"

	def __eq__(self, other):
		if isinstance(other, str):
			other = DistroVersion(other)
		elif not isinstance(other, DistroVersion):
			return NotImplemented
		return self.version == other.version

	def __ne__(self, other):
		if isinstance(other, str):
			other = DistroVersion(other)
		elif not isinstance(other, DistroVersion):
			return NotImplemented
		return self.version != other.version

	def __lt__(self, other):
		if isinstance(other, str):
			other = DistroVersion(other)
		elif not isinstance(other, DistroVersion):
			return NotImplemented
		return self.version < other.version

	def __le__(self, other):
		if isinstance(other, str):
			other = DistroVersion(other)
		elif not isinstance(other, DistroVersion):
			return NotImplemented
		return self.version <= other.version

	def __gt__(self, other):
		if isinstance(other, str):
			other = DistroVersion(other)
		elif not isinstance(other, DistroVersion):
			return NotImplemented
		return self.version > other.version

	def __ge__(self, other):
		if isinstance(other, str):
			other = DistroVersion(other)
		elif not isinstance(other, DistroVersion):
			return NotImplemented
		return self.version >= other.version

# Distills a distribution down to its primary base.
# For example, CentOS and Rocky Linux will return `rhel`,
# Linux Mint and Elementary will return ubuntu,
# Manjaro will return arch.
def distribution_type(distro_id_chain = None):
	global _cached_distribution_type
	if distro_id_chain is None:
		if _cached_distribution_type is not None:
			return _cached_distribution_type.value
		distro_id = distro.id()
		distro_like = distro.like()
		if distro_like in [None, 'n/a']:
			distro_like = ''
		distro_id_chain = distro_like.split()
	else:
		distro_id = distro_id_chain.pop(0)
	if distro_id == 'ol': # special handling for Oracle Linux
		_cached_distribution_type = _CachedValue('rhel')
		return _cached_distribution_type.value
	if len(distro_id_chain) == 0 or distro_id in ['rhel', 'ubuntu', 'suse']:
		_cached_distribution_type = _CachedValue(distro_id)
		return _cached_distribution_type.value
	return distribution_type(distro_id_chain)
_cached_distribution_type = None

# Get the relevant parts of the distribution version number.
# Distributions without a version number will return None.
# Attempts to account for derivatives that deviate from the upstream version scheme.
# For example, Amazon Linux 2 will return '7' since it is based on EL7.
def distribution_version():
	global _cached_distribution_version
	if _cached_distribution_version is not None:
		return _cached_distribution_version.value
	distro_type = distribution_type()
	if distro_type in ['ubuntu', 'alpine', 'suse', 'sles', 'sled']:
		distro_version = '{0}.{1}'.format(distro.major_version(), distro.minor_version())
		if distro_type == 'ubuntu':
			upstream_version = distro.os_release_info().get('ubuntu_version_id', None)
			if upstream_version:
				distro_version = upstream_version
			elif os.path.isfile('/etc/upstream-release/lsb-release'):
				upstream_ld = distro.LinuxDistribution(include_lsb=False, include_uname=False, os_release_file='/etc/upstream-release/lsb-release')
				distro_version = upstream_ld.os_release_info().get('distrib_release', distro_version)
		distro_version = DistroVersion(distro_version)
	elif distro_type in ['rhel']:
		encoding = locale.getpreferredencoding(False)
		el_ver = subprocess.check_output(['rpm', '-E', r'%{rhel}'], timeout=5, encoding=encoding)
		el_ver = el_ver.strip()
		if el_ver == r'%{rhel}':
			el_ver = distro.major_version()
		if el_ver in ['', 'n/a']:
			distro_version = None
		else:
			distro_version = DistroVersion(el_ver)
	elif distro_type in ['fedora']:
		encoding = locale.getpreferredencoding(False)
		fc_ver = subprocess.check_output(['rpm', '-E', r'%{fedora}'], timeout=5, encoding=encoding)
		fc_ver = fc_ver.strip()
		if fc_ver == r'%{fedora}':
			fc_ver = distro.major_version()
		if fc_ver in ['', 'n/a']:
			distro_version = None
		else:
			distro_version = DistroVersion(fc_ver)
	else:
		distro_version = distro.major_version()
		if distro_version in ['', 'n/a']:
			distro_version = None
		else:
			distro_version = DistroVersion(distro_version)
	_cached_distribution_version = _CachedValue(distro_version)
	return distro_version
_cached_distribution_version = None

# Get the distribution version codename.
# Distributions without a codename will return None.
def distribution_codename():
	global _cached_distribution_codename
	if _cached_distribution_codename is not None:
		return _cached_distribution_codename.value
	codename = None
	try:
		# Older versions of distro return a bad string here... Try using the deprecated method if available. This
		# function was removed in Python 3.8 so newer platforms will fall back to distro.
		from platform import linux_distribution
		codename = linux_distribution()[2]
	except ImportError:
		codename = distro.codename()
	if codename in ['', 'n/a']:
		codename = None
	elif distribution_type() == 'ubuntu':
		codename = distro.os_release_info().get('ubuntu_codename', codename)
	_cached_distribution_codename = _CachedValue(codename)
	return codename
_cached_distribution_codename = None

# Returns the fpm package type for the distribution packages
def package_type():
	global _cached_package_type
	if _cached_package_type is not None:
		return _cached_package_type.value
	log = logging.getLogger(__name__)
	distro_type = distribution_type()
	if distro_type in ['debian', 'ubuntu']:
		pkg_type = 'deb'
	elif distro_type in ['amzn', 'fedora', 'rhel', 'scientific', 'sles', 'sled', 'suse']:
		pkg_type = 'rpm'
	elif distro_type in ['alpine']:
		pkg_type = 'apk'
	elif distro_type in ['arch']:
		pkg_type = 'pacman'
	else:
		module_fname = Path(__file__).name
		log.warning(f'{module_fname} does not know what package type is used in distro \'{distro_type}\'.')
		pkg_type = None
	_cached_package_type = _CachedValue(pkg_type)
	return pkg_type
_cached_package_type = None

# Returns the filename extension of distribution packages
# NOTE: If we need to add proper support for platforms that have native packages with
# multiple filename extensions (arch, for example), this will need some tweaking
def package_filename_extension():
	global _cached_package_filename_extension
	if _cached_package_filename_extension is not None:
		return _cached_package_filename_extension.value
	log = logging.getLogger(__name__)
	pkg_type = package_type()
	if pkg_type == 'deb':
		pkg_suffix = 'deb'
	elif pkg_type == 'rpm':
		pkg_suffix = 'rpm'
	elif pkg_type == 'apk':
		pkg_suffix = 'apk'
	elif pkg_type == 'pacman':
		pkg_suffix = 'pkg.tar'
	elif pkg_type is None:
		log.warning('Package type must be known before package filename extension can be discerned.')
		pkg_suffix = None
	else:
		module_fname = Path(__file__).name
		log.warning(f'{module_fname} does not know what package filename extension is used for package type \'{pkg_type}\'.')
		pkg_suffix = None
	_cached_package_filename_extension = _CachedValue(pkg_suffix)
	return pkg_suffix
_cached_package_filename_extension = None

# Returns the architecture string to use for packaging
def package_architecture_string():
	global _cached_package_architecture_string
	if _cached_package_architecture_string is not None:
		return _cached_package_architecture_string.value
	log = logging.getLogger(__name__)
	if package_type() == 'deb':
		encoding = locale.getpreferredencoding(False)
		pkg_arch = subprocess.check_output(['dpkg', '--print-architecture'], timeout=5, encoding=encoding)
		pkg_arch = pkg_arch.strip()
	else:
		if package_type() is None:
			log.warning('Unknown package type, architecture string may be inaccurate.')
		pkg_arch = platform.machine()
	_cached_package_architecture_string = _CachedValue(pkg_arch)
	return pkg_arch
_cached_package_architecture_string = None


# This implementation of main is written specifically for use by the irods buildsystem, and will be changed
# significantly for the standalone release of this tool.
def main():
	parser = argparse.ArgumentParser(description='distro info tool', add_help=True)
	data_group = parser.add_mutually_exclusive_group(required=True)
	data_group.add_argument(
		'--distilled-type',
		help='Get distilled distro type',
		action='store_true',
		dest='distilled_type'
	)
	data_group.add_argument(
		'--distilled-version',
		help='Get distilled distro version',
		action='store_true',
		dest='distilled_version'
	)
	data_group.add_argument(
		'--distilled-codename',
		help='Get distilled distro codename',
		action='store_true',
		dest='distilled_codename'
	)
	data_group.add_argument(
		'--distro-package-type',
		help='Get distro package type',
		action='store_true',
		dest='package_type'
	)

	args = parser.parse_args()

	if args.distilled_type:
		dt = distribution_type()
		if dt is None:
			print('none', end='')
		else:
			print(dt, end='')
		return 0

	if args.distilled_version:
		dv = distribution_version()
		if dv is None:
			print('none', end='')
		else:
			print(str(dv), end='')
		return 0

	if args.distilled_codename:
		dc = distribution_codename()
		if dc is None:
			print('none', end='')
		else:
			print(dc, end='')
		return 0

	if args.package_type:
		pt = package_type()
		if pt is None:
			print('none', end='')
		else:
			print(pt, end='')
		return 0

	return -1

if __name__ == "__main__":
    main()
