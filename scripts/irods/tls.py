import os
import pathlib

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import dh, rsa
from cryptography.x509.oid import NameOID

from . import lib


def generate_tls_certificate_key(directory=None):
    key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=2048,
    )

    keyfile = pathlib.Path(directory or os.getcwd()) / 'server.key'

    with open(keyfile, "wb", opener=lib.read_write_owner_opener) as f:
        f.write(key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.TraditionalOpenSSL,
            encryption_algorithm=serialization.NoEncryption(),
            ))

    return key, keyfile


def generate_tls_self_signed_certificate(key, directory=None):
    import datetime

    subject = issuer = x509.Name([
        x509.NameAttribute(NameOID.COUNTRY_NAME, u'XX'),
        x509.NameAttribute(NameOID.STATE_OR_PROVINCE_NAME, u'State'),
        x509.NameAttribute(NameOID.LOCALITY_NAME, u'City'),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME, u'Organization'),
    ])

    cert = x509.CertificateBuilder() \
               .subject_name(subject) \
               .issuer_name(issuer) \
               .public_key(key.public_key()) \
               .serial_number(x509.random_serial_number()) \
               .not_valid_before(datetime.datetime.utcnow()) \
               .not_valid_after(datetime.datetime.utcnow() + datetime.timedelta(days=365)) \
               .add_extension(x509.SubjectAlternativeName([x509.DNSName(u"localhost")]),
                              critical=False) \
               .sign(key, hashes.SHA256())

    certfile = pathlib.Path(directory or os.getcwd()) / 'server.crt'

    with open(certfile, "wb") as f:
        f.write(cert.public_bytes(serialization.Encoding.PEM))

    return certfile


def generate_tls_dh_params(generator=2, key_size=1024, directory=None):
    parameters = dh.generate_parameters(generator=2, key_size=key_size)

    dhfile = pathlib.Path(directory or os.getcwd()) / 'dhparams.pem'

    with open(dhfile, 'wb') as f:
        f.write(parameters.parameter_bytes(encoding=serialization.Encoding.PEM,
                                           format=serialization.ParameterFormat.PKCS3))

    return dhfile
