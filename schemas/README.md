## Schemas

This directory contains various schema definition files.

**NOTE:** All schema files existing in the same directory must share the same schema type.

The schema types for each directory are listed below.

| Directory | Schema Type | URL |
|---|---|---|
| configuration/v4 | JSON Schema | https://json-schema.org |
| messaging/v1 | Avro | https://avro.apache.org/docs/1.11.0/spec.html |

### Updating the schema version

Over the course of development, it is necessary to consider whether the schema version for various things should be adjusted. There are a number of files and locations which this applies to. They are as follows:

- irods_environment.json _(file) (service account only)_
- server_config.json _(file)_
- version.json _(file)_
- R_GRID_CONFIGURATION _(database table)_

To aid with this process, the following guidelines are provided:

- Keep the `schema_version` property in alignment across configuration **files**
- Avoid schema version number conflicts across git branches
    - Do not decrement schema version numbers
    - Do not reuse schema version numbers
- Understand the relationship between R_GRID_CONFIGURATION and version.json
    - A catalog schema upgrade is triggered anytime the `catalog_schema_version` _(in version.json)_ is less than the schema version in the catalog
    - The **current** catalog schema version can be retrieved from the catalog using any of the following:
        - SQL: `select * from R_GRID_CONFIGURATION where namespace = 'database' and option_name = 'schema_version'`
        - icommand: `iadmin get_grid_configuration database schema_version`

In general, a schema version should be adjusted if and only if there's a change in the schema's definition. This is typically reserved for major releases _(e.g. upgrading from iRODS 4.2 to iRODS 4.3)_.

A _change in the schema definition_ is defined as the addition, modification, and/or removal of a JSON property or database table/column.
