# echo-server

## Description

Simple tcp echo server written in Boost.Beast and Boost.Asio (Boost.Asio).

## Build

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

## Usage

After successfule project build you can execute the binary with the following command: `./server <port>`.  
This will launch the echo server locally on your machine with loop back address and listening port: `<port>`.