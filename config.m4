PHP_ARG_ENABLE(gfilter, whether to enable Globals Filter support,
[ --enable-gfilter  Enable Globals Filter  support])

if test "$PHP_GFILTER" != "no"; then
  AC_DEFINE(HAVE_GFILTER, 1, [Whether you have Global Filter])
  PHP_NEW_EXTENSION(gfilter, gfilter.c gfilter_filter.c, $ext_shared)

  PHP_ADD_EXTENSION_DEP(filter, pcre)
fi
