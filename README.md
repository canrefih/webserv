# webserv

A lightweight HTTP server written in **C++98** as part of the 42 curriculum, focused on socket programming, HTTP request/response handling, configuration parsing, and event-driven I/O.

## Engineering Focus

- TCP socket creation, binding, listening and client acceptance
- Non-blocking sockets with `poll()`-based event handling
- HTTP request parsing and response generation
- Multiple virtual server configurations
- Location-based routing and allowed HTTP methods
- Static file serving and directory/index handling
- File uploads and configurable upload locations
- Custom error pages
- Request body limits
- Explicit file-descriptor and resource management

## Architecture

```text
                    configuration file
                           │
                           ▼
                    ┌─────────────┐
                    │ Config      │
                    │ parser      │
                    └──────┬──────┘
                           │
                           ▼
                  ┌─────────────────┐
                  │ ServerConfig(s) │
                  └────────┬────────┘
                           │
                           ▼
                  ┌─────────────────┐
                  │   Server        │
                  │ socket + poll() │
                  └────────┬────────┘
                           │
                    client connection
                           │
              ┌────────────┴────────────┐
              ▼                         ▼
       HttpRequest                 HttpResponse
       parse + validate            status + headers
              │                         │
              └────────────┬────────────┘
                           ▼
                    socket response
```

The implementation keeps configuration parsing, server state, HTTP parsing, response generation and location rules separated into focused components.

## Supported Configuration

Example configuration supports:

- Multiple `server` blocks
- `listen host:port`
- `root` and `index`
- `autoindex`
- `client_max_body_size`
- Custom `error_page` entries
- `location` blocks
- Allowed methods
- File upload configuration
- Multiple listening ports

See [`example.conf`](example.conf) for a multi-server example and [`test.conf`](test.conf) for the automated test configuration.

## HTTP / Server Behavior

The server uses non-blocking sockets and `poll()` to multiplex listening and client sockets. Requests are buffered until the HTTP header terminator is received, then parsed and routed according to the active server and location configuration.

Implemented behavior includes:

- `GET`, `POST` and `DELETE` method handling where configured
- HTTP request header parsing
- `Content-Length` handling
- `400 Bad Request`
- `405 Method Not Allowed`
- `413 Payload Too Large`
- `500 Internal Server Error`
- Static file responses
- Configurable error documents
- POST file uploads

## Build

Requirements:

- Linux / Unix-like environment
- C++ compiler
- `make`

The project intentionally targets **C++98**:

```bash
make
```

Run the server with a configuration file:

```bash
./webserv example.conf
```

The binary expects exactly one configuration argument:

```text
Usage: ./webserv [configuration file]
```

## Testing

The repository contains focused unit-style tests for configuration parsing, server configuration, locations, HTTP requests/responses, and server integration.

Run the test suite:

```bash
cd tests
../tests/compile_tests.sh
./run_all_tests.sh
```

The integration test also verifies server startup and configuration loading through a child process.

## CI

GitHub Actions builds the server with `-Wall -Wextra -Werror -std=c++98` and executes the repository's test suite on pushes and pull requests.

## Project Structure

```text
webserv/
├── include/              # C++ interfaces
├── src/                  # Server, HTTP and configuration implementation
├── config/               # Configuration-related resources
├── tests/                # Unit and integration tests
├── www/                  # Static test content and error pages
├── example.conf          # Example multi-server configuration
├── test.conf             # Test configuration
├── Makefile
└── README.md
```

## Engineering Notes

The project emphasizes low-level networking fundamentals: socket lifecycle management, non-blocking I/O, polling, request buffering, configuration-driven routing, and explicit cleanup. It is designed to make the control flow and system boundaries visible rather than hiding them behind a web framework.

## Development

This project was developed as part of the 42 curriculum. Changes should remain focused and preserve the existing C++98 constraints and test coverage.
