
# #### #### #### #### #### #### #### #### #### #### #### #### ####
#  experimental: build MudOS v21.7b21_fr on modern Debian 12 (bookworm)
#  with gcc 12, using compat flags for old K&R / pre-ANSI C code
# #### #### #### #### #### #### #### #### #### #### #### #### ####

FROM --platform=linux/amd64 debian:bookworm-slim

# install build deps. git+ssh are also added so the resulting image
# can later be reused as a runner that clones the game code at startup.
RUN dpkg --add-architecture i386 \
 && apt-get update \
 && apt-get install -y --no-install-recommends \
      build-essential bison make gcc gcc-multilib libc6-dev libc6-dev-i386 \
      libcrypt-dev:i386 \
      git openssh-client ca-certificates \
 && rm -rf /var/lib/apt/lists/*

# create group and user, same uids as the legacy image
RUN groupadd -g 4200 mud \
 && useradd  -u 4201 -g 4200 -ms /bin/bash mud
USER mud

WORKDIR /opt/mud
COPY --chown=mud:mud driver /opt/mud/driver/

WORKDIR /opt/mud/driver

# build.MudOS reassigns CFLAGS internally and ignores any ENV CFLAGS we set,
# so we patch the script with sed to APPEND our extra flags after its own
# detection, instead of overwriting the line.
#
# Extra flags to make gcc 12 accept legacy MudOS C source:
#   -fgnu89-inline    : restore the pre-C99 GNU semantics of the `inline`
#                       keyword (otherwise `INLINE` functions like whashstr
#                       are not emitted as external symbols and the link fails)
#   -fcommon          : pre-gcc-10 behaviour for tentative definitions
#   -Wno-* / -Wno-error=*:
#                       tolerate K&R prototypes, implicit int, mismatches, etc.
RUN sed -i 's|^CFLAGS="\$OSFLAGS|CFLAGS="-m32 -fgnu89-inline -fcommon -Wno-implicit-function-declaration -Wno-implicit-int -Wno-return-type -Wno-int-conversion -Wno-error=implicit-function-declaration -Wno-error=implicit-int -Wno-error=int-conversion $OSFLAGS|' build.MudOS

RUN make clean || true
RUN ./build.MudOS
RUN make
RUN make install

WORKDIR /opt/mud/

EXPOSE 9997/tcp
EXPOSE 5000/tcp
