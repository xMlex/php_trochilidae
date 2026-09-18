FROM php:8.5.8-fpm-alpine3.24

RUN apk add --no-cache \
      $PHPIZE_DEPS \
      bash \
      fcgi \
      linux-headers \
      ripgrep \
      gdb \
      binutils \
      valgrind
ENV CFLAGS="-O0 -g3 -fno-omit-frame-pointer"
ENV CPPFLAGS="-O0 -g3 -fno-omit-frame-pointer"
ENV LDFLAGS=""

WORKDIR /work
COPY . /work

RUN phpize \
    && ./configure --enable-trochilidae \
    && make -j"$(nproc)"

CMD ["bash", "-lc", "/work/tests/run_fpm_debug.sh --inside-container"]
