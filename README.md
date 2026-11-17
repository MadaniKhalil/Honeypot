# Honeypot

A small honeypot written in C. It listens on a TCP port, shows a fake telnet login, always answers "Login incorrect", and records every attempt (time, IP, username, password) in SQLite.

**Status: work in progress.**

## What works

- TCP listener (`socket`, `bind`, `listen`, `accept`), restartable with `SO_REUSEADDR`
- Fake login: `username:` and `password:` prompts, always rejected
- Visitor IP extracted and logged
- SQLite storage with prepared statements (inputs are bound, never pasted into SQL)
- Accept loop: a failed visitor never stops the trap

## Planned

- Timeout for silent visitors
- Log visitors who disconnect mid-login
- Tarpit (slow replies) and fake shell with canned replies only
- Flask dashboard: top passwords, attacks per hour
- Docker, one-command run

## Build

Requires `gcc` and the SQLite development package:

```
sudo apt install build-essential libsqlite3-dev
gcc -Wall -Wextra -g trap.c -o trap -lsqlite3
```

## Run and test

```
./trap
```

In another terminal:

```
nc 127.0.0.1 2323
sqlite3 attempts.db "select * from attempts;"
```

The trap listens on `127.0.0.1:2323` only, so nothing outside your machine can reach it.

## Security notes

- Every network read has a fixed size limit.
- Visitor input is stored with `sqlite3_bind_text`, so a password like `x'); DROP TABLE attempts; --` is saved as plain text.
- Visitor input never reaches `system()` or a shell.
- The database and logs are git-ignored, so captured data is never committed.
