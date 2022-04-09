#### iRODS schemas for configuration files and grid components.

Used by the iRODS server, on startup, to validate its configuration files.

#### Example usage:

```
git clone https://github.com/irods/irods_schema_configuration
cd irods_schema_configuration
git checkout v3
python3 deploy_schemas_locally.py --output_directory_base /tmp/irods_schemas
```

This will create, if necessary, `/tmp/irods_schemas` and `/tmp/irods_schemas/v3`, then populate the latter with copies of the schema files currently found in `./schemas`. The schema copies will have their `id` fields updated to reflect their new locations (e.g. `server_config.json` will have `id` field `file:///tmp/irods_schemas/v3/server_config.json#`.

To have an iRODS installation validate its configuration files against this local deployment, set the iRODS `schema_validation_base_uri` (found in `/etc/irods/server_config.json` or fed into the prompt during setup) to `file:///tmp/irods_schemas`.

Make sure that the Linux account running iRODS has read access to the deployed schemas.

Note that the above steps will only create local instances of the schemas for `v3`. To create local instances of other versions, checkout the desired version's git branch (e.g. `v2`).

To determine which version of the schemas is used by an iRODS installation, check the `configuration_schema_version` entry of `/var/lib/irods/version.json`.

#### `deploy_schemas_locally.py` usage:

```
python3 deploy_schemas_locally.py --output_directory_base <path to desired deployment directory> [options]

Options:
  -h, --help            show this help message and exit
  --template_directory=<path to alternate schema templates>
  --url_base=<alternate url base for "id" field>
                        Useful for creating web deployments

Deploys packaged schemas to <output_directory_base>.

Sets "id" field of deployed schemas appropriately based on <url_base>.
```
