
# #### #### #### #### #### #### #### #### #### #### #### #### ####
#  build MudOS v21.7b21_fr on modern Debian 12 (bookworm) with gcc 12,
#  using compat flags for old K&R / pre-ANSI C code
# #### #### #### #### #### #### #### #### #### #### #### #### ####

# We force --platform=linux/amd64 so the resulting image is always built
# for amd64 regardless of the host architecture (Apple Silicon, Windows
# ARM, etc). The driver is then compiled as a 32-bit i386 binary inside
# this amd64 image via gcc-multilib + -m32 (see CFLAGS patch below).
FROM --platform=linux/amd64 debian:bookworm-slim

# OCI label that ghcr uses to auto-link the published package to its source
# repository, so the package shows up in the repo's sidebar on github.com.
LABEL org.opencontainers.image.source="https://github.com/maldorne/mudos"

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

# ensure shell scripts are executable even when the repo was cloned on a
# filesystem (e.g. NTFS) that does not preserve the unix execute bit
RUN chmod +x build.MudOS

# build.MudOS reassigns CFLAGS internally and ignores any ENV CFLAGS we set,
# so we patch the script with sed to PREPEND our extra flags to its own
# detection, instead of overwriting the line.
#
# Extra flags to make gcc 12 accept the legacy MudOS C source:
#   -m32                : compile as 32-bit so int and pointer have the same
#                         width, sidestepping the implicit-int pointer
#                         truncation that breaks K&R-style functions on amd64
#   -fgnu89-inline      : restore the pre-C99 GNU semantics of `inline` so
#                         INLINE functions like whashstr are emitted as
#                         external symbols and the final link succeeds
#   -fcommon            : pre-gcc-10 behaviour for tentative definitions
#   -Wno-* / -Wno-error=*:
#                         tolerate K&R prototypes, implicit int and mismatches
RUN set -eux; \
    EXTRA="-m32"; \
    EXTRA="$EXTRA -fgnu89-inline -fcommon"; \
    EXTRA="$EXTRA -Wno-implicit-function-declaration"; \
    EXTRA="$EXTRA -Wno-implicit-int"; \
    EXTRA="$EXTRA -Wno-return-type"; \
    EXTRA="$EXTRA -Wno-int-conversion"; \
    EXTRA="$EXTRA -Wno-error=implicit-function-declaration"; \
    EXTRA="$EXTRA -Wno-error=implicit-int"; \
    EXTRA="$EXTRA -Wno-error=int-conversion"; \
    sed -i "s|^CFLAGS=\"|CFLAGS=\"$EXTRA |" build.MudOS

RUN make clean || true
RUN ./build.MudOS

# Run `edit_source -configure` in isolation (via the `configure.h` make target,
# which is what generates configure.h + system_libs together) so we can patch
# the resulting `system_libs` file BEFORE the final link step. Legacy libraries
# that `check_library()` in edit_source probes for but that modern Debian 12
# amd64 + gcc-multilib i386 no longer ships:
#   -ly   : yacc runtime library (historical, not needed — MudOS provides its
#           own yyerror/main implementations, liby was only there to supply
#           defaults on old Unix systems).
#   -lnsl : legacy network services library, whose functions (gethostbyname,
#           etc.) are now part of glibc proper; bookworm still ships
#           libnsl.so on amd64 but not on i386, and it is not actually needed.
RUN make configure.h
RUN sed -i -E 's/(^| )-ly($| )/\1\2/g; s/(^| )-lnsl($| )/\1\2/g' system_libs

RUN make
RUN make install

WORKDIR /opt/mud/

EXPOSE 9997/tcp
EXPOSE 5000/tcp
