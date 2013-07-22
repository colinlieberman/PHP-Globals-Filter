#include "php_gfilter.h"
#include "filter/php_filter.h"

ZEND_DECLARE_MODULE_GLOBALS(gfilter)

// returns stripped string for feeding back into super global
char * build_filtered(zval **new_item, zval **data TSRMLS_DC)
{
	zval *tmp_new_item = NULL, *options_array = NULL;
	
	char *charset = GFILTER_G(filter_default_charset), *out;

	// hash stuff for itterating over arrays
	HashTable *arr_hash = NULL;
	HashPosition pos;
	zval **adata = NULL;
	int i = 0;

	// this is http data, so either string or array (if like 'ab[]')
	switch( Z_TYPE_PP(data) )
	{
		/**
		 * some peole are completely insane and like to do dumb hacks like
		 * <input name="myvar[]"> or worse <input name="myvar[opts1][]"> and so on
		 *
		 * this recursively supports that, even though it will likely be very slow
		 */
		case IS_ARRAY:
			arr_hash = Z_ARRVAL_PP(data);
			
			for( zend_hash_internal_pointer_reset_ex(arr_hash, &pos);
				zend_hash_get_current_data_ex(arr_hash, (void **)&adata, &pos) != FAILURE;
				zend_hash_move_forward_ex(arr_hash, &pos) )
			{
				ALLOC_INIT_ZVAL(tmp_new_item);
				array_init(tmp_new_item);
				
				if ( Z_TYPE_PP(adata) == IS_ARRAY )
				{
					char *key = NULL;
					long num_key;
					int key_len, key_type;
					
					key_type = zend_hash_get_current_key_ex(arr_hash, &key, &key_len, &num_key, 0, &pos);

					out = build_filtered(&tmp_new_item, adata TSRMLS_CC);

					switch ( key_type )
					{
						case HASH_KEY_IS_LONG:
							add_index_zval(*new_item, num_key, tmp_new_item);
							break;

						case HASH_KEY_IS_STRING:
						default:
							add_assoc_zval(*new_item, key, tmp_new_item);
							break;
					}
				}
				else
				{
					// out is the stripped value, and that gets written over the original
					out = build_filtered(&tmp_new_item, adata TSRMLS_CC);
					ZVAL_STRING(*adata, out, 1);

					add_index_zval(*new_item, i++, tmp_new_item);
				}

				// make the new item a copy, not a reference
				tmp_new_item->is_ref = 0;
			}

			// lastly, set out to null so the caller knows not to overwrite original (we did that manually for the array inputs))
			out = NULL;
				
			break;

		/* here's the bulk of the filtration */
		case IS_STRING:
		default:
			//html encoded
			if ( GFILTER_G(filter_encoded) )
			{
				reset_tmp(&tmp_new_item, data);
				php_filter_special_chars(tmp_new_item, FILTER_FLAG_NONE, options_array, charset TSRMLS_CC);
				add_assoc_string(*new_item, GFILTER_OUTPUT_ENCODED, Z_STRVAL_P(tmp_new_item),1);
				zval_ptr_dtor(&tmp_new_item);
			}

			//url
			if ( GFILTER_G(filter_url) )
			{	
				reset_tmp(&tmp_new_item, data);
				php_filter_validate_url(tmp_new_item, FILTER_FLAG_NONE, options_array, charset TSRMLS_CC);
				// if validation failed, it's set to a bool and is false, otherwise, the value remains intact
				if( Z_TYPE_P(tmp_new_item) == IS_BOOL)
				{
					add_assoc_string(*new_item, GFILTER_OUTPUT_URL, "", 1);
				}
				else
				{
					add_assoc_string(*new_item, GFILTER_OUTPUT_URL, Z_STRVAL_P(tmp_new_item),1);
				}
				zval_ptr_dtor(&tmp_new_item);
			}

			//email
			if ( GFILTER_G(filter_email) )
			{
				// known bug - http://bonsai.php.net/bug.php?id=43402 filter validate email not rfc2822 compliant
				reset_tmp(&tmp_new_item, data);
				php_filter_validate_email(tmp_new_item, FILTER_FLAG_NONE, options_array, charset TSRMLS_CC);
				// if validation failed, it's set to a bool and is false, otherwise, the value remains intact
				if( Z_TYPE_P(tmp_new_item) == IS_BOOL)
				{
					add_assoc_string(*new_item, GFILTER_OUTPUT_EMAIL, "", 1);
				}
				else
				{
					add_assoc_string(*new_item, GFILTER_OUTPUT_EMAIL, Z_STRVAL_P(tmp_new_item),1);
				}
				zval_ptr_dtor(&tmp_new_item);
			}

			//integer value
			if ( GFILTER_G(filter_integer) )
			{
				reset_tmp(&tmp_new_item, data);
				php_filter_number_int(tmp_new_item, FILTER_FLAG_NONE, options_array, charset TSRMLS_CC);
				add_assoc_string(*new_item, GFILTER_OUTPUT_INTEGER, Z_STRVAL_P(tmp_new_item),1);
				zval_ptr_dtor(&tmp_new_item);
			}

			//decimal value
			if ( GFILTER_G(filter_decimal) )
			{
				reset_tmp(&tmp_new_item, data);
				php_filter_number_float(tmp_new_item, GFILTER_G(filter_flags_decimal), options_array, charset TSRMLS_CC);
				add_assoc_string(*new_item, GFILTER_OUTPUT_DECIMAL, Z_STRVAL_P(tmp_new_item),1);
				zval_ptr_dtor(&tmp_new_item);
			}
			
			// stripped
			if ( GFILTER_G(filter_stripped) )
			{
				reset_tmp(&tmp_new_item, data);
				php_filter_string(tmp_new_item, FILTER_FLAG_NONE, options_array, charset TSRMLS_CC);
				add_assoc_string(*new_item, GFILTER_OUTPUT_STRIPED, Z_STRVAL_P(tmp_new_item),1);
				// set the stripped val for out
				out = estrdup(Z_STRVAL_P(tmp_new_item));
				zval_ptr_dtor(&tmp_new_item);
			}
			else
			{
				out = estrdup(Z_STRVAL_PP(data));
			}
			
			break;
	}

	// return the stripped value, or NULL if the input was an array
	return out;
}
