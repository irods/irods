from __future__ import print_function
import json
import logging
import pprint
import sys

import irods_six

import irods_logging

try:
    import jsonschema
except ImportError:
    pass

try:
    import requests
except ImportError:
    pass

class ValidationError(Exception):
    pass

class ValidationWarning(Warning):
    pass

def load_and_validate(config_file, schema_uri):
    l = logging.getLogger(__name__)
    try:
        # load configuration file
        with open(config_file, 'r') as f:
            config_dict = json.load(f)
    except BaseException as e:
        irods_six.reraise(ValidationError, ValidationError('\n\t'.join([
            'ERROR: Validation Failed for [{0}]:'.format(config_file),
            'against [{0}]'.format(schema_uri),
            '{0}: {1}'.format(e.__class__.__name__, e)])),
                          sys.exc_info()[2])
    validate_dict(config_dict, schema_uri, name=config_file)
    return config_dict

def validate_dict(config_dict, schema_uri, name=None):
    l = logging.getLogger(__name__)
    if name is None:
        name = schema_uri.rpartition('/')[2]
    try:
        e = jsonschema.exceptions
    except AttributeError:
        irods_six.reraise(ValidationWarning, ValidationWarning(
                'WARNING: Validation failed for {0} -- jsonschema too old v[{1}]'.format(
                    name, jsonschema.__version__)),
            sys.exc_info()[2])
    except NameError:
        irods_six.reraise(ValidationWarning, ValidationWarning(
                'WARNING: Validation failed for {0} -- jsonschema not installed'.format(
                    name)),
            sys.exc_info()[2])

    try:
        schema = get_initial_schema(schema_uri)
        l.debug('Validating %s against json schema:', name)
        l.debug(pprint.pformat(schema))
        jsonschema.validate(config_dict, schema)

    except (jsonschema.exceptions.RefResolutionError,   # could not resolve recursive schema $ref
            ValueError                                  # most network errors and 404s
    ) as e:
        irods_six.reraise(ValidationWarning, ValidationWarning('\n\t'.join([
                'WARNING: Validation Failed for [{0}]:'.format(name),
                'against [{0}]'.format(schema_uri),
                '{0}: {1}'.format(e.__class__.__name__, e)])),
                sys.exc_info()[2])
    except (jsonschema.exceptions.ValidationError,
            jsonschema.exceptions.SchemaError,
            BaseException
    ) as e:
        irods_six.reraise(ValidationError,  ValidationError('\n\t'.join([
                'ERROR: Validation Failed for [{0}]:'.format(name),
                'against [{0}]'.format(schema_uri),
                '{0}: {1}'.format(e.__class__.__name__, e)])),
                sys.exc_info()[2])

    l.info("Validating [%s]... Success", name)

def get_initial_schema(schema_uri):
    l = logging.getLogger(__name__)
    l.debug('Loading schema from %s', schema_uri)
    url_scheme = irods_six.moves.urllib.parse.urlparse(schema_uri).scheme
    l.debug('Parsed URL: %s', schema_uri)
    scheme_dispatch = {
        'file': get_initial_schema_from_file,
        'http': get_initial_schema_from_web,
        'https': get_initial_schema_from_web,
    }

    try:
        return scheme_dispatch[url_scheme](schema_uri)
    except KeyError:
        raise ValidationError('ERROR: Invalid schema url: {}'.format(schema_uri))

def get_initial_schema_from_web(schema_uri):
    try:
        response = requests.get(schema_uri)
    except NameError:
        irods_six.reraise(ValidationError, ValidationError(
            'WARNING: Validation failed for {0} -- requests not installed'.format(
                name)),
                          sys.exc_info()[2])

    # check response values
    try:
        # modern requests
        schema = json.loads(response.text)
    except AttributeError:
        # requests pre-v1.0.0
        response.encoding = 'utf8'
        schema = json.loads(response.content)
    return schema

def get_initial_schema_from_file(schema_uri):
    with open(schema_uri[7:]) as f:
        schema = json.load(f)
    return schema

if __name__ == '__main__':
    logging.getLogger().setLevel(logging.NOTSET)
    l = logging.getLogger(__name__)

    irods_logging.register_tty_handler(sys.stdout, logging.INFO, logging.WARNING)
    irods_logging.register_tty_handler(sys.stderr, logging.WARNING, None)
    if len(sys.argv) != 3:
        l.error('Usage: %s <configuration_file> <schema_url>', sys.argv[0])
        sys.exit(1)

    config_file = sys.argv[1]
    schema_uri = sys.argv[2]

    try:
        load_and_validate(config_file, schema_uri)
    except ValidationError as e:
        l.error('Encounterd an error in validation.', exc_info=True)
        sys.exit(1)
    except ValidationWarning as e:
        l.warning('Encountered a warning in validation.', exc_info=True)
    sys.exit(0)
