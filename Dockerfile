
# #### #### #### #### #### #### #### #### #### #### #### #### ####
#  first stage, build using debian sage (debian 3.1) and gcc 3.3.5
# #### #### #### #### #### #### #### #### #### #### #### #### ####

FROM debian/eol:sarge-slim

# update sources list
COPY container/sources.list /etc/apt/sources.list
RUN apt-get -o Acquire::Check-Valid-Until=false update

# install needed tools
RUN apt-get update && apt-get -f dist-upgrade
RUN apt-get install -f -y --force-yes git gcc bison make libc6-dev

# create group and user, with uids
RUN groupadd -g 4200 mud
RUN useradd -u 4201 -g 4200 -ms /bin/bash mud
USER mud

WORKDIR /opt/mud
COPY --chown=mud:mud driver /opt/mud/driver/

WORKDIR /opt/mud/driver
RUN make clean
RUN ./build.MudOS
RUN make
RUN make install

WORKDIR /opt/mud/

# expose telnet mudos ports
EXPOSE 9997/tcp
EXPOSE 5000/tcp
