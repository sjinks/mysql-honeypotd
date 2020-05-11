FROM alpine:latest AS build
COPY . /src/mysql-honeypotd
WORKDIR /src/mysql-honeypotd
RUN apk add --no-cache gcc make libc-dev libev-dev && make

FROM alpine:latest
RUN apk add --no-cache libev
COPY --from=build /src/mysql-honeypotd/mysql-honeypotd /usr/bin/mysql-honeypotd
EXPOSE 3306
CMD ["/usr/bin/mysql-honeypotd", "--foreground"]
