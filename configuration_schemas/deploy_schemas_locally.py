import inspect
import json
import optparse
import os
import sys


SCHEMA_VERSION = '3'

def get_script_directory():
    return os.path.dirname(os.path.abspath(
        inspect.stack()[0][1]))

def makedirs_catch_preexisting(*args, **kwargs):
    try:
        os.makedirs(*args, **kwargs)
    except OSError as e:
        if e[0] != 17: # 17 == File exists
            raise

def is_schema_template(filename):
    return filename.endswith('.json') and not filename.startswith('sample')

def update_schema_dict(schema_dict, url_base_with_version, filename):
    schema_dict['id'] = '{url_base_with_version}/{filename}#'.format(**vars())

def main(template_directory, output_directory_base, url_base):
    url_base_with_version = '{0}/v{1}'.format(url_base, SCHEMA_VERSION)
    output_directory_with_version = os.path.join(output_directory_base, 'v' + SCHEMA_VERSION)
    makedirs_catch_preexisting(output_directory_with_version)
    for dirpath, _, filenames in os.walk(template_directory):
        for filename in filenames:
            if is_schema_template(filename):
                with open(os.path.join(dirpath, filename)) as f:
                    schema_dict = json.load(f)
                update_schema_dict(schema_dict, url_base_with_version, filename)
                with open(os.path.join(output_directory_with_version, filename), 'w') as f:
                    json.dump(schema_dict, f, indent=4, sort_keys=True)

if __name__ == '__main__':
    usage = '''\
Usage: python2 %prog --output_directory_base <path to desired deployment directory> [options]

  Deploys packaged schemas to <output_directory_base>.

  Sets "id" field of deployed schemas appropriately based on <url_base>.

  Example usage:

    python2 %prog --output_directory_base /tmp/irods_schemas

    This will create, if necessary, /tmp/irods_schemas and /tmp/irods_schemas/v{SCHEMA_VERSION}, then populate the latter with copies of the schema files currently found in {SCHEMAS_DIR}. The schema copies will have their "id" fields updated to reflect their new locations (e.g. "server_config.json" will have "id" field "file:///tmp/irods_schemas/v{SCHEMA_VERSION}/server_config.json#".

    To have an iRODS installation validate its configuration files against this local deployment, set the iRODS "schema_validation_base_uri" (found in "/etc/irods/server_config.json" or fed into the prompt during setup) to "file:///tmp/irods_schemas".

    Make sure that the Linux account running iRODS has read access to the deployed schemas.
'''.format(SCHEMA_VERSION=SCHEMA_VERSION, SCHEMAS_DIR=os.path.join(get_script_directory(), 'schemas'))

    parser = optparse.OptionParser(usage=usage)
    parser.add_option('--output_directory_base', help=optparse.SUPPRESS_HELP)
    parser.add_option('--template_directory', metavar='<path to alternate schema templates>')
    parser.add_option('--url_base', metavar='<alternate url base for "id" field>', help='Useful for creating web deployments')
    options, _ = parser.parse_args()

    if not options.output_directory_base:
        parser.print_help()
        sys.exit(1)
    options.output_directory_base = os.path.abspath(options.output_directory_base)

    if not options.template_directory:
        options.template_directory = os.path.join(get_script_directory(), 'schemas')

    if not options.url_base:
        options.url_base = 'file://' + options.output_directory_base

    options.url_base = options.url_base.rstrip('/')

    main(options.template_directory, options.output_directory_base, options.url_base)
