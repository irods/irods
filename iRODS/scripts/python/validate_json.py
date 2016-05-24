from __future__ import print_function
import json
import sys

if len(sys.argv) != 3:
    sys.exit('Usage: {0} <configuration_file> <schema_url>'.format(sys.argv[0]))

config_file = sys.argv[1]
schema_uri = sys.argv[2]

def print_error(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

try:
    import requests
except ImportError:
    print_error('ERROR: python module [requests] required')
    sys.exit(1)

try:
    import jsonschema
except ImportError:
    print_error('WARNING: Validation Failed for [%s] -- jsonschema not installed' % config_file)
    sys.exit(0)

try:
    e = jsonschema.exceptions
except AttributeError:
    print_error('WARNING: Validation Failed for [%s] -- jsonschema too old v[%s]' %
                (config_file, jsonschema.__version__))
    sys.exit(0)

def get_initial_schema(schema_uri):
    url_scheme = schema_uri.partition(':')[0]
    scheme_dispatch = {
        'file': get_initial_schema_from_file,
        'http': get_initial_schema_from_web,
        'https': get_initial_schema_from_web,
    }

    try:
        return scheme_dispatch[url_scheme](schema_uri)
    except KeyError:
        print('ERROR: Invalid schema url: {0}'.format(schema_uri))
        sys.exit(1)

def get_initial_schema_from_web(schema_uri):
    response = requests.get(schema_uri, timeout=5)
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

try:
    # load configuration file
    with open(config_file, 'r') as f:
        config = json.load(f)

    schema = get_initial_schema(schema_uri)
    # validate
    jsonschema.validate(config, schema)
except (jsonschema.exceptions.RefResolutionError) as e:
    # could not resolve recursive schema $ref
    print_error('WARNING: Validation Failed for [%s]' % config_file)
    print_error('       : against [%s]' % schema_uri)
    print_error("  {0}: {1}".format(e.__class__.__name__, e))
    sys.exit(0)
except (
        requests.exceptions.ConnectionError,
        requests.exceptions.Timeout
) as e:
    # network connection error or timeout
    print_error('WARNING: Validation Failed for [%s]' % config_file)
    print_error('       : against [%s]' % schema_uri)
    print_error("  {0}: {1}".format(e.__class__.__name__, e))
    sys.exit(0)
except (ValueError) as e:
    # 404s and bad JSON
    print_error('WARNING: Validation Failed for [%s]' % config_file)
    print_error('       : against [%s]' % schema_uri)
    print_error("  {0}: {1}".format(e.__class__.__name__, e))
    sys.exit(0)
except (
        jsonschema.exceptions.ValidationError,
        jsonschema.exceptions.SchemaError
) as e:
    # actual bad validation, hard stop
    print_error('ERROR: Validation Failed for [%s]' % config_file)
    print_error('     : against [%s]' % schema_uri)
    print_error("  {0}: {1}".format(e.__class__.__name__, e))
    sys.exit(1)
except Exception as e:
    # anything else, hard stop
    print_error('ERROR: Validation Failed for [%s]' % config_file)
    print_error('     : against [%s]' % schema_uri)
    print_error("  {0}: {1}".format(e.__class__.__name__, e))
    sys.exit(1)
except:
    sys.exit(1)

else:
    print("Validating [" + sys.argv[1] + "]... Success")
    sys.exit(0)
