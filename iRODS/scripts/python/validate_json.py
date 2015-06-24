from __future__ import print_function
import json
import sys
import irods_six

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

def load_and_validate(config_file, schema_uri, verbose=False):
    try:
        # load configuration file
        with open(config_file, 'r') as f:
            config_dict = json.load(f)
    except BaseException as e:
        irods_six.reraise(ValidationError, ValidationError('\n\t'.join([
            'WARNING: Validation Failed for [{0}]:'.format(config_file),
            'against [{0}]'.format(schema_uri),
            '{0}: {1}'.format(e.__class__.__name__, e)])),
                          sys.exc_info()[2])
    validate_dict(config_dict, schema_uri, name=config_file, verbose=verbose)
    return config_dict

def validate_dict(config_dict, schema_uri, name=None, verbose=False):
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
        # load the schema url
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

        # validate
        jsonschema.validate(config_dict, schema)
    except (
            jsonschema.exceptions.RefResolutionError,   # could not resolve recursive schema $ref
            ValueError                                  # most network errors and 404s
    ) as e:
        irods_six.reraise(ValidationWarning, ValidationWarning('\n\t'.join([
                'WARNING: Validation Failed for [{0}]:'.format(name),
                'against [{0}]'.format(schema_uri),
                '{0}: {1}'.format(e.__class__.__name__, e)])),
                sys.exc_info()[2])
    except (
            jsonschema.exceptions.ValidationError,
            jsonschema.exceptions.SchemaError,
            BaseException
    ) as e:
        irods_six.reraise(ValidationError,  ValidationError('\n\t'.join([
                'ERROR: Validation Failed for [{0}]:'.format(name),
                'against [{0}]'.format(schema_uri),
                '{0}: {1}'.format(e.__class__.__name__, e)])),
                sys.exc_info()[2])

    if verbose and name:
        print("Validating [{0}]... Success".format(name))

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Usage: {0} <configuration_file> <schema_url>'.format(sys.argv[0]),
              file=sys.stderr)
        sys.exit(1)

    config_file = sys.argv[1]
    schema_uri = sys.argv[2]

    try:
        validate(config_file, schema_uri, verbose=True)
    except ValidationError as e:
        print(e, file=sys.stderr)
        sys.exit(1)
    except ValidationWarning as e:
        print(e, file=sys.stderr)
        sys.exit(0)
