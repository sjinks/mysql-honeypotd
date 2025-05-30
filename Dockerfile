FROM alpine:3.22@sha256:8a1f59ffb675680d47db6337b49d22281a139e9d709335b492be023728e11715 AS deps
RUN apk add --no-cache gcc libc-dev libev-dev make
WORKDIR /src/mysql-honeypotd
COPY . /src/mysql-honeypotd

FROM deps AS build-dynamic
ENV \
    CFLAGS="-Os -g0 -Wall -Wextra -Wno-unknown-pragmas -fvisibility=hidden -Wno-unused-parameter" \
    CPPFLAGS="-D_DEFAULT_SOURCE"
RUN make && strip mysql-honeypotd

FROM deps AS build-static
ENV \
    CFLAGS="-Os -g0 -Wall -Wextra -Wno-unknown-pragmas -fvisibility=hidden -Wno-unused-parameter" \
    CPPFLAGS="-D_DEFAULT_SOURCE -DMINIMALISTIC_BUILD" \
    LDFLAGS="-static"
RUN make && strip mysql-honeypotd

FROM alpine:3.22@sha256:8a1f59ffb675680d47db6337b49d22281a139e9d709335b492be023728e11715 AS release-dynamic
RUN apk add --no-cache libev
COPY --from=build-dynamic /src/mysql-honeypotd/mysql-honeypotd /usr/bin/mysql-honeypotd
EXPOSE 3306
CMD ["/usr/bin/mysql-honeypotd", "--foreground", "--no-syslog"]

FROM scratch AS release-static
COPY --from=build-static /src/mysql-honeypotd/mysql-honeypotd /mysql-honeypotd
EXPOSE 3306
ENTRYPOINT ["/mysql-honeypotd"]
