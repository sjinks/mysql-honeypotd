FROM alpine:latest AS build
COPY . /src/mysql-honeypotd
WORKDIR /src/mysql-honeypotd
ENV CFLAGS="-Os -g0 -Wall -Wextra -fvisibility=hidden -fno-strict-aliasing -Wno-unused-parameter" CPPFLAGS="-D_DEFAULT_SOURCE"
RUN apk add --no-cache gcc make libc-dev libev-dev && make

FROM alpine:latest
RUN apk add --no-cache libev
COPY --from=build /src/mysql-honeypotd/mysql-honeypotd /usr/bin/mysql-honeypotd
EXPOSE 3306
CMD ["/usr/bin/mysql-honeypotd", "--foreground", "--no-syslog"]
