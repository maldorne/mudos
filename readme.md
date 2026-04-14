<p align="center">
  <img width="400" alt="MudOS logo" src="/mudos_logo.webp">
</p>

# MudOS MUD driver

This was the old description of the MudOS project included in its old `README` file:

```
MudOS is an LPmud server (driver) which was originally distributed as an
enhanced version of the LPmud 3.x driver.

The official support site for MudOS is http://www.mudos.org/

For more information about LPmuds and MUDs in general try reading
the USENET groups rec.games.mud.{admin,announce,misc,lp,tiny,diku}.
```

## Branches

This branch contains `v21.7-maldorne`, a Maldorne fork that merges `v21.7` with the patches from the `v21.7b21_fr` branch, plus our own additions:

**From v21.7b21_fr:**
- Build fixes: missing includes (`unistd.h`, `crypt.h`), `-Bshared` flag for LPC-to-C
- lex.c memory corruption bugfix in `get_text_block()`
- `umask(002)` for group-writable file creation
- Anti-`@@` input sanitization to prevent `process_string` injection
- `DISCWORLD_ADD_ACTION`: priority-based action matching, `event()`, `actions_defined()`, compat efuns
- `member_array()` 4th argument for prefix matching
- `add_action()` flag passthrough (removed `& 3` mask)
- regexp error message cleanup (remove trailing newlines)
- NLP parser relocated from `packages/` to driver root with bugfixes
- Mapping restore hash distribution fix, `stralloc.h` macro precedence fix

**Maldorne additions:**
- PROXY protocol v1 support (`#define SUPPORT_PROXY_PROTOCOL` in `local_options`)

The `master` branch is empty, try any other branch to see different versions.

## How to use this version with Docker

- Build the container image (from project base directory)

  `docker build --no-cache . -t ghcr.io/maldorne/mudos:v21.7-maldorne`

- Run the container and take a look inside using a terminal

  `docker run --rm -ti ghcr.io/maldorne/mudos:v21.7-maldorne /bin/bash`

  Inside the container, in `/opt/mud`, you can find the directories `driver` 
  (with the source code of MudOS) and `bin`, with the two binaries needed
  to use MudOS (`addr_server` and `driver`).

- Publish the container in Docker Hub

  `docker push ghcr.io/maldorne/mudos:v21.7-maldorne`
