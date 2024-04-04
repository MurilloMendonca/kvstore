# Key-Value Store System

This project implements a simple key-value store system with a client-server architecture. It includes a custom mapper library (`libmapper`) for internal data handling, a server application that manages key-value pairs, and a client library (`libkvclient`) for interacting with the server.

## Features

What this project is NOT capable of:
- Being a Redis replacement for production use.
- Handling large amounts of data.
- Being a distributed system.
- Being a secure system.
- Support multiple clients accessing the same map.

What this project tries to achieve:

- Lightweight key-value store server.
- Client library for easy interaction with the server.
- Custom data mapper for efficient data manipulation.
- Comprehensive test suite for validation of components.
- Benchmarking tool for performance analysis.

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

### Prerequisites

- GNU Compiler Collection (GCC) for C and C++ with support for C++17 and C99
- GNU Make

### Building the Project

Clone the repository to your local machine:

```bash
git clone https://your-repository-url.git
cd your-repository-name
```

To compile the project, including the server, client library, and tests, run:
```bash
make all
```

This will generate the dynamic and static libraries (`libmapper.so`, `libmapper.a`, `libkvclient.so`, `libkvclient.a`), the server and client test executables, and the mapper test.
### Running the Tests

To run the provided tests for the mapper and client components, execute:
```bash
make test
```
The client test needs some instance of the server running.
### Cleaning the Build

To clean the project directory of all compiled files, run:
```bash
make clean
```

### Debugging

To compile the project with debugging information, run:
```bash
make DEBUG=1
```
This will generate the debug executables and libraries with the -g flag, and sanitizers enabled.

## Usage
### Server

Start the server by running:

```bash
./server [port]
```
If no port is specified, the server will listen on the default port 8080.
Client Test

Run the client test executable to validate client functionality:
```bash
./clientTest [port]
```

The benchmark tool is used to analyze the performance of the key-value store system. Run it by specifying the number of messages to send as an argument:
```bash
./benchmark [port] [number_of_messages]
```

### Client

The `client.cpp` and `client.hpp` files provide a high-level C++ wrapper around the Key-Value system. They are designed to be included and linked into other C++ projects through the `libkvclient.so` and `libkvclient.a` files.

The API is documented in the .hpp file, and usage examples are provided under the TEST macro at the bottom of the .cpp file.

The wrapper is a class that provides methods to connect, create, read and write key-values pairs to the server. 
The is also a namespace `kv::utils` that provides helper functions get and set non-string values that are trivially copiable.
### Mapper

The low-level functionality is entirely contained in pure C, with the API described in the mapper.h file. The implementation and a simple test are in the mapper.c file, and the final library is compiled into `libmapper.so` and `libmapper.a` files (for dynamic and static linking, respectively).

This API is not designed for direct access by end-users but rather to be wrapped by higher-level components. The primary interface for consumers is provided by the Client library.
