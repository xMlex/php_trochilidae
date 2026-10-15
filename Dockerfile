FROM php:8.5.11-fpm-alpine3.24 AS php-base

ENV TZ Europe/Moscow
ENV HOME /app
WORKDIR /app

RUN sed -i 's|https\?://dl-cdn.alpinelinux.org/alpine|https://mirror.yandex.ru/mirrors/alpine|g' /etc/apk/repositories

# PIE (PHP Installer for Extensions)
ARG PIE_VERSION=1.4.10
ARG PIE_SHA256=b88792235c8e80be568436d4cb043b49fd1869c89b64e83d23e2882ae19d70a8
RUN curl -fL "https://github.com/php/pie/releases/download/${PIE_VERSION}/pie.phar" --output /usr/local/bin/pie && \
    echo "${PIE_SHA256}  /usr/local/bin/pie" | sha256sum -c - && \
    chmod +x /usr/local/bin/pie && \
    pie --version

# Build GOST-engine for OpenSSL - NEED FOR KM!
ARG GOST_ENGINE_VERSION=3.0.3
RUN apk add --no-cache --virtual .build-deps build-base openssl-dev cmake unzip git  \
  && mkdir -p cd /usr/local/src && cd /usr/local/src \
  && git clone --depth 1 --single-branch https://github.com/gost-engine/engine.git \
  && cd engine && git fetch --tags && git checkout tags/v$GOST_ENGINE_VERSION -b v$GOST_ENGINE_VERSION \
  && git submodule init && git -c protocol.file.allow=always submodule update \
  && mkdir -p build && cd build \
  && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_INSTALL_PREFIX=/usr \
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

# pkgs for extensions
RUN apk add --no-cache openssl libzip libxml2 libxslt libldap libpq libintl c-client libffi \
    icu-libs icu-data-full \
    rabbitmq-c libmcrypt libssh2 \
    # dg
    libpng libjpeg-turbo freetype zlib libwebp \
    #librdkafka \
    # imagemagick
    libgomp imagemagick

# base extensions
RUN apk add --no-cache --virtual .build-deps $PHPIZE_DEPS \
    icu-dev libzip-dev libxml2-dev libxslt-dev openldap-dev gettext-dev libpq-dev openssl-dev libffi-dev linux-headers && \
    docker-php-ext-install -j$(nproc) intl zip soap pdo_mysql mysqli bcmath sockets pgsql pdo_pgsql xsl ldap \
    gettext calendar bz2 ffi exif ftp && \
    apk del .build-deps && \
    php -m

RUN apk add --no-cache --virtual .build-deps $PHPIZE_DEPS libtool \
    # imap
    imap-dev openssl-dev krb5-dev \
    # gd
    zlib-dev libpng-dev libjpeg-turbo-dev freetype-dev libwebp-dev \
    # imagemagick
    imagemagick-dev \
    # ampq
    rabbitmq-c-dev \
    # mcrypt
    libmcrypt-dev \
    # ssh2
    libssh2-dev \
    && \
    #pecl install imap-1.0.3 && docker-php-ext-enable imap && php -m | grep -F 'imap'  && \
    pie repository:add vcs https://github.com/xMlex/pecl-mail-imap && \
    pie install --no-cache --force php/pecl-mail-imap:1.0.3 && php -m && \
    # ssh2 - pecl install ssh2-1.5.0 && docker-php-ext-enable ssh2 && \
    pie repository:add vcs https://github.com/php/pecl-networking-ssh2 && \
    pie install --no-cache --force php/pecl-networking-ssh2:1.5.0 && php -m && \
    # gd
    docker-php-ext-configure gd --with-freetype --with-jpeg --with-webp && docker-php-ext-install -j$(nproc) gd &&  \
    # imagick
    pie install --no-cache imagick/imagick:3.8.1 && php -v && php -m && \
    # amqp
    pie install --no-cache php-amqp/php-amqp:2.2.0 && \
    # mcrypt
    pie install --no-cache pecl/mcrypt:1.0.9 && php -m | grep -F 'mcrypt' && \
    # igbinary
    pie install --no-cache igbinary/igbinary:3.2.17RC1 && cd /app && rm -Rf /ext/mcrypt && \
    # redis
    pie install --no-cache phpredis/phpredis:6.3.0 && \
    rm -rf /app/.pie /app/.composer && \
    # remove dev pkg
    apk del .build-deps

# COPY ext/ /ext

RUN php -v && php -m

# debug extension
#RUN apk add --no-cache --virtual .build-deps $PHPIZE_DEPS valgrind
#COPY ./ext/trochilidae /app
#RUN cd /app &&  ls -lah && \
#    phpize &&  echo "phpize ok" && \
#    ./configure --enable-trochilidae &&   echo "configure ok" && \
#    make install && docker-php-ext-enable trochilidae && rm -Rf /app/* && echo "ok"
#RUN valgrind --tool=memcheck php -m
#RUN echo "OK"; exit 1

# locale & timezone
ENV LANG ru_RU.UTF-8
ENV LANGUAGE ru_RU.UTF-8
ENV LC_ALL ru_RU.UTF-8
ENV MUSL_LOCPATH /usr/share/i18n/locales/musl
RUN apk add --no-cache tzdata musl-locales-lang musl-locales && \
     ln -sf /usr/share/zoneinfo/Europe/Moscow /etc/localtime

# ** CRON ** Latest releases available at https://github.com/aptible/supercronic/releases
ENV SUPERCRONIC_URL "https://github.com/aptible/supercronic/releases/download/v0.2.47/supercronic-linux-amd64"
ENV SUPERCRONIC_SHA1SUM "712d2ece75da6f6e530192a151488578153e4e96"
ENV SUPERCRONIC "supercronic-linux-amd64"

RUN curl -m 15 -fsSLO "$SUPERCRONIC_URL" \
 && echo "${SUPERCRONIC_SHA1SUM}  ${SUPERCRONIC}" | sha1sum -c - \
 && chmod +x "$SUPERCRONIC" \
 && mv "$SUPERCRONIC" "/usr/local/bin/${SUPERCRONIC}" \
 && ln -s "/usr/local/bin/${SUPERCRONIC}" /usr/bin/supercronic
# *** CRON END ***

# Pkg`s - ghostscript (FS.KM), exiftool (FS.AUTO)
RUN apk add --no-cache \
    ghostscript perl-image-exiftool exiftool \
    unzip git bash

# composer
RUN curl -vL -m 30 https://github.com/composer/composer/releases/download/2.10.2/composer.phar --output /usr/local/bin/composer && \
    chmod +x /usr/local/bin/composer && \
    php -v && COMPOSER_ALLOW_SUPERUSER=1 composer --version

# cgi-fcgi for healthcheck
ADD healthcheck.sh /healthcheck.sh
RUN apk add --no-cache fcgi && chmod +x /healthcheck.sh

# CA Certificates
ADD ca-certificates/ /usr/local/share/ca-certificates/
# CryptoPro TLS CA
RUN curl -vk -m 5 https://tlsca.cryptopro.ru/tlscaroot.p7b -o /usr/local/share/ca-certificates/cryptopro-tlscaroot.p7b && \
    curl -vk -m 5 https://tlsca.cryptopro.ru/tlsca.p7b -o /usr/local/share/ca-certificates/cryptopro-tlsca.p7b && \
    openssl pkcs7 -inform DER -print_certs \
      -in /usr/local/share/ca-certificates/cryptopro-tlscaroot.p7b \
      -out /usr/local/share/ca-certificates/cryptopro-tlscaroot.pem && \
    openssl pkcs7 -inform DER -print_certs \
      -in /usr/local/share/ca-certificates/cryptopro-tlsca.p7b \
      -out /usr/local/share/ca-certificates/cryptopro-tlsca.pem
RUN update-ca-certificates
ENV USER_AGENT_HEADER "User-Agent: validate Russian Trusted CA connection and GOST ciphers tester. Abuse report to novikov.maksim@lab-company.ru"
# test GOST
RUN curl -v --fail -m 5 -H "$USER_AGENT_HEADER" --ciphers LEGACY-GOST2012-GOST8912-GOST8912 https://gost.cryptopro.ru/ -o /dev/null
# test Russian Trusted - The Ministry of Digital Development and Communications
#RUN curl -v --fail -m 5 -H "$USER_AGENT_HEADER" https://www.nspk.ru/certificates -o /dev/null

# trochilidae extension
ENV TR_BRANCH "main"
ENV TR_VERSION "66311069a6f5853780e9673495b374d4e1f5e513"
RUN echo "$TR_BRANCH" && apk add --no-cache --virtual .build-deps-tr $PHPIZE_DEPS && \
    mkdir /tr && cd /tr && curl https://codeload.github.com/xMlex/php_trochilidae/zip/refs/heads/$TR_BRANCH --output main.zip && unzip main.zip && \
    cd php_trochilidae-$TR_BRANCH && phpize && ./configure --enable-trochilidae && make install && docker-php-ext-enable trochilidae && rm -Rf /tr && php -v && php -m && \
    apk del .build-deps-tr