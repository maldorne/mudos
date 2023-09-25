
# #### #### #### #### #### #### #### #### #### #### #### #### ####
#  first stage, build using debian sage (debian 3.1) and gcc 3.3.5
# #### #### #### #### #### #### #### #### #### #### #### #### ####

FROM debian/eol:sarge-slim

# update sources list
COPY container/sources.list /etc/apt/sources.list
# RUN echo "deb [check-valid-until=no] http://cdn-fastly.deb.debian.org/debian jessie main" > /etc/apt/sources.list.d/jessie.list
# RUN echo "deb [check-valid-until=no] http://archive.debian.org/debian jessie-backports main" > /etc/apt/sources.list.d/jessie-backports.list
# RUN sed -i '/deb http:\/\/deb.debian.org\/debian jessie-updates main/d' /etc/apt/sources.list
RUN apt-get -o Acquire::Check-Valid-Until=false update


# install needed tools
RUN apt-get update && apt-get -f dist-upgrade
RUN apt-get install -f -y --force-yes git gcc bison make libc6-dev

# # show current OS version
# RUN cat /etc/issue
# RUN dpkg --print-architecture

# # fix needed in different architectures (jessie container will only
# # generate armhf executables)
# RUN ARCH=$(dpkg --print-architecture) && \
#   if [ "$ARCH" = "arm64" ]; then \
#   dpkg --add-architecture armhf && \
#   apt-get update && \
#   apt-get install -y libc6:armhf libstdc++6:armhf; \
#   else echo "Nothing to do"; \
#   fi

# create group and user, with uids
RUN groupadd -g 4200 mud
RUN useradd -u 4201 -g 4200 -ms /bin/bash mud
USER mud

WORKDIR /opt/mud
COPY --chown=mud:mud driver /opt/mud/driver/
# COPY --chown=mud:mud container /opt/mud/container/

WORKDIR /opt/mud/driver
RUN make clean
RUN ./build.MudOS
RUN make
RUN make install

WORKDIR /opt/mud/

# expose telnet mudos ports
EXPOSE 9997/tcp
EXPOSE 5000/tcp
