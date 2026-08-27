# Webserv Project — Current Work Plan (v2)

*Team of 3, Linux, C++98 — updated based on current codebase*

---

## 1. Current Status Summary

The project was started solo (by REFIH), and initial drafts of all three blocks
(Core, HTTP, Config/Routing) already exist:

| Block | Status |
|---|---|
| Core (poll loop, non-blocking I/O, accept/read/write) | 🟡 Basic flow works; multi-port/keep-alive/timeout missing |
| HTTP Protocol (parser/response) | 🟡 Basic GET/POST/DELETE flow works; chunked/URL-decode/missing status codes not done |
| Config/Routing | 🟡 Single server block supported; multiple `server{}` blocks and CGI/redirect directives missing |
| CGI | 🔴 Not started |
| Bonus (cookie/session, 2nd CGI type) | 🔴 Not started |

**Critical blocker:** `ServerConfig` (support for multiple `server {}` blocks) is not
yet finished. This is the shared foundation that needs to be completed first so
teammates can work in isolation.

---

## 2. Phase 0 — REFIH: Deliver ServerConfig (blocking, before the team starts)

Nothing else can proceed fully independently until this phase is done.

- [ ] `ServerConfig` class: move the current `Config` contents (host, port, root,
      index, autoindex, client_max_body_size, error_pages, locations) so each
      instance represents a single server block
- [ ] `Config`: turn into a top-level container holding `std::vector<ServerConfig>`
- [ ] Parser: extend to split multiple `server {}` blocks into separate
      `ServerConfig` objects
- [ ] Commit and share the updated `.hpp` files (Config, ServerConfig, Location)
      with the team → **from this point the interface is considered frozen**

**Deliverable:** A stable `Config`/`ServerConfig`/`Location` interface that
Person 2 and Person 3 can build on top of.

---

## 3. After Phase 0 — Parallel Work Plan

Once ServerConfig is delivered, all three of us can work independently.
The table below shows which tasks depend on someone else's work and which don't.

### 👤 REFIH — Core / Network (independent)

| Task | Dependency |
|---|---|
| Multi-socket listening (each ServerConfig gets its own fd, all in one poll) | None |
| Timeout / hang protection (poll timeout + last-activity tracking per client) | None |
| Signal handling (clean shutdown on SIGINT) | None |
| Setting up stress-test infrastructure | None (full scenarios added once CGI/upload are done) |
| Keep-Alive (keeping connection open based on Connection header) | **Weak dependency** — requires early agreement with Person 2 on a generic `setHeader()` interface on `HttpResponse` |

### 👤 Person 2 — HTTP Protocol (independent)

| Task | Dependency |
|---|---|
| Chunked transfer-encoding (request unchunking + chunked response writing) | None |
| URL decoding (`%2e`, `%20`, etc.) and hardening the `..` security check | None |
| Missing status codes (411, 505, 301/302, etc.) | None |
| Redirect header (`Location:`) generation | Depends on the `redirect` field Person 3 will add to `Location` — **can be written in parallel, only merges at integration** |
| Making the parser's body-reading stream-based instead of `\0`-sensitive | None |
| `HttpResponse::setHeader()` generic header interface (needed for Keep-Alive) | None — best written early and shared with REFIH |

### 👤 Person 3 — Config / Routing / CGI (independent)

| Task | Dependency |
|---|---|
| CGI engine (fork/execve/pipe/dup2, env variables, EOF/Content-Length handling) | Done |
| Add `redirect` field to `Location` | None |
| Add `cgi_extension` mapping to `Location` | None |
| Make upload support multipart/form-data | None |
| Prepare test config files and a sample static site | None |

**Conclusion:** Once Phase 0 is done, all three of us are functionally independent.
The only coordination point is the `HttpResponse` header interface between REFIH and
Person 2 — a quick early agreement is enough.

---

## 4. Reducing Git Conflicts

All three of us will likely touch `Server.cpp` (REFIH for poll/connection logic,
Person 2/3 for handler logic) — not a real dependency, but a **merge-conflict risk**.

Suggestion: extract the GET/POST/DELETE/CGI handling logic out of `Server.cpp`
into a separate `RequestHandler` class:

- `Server.cpp` → only poll loop, sockets, connection lifecycle (REFIH's domain)
- `RequestHandler.cpp` → GET/POST/DELETE/CGI logic (where Person 2 and 3 will work)

Doing this split together with the Phase 0 delivery largely prevents the three of
us from colliding in the same file.

---

## 5. Updated Timeline

| Stage | REFIH (Core) | Person 2 (HTTP) | Person 3 (Config/CGI) |
|---|---|---|---|
| **Phase 0** (blocking) | ServerConfig + Config refactor + RequestHandler split | Waiting / reviewing existing code | Waiting / reviewing existing code |
| **Phase 1** | Multi-socket listening, timeout, signal handling | Chunked decoding, URL decoding, `setHeader()` interface | CGI env variables + fork/execve skeleton |
| **Phase 2** | Keep-Alive (in sync with Person 2) | Missing status codes, redirect header generation | CGI output reading (EOF/Content-Length), pipe management |
| **Phase 3** | Stress-test infrastructure | Parser hardening, edge-case testing | Redirect + cgi_extension config integration, multipart upload |
| **Phase 4 — Integration** | Merging the three blocks, general bug fixing (everyone) |
| **Bonus** | — | Leads cookie/session | 2nd CGI type, session store support |
| **Final** | README.md, test config files, demo prep (shared) |

---

## 6. Next Concrete Step

Recommended order to finish Phase 0:
1. Draft `ServerConfig.hpp/.cpp` (move current `Config` contents into it)
2. Update `Config.hpp/.cpp` to hold a `vector<ServerConfig>`
3. Extend the parser to distinguish multiple `server {}` blocks
4. Extract handler logic from `Server.cpp` into `RequestHandler` (optional but recommended)
5. Share the updated `.hpp` files with the team → Phase 1 begins
