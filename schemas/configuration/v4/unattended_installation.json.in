{
    "$id": "file:///@IRODS_HOME_DIRECTORY@/configuration_schemas/v4/unattended_installation.json",
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "properties": {
        "admin_password": {"type": "string"},
        "default_resource_directory": {"type": "string"},
        "default_resource_name": {"type": "string"},
        "host_system_information": {
            "type": "object",
            "properties": {
                "service_account_user_name": {"type": "string"},
                "service_account_group_name": {"type": "string"}
            },
            "required": ["service_account_user_name", "service_account_group_name"]
        },
        "server_config": {
            "$ref": "server_config.json"
        },
        "service_account_environment": {
            "$ref": "service_account_environment.json"
        }
    },
    "required": [
        "admin_password",
        "default_resource_name",
        "host_system_information",
        "server_config",
        "service_account_environment"
    ]
}
