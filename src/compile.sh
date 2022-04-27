#!/bin/bash

# chmod u+x /certificataes/generate-certificate.sh
# ./certificates/generate-certificate.sh
# sudo apt update
# sudo apt install -y libssl-dev
gcc ftps-server.c -o server -lpthread -lssl -lcrypto

./server
# ./server -default
