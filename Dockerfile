FROM alpine:3.20@sha256:77726ef6b57ddf65bb551896826ec38bc3e53f75cdde31354fbffb4f25238ebd AS deps
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

FROM alpine:3.20@sha256:77726ef6b57ddf65bb551896826ec38bc3e53f75cdde31354fbffb4f25238ebd AS release-dynamic
RUN apk add --no-cache libev
COPY --from=build-dynamic /src/mysql-honeypotd/mysql-honeypotd /usr/bin/mysql-honeypotd
EXPOSE 3306
CMD ["/usr/bin/mysql-honeypotd", "--foreground", "--no-syslog"]

FROM scratch AS release-static
COPY --from=build-static /src/mysql-honeypotd/mysql-honeypotd /mysql-honeypotd
EXPOSE 3306
ENTRYPOINT ["/mysql-honeypotd"]
