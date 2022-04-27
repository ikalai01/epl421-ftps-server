# FTPS Server - Assignment 4
## EPL421 System Programming - Spring 2021/2022
### **This is an implementation of an FTP server over TLSv3 Socket Connection**

We implemented a server that is running the `FTP protocol` over `TLSv3 Sockets` in the programming language C. This implementation is based on the following paper:
RFC 959 - File Transfer Protocol (FTP).

### Before you start
- In order for the server to run, a certificate and a key must be provided.
  
> *Those must be added inside the folder named `certificates` and named:*
>> - _Certificate_ : `server-cert.pem`
>> - _Key_ : `server-key.pem `

- *We provided a shell script that will generate a self-signed certificate with a private key. The shell script is located in the `certificates` folder and is called `generate_certificate.sh`*

- *We provide a default configuration file named `config.conf` that will be used by the server. The configuration file is located in the `config` folder and is called `config.conf`*

- *In case you want to change the configuration file, you can do it by editing the file inside the `config` folder.*
> *The configuration file is a simple text file that contains the following parameters:*
> - *`NUM_THREADS`: The number of threads that will be used by the server.*
> - *`MAX_CONNECTIONS`: The maximum number of connections that the server will accept.*
> - *`IP`: The IP address that the server will listen to.*
> - *`PORT`: The port that the server will listen to.*
> - *`REUSE_SOCKET`: If set to `1`, the server will reuse the socket.*
> - *`HOME_DIR`: The home directory of the server.*
> - *`CERT_FILE`: The certificate file that will be used by the server.*
> - *`KEY_FILE`: The key file that will be used by the server.*

## How to compile and run

- *Compile code using: `make` or `make all` .*
- *Run code using: `./server` or `./server -default`*
- *File can be manually compiled using the provided compile.sh file*

>#### The server runs in 2 modes
>- Mode A: Run using: `./server` : _The server will use the values defined in the `config.conf` file. In case the file does not exist, the program will exit._
>>Default values are: 
>>- `NUM_THREADS=8` # _Number of threads running in the thread pool_
>>- `MAX_CONNECTIONS=16` # _Maximum number of connections the server can handle_
>>- `IP=127.0.0.1` # _IP address of the server_
>>- `PORT=30000` # _Port the server will listen on_
>>- `REUSE_SOCKET=0` # _If set to 1, the server will reuse the socket_
>>- `HOME_DIR=./ftphome` # _The home directory of the server_
>>- `CERT_FILE=./certificates/server-cert.pem` # _The server's certificate_
>>- `KEY_FILE=./certificates/server-key.pem` # _The server's private key_
>- Mode B: Run using: `./server -default` : _The server will use default values predefined in `common.h`_

## Features Implemented

- _USER_ `<username>`
- _PASS_ `<password>`
- _SYST_ `<system>`
- _FEAT_ `<features>`
- _PBSZ_ `<size>`
- _PROT_ `<protection>`
- _PWD_ `<path>`
- _TYPE_ `<type>`
- _CWD_ `<path>`
- _LIST_ `<path>`
- _RETR_ `<path>`
- _STOR_ `<path>`
- _MKD_ `<path>`
- _DELE_ `<path>`