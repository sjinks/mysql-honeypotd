FROM alpine:3.13@sha256:469b6e04ee185740477efa44ed5bdd64a07bbdd6c7e5f5d169e540889597b911 AS deps
RUN apk add --no-cache gcc make libc-dev libev-dev
WORKDIR /src/mysql-honeypotd
COPY . /src/mysql-honeypotd

FROM deps AS build-dynamic
ENV \
    CFLAGS="-Os -g0 -Wall -Wextra -Wno-unknown-pragmas -fvisibility=hidden -fno-strict-aliasing -Wno-unused-parameter" \
    CPPFLAGS="-D_DEFAULT_SOURCE"
RUN make && strip mysql-honeypotd

FROM deps AS build-static
ENV \
    CFLAGS="-Os -g0 -Wall -Wextra -Wno-unknown-pragmas -fvisibility=hidden -fno-strict-aliasing -Wno-unused-parameter" \
    CPPFLAGS="-D_DEFAULT_SOURCE -DMINIMALISTIC_BUILD" \
    LDFLAGS="-static"
RUN make && strip mysql-honeypotd

FROM alpine:3.13@sha256:469b6e04ee185740477efa44ed5bdd64a07bbdd6c7e5f5d169e540889597b911 AS release-dynamic
RUN apk add --no-cache libev
COPY --from=build-dynamic /src/mysql-honeypotd/mysql-honeypotd /usr/bin/mysql-honeypotd
EXPOSE 3306
CMD ["/usr/bin/mysql-honeypotd", "--foreground", "--no-syslog"]

FROM scratch AS release-static
COPY --from=build-static /src/mysql-honeypotd/mysql-honeypotd /mysql-honeypotd
EXPOSE 3306
ENTRYPOINT ["/mysql-honeypotd"]
