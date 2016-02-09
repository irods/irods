# Authentication

By default, iRODS uses a secure password system for user authentication.  The user passwords are scrambled and stored in the iCAT database.  Additionally, iRODS supports user authentication via PAM (Pluggable Authentication Modules), which can be configured to support many things, including the LDAP authentication system.  PAM and SSL have been configured 'available' out of the box with iRODS, but there is still some setup required to configure an installation to communicate with your external authentication server of choice.

The iRODS administrator can 'force' a particular authentication scheme for a rodsuser by 'blanking' the native password for the rodsuser.  There is currently no way to signal to a particular login attempt that it is using an incorrect scheme ([GitHub Issue #2005](https://github.com/irods/irods/issues/2005)).

## GSI

Grid Security Infrastructure (GSI) setup in iRODS 4.0+ has been greatly simplified.  The functionality itself is provided by the [GSI authentication plugin](https://github.com/irods/irods_auth_plugin_gsi).

### GSI Configuration

Configuration of GSI is out of scope for this document, but consists of the following three main steps:

1. Install GSI (most easily done via package manager)
2. Confirm the (default) irods service account has a certificate in good standing (signed)
3. Confirm the local system account for client "newuser" has a certificate in good standing (signed)

### iRODS Configuration

Configuring iRODS to authenticate via GSI requires a few simple steps.

First, if GSI is being configured for a new user, it must be created:

~~~
iadmin mkuser newuser rodsuser
~~~

Then that user must be configured so its Distiguished Name (DN) matches its certificate:

~~~
iadmin aua newuser '/DC=org/DC=example/O=Example/OU=People/CN=New User/CN=UID:drexample'
~~~

!!! Note
    The comma characters (,) in the Distiguished Name (DN) must be replaced with forward slash characters (/).

On the client side, the user's 'irods_authentication_scheme' must be set to 'GSI'.  This can be configured via an `irods_environment.json` property:

~~~
"irods_authentication_scheme": "GSI",
~~~

Then, to authenticate with a temporary proxy certificate:

~~~
grid-proxy-init
~~~

This will prompt for the user's GSI password.  If the user is successfully authenticated, temporary certificates are issued and setup in the user's environment.  The certificates are good, by default, for 24 hours.

In addition, if users want to authenticate the server, they can set 'irods_gsi_server_dn' in their user environment. This will cause the system to do mutual authentication instead of just authenticating the client user to the server.

### Limitations

The iRODS administrator will see two limitations when using GSI authentication:

1. The 'client_user_name' environment variable will fail (the admin cannot alias as another user)
2. The `iadmin moduser password` will fail (cannot update the user's password)

The workaround is to use iRODS native authentication when using these.

`ipasswd` for rodsusers will also fail, but it is not an issue as it would be trying to update their (unused) iRODS native password.  They should not be updating their GSI passwords via iCommands.

## Kerberos

Kerberos setup in iRODS 4.0+ has been greatly simplified.  The functionality itself is provided by the [Kerberos authentication plugin](https://github.com/irods/irods_auth_plugin_kerberos).

### Kerberos Configuration

Configuration of a Kerberos server for authentication is out of scope for this document, but consists of the following four main steps:

1. Set up Kerberos (Key Distribution Center (KDC) and Kerberos Admin Server)
2. Confirm the (default) irods service account has a service principal in KDC (with the hostname of the rodsServer) (e.g. irodsserver/serverhost.example.org@EXAMPLE.ORG)
3. Confirm the local system account for client "newuser" has principal in KDC (e.g. newuser@EXAMPLE.ORG)
4. Create an appropriate keytab entry (adding to an existing file or creating a new one)

A new keytab file can be created with the following command:

~~~
kadmin ktadd -k /var/lib/irods/irods.keytab irodsserver/serverhost.example.org@EXAMPLE.ORG
~~~

### iRODS Configuration

Configuring iRODS to authenticate via Kerberos requires a few simple steps.

First, if Kerberos is being configured for a new user, the new user must be created:

~~~
iadmin mkuser newuser rodsuser
~~~

Then that user must be configured so its principal matches the KDC:

~~~
iadmin aua newuser newuser@EXAMPLE.ORG
~~~

The `/etc/irods/server_config.json` must be updated to include:

~~~
"KerberosServicePrincipal": "irodsserver/serverhost.example.org@EXAMPLE.ORG",
"KerberosKeytab": "/var/lib/irods/irods.keytab",
~~~

An `/etc/irods/server_config.json` environment variable must also be included to point the GSS API to the keytab mentioned above:

~~~
"environment_variables": {
    "KRB5_KTNAME": "/var/lib/irods/irods.keytab"
},
~~~

On the client side, the user's 'irods_authentication_scheme' must be set to 'KRB'.  This can be configured via an `irods_environment.json` property:

~~~
"irods_authentication_scheme": "KRB",
~~~

Then, to initialize the Kerberos session ticket and authenticate:

~~~
kinit
~~~

### Limitations

The iRODS administrator will see two limitations when using Kerberos authentication:

1. The 'clientUserName' environment variable will fail (the admin cannot alias as another user)
2. The `iadmin moduser password` will fail (cannot update the user's password)

The workaround is to use iRODS native authentication when using these.

`ipasswd` for rodsusers will also fail, but it is not an issue as it would be trying to update their (unused) iRODS native password.  They should not be updating their Kerberos passwords via iCommands.

### Weak Encryption Workaround

If you are seeing either of the errors `GSS-API error initializing context: KDC has no support for encryption type` or `KRB_ERROR_INIT_SECURITY_CONTEXT` when setting up Kerberos, then you probably have an available cypher mismatch between the Kerberos server and the Active Directory (AD) server.  This is not an iRODS setting, and can be addressed in the Kerberos configuration.

The MIT Key Distribution Center (KDC) uses the most secure encoding type when sending the ticket to the AD server. When the AD server is unable to handle that encoding, it replies with the error that the encryption type is not supported.

To override this mismatch and allow a weaker algorithm to be sufficient, set `allow_weak_crypto = yes` in the `libdefaults` stanza of `/etc/krb5.conf`:

~~~
$ head /etc/krb5.conf
[libdefaults]
        default_realm = EXAMPLE.ORG
        allow_weak_crypto = yes
...
~~~

This will allow the Kerberos handshake to succeed, which allows the iRODS connection to continue.

## PAM

### User Setup

PAM can be configured to to support various authentication systems; however the iRODS administrator still needs to add the users to the iRODS database:

~~~
irods@hostname:~/ $ iadmin mkuser newuser rodsuser
~~~

If the user's credentials will be exclusively authenticated with PAM, a password need not be assigned.

For PAM Authentication, the iRODS user selects the new iRODS PAM authentication choice (instead of Native, or Kerberos) via an `irods_environment.json` property:

~~~
"irods_authentication_scheme": "PAM",
~~~

Then, the user runs 'iinit' and enters their system password.  To protect the system password, SSL (via OpenSSL) is used to encrypt the `iinit` session.

Configuring the operating system, the service name used for PAM is 'irods'.  An addition to /etc/pam.d/ is required if the fall-through behavior is not desired.

For example:
~~~
$ cat /etc/pam.d/irods
auth        required      pam_env.so
auth        sufficient    pam_unix.so
auth        requisite     pam_succeed_if.so uid >= 500 quiet
auth        required      pam_deny.so
~~~

For more information on the syntax of the pam.d configuration please refer to [The Linux Documentation Project](http://tldp.org/HOWTO/User-Authentication-HOWTO/x115.html)

A quick test for the basic authentication mechanism for PAM is to run the `iRODS/server/bin/PamAuthCheck` tool.
PamAuthCheck reads the password from stdin (without any prompting).

If PamAuthCheck returns `Not Authenticated`, that suggests that PAM is not set up correctly. You will need to configure PAM correctly (and therefore get PamAuthCheck returning Authenticated) before using PAM through iRODS.

A simple way to check that you are using PamAuthCheck correctly, and that it is the PAM settings that need updated, is to create a fully permissive PAM setup with the following command.

~~~
sudo su - root -c 'echo "auth sufficient pam_permit.so" > /etc/pam.d/irods'
~~~

This will allow any username/password combination to successfully authenticate with the irods PAM service, meaning that any username/password combination should cause PamAuthCheck to return `Authenticated`.

With the permissive configuration working with PamAuthCheck, the next step is to adjust your PAM configuration to your desired settings (LDAP, in this case). You will know that is correct when PamAuthCheck behaves as you would expect when using LDAP username/passwords. iRODS uses PamAuthCheck directly, so if it is working on the command line, it should work when run by iRODS.

Since PAM requires the user's password in plaintext, iRODS relies on SSL encryption to protect these credentials.  PAM authentication makes use of SSL regardless of the iRODS Zone SSL configuration (meaning even if iRODS explicitly does *not* encrypt data traffic, PAM will use SSL during authentication).

In order to use the iRODS PAM support, you also need to have SSL working between the iRODS client and server. The SSL communication between client and iRODS server needs some basic setup in order to function properly. Much of the setup concerns getting a proper X.509 certificate setup on the server side, and setting up the trust for the server certificate on the client side. You can use either a self-signed certificate (best for testing) or a certificate from a trusted CA.

### Server Configuration

The following keywords are used to set values for PAM server configuration.  These were previously defined as compile-time options.  They are now configurable via the `/etc/irods/server_config.json` configuration file.  The default values have been preserved.

- pam_password_length
- pam_no_extend
- pam_password_min_time
- pam_password_max_time

### Server SSL Setup

Here are the basic steps to configure the server:

#### Generate a new RSA key

Make sure it does not have a passphrase (i.e. do not use the -des, -des3 or -idea options to genrsa):

~~~
irods@hostname:~/ $ openssl genrsa -out server.key
~~~

#### Acquire a certificate for the server

The certificate can be either from a trusted CA (internal or external), or can be self-signed (common for development and testing). To request a certificate from a CA, create your certificate signing request, and then follow the instructions given by the CA. When running the 'openssl req' command, some questions will be asked about what to put in the certificate. The locality fields do not really matter from the point of view of verification, but you probably want to try to be accurate. What is important, especially since this is a certificate for a server host, is to make sure to use the FQDN of the server as the "common name" for the certificate (should be the same name that clients use as their `irods_host`), and do not add an email address. If you are working with a CA, you can also put host aliases that users might use to access the host in the 'subjectAltName' X.509 extension field if the CA offers this capability.

To generate a Certificate Signing Request that can be sent to a CA, run the 'openssl req' command using the previously generated key:

~~~
irods@hostname:~/ $ openssl req -new -key server.key -out server.csr
~~~

To generate a self-signed certificate, also run 'openssl req', but with slightly different parameters. In the openssl command, you can put as many days as you wish:

~~~
irods@hostname:~/ $ openssl req -new -x509 -key server.key -out server.crt -days 365
~~~

#### Create the certificate chain file

If you are using a self-signed certificate, the chain file is just the same as the file with the certificate (server.crt).  If you have received a certificate from a CA, this file contains all the certificates that together can be used to verify the certificate, from the host certificate through the chain of intermediate CAs to the ultimate root CA.

An example best illustrates how to create this file. A certificate for a host 'irods.example.org' is requested from the proper domain registrar. Three files are received from the CA: irods.crt, PositiveSSLCA2.crt and AddTrustExternalCARoot.crt. The certificates have the following 'subjects' and 'issuers':

~~~
openssl x509 -noout -subject -issuer -in irods.crt
subject= /OU=Domain Control Validated/OU=PositiveSSL/CN=irods.example.org
issuer= /C=GB/ST=Greater Manchester/L=Salford/O=COMODO CA Limited/CN=PositiveSSL CA 2
openssl x509 -noout -subject -issuer -in PositiveSSLCA2.crt
subject= /C=GB/ST=Greater Manchester/L=Salford/O=COMODO CA Limited/CN=PositiveSSL CA 2
issuer= /C=SE/O=AddTrust AB/OU=AddTrust External TTP Network/CN=AddTrust External CA Root
openssl x509 -noout -subject -issuer -in AddTrustExternalCARoot.crt
subject= /C=SE/O=AddTrust AB/OU=AddTrust External TTP Network/CN=AddTrust External CA Root
issuer= /C=SE/O=AddTrust AB/OU=AddTrust External TTP Network/CN=AddTrust External CA Root
~~~

The irods.example.org cert was signed by the PositiveSSL CA 2, and that the PositiveSSL CA 2 cert was signed by the AddTrust External CA Root, and that the AddTrust External CA Root cert was self-signed, indicating that it is the root CA (and the end of the chain).

To create the chain file for irods.example.org:

~~~
irods@hostname:~/ $ cat irods.crt PositiveSSLCA2.crt AddTrustExternalCARoot.crt > chain.pem
~~~

#### Generate OpenSSL parameters

Generate some Diffie-Hellman parameters for OpenSSL:

~~~
irods@hostname:~/ $ openssl dhparam -2 -out dhparams.pem 2048
~~~

#### Place files within accessible area

Put the dhparams.pem, server.key and chain.pem files somewhere that the iRODS server can access them (e.g. in /etc/irods).  Make sure that the irods unix user can read the files (although you also want to make sure that the key file is only readable by the irods user).

#### Set the iRODS SSL environment

The server expects to have the following irods service account's `irods_environment.json` properties set on startup:

~~~
"irods_ssl_certificate_chain_file": "/etc/irods/chain.pem",
"irods_ssl_certificate_key_file": "/etc/irods/server.key",
"irods_ssl_dh_params_file": "/etc/irods/dhparams.pem",
~~~

#### Restart iRODS

Restart the server:

~~~
irods@hostname:~/ $ ./iRODS/irodsctl restart
~~~

### Client SSL Setup

The client may or may not require configuration at the SSL level, but there are a few parameters that can be set via `irods_environment.json` properties to customize the client SSL interaction if necessary. In many cases, if the server's certificate comes from a common CA, your system might already be configured to accept certificates from that CA, and you will not have to adjust the client configuration at all. For example, on an Ubuntu12 (Precise) system, the /etc/ssl/certs directory is used as a repository for system trusted certificates installed via an Ubuntu package. Many of the commercial certificate vendors such as VeriSign and AddTrust have their certificates already installed.

After setting up SSL on the server side, test SSL by using the PAM authentication (which requires an SSL connection) and running ``iinit`` with the log level set to LOG_NOTICE. If you see messages as follows, you need to set up trust for the server's certificate, or you need to turn off server verification.

Error from non-trusted self-signed certificate:

~~~
irods@hostname:~/ $ IRODS_LOG_LEVEL=LOG_NOTICE iinit
NOTICE: environment variable set, irods_log_level(input)=LOG_NOTICE, value=5
NOTICE: created irods_home=/dn/home/irods
NOTICE: created irods_cwd=/dn/home/irods
Enter your current PAM (system) password:
NOTICE: sslVerifyCallback: problem with certificate at depth: 0
NOTICE: sslVerifyCallback:   issuer = /C=US/ST=North Carolina/L=Chapel Hill/O=RENCI/CN=irods.example.org
NOTICE: sslVerifyCallback:   subject = /C=US/ST=North Carolina/L=Chapel Hill/O=RENCI/CN=irods.example.org
NOTICE: sslVerifyCallback:   err 18:self signed certificate
ERROR: sslStart: error in SSL_connect. SSL error: error:14090086:SSL routines:SSL3_GET_SERVER_CERTIFICATE:certificate verify failed
sslStart failed with error -2103000 SSL_HANDSHAKE_ERROR
~~~

Error from untrusted CA that signed the server certificate:

~~~
irods@hostname:~/ $ IRODS_LOG_LEVEL=LOG_NOTICE iinit
NOTICE: environment variable set, irods_log_level(input)=LOG_NOTICE, value=5
NOTICE: created irods_home=/dn/home/irods
NOTICE: created irods_cwd=/dn/home/irods
Enter your current PAM (system) password:
NOTICE: sslVerifyCallback: problem with certificate at depth: 1
NOTICE: sslVerifyCallback:   issuer = /C=US/ST=North Carolina/O=example.org/CN=irods.example.org Certificate Authority
NOTICE: sslVerifyCallback:   subject = /C=US/ST=North Carolina/O=example.org/CN=irods.example.org Certificate Authority
NOTICE: sslVerifyCallback:   err 19:self signed certificate in certificate chain
ERROR: sslStart: error in SSL_connect. SSL error: error:14090086:SSL routines:SSL3_GET_SERVER_CERTIFICATE:certificate verify failed
sslStart failed with error -2103000 SSL_HANDSHAKE_ERROR
~~~

Server verification can be turned off using the irods_ssl_verify_server `irods_environment.json` property. If this variable is set to 'none', then any certificate (or none) is accepted by the client. This means that your connection will be encrypted, but you cannot be sure to what server (i.e. there is no server authentication). For that reason, this mode is discouraged.

It is much better to set up trust for the server's certificate, even if it is a self-signed certificate. The easiest way is to use the irods_ssl_ca_certificate_file `irods_environment.json` property to contain all the certificates of either hosts or CAs that you trust. If you configured the server as described above, you could just set the following property in your `irods_environment.json`:

~~~
"irods_ssl_ca_certificate_file": "/etc/irods/chain.pem"
~~~


Or this file could just contain the root CA certificate for a CA-signed server certificate. Another potential issue is that the server certificate does not contain the proper FQDN (in either the Common Name field or the subjectAltName field) to match the client's 'irods_host' property. If this situation cannot be corrected on the server side, the client can set:

~~~
"irods_ssl_verify_server": "cert"
~~~

Then, the client library will only require certificate validation, but will not check that the hostname of the iRODS server matches the hostname(s) embedded within the certificate.

### Environment Variables

All the `irods_environment.json` properties used by the SSL support (both server and client side) are listed below:

irods_ssl_certificate_chain_file (server)
:       The file containing the server's certificate chain. The certificates must be in PEM format and must be sorted starting with the subject's certificate (actual client or server certificate), followed by intermediate CA certificates if applicable, and ending at the highest level (root) CA.

irods_ssl_certificate_key_file (server)
:       Private key corresponding to the server's certificate in the certificate chain file.

irods_ssl_dh_params_file (server)
:       The Diffie-Hellman parameter file location.

irods_ssl_verify_server (client)
:       What level of server certificate based authentication to perform. 'none' means not to perform any authentication at all. 'cert' means to verify the certificate validity (i.e. that it was signed by a trusted CA). 'hostname' means to validate the certificate and to verify that the irods_host's FQDN matches either the common name or one of the subjectAltNames of the certificate. 'hostname' is the default setting.

irods_ssl_ca_certificate_file (client)
:       Location of a file of trusted CA certificates in PEM format. Note that the certificates in this file are used in conjunction with the system default trusted certificates.

irods_ssl_ca_certificate_path (client)
:       Location of a directory containing CA certificates in PEM format. The files each contain one CA certificate. The files are looked up by the CA subject name hash value, which must be available. If more than one CA certificate with the same name hash value exist, the extension must be different (e.g. 9d66eef0.0, 9d66eef0.1, etc.).  The search is performed based on the ordering of the extension number, regardless of other properties of the certificates.  Use the 'c_rehash' utility to create the necessary links.



