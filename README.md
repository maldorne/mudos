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

## Current status of each branch

 - `v22.2b14` (last version of MudOS)
   - Working with Docker.
 - `v22.2b13` 
   - Working with Docker.
 - `v21.7`
   - Working with Docker.
 - `v21.7b21_fr`
   - Working with Docker.

If you want to test the images in your local machine, you can use them directly 
from Docker Hub.

https://hub.docker.com/repository/docker/neverbot/maldorne-mudos/general

### Some notes

- Versions of MudOS advanced up to `0.8` updating its minor version (using something like semantic versioning).
- `0.9` started naming versions with `0.9.x`, from `0.9.1` to `0.9.19`.
- What would have been `0.9.20` started with a new naming system, as the first `v20`. And it went from `v20.1` to `v20.26`.
- This `vXX` had some alpha and beta versions (`v21ax` and `v21bx`), you can check the `ChangeLog.alpha` and `ChangeLog.beta` files in the `v21.7` branch.
- **v21** went from `v21.1` to `v21.7`.
- I'm not really sure about the contents of the `v21.7b21_fr` branch, seems to be a fork made from the last beta version of `v21.7` (`v21.7b22` was renamed as `v21.7`). It seems to have a different parser and you can define in the `local_options` file: `#define DISCWORLD_ADD_ACTION`. It's included here, but I'm not really sure about the changes included, and the `v21.7` branch should be newer, although without some of the changes done in that fork.
- **v22** had `v22.1` and several alphas and betas of `v22.2`, which was never published. `v22.2b14` (Dec 12 2003) was the last version of MudOS.
