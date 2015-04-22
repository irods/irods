from __future__ import print_function
import json
import sys
import requests

if len(sys.argv) != 3:
    sys.exit('Usage: {0} <configuration_file> <schema_url>'.format(sys.argv[0]))

def print_error(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

try:
    import jsonschema
except ImportError:
    print_error('WARNING: Validation Failed for ['+sys.argv[1]+'] -- jsonschema not installed')
    sys.exit(0)

try:
    # load configuration file
    with open(sys.argv[1], 'r+') as f:
        config = json.load(f)
    # load the schema url
    response = requests.get(sys.argv[2])
    schema = json.loads(response.text)
    # validate
    jsonschema.validate( config, schema )
except (jsonschema.exceptions.RefResolutionError) as e:
    print_error('WARNING: Validation Failed for ['+sys.argv[1]+']')
    print_error("  {0}: {1}".format(e.__class__.__name__, e))
    sys.exit(0)
except (
        ValueError,
        jsonschema.exceptions.ValidationError,
        jsonschema.exceptions.SchemaError
        ) as e:
    print_error('ERROR: Validation Failed for ['+sys.argv[1]+']')
    print_error("  {0}: {1}".format(e.__class__.__name__, e))
    sys.exit(1)
except Exception as e:
    print_error('ERROR: Validation Failed for ['+sys.argv[1]+']')
    print_error("  {0}: {1}".format(e.__class__.__name__, e))
    sys.exit(1)
except:
    sys.exit(1)

else:
    print("Validating ["+sys.argv[1]+"]... Success")
    sys.exit(0)

