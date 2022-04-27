#!/bin/bash

openssl req -newkey rsa:2048 -new -nodes -x509 -keyout -days 30 server-key.pem -out server-cert.pem