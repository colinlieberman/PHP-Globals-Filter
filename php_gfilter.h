#ifndef PHP_GFILTER_H
#define PHP_GFILTER_H 1

#ifdef ZTS
#include "TSRM.h"
#endif

#include "php.h"
#include "php_ini.h"

ZEND_BEGIN_MODULE_GLOBALS(gfilter)
	zend_bool filter_get;
	zend_bool filter_post;
	zend_bool filter_cookies;
	zend_bool filter_env;

	zend_bool filter_stripped;
	zend_bool filter_encoded;
	zend_bool filter_email;
	zend_bool filter_url;
	zend_bool filter_integer;
	zend_bool filter_decimal;

	long filter_flags_decimal;

	char *filter_default_charset;

ZEND_END_MODULE_GLOBALS(gfilter)

#ifdef ZTS
#define GFILTER_G(v) TSRMG(gfilter_globals_id, zend_gfilter_globals *, v)
#else
#define GFILTER_G(v) (gfilter_globals.v)
#endif

#define PHP_GFILTER_VERSION "1.0"
#define PHP_GFILTER_EXTNAME "gfilter"

#define GFILTER_OUTPUT_STRIPED	"stripped"
#define GFILTER_OUTPUT_ENCODED	"encoded"
#define GFILTER_OUTPUT_URL		"url"
#define GFILTER_OUTPUT_EMAIL	"email"
#define GFILTER_OUTPUT_INTEGER	"integer"
#define GFILTER_OUTPUT_DECIMAL	"decimal"

// constants for filters, copying from /ext/filter/filter_private.
#define FILTER_FLAG_NONE                    0x0000

#define FILTER_REQUIRE_ARRAY			0x1000000
#define FILTER_REQUIRE_SCALAR			0x2000000

#define FILTER_FORCE_ARRAY			0x4000000
#define FILTER_NULL_ON_FAILURE			0x8000000

#define FILTER_FLAG_ALLOW_OCTAL             0x0001
#define FILTER_FLAG_ALLOW_HEX               0x0002

#define FILTER_FLAG_STRIP_LOW               0x0004
#define FILTER_FLAG_STRIP_HIGH              0x0008
#define FILTER_FLAG_ENCODE_LOW              0x0010
#define FILTER_FLAG_ENCODE_HIGH             0x0020
#define FILTER_FLAG_ENCODE_AMP              0x0040
#define FILTER_FLAG_NO_ENCODE_QUOTES        0x0080
#define FILTER_FLAG_EMPTY_STRING_NULL       0x0100

#define FILTER_FLAG_ALLOW_FRACTION          0x1000
#define FILTER_FLAG_ALLOW_THOUSAND          0x2000
#define FILTER_FLAG_ALLOW_SCIENTIFIC        0x4000

#define FILTER_FLAG_SCHEME_REQUIRED         0x010000
#define FILTER_FLAG_HOST_REQUIRED           0x020000
#define FILTER_FLAG_PATH_REQUIRED           0x040000
#define FILTER_FLAG_QUERY_REQUIRED          0x080000

#define FILTER_FLAG_IPV4                    0x100000
#define FILTER_FLAG_IPV6                    0x200000
#define FILTER_FLAG_NO_RES_RANGE            0x400000
#define FILTER_FLAG_NO_PRIV_RANGE           0x800000

#define GFILTER_INI_ACCESS PHP_INI_SYSTEM | PHP_INI_PERDIR
#define GFILTER_DEFAULT_FLAGS_DECIMAL FILTER_FLAG_ALLOW_FRACTION | FILTER_FLAG_ALLOW_THOUSAND | FILTER_FLAG_ALLOW_SCIENTIFIC
#define GFILTER_DEFAULT_CHARSET "UTF-8"

PHP_MINIT_FUNCTION(gfilter);
PHP_MSHUTDOWN_FUNCTION(gfilter);
PHP_RINIT_FUNCTION(gfilter);

void reset_tmp(zval **tmp, zval **data);
void filter_superglobal(zval **sg_ptr, zval **clean_ptr, char *name TSRMLS_DC);
char *build_filtered(zval **new_item, zval **data TSRMLS_DC);

extern zend_module_entry gfilter_module_entry;
#define phpext_gfilter_ptr &gfilter_module_entry

#endif

