from __future__ import print_function

import glob
import itertools
import logging
import multiprocessing
import optparse
import os
import shutil
import sys
import tempfile
import time

import irods_python_ci_utilities


def build(debug_build, output_root_directory):
    install_os_specific_dependencies()
    try:
        irods_source_dir = build_irods(debug_build)
    finally:
        if output_root_directory:
            copy_output_packages(irods_source_dir, output_root_directory)

def install_os_specific_dependencies():
    dispatch_map = {
        'Ubuntu': install_os_specific_dependencies_apt,
        'Centos': install_os_specific_dependencies_yum,
        'Centos linux': install_os_specific_dependencies_yum,
    }
    try:
        return dispatch_map[irods_python_ci_utilities.get_distribution()]()
    except KeyError:
        irods_python_ci_utilities.raise_not_implemented_for_distribution()

def install_os_specific_dependencies_apt():
    irods_python_ci_utilities.install_os_packages([
        'fakeroot', 'g++', 'help2man', 'libbz2-dev', 'libcurl4-gnutls-dev', 'libfuse-dev',
        'libjson-perl', 'libkrb5-dev', 'libpam0g-dev', 'libssl-dev', 'libxml2-dev',
        'make', 'python-dev', 'unixodbc', 'unixodbc-dev', 'zlib1g-dev',
    ])

    extract_dir = extract_oci_tar()
    irods_python_ci_utilities.install_os_packages(['alien', 'libaio1'])
    irods_python_ci_utilities.subprocess_get_output('sudo alien -i {0}/*'.format(extract_dir), shell=True, check_rc=True)

def install_os_specific_dependencies_yum():
    packages_to_install = [
        'bzip2-devel', 'curl-devel', 'fakeroot', 'fuse-devel', 'help2man', 'krb5-devel',
        'libxml2-devel', 'openssl-devel', 'pam-devel', 'perl-JSON', 'python-devel',
        'python-psutil', 'unixODBC', 'unixODBC-devel', 'zlib-devel',
    ]
    if irods_python_ci_utilities.get_distribution_version_major() == '7':
        packages_to_install.append('mysql++-devel')
    irods_python_ci_utilities.install_os_packages(packages_to_install)

    if irods_python_ci_utilities.get_distribution_version_major() == '6':
        extract_dir = extract_oci_tar()
        irods_python_ci_utilities.subprocess_get_output('sudo rpm -i --nodeps {0}/*'.format(extract_dir), shell=True, check_rc=True)

def extract_oci_tar():
    with tempfile.NamedTemporaryFile() as f:
        irods_python_ci_utilities.subprocess_get_output(['wget', 'http://people.renci.org/~jasonc/irods/oci.tar', '-O', f.name], check_rc=True)
        extract_dir = tempfile.mkdtemp(prefix='extracted_oci.tar')
        logging.getLogger(__name__).info('Extracting oci.tar to: %s', extract_dir)
        irods_python_ci_utilities.subprocess_get_output(['tar', '-xf', f.name, '-C', extract_dir], check_rc=True)
    return extract_dir

def build_irods(debug_build):
    irods_source_dir = os.path.dirname(os.path.realpath(__file__))
    os.makedirs(os.path.join(irods_source_dir, 'build'))
    build_flags = '' if debug_build else '-r'
    irods_python_ci_utilities.subprocess_get_output(
        'sudo ./packaging/build.sh {0} icat postgres 2>&1 | tee ./build/build_icat_psql.out; exit $PIPESTATUS'.format(build_flags), cwd=irods_source_dir, shell=True, check_rc=True)
    irods_python_ci_utilities.subprocess_get_output(
        'sudo ./packaging/build.sh {0} resource postgres 2>&1 | tee ./build/build_resource_psql.out; exit $PIPESTATUS'.format(build_flags), cwd=irods_source_dir, shell=True, check_rc=True)
    irods_python_ci_utilities.subprocess_get_output(
        'sudo ./packaging/build.sh {0} icat mysql 2>&1 | tee ./build/build_icat_mysql.out; exit $PIPESTATUS'.format(build_flags), cwd=irods_source_dir, shell=True, check_rc=True)
    if should_build_oracle_plugin():
        build_oracle_plugin(irods_source_dir, build_flags)
    return irods_source_dir

def should_build_oracle_plugin():
    if irods_python_ci_utilities.get_distribution() == 'Ubuntu':
        return True
    if irods_python_ci_utilities.get_distribution() == 'Centos':
        return True
    return False

def build_oracle_plugin(irods_source_dir, build_flags):
    irods_python_ci_utilities.subprocess_get_output(
        'sudo ./packaging/build.sh {0} icat oracle 2>&1 | tee ./build/build_icat_oracle.out; exit $PIPESTATUS'.format(build_flags), cwd=irods_source_dir, shell=True, check_rc=True)

def copy_output_packages(irods_source_dir, output_root_directory):
    irods_python_ci_utilities.mkdir_p(output_root_directory)
    shutil.copytree(os.path.join(irods_source_dir, 'build'), irods_python_ci_utilities.append_os_specific_directory(output_root_directory))

def register_log_handler():
    logging.getLogger().setLevel(logging.INFO)
    logging_handler = logging.StreamHandler(sys.stdout)
    logging_handler.setFormatter(logging.Formatter(
       '%(asctime)s - %(levelname)7s - %(filename)30s:%(lineno)4d - %(message)s',
       '%Y-%m-%dT%H:%M:%SZ'))
    logging_handler.formatter.converter = time.gmtime
    logging_handler.setLevel(logging.INFO)
    logging.getLogger().addHandler(logging_handler)

def main():
    parser = optparse.OptionParser()
    parser.add_option('--debug_build', default='false')
    parser.add_option('--output_root_directory')
    parser.add_option('--just_install_dependencies', action='store_true', default=False)
    parser.add_option('--verbose', action='store_true', default=False)
    options, _ = parser.parse_args()

    if options.verbose:
        register_log_handler()

    if options.just_install_dependencies:
        install_os_specific_dependencies()
        return

    if options.debug_build not in ['false', 'true']:
        print('--debug_build must be either "false" or "true"', file=sys.stderr)
        sys.exit(1)

    build(options.debug_build == 'true', options.output_root_directory)

if __name__ == '__main__':
    main()
