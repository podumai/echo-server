# echo-server

## Description

Simple tcp echo server written in Boost.Beast and Boost.Asio (Boost.Asio).  
Also there is additional test impl in Linux (epoll).

## Build

### (Test) Beast implementation

> [!IMPORTANT]
> To build the project you need to have locally installed:  
> 1. C/C++ compiler such as clang/gcc;  
> 2. CMake build system;  
> 3. Boost.Beast and Boost.Asio;  
> 4. Fmt library installed.  
> 
> Note: [fmt](https://github.com/fmtlib/fmt) library;

To build the project you can execute this command:  
```
cmake -DCMAKE_BUILD_TYPE=Release -S . -B ./build
cmake --build build
```
This will put all build artifacts into build directory with executable in build/echo-server/bin.  

### (Test) Linux implementation

This project also contains linux specific implementation of sync server.  
To build Linux specific implementation you need to set option: `ON`.  
| Option | Supported values | Default value |
| :---: | :---: | :---: |
| BUILD_LINUX_IMPL | ON/OFF | OFF |  
> [!WARNING]
> If `pthread_t` type is not equal or convertible to `unsigned long`:
> The compilation will fail as logger uses this feature to print the
> threads ids.
> (P.S. Maybe it will be a better idea to print `gettid` instead of `pthread_self`)  
> (But it only test impl that is progressing to support nonblocking IO in future versions and etc)

## Usage

### (Test) Beast implementation

After successful project build you can execute the binary with the following command: `./server <port>`.  
This will launch the echo server locally on your machine with loop back address and listening port: `<port>`.

### (Test) Linux implementation

After successful project build you can execute the binary with the followin command: `./server`.  
It will launch the server on the range of ports: `10000-10009`; listening on you local address.  
You can connect to it using `telnet`. Try following command to connect to the server: `telnet 127.0.0.1 10000`.
