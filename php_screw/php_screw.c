/*
* php_screw
* (C) 2007, Kunimasa Noda/PM9.com, Inc. <http://www.pm9.com, kuni@pm9.com>
* see file LICENSE for license details
*/

/*
2015-11, bab2 ported to MS-Windows. http://enjoytools.net
*/

#include "stdafx.h"

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"

#include <sys/stat.h>

#include "php_screw.h"
#include "my_screw.h"

PHP_MINIT_FUNCTION(php_screw);
PHP_MSHUTDOWN_FUNCTION(php_screw);
PHP_MINFO_FUNCTION(php_screw);

char *pm9screw_ext_fopen(FILE *fp)
{
	struct stat stat_buf;
	char *datap, *newdatap, *result;
	int datalen, newdatalen;
	int cryptkey_len = sizeof pm9screw_mycryptkey / 2;
	int i;

#ifdef WIN32
	fstat(_fileno(fp), &stat_buf);
#else
	fstat(fileno(fp), &stat_buf);
#endif

	datalen = stat_buf.st_size - PM9SCREW_LEN;
	datap = (char*)malloc(datalen);
	memset(datap, 0, datalen);
	fread(datap, datalen, 1, fp);
	fclose(fp);

	for(i = 0; i < datalen; i++) {
		datap[i] = (char)pm9screw_mycryptkey[(datalen - i) % cryptkey_len] ^ (~(datap[i]));
	}
	newdatap = zdecode(datap, datalen, &newdatalen);

	result = (char*)malloc(newdatalen);
	memset(result, 0, newdatalen + 1);

	//printf("datalen: en-%zd/de-%zd/zdecode-%d\r\n", strlen(datap), strlen(newdatap), newdatalen);
	//printf("newdatap: %s\r\n", newdatap);
	strncpy(result, newdatap, newdatalen);

	free(datap);
	free(newdatap);

	return result;
}

zend_op_array *(*org_compile_file)(zend_file_handle *file_handle, int type TSRMLS_DC);

zend_op_array *pm9screw_compile_file(zend_file_handle *file_handle, int type TSRMLS_DC)
{
	FILE *fp;
	char buf[PM9SCREW_LEN + 1];
	char fname[32];
	char *buffer, *res;
	size_t size, res_size;

	memset(fname, 0, sizeof fname);
	if (zend_is_executing(TSRMLS_C)) {
		if (get_active_function_name(TSRMLS_C)) {
			strncpy(fname, get_active_function_name(TSRMLS_C), sizeof fname - 2);
		}
	}
	if (fname[0]) {
#ifdef WIN32
		if (_stricmp(fname, "show_source") == 0
			|| _stricmp(fname, "highlight_file") == 0) {
				return NULL;
		}
#else
		if (strcasecmp(fname, "show_source") == 0
			|| strcasecmp(fname, "highlight_file") == 0) {
				return NULL;
		}
#endif
	}

	fp = fopen(file_handle->filename, "rb");
	if (!fp) {
		return org_compile_file(file_handle, type TSRMLS_CC);
	}

	fread(buf, PM9SCREW_LEN, 1, fp);
	if (memcmp(buf, PM9SCREW, PM9SCREW_LEN) != 0) {
		//printf("Not a crypted file. - %s\r\n", file_handle->filename);

		fclose(fp);
		return org_compile_file(file_handle, type TSRMLS_CC);
	} else {
		//printf("A crypted file. - %s\r\n", file_handle->filename);

		res = pm9screw_ext_fopen(fp);
		res_size = strlen(res);

		//printf("%s\r\nlength-%zd\r\n", res, res_size);

		if (zend_stream_fixup(file_handle, &buffer, &size TSRMLS_CC) == FAILURE) {
			return NULL;
		}

		file_handle->handle.stream.mmap.buf = res;
		file_handle->handle.stream.mmap.len = res_size;
		file_handle->handle.stream.closer = NULL;

		return org_compile_file(file_handle, type TSRMLS_CC);
	}
}

zend_module_entry php_screw_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"php_screw",
	NULL,
	PHP_MINIT(php_screw),
	PHP_MSHUTDOWN(php_screw),
	NULL, NULL,
	PHP_MINFO(php_screw),
#if ZEND_MODULE_API_NO >= 20010901
	"1.5.0", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};

ZEND_GET_MODULE(php_screw);

PHP_MINFO_FUNCTION(php_screw)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "php_screw-bab2", "enabled");
	php_info_print_table_end();
}

PHP_MINIT_FUNCTION(php_screw)
{
	CG(compiler_options) |= ZEND_COMPILE_EXTENDED_INFO;

	org_compile_file = zend_compile_file;
	zend_compile_file = pm9screw_compile_file;

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(php_screw)
{
	CG(compiler_options) |= ZEND_COMPILE_EXTENDED_INFO;

	zend_compile_file = org_compile_file;

	return SUCCESS;
}
