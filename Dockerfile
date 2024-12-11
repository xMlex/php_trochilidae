FROM php:8.2.25-fpm-alpine3.20 as php-base

ENV TZ=Europe/Moscow
ENV HOME=/app
WORKDIR /app

# Build GOST-engine for OpenSSL - NEED FOR KM!
ARG GOST_ENGINE_VERSION=3.0.3
RUN apk add  --no-cache --virtual .build-deps build-base openssl-dev cmake unzip git \
  && mkdir -p cd /usr/local/src && cd /usr/local/src \
  && git clone --depth 1 --single-branch https://github.com/gost-engine/engine.git \
  && cd engine && git fetch --tags && git checkout tags/v$GOST_ENGINE_VERSION -b v$GOST_ENGINE_VERSION \
  && git submodule init && git -c protocol.file.allow=always submodule update \
  && mkdir -p build && cd build \
  && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr \
  && cmake --build . --config Release \
  && cmake --build . --target install --config Release \
  && mv bin/gost.so /usr/lib/engines-3/ \
  && mv bin/gost12sum /bin/ \
  && mv bin/gostsum /bin/ \
  && mv bin/sign /bin/ \
  && mv bin/*.so /usr/lib/ \
  && apk del --no-cache .build-deps && rm -Rf /usr/local/src

# Enable engine GOST
ADD openssl.cnf /openssl.cnf
RUN sed -i 's/openssl_conf = openssl_init/openssl_conf = openssl_gost/g' /etc/ssl/openssl.cnf \
    && sed -i "s#\[default_sect\]# #g" /etc/ssl/openssl.cnf \
    && cat /openssl.cnf >> /etc/ssl/openssl.cnf \
    && cat /etc/ssl/openssl.cnf  \
    && openssl engine gost -c \
    && openssl ciphers | tr ':' '\n' | grep GOST
#    && cat /etc/ssl/openssl.cnf \
#    && sed -i "s#\[default_sect\]#\[default_sect\]\nMinProtocol\=TLSv1.2\nCipherString = DEFAULT:@SECLEVEL=1#g" /etc/ssl/openssl.cnf \

# imap
RUN apk add --update --no-cache \
    openssl \
    imap-dev \
    openssl-dev \
    krb5-dev && \
    (docker-php-ext-configure imap --with-kerberos --with-imap-ssl) && \
    (docker-php-ext-install imap > /dev/null) && \
    php -m | grep -F 'imap' && \
    apk del --no-cache imap-dev openssl-dev krb5-dev

# base packages
RUN apk add --no-cache \
    icu-dev libzip libxml2 libxslt libldap libpq libintl c-client libffi \
    libzip-dev libxml2-dev libxslt-dev openldap-dev gettext-dev libpq-dev openssl-dev libffi-dev linux-headers && \
    docker-php-ext-install -j$(nproc) opcache intl zip soap pdo_mysql mysqli bcmath sockets pgsql pdo_pgsql xsl ldap \
    gettext calendar bz2 ffi exif ftp && \
    apk del --no-cache libzip-dev libxml2-dev libpq-dev libxslt-dev openldap-dev gettext-dev openssl-dev libffi-dev linux-headers && \
    php -m

# ampq mcrypt ssh2 redis
RUN apk add --no-cache build-base autoconf \
    rabbitmq-c rabbitmq-c-dev \
    libmcrypt libmcrypt-dev  \
    libssh2 libssh2-dev && \
    pecl install amqp && \
    docker-php-ext-enable amqp && \
    pecl install igbinary && \
    docker-php-ext-enable igbinary && \
    pecl install mcrypt-1.0.7 && \
    docker-php-ext-enable mcrypt && \
    pecl install ssh2-1.4.1 && \
    docker-php-ext-enable ssh2 && \
    pecl install redis-6.1.0 && \
    docker-php-ext-enable redis && \
    apk del --no-cache build-base autoconf rabbitmq-c-dev libmcrypt-dev libssh2-dev

# dg
RUN apk add --no-cache \
    libpng libjpeg-turbo freetype zlib libwebp \
    zlib-dev libpng-dev libjpeg-turbo-dev freetype-dev libwebp-dev && \
    docker-php-ext-configure gd --with-freetype --with-jpeg --with-webp && \
    docker-php-ext-install -j$(nproc) gd && \
    apk del --no-cache zlib-dev libpng-dev libjpeg-turbo-dev freetype-dev libwebp-dev && php -r 'print_r(gd_info());'

# locale & timezone
ENV LANG ru_RU.UTF-8
ENV LANGUAGE ru_RU.UTF-8
ENV LC_ALL ru_RU.UTF-8
ENV MUSL_LOCPATH /usr/share/i18n/locales/musl
RUN apk add --no-cache tzdata musl-locales-lang musl-locales icu-libs icu-data-full && \
     ln -sf /usr/share/zoneinfo/Europe/Moscow /etc/localtime

# ** CRON ** Latest releases available at https://github.com/aptible/supercronic/releases
ENV SUPERCRONIC_URL=https://github.com/aptible/supercronic/releases/download/v0.2.30/supercronic-linux-amd64 \
    SUPERCRONIC=supercronic-linux-amd64 \
    SUPERCRONIC_SHA1SUM=9f27ad28c5c57cd133325b2a66bba69ba2235799

RUN curl -fsSLO "$SUPERCRONIC_URL" \
 && echo "${SUPERCRONIC_SHA1SUM}  ${SUPERCRONIC}" | sha1sum -c - \
 && chmod +x "$SUPERCRONIC" \
 && mv "$SUPERCRONIC" "/usr/local/bin/${SUPERCRONIC}" \
 && ln -s "/usr/local/bin/${SUPERCRONIC}" /usr/bin/supercronic

# PKG`s - ghostscript (FS.KM), exiftool (FS.AUTO)
RUN apk add --no-cache \
     ghostscript perl-image-exiftool exiftool

# additional extensions
#RUN docker-php-ext-install -j$(nproc) ftp && \
#  docker-php-ext-enable ftp

RUN apk add --no-cache --virtual .build-deps $PHPIZE_DEPS imagemagick-dev \
&& apk add --no-cache libgomp imagemagick \
&& pecl install imagick \
&& docker-php-ext-enable imagick \
&& apk del .build-deps \
&& php -m

# composer
RUN apk add --no-cache unzip git && \
    curl -Lv https://github.com/composer/composer/releases/download/2.8.3/composer.phar --output /usr/local/bin/composer && \
    chmod +x /usr/local/bin/composer && \
    ls -lah /usr/local/bin/composer && \
    php -m && php -v && composer --version

# trochilidae
RUN apk add --no-cache build-base autoconf unzip git curl
ADD . /app
# php /app/test.php && \
RUN phpize && ./configure --enable-trochilidae && make clean && make install && docker-php-ext-enable trochilidae && \
    php -m && \
    composer install --prefer-dist && \
    echo "OK  -exit" && \
    exit 1

