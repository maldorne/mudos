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

This branch of the project contains the version `v22.2b13`, ready to use.

The `master` branch is empty, try any other branch to see different versions.

## How to use this version with Docker

- Build the container image (from project base directory)

  `docker build --no-cache -t neverbot/maldorne-mudos:v22.2b13 -f Dockerfile .`

- Run the container and take a look inside using a terminal

  `docker run --rm -ti neverbot/maldorne-mudos:v22.2b13 /bin/bash`

  Inside the container, in `/opt/mud`, you can find the directories `driver` 
  (with the source code of MudOS) and `bin`, with the two binaries needed
  to use MudOS (`addr_server` and `driver`).

- Publish the container in Docker Hub

  `docker push neverbot/maldorne-mudos:v22.2b13`
