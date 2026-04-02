# redis-clone

A lightweight Redis-compatible server implemented in C++, built as a systems programming exercise. The server speaks the Redis Serialization Protocol (RESP), accepts concurrent client connections over TCP, and supports a growing subset of Redis commands including `GET`, `SET`, `DEL`, `EXPIRE`, and `TTL`.

The goal is not to replace Redis, but to understand it — by implementing the core pieces from scratch: a TCP server, a protocol parser, a thread-safe key-value store, and a command dispatcher. The project is built incrementally, with each commit leaving the server in a working state.

![Build](https://github.com/YazeedSal/Redis-Clone/actions/workflows/build.yml/badge.svg)

## Build
```bash
cmake -S . -B build
cmake --build build
./build/redis_clone
```

## Supported Commands

| Command | Syntax | Description |
|---------|--------|-------------|
| PING | `PING` | Returns PONG, used to test connection |

## Project Status

| Component          | Status      |
|--------------------|-------------|
| TCP Server         | ✅ Done     |
| RESP Parser        | ✅ Done     |
| RESP Serializer    | ✅ Done     |
| Key-Value Store    | ⏳ Pending  |
| Command Dispatcher | ⏳ Pending  |
| CI (GitHub Actions)| ✅ Done     |
