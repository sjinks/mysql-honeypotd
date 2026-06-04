FROM --platform=${BUILDPLATFORM} tonistiigi/xx:latest@sha256:c64defb9ed5a91eacb37f96ccc3d4cd72521c4bd18d5442905b95e2226b0e707 AS xx

FROM --platform=${BUILDPLATFORM} alpine:3.23@sha256:5b10f432ef3da1b8d4c7eb6c487f2f5a8f096bc91145e68878dd4a5019afde11 AS build
COPY --from=xx / /
ARG TARGETPLATFORM

RUN \
    apk add --no-cache clang llvm lld make cmake && \
    xx-apk add --no-cache gcc musl-dev libev-dev

WORKDIR /src/mysql-honeypotd
COPY . .

ENV \
    CFLAGS="-Os -g0 -Wall -Wextra -Wno-unknown-pragmas -fvisibility=hidden -Wno-unused-parameter" \
    CPPFLAGS="-D_DEFAULT_SOURCE -DMINIMALISTIC_BUILD" \
    LDFLAGS="-static" \
    CC="xx-clang"

RUN \
    set -x && \
    export ARCHITECTURE=$(xx-info alpine-arch) && \
    export XX_CC_PREFER_LINKER=ld && \
    export SYSROOT=$(xx-info sysroot) && \
    export HOSTSPEC=$(xx-info triple) && \
    xx-clang --setup-target-triple && \
    make && $(xx-info triple)-strip mysql-honeypotd

FROM scratch AS release-static
COPY --from=build /src/mysql-honeypotd/mysql-honeypotd /mysql-honeypotd
EXPOSE 3306
ENTRYPOINT ["/mysql-honeypotd"]
