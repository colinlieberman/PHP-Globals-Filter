#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_gfilter.h"

ZEND_DECLARE_MODULE_GLOBALS(gfilter)

zend_module_entry gfilter_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_GFILTER_EXTNAME,
    NULL, // extension functions
	PHP_MINIT(gfilter),
    PHP_MSHUTDOWN(gfilter),
	PHP_RINIT(gfilter),
    NULL,
    NULL,
#if ZEND_MODULE_API_NO >= 20010901
    PHP_GFILTER_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_GFILTER
ZEND_GET_MODULE(gfilter)
#endif

PHP_INI_BEGIN()
	//ini_all is pointless because these run at rinit
	STD_PHP_INI_ENTRY("gfilter.get", "1", GFILTER_INI_ACCESS, OnUpdateBool, filter_get, zend_gfilter_globals, gfilter_globals)
	STD_PHP_INI_ENTRY("gfilter.post", "1", GFILTER_INI_ACCESS, OnUpdateBool, filter_post, zend_gfilter_globals, gfilter_globals)
	STD_PHP_INI_ENTRY("gfilter.cookies", "1", GFILTER_INI_ACCESS, OnUpdateBool, filter_cookies, zend_gfilter_globals, gfilter_globals)

	STD_PHP_INI_ENTRY("gfilter.stripped", "1", GFILTER_INI_ACCESS, OnUpdateBool, filter_stripped, zend_gfilter_globals, gfilter_globals)
	STD_PHP_INI_ENTRY("gfilter.encoded", "1", GFILTER_INI_ACCESS, OnUpdateBool, filter_encoded, zend_gfilter_globals, gfilter_globals)
	STD_PHP_INI_ENTRY("gfilter.email", "1", GFILTER_INI_ACCESS, OnUpdateBool, filter_email, zend_gfilter_globals, gfilter_globals)
	STD_PHP_INI_ENTRY("gfilter.url", "1", GFILTER_INI_ACCESS, OnUpdateBool, filter_url, zend_gfilter_globals, gfilter_globals)
	STD_PHP_INI_ENTRY("gfilter.integer", "1", GFILTER_INI_ACCESS, OnUpdateBool, filter_integer, zend_gfilter_globals, gfilter_globals)
	STD_PHP_INI_ENTRY("gfilter.decimal", "1", GFILTER_INI_ACCESS, OnUpdateBool, filter_decimal, zend_gfilter_globals, gfilter_globals)
	
	// ini values must have string default, 28672 is the string flavor of the default defined in php_gfilter.h
	STD_PHP_INI_ENTRY("gfilter.flags.decimal", "28672", GFILTER_INI_ACCESS, OnUpdateLong, filter_flags_decimal, zend_gfilter_globals, gfilter_globals)
	
	STD_PHP_INI_ENTRY("gfilter.default.charset", GFILTER_DEFAULT_CHARSET, GFILTER_INI_ACCESS, OnUpdateStringUnempty, filter_default_charset, zend_gfilter_globals, gfilter_globals)
PHP_INI_END()

static void php_gfilter_init_globals(zend_gfilter_globals *gfilter_globals)
{
	gfilter_globals->filter_flags_decimal = GFILTER_DEFAULT_FLAGS_DECIMAL;
}

PHP_MINIT_FUNCTION(gfilter)
{
	char *ag_names[] = {
		"_CLEAN_GET",
		"_CLEAN_POST",
		"_CLEAN_COOKIES"
	};

	zend_bool *ag_active[3]; 
	int i;

	// set up ini vals into globals
	ZEND_INIT_MODULE_GLOBALS(gfilter, php_gfilter_init_globals, NULL);	
	REGISTER_INI_ENTRIES();

	// now that globals and inis are set, i can build ag_active
	ag_active[0] = &GFILTER_G(filter_get);
	ag_active[1] = &GFILTER_G(filter_post);
	ag_active[2] = &GFILTER_G(filter_cookies);

	// build auto globals for storing filtered data
	for ( i=0; i<3; i++ )
	{
		if ( *ag_active[i] )
		{
			zend_register_auto_global( ag_names[i], strlen(ag_names[i]), NULL TSRMLS_CC);
		}
	}

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(gfilter)
{
	UNREGISTER_INI_ENTRIES();

	return SUCCESS;

}

void reset_tmp(zval **tmp, zval **data)
{
	ALLOC_INIT_ZVAL(*tmp);
	ZVAL_STRING(*tmp, Z_STRVAL_PP(data), 1);
}

// the name param is only used when debugging
void filter_superglobal(zval **sg_ptr, zval **clean_ptr, char *name TSRMLS_DC) 
{
	zval *new_item = NULL, **data = NULL;
	HashTable *arr_hash = NULL;
	HashPosition posPtr;
	char *key = NULL, *stripped_string = NULL;
	int key_type, key_len, num_items;
	long num_key;

	// make hash for itteration
	arr_hash = Z_ARRVAL_PP(sg_ptr);
	
	/* debugging hooks -- to turn on debugging for use with cli,
	*  replace the end-comment after the folding brackets {{{  
	num_items = zend_hash_num_elements(arr_hash);
	if( num_items == 0 )
	{
		php_printf("debug - adding members to %s\n", name);
		add_assoc_string(*sg_ptr, "a", "123.7 I'm your \"worst\" nightmare, <b>foo!</b>", 0);
	} /* }}} */
	
	for( zend_hash_internal_pointer_reset_ex(arr_hash, &posPtr);
		 zend_hash_get_current_data_ex(arr_hash, (void **)&data, &posPtr) != FAILURE;
		 zend_hash_move_forward_ex(arr_hash, &posPtr) )
	{
	 	key_type = zend_hash_get_current_key_ex(arr_hash, &key, &key_len, &num_key, 0, &posPtr);

		// allocate memory and set up the new value
		ALLOC_INIT_ZVAL(new_item);
		array_init(new_item);

		stripped_string = build_filtered(&new_item, data TSRMLS_CC);
		
		switch ( key_type )
		{
			case HASH_KEY_IS_LONG:
				add_index_zval(*clean_ptr, num_key, new_item);
				break;

			case HASH_KEY_IS_STRING:
			default:
				add_assoc_zval(*clean_ptr, key, new_item);
				break;
		}

		// and set the original value to the stripped value
		// but only if it isn't null (NULL is used by array, which set the original value manually)
		if ( stripped_string != NULL )
		{
			ZVAL_STRING(*data, stripped_string, 1);
			efree(stripped_string);
		}

		// set the item's is_ref to 0 so that it becomes a copy
		new_item->is_ref = 0;	
	}
}

PHP_RINIT_FUNCTION(gfilter)
{
	zval **sg_ptr = NULL, *clean_ptr = NULL;
	int i;
	
	char *sg_names[] = {
		"_GET",
		"_POST",
		"_COOKIES"
	};

	char *clean_names[] = {
		"_CLEAN_GET",
		"_CLEAN_POST",
		"_CLEAN_COOKIES",
	};

	zend_bool *sg_active[] = 
	{
		&GFILTER_G(filter_get),
		&GFILTER_G(filter_post),
		&GFILTER_G(filter_cookies),
	};

	for( i=0; i<3; i++)
	{
		// check if we're filtering this
		if( ! *sg_active[i] ||
			zend_hash_find( &EG(symbol_table), sg_names[i], strlen(sg_names[i])+1, (void **)&sg_ptr) == FAILURE)
		{
			continue;
		}
		
		ALLOC_INIT_ZVAL(clean_ptr);
		array_init(clean_ptr);
		
		// write the stripped value into the original superglobals,
		// and populate a new zval with arrays of different filter values
		filter_superglobal(sg_ptr, &clean_ptr, sg_names[i] TSRMLS_CC);	
		
		// write the new zval into the new superglobal
		ZEND_SET_GLOBAL_VAR(clean_names[i], clean_ptr);
			
		zval_ptr_dtor( &(*sg_ptr) );
	}
	
	return SUCCESS;
}

