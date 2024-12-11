FROM php:8.2.25-fpm-alpine3.20

# locale & timezone
ENV TZ=Europe/Moscow
ENV LANG ru_RU.UTF-8
ENV LANGUAGE ru_RU.UTF-8
ENV LC_ALL ru_RU.UTF-8
ENV MUSL_LOCPATH /usr/share/i18n/locales/musl
RUN apk add --no-cache tzdata musl-locales-lang musl-locales icu-libs icu-data-full && \
     ln -sf /usr/share/zoneinfo/Europe/Moscow /etc/localtime \
     && ln -sf /usr/share/zoneinfo/$TZ /etc/localtime

ENV HOME=/app
WORKDIR /app

RUN apk add --no-cache build-base autoconf unzip git curl
RUN curl -Lv https://github.com/composer/composer/releases/download/2.8.3/composer.phar --output /usr/local/bin/composer && \
    chmod +x /usr/local/bin/composer && \
    ls -lah /usr/local/bin/composer && \
    php -m && php -v && composer --version

# trochilidae
ADD . /app
RUN phpize && ./configure --enable-trochilidae && make clean && make install && docker-php-ext-enable trochilidae && \
    php -m && \
    composer --version && \
    echo "OK  -exit" && exit 1