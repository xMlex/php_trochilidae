PHP_ARG_ENABLE(trochilidae, whether to enable trochilidae support,
[ --enable-trochilidae      Enable trochilidae support])

if test "$PHP_TROCHILIDAE" != "no"; then
  PHP_NEW_EXTENSION(trochilidae, tr_network.c, trochilidae.c, $ext_shared)
fi
