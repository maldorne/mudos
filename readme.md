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

On this repository, you could find different branches for the following versions of MudOS (from newer to older):

 - `v22.2b14`
 - `v22.2b13`
 - `v21.7`
 - `v21.7b21_fr`

The `master` branch is empty, try any other branch to see its contents.

### Current status of each branch

| Branch        | Notes                 | Status              |
| ------------- | --------------------- | ------------------- |
| `v22.2b14`    | Last version of MudOS | Working with Docker |
| `v22.2b13`    |                       | Working with Docker |
| `v21.7`       |                       | Working with Docker |
| `v21.7b21_fr` |                       | Working with Docker |

If you want to test the images in your local machine, you can use them directly 
from the [Github Container Registry](https://github.com/maldorne/mudos/pkgs/container/mudos).

### Modern build on Debian 12

Every branch now compiles on **Debian 12 Bookworm** with `gcc 12`, replacing the old Debian Sarge (2005) + `gcc 3.3.5` pipeline. The driver itself is still built as a 32-bit i386 ELF binary — `int` and `void *` need to be the same width for the K&R C code in MudOS to behave correctly — but the surrounding container userland is now fully modern: current `glibc`, current `git`/`openssh`, security updates, and any host architecture that can run Docker.

All four version branches (`v21.7b21_fr`, `v21.7`, `v22.2b13`, `v22.2b14`) share the **exact same `Dockerfile` recipe**, differing only in the version string printed in the header comment. Porting any future patch or fix across branches is a simple copy-paste.

To make `gcc 12` accept the legacy source without patching it, the `Dockerfile` injects a set of compatibility flags into `build.MudOS` via `sed`: `-m32` (i386), `-fgnu89-inline` (pre-C99 inline semantics), `-fcommon` (pre-`gcc 10` tentative definitions), and several `-Wno-*` / `-Wno-error=*` flags to tolerate K&R-style prototypes, implicit int and mismatched conversions.

A second patch is applied to the generated `system_libs` file (produced by `./edit_source -configure`) to strip `-ly` (the legacy yacc runtime library, not needed — MudOS provides its own `yyerror`/`main`) and `-lnsl` (the legacy network-services library, whose functions are now part of glibc proper). Both of these libraries used to exist on Sarge but are no longer shipped for i386 on modern Debian, so the final link step fails without this patch.

The full story — including the 64-bit pointer-truncation segfault that drove the decision to go back to i386 — is in the blog post [Modernizing the MudOS Driver: From Sarge to Bookworm](https://maldorne.org/2026/04/09/modernizing-the-mudos-driver/).

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

| Version       | Used by                                                                                                                                                                   | Notes                                                                                                         |
| ------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------- |
| `v21.7b21_fr` | [Iluminado](https://maldorne.org/games/#iluminado-mud), [Ancient Kingdoms](https://maldorne.org/games/#ancient-kingdoms), [Reinos de Leyenda](https://maldorne.org/games/#reinos-de-leyenda) | In production.                                                                                                |
| `v22.2b13`    | [Ciudad Capital](https://maldorne.org/games/#ciudad-capital-v1)                                                                                                               | In production.                                                                                                |
| `v21.7`       | —                                                                                                                                                                             | Compiled and published, but not used by any of our MUDs. Should work; feedback from other games very welcome. |
| `v22.2b14`    | —                                                                                                                                                                             | Compiled and published, but not used by any of our MUDs. Should work; feedback from other games very welcome. |