# Configuration Files


The following configuration files control nearly all aspects of how an iRODS deployment functions.  All JSON files validate against the configuration schema defined at [https://schemas.irods.org/configuration](https://schemas.irods.org/configuration).

## ~/.odbc.ini

This file, in the home directory of the unix service account (default 'irods'), defines the unixODBC connection details needed for the iCommands to communicate with the iCAT database. This file was created by the installer package and probably should not be changed.

## ~/.irods/.irodsA

This scrambled password file is saved after an `iinit` is run. If this file does not exist, then each iCommand will prompt for a password before authenticating with the iRODS server. If this file does exist, then each iCommand will read this file and use the contents as a cached password token and skip the password prompt. This file can be deleted manually or can be removed by running `iexit full`.

## /etc/irods/server_config.json

This file defines the behavior of the server Agent that answers individual requests coming into iRODS. It is created and populated by the installer package.

This file contains the following top level entries:

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

  - `environment_variables` (required) - Contains a set of key/value properties of the form VARIABLE=VALUE such as "ORACLE_HOME=/full/path" from the server's environment.  Can be empty.

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


## /etc/irods/database_config.json

This file defines the database settings for the iRODS installation. It is created and populated by the installer package.

This file contains the following top level entries:

  - `catalog_database_type` (required) - The type of database iRODS is using for the iCAT, either 'postgres', 'mysql', or 'oracle'
  - `db_host` (required) - The hostname of the database server (can be localhost)
  - `db_odbc_type` (required) - The ODBC type, usually 'unix'
  - `db_password` (required) - The password for the `db_username` to connect to the `db_name`
  - `db_port` (required) - The port on which the database server is listening
  - `db_name` (required) - The name of the database used as the iCAT
  - `db_username` (required) - The database user name

## ~/.irods/irods_environment.json

This is the main iRODS configuration file defining the iRODS environment. Any changes are effective immediately since iCommands reload their environment on every execution.

The service account environment file contains the following top level entries:

  - `irods_authentication_file` (optional) - Fully qualified path to a file holding the credentials of an authenticated iRODS user
  - `irods_authentication_scheme` (optional) - This user's iRODS authentication method, currently: "pam", "krb", "gsi" or "native"
  - `irods_client_server_negotiation` (required) - Set to "request_server_negotiation" indicating advanced negotiation is desired, for use in enabling SSL and other technologies
  - `irods_client_server_policy` (required) - "CS_NEG_REFUSE" for no SSL, "CS_NEG_REQUIRE" to demand SSL, or "CS_NEG_DONT_CARE" to allow the server to decide
  - `irods_control_plane_port` (optional) - The port on which the control plane operates.
  - `irods_control_plane_key` (optional) - The encryption key required for communicating with the iRODS grid control plane.
  - `irods_cwd` (required) - The current working directory within iRODS
  - `irods_debug` (optional) - Desired verbosity of the debug logging level
  - `irods_default_hash_scheme` (required) - Currently either MD5 or SHA256
  - `irods_default_resource` (required) - The name of the resource used for iRODS operations if one is not specified
  - `irods_encryption_algorithm` (required) - EVP-supplied encryption algorithm for parallel transfer encryption
  - `irods_encryption_key_size` (required) - Key size for parallel transfer encryption
  - `irods_encryption_num_hash_rounds` (required) - Number of hash rounds for parallel transfer encryption
  - `irods_encryption_salt_size` (required) - Salt size for parallel transfer encryption
  - `irods_gsi_server_dn` (optional) - The Distinguished Name of the GSI Server
  - `irods_home` (required) - The home directory within the iRODS Zone for a given user
  - `irods_host` (required) - A fully qualified domain name for the given iRODS server
  - `irods_log_level` (optional) - Desired verbosity of the iRODS logging
  - `irods_match_hash_policy` (required) - Use 'strict' to refuse defaulting to another scheme or 'compatible' for supporting alternate schemes
  - `irods_plugins_home` (optional) - Directory to use for the client side plugins (useful when [Distributing iCommands to Users](distributing_icommands.md))
  - `irods_port` (required) - The port number for the given iRODS Zone
  - `irods_ssl_ca_certificate_file` (optional) - Location of a file of trusted CA certificates in PEM format. Note that the certificates in this file are used in conjunction with the system default trusted certificates.
  - `irods_ssl_ca_certificate_path` (optional) - Location of a directory containing CA certificates in PEM format. The files each contain one CA certificate. The files are looked up by the CA subject name hash value, which must hence be available. If more than one CA certificate with the same name hash value exist, the extension must be different (e.g. 9d66eef0.0, 9d66eef0.1 etc). The search is performed in the ordering of the extension number, regardless of other properties of the certificates. Use the ‘c_rehash’ utility to create the necessary links.
  - `irods_ssl_certificate_chain_file` (optional) - The file containing the server's certificate chain. The certificates must be in PEM format and must be sorted starting with the subject's certificate (actual client or server certificate), followed by intermediate CA certificates if applicable, and ending at the highest level (root) CA.
  - `irods_ssl_certificate_key_file` (optional) - Private key corresponding to the server's certificate in the certificate chain file.
  - `irods_ssl_dh_params_file` (optional) - The Diffie-Hellman parameter file location
  - `irods_ssl_verify_server` (optional) - What level of server certificate based authentication to perform. 'none' means not to perform any authentication at all. 'cert' means to verify the certificate validity (i.e. that it was signed by a trusted CA). 'hostname' means to validate the certificate and to verify that the irods_host's FQDN matches either the common name or one of the subjectAltNames of the certificate. 'hostname' is the default setting.
  - `irods_user_name` (required) - The username within iRODS for this account
  - `irods_xmsg_host` (optional) - The host name of the XMessage server (usually localhost)
  - `irods_xmsg_port` (optional) - The port of the XMessage server
  - `irods_zone_name` (required) - The name of the iRODS Zone

The same schema is used to define (but not enforce) the client side environment.  The following top level entries constitute the minimum working set of properties:

  - `irods_host`
  - `irods_port`
  - `irods_user_name`
  - `irods_zone_name`


# Checksum Configuration

Checksums in iRODS 4.0+ can be calculated using one of multiple hashing schemes.  Since the default hashing scheme for iRODS 4.0+ is SHA256, some existing earlier checksums may need to be recalculated and stored in the iCAT.

The following two settings, the default hash scheme and the match hash policy, need to be set on both the client and the server:

<table>
<tr>
<th>Client (irods_environment.json)</th>
<th>Server (server_config.json)</th>
</tr>
<tr>
<td>irods_default_hash_scheme<br />
 - SHA256 (default)<br />
 - MD5
</td>
<td>default_hash_scheme<br />
 - SHA256 (default)<br />
 - MD5
</td>
</tr>
<tr>
<td>irods_match_hash_policy<br />
 - Compatible (default)<br />
 - Strict
</td>
<td>match_hash_policy<br />
 - Compatible (default)<br />
 - Strict
</td>
</tr>
</table>


When a request is made, the sender and receiver's hash schemes and the receiver's policy are considered:

|  Sender      |   Receiver             |   Result                          |
| ------------ | ---------------------- | --------------------------------- |
|  MD5         |   MD5                  |   Success with MD5                |
|  SHA256      |   SHA256               |   Success with SHA256             |
|  MD5         |   SHA256, Compatible   |   Success with MD5                |
|  MD5         |   SHA256, Strict       |   Error, USER_HASH_TYPE_MISMATCH  |
|  SHA256      |   MD5, Compatible      |   Success with SHA256             |
|  SHA256      |   MD5, Strict          |   Error, USER_HASH_TYPE_MISMATCH  |

If the sender and receiver have consistent hash schemes defined, everything will match.

If the sender and receiver have inconsistent hash schemes defined, and the receiver's policy is set to 'compatible', the sender's hash scheme is used.

If the sender and receiver have inconsistent hash schemes defined, and the receiver's policy is set to 'strict', a USER_HASH_TYPE_MISMATCH error occurs.

# Special Characters

The default setting for 'standard_conforming_strings' in PostgreSQL 9.1+ was changed to 'on'.  Non-standard characters in iRODS Object names will require this setting to be changed to 'off'.  Without the correct setting, this may generate a USER_INPUT_PATH_ERROR error.
