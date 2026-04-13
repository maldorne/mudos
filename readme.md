<p align="center">
  <img width="400" alt="MudOS logo" src="/mudos_logo.webp">
</p>

> [!IMPORTANT]  
> The `master` branch is empty, the code for different versions is located in other branches.

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

On this repository, you could find different branches for the following versions of MudOS (from older to newer):

 - `v21.7b21_fr`
 - `v21.7`
 - `v22.2b13`
 - `v22.2b14`
 - `v22.2-maldorne` (Maldorne fork, based on `v22.2b14` with fixes)

The `master` branch is empty, try any other branch to see its contents.

### Current status of each branch

| Branch           | Notes                                   | Status              |
| ---------------- | --------------------------------------- | ------------------- |
| `v21.7b21_fr`    |                                         | Working with Docker |
| `v21.7`          |                                         | Working with Docker |
| `v22.2b13`       |                                         | Working with Docker |
| `v22.2b14`       | Last version of MudOS                   | See note below*     |
| `v22.2-maldorne` | Fork of b14 with minor fixes and addons | Working with Docker |

If you want to test the images in your local machine, you can use them directly 
from the [Github Container Registry](https://github.com/maldorne/mudos/pkgs/container/mudos).

### Modern build on Debian 12

Every branch now compiles on **Debian 12 Bookworm** with `gcc 12`, replacing the old Debian Sarge (2005) + `gcc 3.3.5` pipeline. The driver itself is still built as a 32-bit i386 ELF binary — `int` and `void *` need to be the same width for the K&R C code in MudOS to behave correctly — but the surrounding container userland is now fully modern: current `glibc`, current `git`/`openssh`, security updates, and any host architecture that can run Docker.

All version branches share the **exact same `Dockerfile` recipe**, differing only in the version string printed in the header comment. Porting any future patch or fix across branches is a simple copy-paste.

To make `gcc 12` accept the legacy source without patching it, the `Dockerfile` injects a set of compatibility flags into `build.MudOS` via `sed`: `-m32` (i386), `-fgnu89-inline` (pre-C99 inline semantics), `-fcommon` (pre-`gcc 10` tentative definitions), and several `-Wno-*` / `-Wno-error=*` flags to tolerate K&R-style prototypes, implicit int and mismatched conversions.

A second patch is applied to the generated `system_libs` file (produced by `./edit_source -configure`) to strip `-ly` (the legacy yacc runtime library, not needed — MudOS provides its own `yyerror`/`main`) and `-lnsl` (the legacy network-services library, whose functions are now part of glibc proper). Both of these libraries used to exist on Sarge but are no longer shipped for i386 on modern Debian, so the final link step fails without this patch.

The full story — including the 64-bit pointer-truncation segfault that drove the decision to go back to i386 — is in the blog post [Compiling MudOS on Modern Linux: From Debian Sarge to Bookworm](https://maldorne.org/2026/04/09/compiling-mudos-on-modern-linux/).

## Some notes about version history

- Versions of MudOS advanced up to `0.8` updating its minor version (using something like semantic versioning).
- `0.9` started naming versions with `0.9.x`, from `0.9.1` to `0.9.19`.
- What would have been `0.9.20` started with a new naming system, as the first `v20`. And it went from `v20.1` to `v20.26`.
- This `vXX` had some alpha and beta versions (`v21ax` and `v21bx`), you can check the `ChangeLog.alpha` and `ChangeLog.beta` files in the `v21.7` branch.
- **v21** went from `v21.1` to `v21.7`.
- I'm not really sure about the contents of the `v21.7b21_fr` branch, seems to be a fork made from the last beta version of `v21.7` (`v21.7b22` was renamed as `v21.7`). It seems to have a different parser and you can define in the `local_options` file: `#define DISCWORLD_ADD_ACTION`. It's included here, but I'm not really sure about the changes included, and the `v21.7` branch should be newer, although without some of the changes done in that fork.
- **v22** had `v22.1` and several alphas and betas of `v22.2`, which was never published. `v22.2b14` (Dec 12 2003) was the last version of MudOS.

## Usage at Maldorne

These are the versions we currently run (or plan to run) in the [Maldorne](https://maldorne.org) MUD cluster:

| Version          | Used by                                                                                                                                                                                      | Notes                                                                                       |
| ---------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------- |
| `v21.7b21_fr`    | [Iluminado](https://maldorne.org/games/#iluminado-mud), [Ancient Kingdoms](https://maldorne.org/games/#ancient-kingdoms), [Reinos de Leyenda](https://maldorne.org/games/#reinos-de-leyenda) | In production.                                                                              |
| `v21.7`          | —                                                                                                                                                                                            | Compiled and published, not used by any of our MUDs. Should work; feedback welcome.         |
| `v22.2b13`       | —                                                                                                                                                                                            | Used in production with Ciudad Capital before switching to v22.2-maldorne. Works correctly. |
| `v22.2b14`       | —                                                                                                                                                                                            | Compiled and published, not used by any of our MUDs. See note below*.                       |
| `v22.2-maldorne` | [Ciudad Capital](https://maldorne.org/games/#ciudad-capital-v1)                                                                                                                              | In production. Fork of b14 with minor fixes and addons.                                     |

## About the last available version of MudOS

\*`v22.2b14` compiles and starts, but the heartbeat timer never fires on modern Linux (Debian 12), so `uptime()` is permanently 0. We confirmed this both under emulation (arm64 host) and natively on x86_64.

The cause is a one-line change in `driver/ualarm.c`: the `#include "std.h"` was moved from line 39 (inside the `#ifndef HAS_UALARM` block) to line 1 (before the guard). On modern glibc, `std.h` defines `HAS_UALARM`, so the entire fallback `ualarm()` implementation is skipped. MudOS then uses the system's `ualarm()`, which does not work correctly with the driver's signal-based heartbeat loop.

The fix is to move `#include "std.h"` back inside the `#ifndef` block (i.e. revert that specific change from b14). We verified that b14 with this single revert works correctly with a production mudlib. This fix is applied in the `v22.2-maldorne` branch.

## About our fork branches

The name "MudOS" is copyrighted by Erik Kay, Adam Beeman, Stephan Iannce and John Garnett (1991-1992). The entire package is additionally copyrighted by Tim Hollebeek (1995). The license permits modification and redistribution for non-commercial use. See the `driver/Copyright` file in any version branch for the full text.

Historically, some time after the MudOS project became unmaintained, there was [community discussion](https://groups.google.com/g/rec.games.mud.lp/c/Jwg7B335N3A) about forking and naming rights. The main community fork is [FluffOS](https://github.com/fluffos/fluffos), originally created by the Discworld MUD team to maintain their own driver patches independently.

Our `-maldorne` branches (e.g. `v22.2-maldorne`) are **not** new versions of MudOS. We do not claim any rights over the MudOS name, trademark, or project. These branches are minimal forks of specific MudOS versions with only the fixes strictly necessary to run pre-existing mudlibs on modern Linux (Debian 12), solely for preservation purposes. They exist because the original source code has not been maintained since 2003 and does not compile or run correctly on current systems without small patches. We publish the resulting Docker images solely for our own non-commercial use and for the benefit of the MUD community.

### Changes included in each fork branch

#### `v22.2-maldorne` (based on `v22.2b14`)

| Change     | File              | Description                                                                                                                                                                                   |
| ---------- | ----------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| ualarm fix | `driver/ualarm.c` | Reverted the `#include "std.h"` move that broke the internal timer on modern Linux, causing `uptime()` to be permanently 0. See the [note above](#about-the-last-available-version-of-mudos). |
| PROXY protocol v1 | `driver/comm.c`, `driver/local_options` | Reads a [PROXY protocol v1](https://www.haproxy.org/download/1.8/doc/proxy-protocol.txt) header on new connections and uses the real client IP. Enabled via `#define SUPPORT_PROXY_PROTOCOL` in `local_options`. Backwards compatible. |