# Release Notes

## 4.1.0

### Notable Features

  - Configuration Management - All configuration files are now JSON and schema-backed.

    - Validated Configuration - JSON files are validated against repository of versioned schemas during server startup.

    - Reduced Magic Numbers - Some previously hard coded settings have been moved to server_config.json

    - Integrated izonereport - Produce validated JSON about every server in the Zone.  Useful for debugging and for deployment.

  - Control Plane - New functionality for determining status and grid-wide actions for pause, resume, and graceful shutdown.

  - Weighted Passthru Resource Plugin - A passthru resource can now manipulate its child resource's votes on read and write.

  - Atomic iput with metadata and ACLs - Add metadata and ACLs as soon as data is at rest and registered

  - Key/Value passthru to resource plugins - Can influence resource behavior via user-provided parameters

  - A client hints API to get server configuration information for better user-facing messages

  - Allow only TLS 1.1 and 1.2

  - Dynamic PEP result can halt operation on failure, providing better policy flow control

  - Continuous Testing

    - Automated Ansible-driven python topology suite, including SSL

    - Federation with 3.3.1, 4.0.3, and 4.1.0

    - Well-defined C API for developers

### Notable Bug Fixes

  - Coverity Clean - Fixed over 1100 identified bugs related to memory management, error checking, dead code, and other miscellany.

  - Many permission inconsistencies ironed out

  - Parallel transfer works in multi-homed networked situations, had been resolving IP too early

  - irsync sending only updated files

  - Zip files available via ibun

  - Zero-length file behavior is consistent

  - Delayed rules running correctly

  - Removed built-in PostgreSQL DB Vacuum functionality

  - Removed boot user from install script

  - Removed "run_server_as_root" option

  - Removed roles for storageadmin, domainadmin, and rodscurators

  - Removed obfuscation (SIDKey and DBKey)

### Other Issues

  - Full 4.1.0 listing [available at GitHub](https://github.com/irods/irods/issues?q=is%3Aclosed+milestone%3A4.1.0)
