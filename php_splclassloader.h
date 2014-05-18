/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2009 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: metagoto <runpac314 gmail com>                              |
   +----------------------------------------------------------------------+
 */
 
#ifndef PHP_SPLCLASSLOADER_H
#define PHP_SPLCLASSLOADER_H

extern zend_module_entry splclassloader_module_entry;
#define phpext_splclassloader_ptr &splclassloader_module_entry

#ifdef ZTS
#include "TSRM.h"
#endif

#ifdef ZTS
#define SPLCLASSLOADER_G(v) TSRMG(splclassloader_globals_id, zend_splclassloader_globals *, v)
#else
#define SPLCLASSLOADER_G(v) (splclassloader_globals.v)
#endif

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)) || (PHP_MAJOR_VERSION > 5)
#define SPLCLASSLOADER_HAVE_NAMESPACE
#endif


#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)) || (PHP_MAJOR_VERSION > 5)
#define SPLCLASSLOADER_STORE_EG_ENVIRON() \
    { \
        zval ** __old_return_value_pp   = EG(return_value_ptr_ptr); \
        zend_op ** __old_opline_ptr     = EG(opline_ptr); \
        zend_op_array * __old_op_array  = EG(active_op_array);

#define SPLCLASSLOADER_RESTORE_EG_ENVIRON() \
        EG(return_value_ptr_ptr) = __old_return_value_pp;\
        EG(opline_ptr)           = __old_opline_ptr; \
        EG(active_op_array)      = __old_op_array; \
    }

#else

#define SPLCLASSLOADER_STORE_EG_ENVIRON() \
    { \
        zval ** __old_return_value_pp          = EG(return_value_ptr_ptr); \
        zend_op ** __old_opline_ptr            = EG(opline_ptr); \
        zend_op_array * __old_op_array         = EG(active_op_array); \
        zend_function_state * __old_func_state = EG(function_state_ptr);

#define SPLCLASSLOADER_RESTORE_EG_ENVIRON() \
        EG(return_value_ptr_ptr) = __old_return_value_pp;\
        EG(opline_ptr)           = __old_opline_ptr; \
        EG(active_op_array)      = __old_op_array; \
        EG(function_state_ptr)   = __old_func_state; \
    }

#endif

#define SPLCLASSLOADER_VERSION          "1.0"
#define SPLCLASSLOADER_DEFAULT_EXT      ".php"

typedef struct _splclassloader_object {
    zend_object std;
    char *ns;
    int  ns_len;
    char *inc_path;
    int  inc_path_len;
    char *file_ext;
    int  file_ext_len;
    char *global_path;
    int  global_path_len;
} splclassloader_object;

ZEND_BEGIN_MODULE_GLOBALS(splclassloader)
    char        *ext;
    char        *global_library;
ZEND_END_MODULE_GLOBALS(splclassloader)

PHP_MINIT_FUNCTION(splclassloader);
PHP_MSHUTDOWN_FUNCTION(splclassloader);
PHP_RINIT_FUNCTION(splclassloader);
PHP_RSHUTDOWN_FUNCTION(splclassloader);
PHP_MINFO_FUNCTION(splclassloader);

#endif /* PHP_SPLCLASSLOADER_H */
