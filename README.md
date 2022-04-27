# FTPS Server - Assignment 4 - EPL421 System Programming
## _This is an implementation of an FTP server over TLSv3 Socket Connection_

We implemented a server that is running the `FTP protocol` over `TLSv3 Sockets`.
- *In order for the server to run, it is best to provide a certificat and a private key for the server.*
> *Those must be added inside a folder named `certificates` and named:*
> - _Certificate_ : `server-cert.pem`
> - _Key_ : `server-key.pem `

## How to compile and run

- *Compile code using: `make` or `make all`.*
- *Run code using: `./server` or `./server args`*
- *File can be manually compiled using the provided compile.sh file*

>#### The server runs in 2 modes
>- Mode A: Run using: `./server` : _The server will use the values defined in the `config.conf` file_
>>Default values are: 
>>- `NUM_THREADS=8` # _Number of threads running in the thread pool_
>>- `MAX_CONNECTIONS=16` # _Maximum number of connections the server can handle_
>>- `IP=127.0.0.1` # _IP address of the server_
>>- `PORT=30000` # _Port the server will listen on_
>>- `REUSE_SOCKET=1` # _If set to 1, the server will reuse the socket_
>>- `HOME_DIR=./ftphome` # _The home directory of the server_
>>- `CERT_FILE=./certificates/server-cert.pem` # _The server's certificate_
>>- `KEY_FILE=./certificates/server-key.pem` # _The server's private key_
>- Mode B: Run using: `./server default` : _The server will use default values predefined in `common.h`_

## Features

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