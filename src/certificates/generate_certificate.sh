#!/bin/bash
CERTIFICATE_FILENAME="server-cert.pem"
CERTIFICATE_KEY_FILENAME="server-key.pem"

# Test if the certificate and key files exist
if [ -f $CERTIFICATE_FILENAME ] && [ -f $CERTIFICATE_KEY_FILENAME ]; then
    echo "Certificate and key files already exist."
    exit 0
fi

openssl req -newkey rsa:2048 -new -nodes -x509 -days 30  -keyout ${CERTIFICATE_KEY_FILENAME} -out ${CERTIFICATE_FILENAME}