from __future__ import print_function
import json
import sys
import requests

if len(sys.argv) != 3:
    sys.exit('Usage: {0} <configuration_file> <schema_url>'.format(sys.argv[0]))

config_file = sys.argv[1]
schema_uri = sys.argv[2]

def print_error(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

try:
    import jsonschema
except ImportError:
    print_error('WARNING: Validation Failed for [%s] -- jsonschema not installed' % config_file)
    sys.exit(0)

try:
    # load configuration file
    with open(config_file, 'r') as f:
        config = json.load(f)
    # load the schema url
    response = requests.get(schema_uri)
    # check response values
    try:
        # modern requests
        schema = json.loads(response.text)
    except AttributeError:
        # requests pre-v1.0.0
        response.encoding = 'utf8'
        schema = json.loads(response.content)

    # validate
    jsonschema.validate( config, schema )
except (jsonschema.exceptions.RefResolutionError) as e:
    # could not resolve recursive schema $ref
    print_error('WARNING: Validation Failed for [%s]' % config_file)
    print_error('       : against [%s]' % schema_uri)
    print_error("  {0}: {1}".format(e.__class__.__name__, e))
    sys.exit(0)
except (ValueError) as e:
    # most network errors and 404s
    print_error('WARNING: Validation Failed for [%s]' % config_file)
    print_error('       : against [%s]' % schema_uri)
    print_error("  {0}: {1}".format(e.__class__.__name__, e))
    sys.exit(0)
except (
        jsonschema.exceptions.ValidationError,
        jsonschema.exceptions.SchemaError
        ) as e:
    print_error('ERROR: Validation Failed for [%s]' % config_file)
    print_error('     : against [%s]' % schema_uri)
    print_error("  {0}: {1}".format(e.__class__.__name__, e))
    sys.exit(1)
except Exception as e:
    print_error('ERROR: Validation Failed for [%s]' % config_file)
    print_error('     : against [%s]' % schema_uri)
    print_error("  {0}: {1}".format(e.__class__.__name__, e))
    sys.exit(1)
except:
    sys.exit(1)

else:
    print("Validating ["+sys.argv[1]+"]... Success")
    sys.exit(0)

