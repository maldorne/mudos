
# #### #### #### #### #### #### #### #### #### #### #### #### ####
#  first stage, build using debian jessie (debian 7) and gcc 4.9.2
# #### #### #### #### #### #### #### #### #### #### #### #### ####

FROM debian/eol:jessie-slim AS debian-7

# install needed tools
RUN apt-get update && apt-get install -y --force-yes git gcc bison make libbsd-dev

# create group and user, with uids
RUN groupadd -g 4200 mud
RUN useradd -u 4201 -g 4200 -ms /bin/bash mud
USER mud

WORKDIR /opt/mud
COPY --chown=mud:mud . /opt/mud/

WORKDIR /opt/mud/driver
RUN make clean
RUN ./build.MudOS
RUN make
RUN make install

# #### #### #### #### #### #### #### #### #### #### #### #### ####
#  second stage, use the binaries in debian bullseye (debian 11)
# #### #### #### #### #### #### #### #### #### #### #### #### ####

FROM debian:11.6-slim AS debian-11

RUN apt-get update && apt-get upgrade -y --force-yes

RUN apt-get install -y --force-yes libbsd-dev

# temporary, will be removed from final image
RUN apt-get install -y --force-yes procps telnet

# show current OS version
RUN cat /etc/issue
RUN dpkg --print-architecture

# fix needed in different architectures (jessie container will only
# generate armhf executables)
RUN ARCH=$(dpkg --print-architecture) && \
  if [ "$ARCH" = "arm64" ]; then \
  dpkg --add-architecture armhf && \
  apt-get update && \
  apt-get install -y libc6:armhf libstdc++6:armhf; \
  else echo "Nothing to do"; \
  fi

# recreate group and user, with same uids as previous stage
RUN groupadd -g 4200 mud
RUN useradd -u 4201 -g 4200 -ms /bin/bash mud
USER mud

# copy contents from previous stage
COPY --chown=mud:mud --from=debian-7 /opt/mud/bin /opt/mud/bin
COPY --chown=mud:mud --from=debian-7 /opt/mud/driver /opt/mud/driver

WORKDIR /opt/mud/
