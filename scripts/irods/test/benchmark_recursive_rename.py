#!/usr/bin/env python3

import argparse
import json
import os
import re
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import uuid


def run(command, env, *, input_text=None):
    completed_process = subprocess.run(
        command,
        env=env,
        input=input_text,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True)

    if completed_process.returncode != 0:
        raise RuntimeError(
            'command failed [{}]\nstdout:\n{}\nstderr:\n{}'.format(
                ' '.join(command), completed_process.stdout, completed_process.stderr))

    return completed_process.stdout


def load_environment_file(path):
    with open(path) as f:
        return json.load(f)


def prepare_irods_environment(args, temporary_directory):
    source_environment_file = args.environment_file or os.path.expanduser('~/.irods/irods_environment.json')

    if args.environment_file is None and not os.path.exists(source_environment_file):
        source_environment_file = '/var/lib/irods/.irods/irods_environment.json'

    environment_contents = load_environment_file(source_environment_file)
    temporary_environment_file = os.path.join(temporary_directory, 'irods_environment.json')

    with open(temporary_environment_file, 'w') as f:
        json.dump(environment_contents, f)

    environment = os.environ.copy()
    environment['IRODS_ENVIRONMENT_FILE'] = temporary_environment_file
    environment['IRODS_AUTHENTICATION_FILE'] = os.path.join(temporary_directory, 'irods_authentication')

    if args.password:
        run(['iinit'], environment, input_text='{}\n'.format(args.password))

    return environment, environment_contents


def count_query(attribute, where_clause, env):
    output = run(['iquest', '%s', 'select count({}) where {}'.format(attribute, where_clause)], env)
    match = re.search(r'(?:=\s*)?(\d+)', output)
    if not match:
        raise RuntimeError('failed to parse iquest count output: {}'.format(output))

    return int(match.group(1))


def make_resource(resource_name, vault_directory, policy, env):
    vault = '{}:{}'.format(socket.gethostname(), vault_directory)
    run(['iadmin', 'mkresc', resource_name, 'unixfilesystem', vault], env)
    run(['iadmin', 'modresc', resource_name, 'context', 'file_naming_policy={}'.format(policy)], env)


def remove_resource(resource_name, env):
    try:
        run(['iadmin', 'rmresc', resource_name], env)
    except RuntimeError as e:
        print('warning: failed to remove resource [{}]: {}'.format(resource_name, e), file=sys.stderr)


def make_local_tree(root_directory, collection_count, object_count):
    objects_per_collection = object_count // collection_count
    extra_objects = object_count % collection_count

    for collection_index in range(collection_count):
        collection = os.path.join(root_directory, 'c_{:04d}'.format(collection_index))
        os.mkdir(collection)
        number_of_objects = objects_per_collection + (1 if collection_index < extra_objects else 0)

        for object_index in range(number_of_objects):
            open(os.path.join(collection, 'obj_{:04d}.txt'.format(object_index)), 'wb').close()


def put_collection_tree(local_root_directory, destination_collection, resource_name, env):
    run(['iput', '-R', resource_name, '-r', local_root_directory, destination_collection], env)


def benchmark_policy(policy, args, env, environment_contents):
    run_id = uuid.uuid4().hex[:8]
    resource_name = '{}_{}_{}'.format(args.name_prefix, policy, run_id)
    root_collection_name = '{}_{}_{}'.format(args.name_prefix, policy, run_id)
    home_collection = '/{}/home/{}'.format(
        environment_contents['irods_zone_name'],
        environment_contents['irods_user_name'])
    root_collection = '/{}/home/{}/{}_{}_{}'.format(
        environment_contents['irods_zone_name'],
        environment_contents['irods_user_name'],
        args.name_prefix,
        policy,
        run_id)
    moved_collection = root_collection + '_moved'
    vault_parent = tempfile.mkdtemp(prefix='{}-{}-'.format(args.name_prefix, policy))
    os.chmod(vault_parent, 0o755)
    vault_directory = os.path.join(vault_parent, resource_name + '_vault')
    os.mkdir(vault_directory)
    os.chmod(vault_directory, 0o777)
    local_root_directory = os.path.join(vault_parent, root_collection_name)

    result = {
        'policy': policy,
        'resource': resource_name,
        'root_collection': root_collection,
        'moved_collection': moved_collection,
        'collections': args.collections,
        'data_objects': args.objects,
        'rename_seconds': None,
    }

    try:
        make_resource(resource_name, vault_directory, policy, env)
        os.mkdir(local_root_directory)
        make_local_tree(local_root_directory, args.collections, args.objects)
        put_collection_tree(local_root_directory, home_collection, resource_name, env)

        expected_collection_count = count_query('COLL_ID', "COLL_NAME like '{}%'".format(root_collection), env)
        expected_object_count = count_query('DATA_ID', "COLL_NAME like '{}%'".format(root_collection), env)

        if expected_collection_count != args.collections + 1:
            raise RuntimeError('expected {} collections, found {}'.format(args.collections + 1, expected_collection_count))

        if expected_object_count != args.objects:
            raise RuntimeError('expected {} data objects, found {}'.format(args.objects, expected_object_count))

        start = time.perf_counter()
        run(['imv', root_collection, moved_collection], env)
        result['rename_seconds'] = time.perf_counter() - start

        moved_collection_count = count_query('COLL_ID', "COLL_NAME like '{}%'".format(moved_collection), env)
        moved_object_count = count_query('DATA_ID', "COLL_NAME like '{}%'".format(moved_collection), env)

        if moved_collection_count != args.collections + 1:
            raise RuntimeError('after rename, expected {} collections, found {}'.format(args.collections + 1, moved_collection_count))

        if moved_object_count != args.objects:
            raise RuntimeError('after rename, expected {} data objects, found {}'.format(args.objects, moved_object_count))

        return result
    finally:
        if not args.keep:
            for collection in [moved_collection, root_collection]:
                try:
                    run(['irm', '-rf', collection], env)
                except RuntimeError:
                    pass

            remove_resource(resource_name, env)
            shutil.rmtree(vault_parent, ignore_errors=True)


def main():
    parser = argparse.ArgumentParser(description='Time recursive collection rename for file_naming_policy values.')
    parser.add_argument('--collections', type=int, default=500)
    parser.add_argument('--objects', type=int, default=5000)
    parser.add_argument('--policies', nargs='+', default=['consistent', 'random'], choices=['consistent', 'random'])
    parser.add_argument('--name-prefix', default='recursive_rename_bench')
    parser.add_argument('--environment-file')
    parser.add_argument('--password', default=os.environ.get('IRODS_PASSWORD', 'rods'))
    parser.add_argument('--keep', action='store_true', help='leave collections, resources, and vaults in place')
    args = parser.parse_args()

    if args.collections < 1:
        raise SystemExit('--collections must be greater than zero')

    if args.objects < 0:
        raise SystemExit('--objects cannot be negative')

    with tempfile.TemporaryDirectory(prefix='irods-recursive-rename-benchmark-') as temporary_directory:
        env, environment_contents = prepare_irods_environment(args, temporary_directory)
        results = [benchmark_policy(policy, args, env, environment_contents) for policy in args.policies]

    print(json.dumps(results, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
