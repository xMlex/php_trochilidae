PHP_ARG_ENABLE(trochilidae, whether to enable trochilidae support, [ --enable-trochilidae      Enable trochilidae support])

if test "$PHP_TROCHILIDAE" != "no"; then

  AC_DEFINE(HAVE_TROCHILIDAE, 1, [Whether you have Trochilidae])
  trochilidae_sources="trochilidae/utils.c
  trochilidae/tr_network.c
  trochilidae.c"

  PHP_NEW_EXTENSION(trochilidae, $trochilidae_sources, $ext_shared,, )
fi
