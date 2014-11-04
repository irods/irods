# JSON Server Configuration Files

Given a markup language with structure, scoping, and name spacing, we have created a much richer system for describing iRODS, which has allowed us to consolidate all of the myriad configuration options-once spread out over several locations-into one single source of truth.  These configuration files are now managed in a single repository with subdirectories organized by version number, which allows the configuration model to grow and change as the project develops.

# Basic Configuration

The server_config.json file, found in `/etc/irods` for packaged binary installations, contains a considerable number of knobs and switches.  Starting with the basic configuration parameters, we have the following top level entries:

 - `icat_host` - The fully qualified domain name of the iCAT Enabled Server

 - `zone_name` - The name of the Zone in which the server participates

 - `zone_user` - The name of the rodsadmin user running this iRODS instance

 - `zone_port` - The port used by the Zone for communication

 - `zone_auth_scheme` - The authentication scheme used by the zone_user: native, PAM, OSAuth, KRB, or GSI

 - `zone_id` - The server ID used for authentication and identification of server-to-server communication - this can be a string of any length, excluding the use of hyphens, for historical purposes

 - `negotiation_key` - a 32-byte encryption key shared by the zone for use in the advanced negotiation handshake at the beginning of an iRODS client connection

 - `default_dir_mode` - The unix filesystem octal mode for a newly created directory within a resource vault

 - `default_file_mode` - The unix filesystem octal mode for a newly created file within a resource vault

 - `default_hash_scheme` - The hash scheme used for file integrity checking: MD5 or SHA256

 - `match_hash_policy` - Indicates to iRODS whether to use the hash used by the client or the data at rest, or to force the use of the default hash scheme: strict or compatible

 - `pam_password_length` - Maximum length of a PAM password

 - `pam_no_extend` - Set PAM password lifetime: 8 hours or 2 weeks, either true or false

 - `pam_password_min_time` - Minimum allowed PAM password lifetime

 - `pam_password_max_time` - Maximum allowed PAM password lifetime

 - `xmsg_port` - Port on which the XMessage Server operates, should it be enabled

 - `kerberos_name` - Kerberos Distinguished Name for KRB and GSI authentication

 - `control_plane_port` - Port on which the control plane operates, with a default of 1248

 - `control_plane_key` - Encryption key required for communicating with the iRODS grid control plane

# Rule Engine

Moving on to the new, structured portions of the JSON configuration, we have been able to create a more robust syntax for referencing the configuration files for the rule engine.  The following three sections are used to configure the rule engine:

 - `re_rule_base_set` - this is an array of file names comprising the list of rule files used by the rule engine, for example: { "filename": "core" } which references 'core.re'.  This array is ordered as the order of the rule files affects which (multiply) matching rule would fire first.

 - `re_function_name_mapping_set` - an array of file names comprising the list of function name map used by the rule engine, for example: { "filename": "core" } which references 'core.fnm'

 - `re_data_variable_mapping_set` - an array of file names comprising the list of data to variable mappings used by the rule engine, for example: { "filename": "core" } which references 'core.dvm'

# Server Environment

We now have a section for setting environment variables for the server environment, which allows for the consolidation of the configuration environment:

 - `environment_variables` - This section is an array of strings of the form VARIABLE=VALUE such as "ORACLE_HOME=/full/path"

# Federation

And finally, we have a section which defines the environment in which (multiple) federation operates:

- `federation` - This section is an array of objects which define the parameters necessary for federating with another grid:
    - `zone_name` -  The name of the zone with which we are federating
    - `zone_id` - The server ID for the federated zone
    - `negotiation_key` - A 32 byte encryption key used for connections across a federation
