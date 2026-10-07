*This project has been created as part of the 42 curriculum by recan, emercier, alubrano.*

# webserv

## Description

A non-blocking HTTP/1.1 server written in **C++98**, inspired by nginx.
A single `poll()` loop handles every socket (listeners, clients and CGI pipes), so one slow client or script never blocks the others.

The goal of the project is to understand how a web server works under the hood: socket lifecycle, I/O multiplexing, parsing HTTP requests, building responses, routing driven by a configuration file, and running CGI programs. All of this is written from scratch in C++98 with no external library.

### Features

**Networking**
- Non-blocking sockets multiplexed with a single `poll()` loop
- Several `server` blocks and several listening ports at once
- Virtual hosts through `server_name`
- Keep-alive connections, with idle clients closed after 60 s
- Clean shutdown on `SIGINT` / `SIGTERM`. `SIGPIPE` is ignored.

**HTTP**
- Methods: `GET`, `HEAD`, `POST`, `DELETE`
- Request bodies sent with `Content-Length` or `Transfer-Encoding: chunked`
- Static files, `index` files and directory listings (`autoindex`)
- File uploads to a configurable directory
- Redirects with `return <code> <url>;`
- Custom error pages and a body size limit per server or per location
- Session cookies (`session_id`, `HttpOnly`, valid for 1 h)

**CGI**
- Runs any interpreter mapped to a file extension (Python, PHP, `cgi_tester`, …)
- Runs asynchronously: CGI pipes go through the same `poll()` loop
- Protected against runaway scripts: 30 s timeout (`504`), crashes or empty output (`502`), output size cap

### Architecture

```text
 config file ──▶ Config ──▶ ServerConfig[] ──▶ Location[]
                                  │
                                  ▼
                      ┌───────────────────────┐
                      │  Server  (poll loop)  │◀── Signal (SIGINT/SIGTERM)
                      └───────────┬───────────┘
              accept / read       │        write / timeouts
                                  ▼
                            HttpRequest  ◀── URL
                                  │
                                  ▼
                           RequestHandler ──▶ static file / autoindex /
                                  │           upload / DELETE / redirect
                                  │
                                  ├──▶ CGIManager ──▶ CGIHandler (fork/exec)
                                  │                  CGIEnvBuilder (env)
                                  ▼
                            HttpResponse  ◀── CookiesSession
```

| Component         | Role |
|-------------------|------|
| `Config`          | Tokenizes and validates the config file, builds the `ServerConfig` objects |
| `ServerConfig` / `Location` | Settings of one server block / one location block |
| `Server`          | Opens listening sockets, runs the `poll()` loop, buffers I/O, handles keep-alive and timeouts |
| `HttpRequest`     | Parses the request line, headers and body (including chunked bodies) |
| `URL`             | Splits and decodes the request target (path, query, path info) |
| `RequestHandler`  | Matches the location and turns a request into a response |
| `HttpResponse`    | Builds the status line, headers and body |
| `CGIManager`      | Tracks running CGIs in the poll loop: pipes, timeouts, reaping zombies |
| `CGIHandler`      | `fork` / `execve` of one CGI process |
| `CGIEnvBuilder`   | Builds the CGI environment variables |
| `CookiesSession`  | Creates, checks and expires session cookies |

### Project structure

```text
webserv/
├── src/            # Implementation
├── include/        # Headers
├── config/         # Valid and invalid configuration files + cgi_tester
├── www/            # Demo site, error pages, uploads, CGI scripts
├── YoupiBanane/    # Directory tree used by the 42 tester
├── tests/          # Unit and integration tests
├── project_inf/    # Work plan and diagrams
└── Makefile
```

## Instructions

### Build & run

Requirements: Linux (or another Unix-like system), a C++98 compiler and `make`.

```bash
make                              # builds ./webserv
./webserv config/default.conf     # starts the server
```

Then open <http://127.0.0.1:8080>.

The binary takes exactly one argument, the configuration file:

```text
Usage: ./webserv [configuration file]
```

| Make target      | Description                         |
|------------------|-------------------------------------|
| `make`           | Build the server                    |
| `make run_tests` | Build and run the full test suite   |
| `make test_cgi`  | Build and run only the CGI tests    |
| `make clean`     | Remove object files and test binaries |
| `make fclean`    | `clean` + remove `webserv`          |
| `make re`        | Rebuild from scratch                |

### Configuration

The syntax follows nginx: `directive value;` lines, grouped in `server { }` and `location <path> { }` blocks.

```nginx
server {
    listen 127.0.0.1:8080;
    server_name localhost;

    root ./www;
    index index.html;
    autoindex on;
    client_max_body_size 10M;

    error_page 404 /errors/404.html;
    error_page 405 /errors/405.html;

    location / {
        allow_methods GET DELETE;
    }

    location /upload {
        allow_methods POST;
        upload on;
        upload_store ./www/uploads;
    }

    location /old {
        return 301 /;
    }

    location /cgi-bin {
        allow_methods GET POST;
        cgi_extension .py /usr/bin/python3;
    }
}
```

#### Directives

| Directive              | Context            | Description |
|------------------------|--------------------|-------------|
| `listen`               | server             | `host:port` (or just `port`) to listen on |
| `server_name`          | server             | Name(s) matched against the `Host` header |
| `root`                 | server, location   | Directory files are served from |
| `index`                | server, location   | Default file for a directory request |
| `autoindex`            | server, location   | `on` / `off`: generate a directory listing |
| `client_max_body_size` | server, location   | Max request body size (`10`, `10K`, `10M`, …); larger bodies get `413` |
| `error_page`           | server             | `error_page <code> <uri>;` sets a custom error page |
| `allow_methods`        | location           | Allowed methods among `GET`, `HEAD`, `POST`, `DELETE`; others get `405` |
| `return`               | location           | `return <code> <url>;` sends a redirect |
| `upload`               | location           | `on` / `off`: accept file uploads |
| `upload_store`         | location           | Directory where uploaded files are written |
| `cgi_extension`        | location           | `cgi_extension <ext> <interpreter> [virtual];` (see [CGI](#cgi)) |

Ready-to-use configurations are in [`config/`](config/):
- [`default.conf`](config/default.conf) serves the demo site in `www/`
- [`example.conf`](config/example.conf) runs two servers on two ports
- [`cgi_test.conf`](config/cgi_test.conf) is the setup for the 42 `tester`

The config files whose names contain `invalid`, `unclosed`, … are deliberately broken. The parser must reject them with a clear error message.

### CGI

```nginx
cgi_extension .py /usr/bin/python3;
cgi_extension .bla ./config/cgi_tester virtual;
```

- A request whose path ends with the extension runs `<interpreter> <script>` in a child process.
- The request body goes to the script's stdin, and the CGI/1.1 environment (`REQUEST_METHOD`, `QUERY_STRING`, `PATH_INFO`, `CONTENT_LENGTH`, `CONTENT_TYPE`, `HTTP_*`, …) is set up.
- The script's output (headers + body) is parsed and returned to the client.
- `virtual` runs the interpreter even when the requested file does not exist. This is for programs that don't need a script file, such as the 42 `cgi_tester`.

Sample scripts covering edge cases (crash, timeout, huge output, closed stdout, …) live in [`www/cgi-bin/`](www/cgi-bin/).

### Testing

**Unit and integration tests.** These are the same tests CI runs on every push and pull request to `main`:

```bash
make run_tests
```

They cover config parsing (valid and invalid), HTTP requests and responses, locations, URL parsing, keep-alive, multiple sockets, timeouts, signals, CGI and sessions.

**42 tester:**

```bash
./webserv config/cgi_test.conf
./tester http://127.0.0.1:8080       # in another terminal
```

**By hand:**

```bash
curl -v http://127.0.0.1:8080/
curl -v -X POST -F "file=@README.md" http://127.0.0.1:8080/upload
curl -v -X DELETE http://127.0.0.1:8080/uploads/README.md
curl -v -H "Transfer-Encoding: chunked" -d @README.md http://127.0.0.1:8080/upload
```

## Resources

### References

- [RFC 9110 – HTTP Semantics](https://www.rfc-editor.org/rfc/rfc9110)
- [RFC 9112 – HTTP/1.1](https://www.rfc-editor.org/rfc/rfc9112)
- [RFC 3875 – CGI/1.1](https://www.rfc-editor.org/rfc/rfc3875)
- [nginx documentation](https://nginx.org/en/docs/), the reference for config syntax and behavior
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- `man 2 poll`, `man 2 socket`, `man 2 fork`, `man 2 execve`

### AI usage

- **Documentation**: writing and structuring this README from the source code.
- Create Index pages
- AI was used to test the project more thoroughly, identify errors that we had not noticed, and guide our research on how to solve them.
