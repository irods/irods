# JSON Server Configuration Files

The following configuration files control nearly all aspects of how an iRODS deployment functions.  There are configuration variables available for the server, the client environment, and the database connection.  All files are in JSON and validate against the configuration schema defined at [https://schemas.irods.org/configuration](https://schemas.irods.org/configuration).

# server_config.json

Controlling the basic configuration of the iRODS Server itself, and found in `/etc/irods` for packaged binary installations, this file contains the following top level entries:

  - `advanced_settings` (required) - Contains subtle network and password related variables.  These values should be changed only in concert with all connecting clients and other servers in the Zone.

    - `default_number_of_transfer_threads` (optional) (default 4) - The number of threads enabled when parallel transfer is invoked.

    - `default_temporary_password_lifetime_in_seconds` (optional) (default 120) - The number of seconds a server-side temporary password is good.

    - `maximum_number_of_concurrent_rule_engine_server_processes` (optional) (default 4)

    - `maximum_size_for_single_buffer_in_megabytes` (optional) (default 32)

    - `maximum_temporary_password_lifetime_in_seconds` (optional) (default 1000)

    - `transfer_buffer_size_for_parallel_transfer_in_megabytes` (optional) (default 4)

    - `transfer_chunk_size_for_parallel_transfer_in_megabytes` (optional) (default 40)

  - `default_dir_mode` (required) (default "0750") - The unix filesystem octal mode for a newly created directory within a resource vault

  - `default_file_mode` (required) (default "0600") - The unix filesystem octal mode for a newly created file within a resource vault

  - `default_hash_scheme` (required) (default "SHA256") - The hash scheme used for file integrity checking: MD5 or SHA256

  - `default_resource_directory` (optional) - The default Vault directory for the initial resource on server installation

  - `default_resource_name` (optional) - The name of the initial resource on server installation

  - `environment_variables` (required) - Contains a set of key/value properties from the server's environment.  Can be empty.

  - `federation` (required) - Contains an array of objects which each contain the parameters necessary for federating with another grid.  The array can be empty, but if an object exists, it must contain the following properties:
    - `icat_host` (required) -  The hostname of the iCAT server in the federated zone.
    - `zone_name` (required) -  The name of the federated zone.
    - `zone_key` (required) - The shared authentication secret of the federated zone.
    - `negotiation_key` (required) - The 32-byte encryption key of the federated zone.

  - `icat_host` (required) - The fully qualified domain name of the iCAT Enabled Server

  - `kerberos_name` (optional) - Kerberos Distinguished Name for KRB and GSI authentication

  - `match_hash_policy` (required) (default "compatible") - Indicates to iRODS whether to use the hash used by the client or the data at rest, or to force the use of the default hash scheme: strict or compatible

  - `negotiation_key` (required) - A 32-byte encryption key shared by the zone for use in the advanced negotiation handshake at the beginning of an iRODS client connection

  - `pam_no_extend` (optional) - Set PAM password lifetime: 8 hours or 2 weeks, either true or false

  - `pam_password_length` (optional) - Maximum length of a PAM password

  - `pam_password_max_time` (optional) - Maximum allowed PAM password lifetime

  - `pam_password_min_time` (optional) - Minimum allowed PAM password lifetime

  - `re_data_variable_mapping_set` (required) - an array of file names comprising the list of data to variable mappings used by the rule engine, for example: { "filename": "core" } which references 'core.dvm'

  - `re_function_name_mapping_set` (required) - an array of file names comprising the list of function name map used by the rule engine, for example: { "filename": "core" } which references 'core.fnm'

  - `re_rulebase_set` (required) - this is an array of file names comprising the list of rule files used by the rule engine, for example: { "filename": "core" } which references 'core.re'.  This array is ordered as the order of the rule files affects which (multiply) matching rule would fire first.

  - `schema_validation_base_uri` (required) - The URI against which the iRODS server configuration is validated.  By default, this will be the schemas.irods.org domain.  It can be set to any http(s) endpoint as long as that endpoint has a copy of the irods_schema_configuration repository.  This variable allows a clone of the git repository to live behind an organizational firewall, but still perform its duty as a preflight check on the configuration settings for the entire server.

  - `server_control_plane_encryption_algorithm` (required) - The algorithm used to encrypt the control plane communications. This must be the same across all iRODS servers in a Zone. (default "AES-256-CBC")

  - `server_control_plane_encryption_num_hash_rounds` (required) (default 16) - The number of hash rounds used in the control plane communications. This must be the same across all iRODS servers in a Zone.

  - `server_control_plane_key` (required) - The encryption key required for communicating with the iRODS grid control plane. Must be 32 bytes long. This must be the same across all iRODS servers in a Zone.

  - `server_control_plane_port` (required) (default 1248) - The port on which the control plane operates. This must be the same across all iRODS servers in a Zone.

  - `server_control_plane_timeout_milliseconds` (required) (default 10000) - 

  - `server_port_range_start` (required) (default 20000) - The beginning of the port range available for parallel transfers. This must be the same across all iRODS servers in a Zone.
  - `server_port_range_end` (required) (default 20199) - The end of the port range available for parallel transfers.  This must be the same across all iRODS servers in a Zone.

  - `xmsg_port` (default 1279) - The port on which the XMessage Server operates, should it be enabled. This must be the same across all iRODS servers in a Zone.
  - `zone_auth_scheme` (required) - The authentication scheme used by the zone_user: native, PAM, OSAuth, KRB, or GSI
  - `zone_key` (required) - The shared secret used for authentication and identification of server-to-server communication - this can be a string of any length, excluding the use of hyphens, for historical purposes.  This must be the same across all iRODS servers in a Zone.
  - `zone_name` (required) - The name of the Zone in which the server participates. This must be the same across all iRODS servers in a Zone.
  - `zone_port` (required) (default 1247) - The main port used by the Zone for communication. This must be the same across all iRODS servers in a Zone.
  - `zone_user` (required) - The name of the rodsadmin user running this iRODS instance.


# Rule Engine

Moving on to the new, structured portions of the JSON configuration, we have been able to create a more robust syntax for referencing the configuration files for the rule engine.  The following three sections are used to configure the rule engine:


# Server Environment

We now have a section for setting environment variables for the server environment, which allows for the consolidation of the configuration environment:

  - `environment_variables` - This section is an array of strings of the form VARIABLE=VALUE such as "ORACLE_HOME=/full/path"

