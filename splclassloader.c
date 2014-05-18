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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "Zend/zend_alloc.h"
#include "ext/standard/info.h"
#include "zend_interfaces.h"
#include "php_splclassloader.h"

#define SPLCLASSLD_NS_SEPARATOR '\\'

ZEND_DECLARE_MODULE_GLOBALS(splclassloader);

static zend_class_entry *splclassloader_ce;
zend_object_handlers splclassloader_object_handlers;

ZEND_BEGIN_ARG_INFO_EX(splclassloader_void_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(splclassloader_construct_arginfo, 0, 0, 0)
    ZEND_ARG_INFO(0, namespace_prefix)
    ZEND_ARG_INFO(0, local_library_path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(splclassloader_loadclass_arginfo, 0, 0, 1)
    ZEND_ARG_INFO(0, class_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(splclassloader_set_library_path_arginfo, 0, 0, 1)
    ZEND_ARG_INFO(0, library_path)
    ZEND_ARG_INFO(0, global)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(splclassloader_get_library_path_arginfo, 0, 0, 0)
    ZEND_ARG_INFO(0, global)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(splclassloader_get_set_file_extension_arginfo, 0, 0, 0)
    ZEND_ARG_INFO(0, extension)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(splclassloader_get_path_arginfo, 0, 0, 1)
    ZEND_ARG_INFO(0, class_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(splclassloader_register_local_ns_arginfo, 0, 0, 1)
    ZEND_ARG_INFO(0, namespace)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(splclassloader_is_local_ns_arginfo, 0, 0, 1)
    ZEND_ARG_INFO(0, namespace)
ZEND_END_ARG_INFO()

/** {{{ PHP_INI
 */
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("splclassloader.ext",             SPLCLASSLOADER_DEFAULT_EXT,  PHP_INI_ALL, OnUpdateString, ext, zend_splclassloader_globals, splclassloader_globals)
    STD_PHP_INI_ENTRY("splclassloader.global_library",  "",  PHP_INI_ALL, OnUpdateString, global_library, zend_splclassloader_globals, splclassloader_globals)
PHP_INI_END();
/* }}} */

/** {{{ PHP_GINIT_FUNCTION
*/
PHP_GINIT_FUNCTION(splclassloader)
{
    splclassloader_globals->ext = SPLCLASSLOADER_DEFAULT_EXT;
    splclassloader_globals->global_library = NULL;
}
/* }}} */

/** {{{ void splclassloader_object_dtor(void *object, zend_object_handle handle TSRMLS_DC)
 */
static void splclassloader_object_dtor(void *object, zend_object_handle handle TSRMLS_DC)
{ 
    splclassloader_object *obj = (splclassloader_object *)object;
    if (obj->ns) {
        efree(obj->ns);
        obj->ns = NULL;
    }
    if (obj->inc_path) {
        efree(obj->inc_path);
        obj->inc_path = NULL;
    }
    if (obj->file_ext) {
        efree(obj->file_ext);
        obj->file_ext = NULL;
    }

    if (obj->global_path) {
        efree(obj->global_path);
        obj->global_path = NULL;
    }
    zend_object_std_dtor(&obj->std TSRMLS_CC);
    efree(obj);
}
/* }}} */

/** {{{ zend_object_value splclassloader_create_object(zend_class_entry *class_type TSRMLS_DC)
 */
zend_object_value splclassloader_create_object(zend_class_entry *class_type TSRMLS_DC)
{
    zend_object_value retval;

    splclassloader_object *obj = (splclassloader_object *)emalloc(sizeof(splclassloader_object));
    memset(obj, 0, sizeof(splclassloader_object));

    zend_object_std_init(&obj->std, class_type TSRMLS_CC);
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 3)) || (PHP_MAJOR_VERSION > 5)
    object_properties_init(&obj->std, class_type);
#else
    zend_hash_copy(obj->std.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref, (void *)0, sizeof(zval *));
#endif
    obj->file_ext_len = strlen(SPLCLASSLOADER_G(ext));
    obj->file_ext = estrndup(SPLCLASSLOADER_G(ext), obj->file_ext_len);

    obj->global_path_len = strlen(SPLCLASSLOADER_G(global_library));
    obj->global_path = estrndup(SPLCLASSLOADER_G(global_library), obj->global_path_len);

    retval.handle = zend_objects_store_put(obj, (zend_objects_store_dtor_t)zend_objects_destroy_object, (void *)splclassloader_object_dtor, NULL TSRMLS_CC);
    retval.handlers = &splclassloader_object_handlers;

    return retval;
}
/* }}} */

/** {{{ int splclassloader_is_local_namespace(char *ns, char *class_name, int len TSRMLS_DC)
 */
int splclassloader_is_local_namespace(char *ns, char *class_name, int len TSRMLS_DC) {
    char *pos, *prefix = NULL;
    char orig_char = 0, *backup = NULL;
    uint prefix_len = 0;

    if (!ns) {
        return 0;
    }

    pos = strstr(class_name, "_");
    if (pos) {
        prefix_len  = pos - class_name;
        prefix      = class_name;
        backup = class_name + prefix_len;
        orig_char = '_';
        *backup = '\0';
    }
#ifdef SPLCLASSLOADER_HAVE_NAMESPACE
    else if ((pos = strstr(class_name, "\\"))) {
        prefix_len  = pos - class_name;
        prefix      = estrndup(class_name, prefix_len);
        orig_char = '\\';
        backup = class_name + prefix_len;
        *backup = '\0';
    }
#endif
    else {
        prefix = class_name;
        prefix_len = len;
    }

    while ((pos = strstr(ns, prefix))) {
        if ((pos == ns) && (*(pos + prefix_len) == DEFAULT_DIR_SEPARATOR || *(pos + prefix_len) == '\0')) {
            if (backup) {
                *backup = orig_char;
            }
#ifdef SPLCLASSLOADER_HAVE_NAMESPACE
            if (prefix != class_name) {
                efree(prefix);
            }
#endif
            return 1;
        } else if (*(pos - 1) == DEFAULT_DIR_SEPARATOR
                && (*(pos + prefix_len) == DEFAULT_DIR_SEPARATOR || *(pos + prefix_len) == '\0')) {
            if (backup) {
                *backup = orig_char;
            }
#ifdef SPLCLASSLOADER_HAVE_NAMESPACE
            if (prefix != class_name) {
                efree(prefix);
            }
#endif
            return 1;
        }
        ns = pos + prefix_len;
    }

    if (backup) {
        *backup = orig_char;
    }
#ifdef SPLCLASSLOADER_HAVE_NAMESPACE
    if (prefix != class_name) {
        efree(prefix);
    }
#endif

    return 0;
}
/* }}} */

/** {{{ int get_filename(splclassloader_object *obj, char *cl, int cl_len, char *filename TSRMLS_DC)
 */
static int get_filename(splclassloader_object *obj, const char *cl, int cl_len, char *filename TSRMLS_DC)
{
    char *ccl = estrndup(cl, cl_len);
    char *cns = obj->ns;
    char *cur = &ccl[cl_len-1];
    char *library_path;
    int  library_path_len;
    int  len = 0;

    /* we have a ns to compare to the qualified class name */
    if (obj->ns_len) {
        if (splclassloader_is_local_namespace(obj->ns, ccl, cl_len TSRMLS_CC)) {
            if (obj->inc_path_len) {
                library_path     = obj->inc_path;
                library_path_len = obj->inc_path_len;
            } else {
                return 0;
            }
        } else {
            library_path_len = obj->global_path_len;
            if (library_path_len) {
                library_path = obj->global_path;
            } else {
                library_path = obj->inc_path;
                library_path_len = obj->inc_path_len;
            }
        }
    } else {
        if (obj->inc_path_len) {
            library_path     = obj->inc_path;
            library_path_len = obj->inc_path_len;
        } else {
            library_path = obj->global_path;
            library_path_len = obj->global_path_len;
        }
    }
    

    /* transform '_' in the class name from right to left */
    for ( ; cur != ccl; --cur) {
        if (*cur == SPLCLASSLD_NS_SEPARATOR) {
            break;
        }
        if (*cur == '_') {
            *cur = PHP_DIR_SEPARATOR;
        }
    }

    /* tranform the remaining ns separator from right to left */
    for ( ; cur != ccl; --cur) {
        if (*cur == SPLCLASSLD_NS_SEPARATOR) {
            *cur = PHP_DIR_SEPARATOR;
        }
    }

    if (library_path_len) {
        len = library_path_len + cl_len + obj->file_ext_len + 1;
        if ((len + 1) > MAXPATHLEN) {
            return 0;
        }
        /* sprintf: we know the len */
        sprintf(filename, "%s%c%s%s", library_path, PHP_DIR_SEPARATOR, ccl, obj->file_ext);
        return len;
    }
    len = cl_len + obj->file_ext_len;

    if ((len + 1) > MAXPATHLEN) {
        return 0;
    }
    /* sprintf: we know the len */
    sprintf(filename, "%s%s", ccl, obj->file_ext);
    return len;
}
/* }}} */

/** {{{ char *namespace_dir_separator(const char *namespace, int ns_len TSRMLS_DC)
 */
static char *namespace_dir_separator(const char *namespace, int ns_len TSRMLS_DC)
{
    char *target = (char *)emalloc(ns_len + 1);
    uint i = 0 , len = 0;

    for ( ; i < ns_len ; i++) {
        if (namespace[i] == ',') {
            target[len++] = DEFAULT_DIR_SEPARATOR;
        } else if (namespace[i] != ' ') {
            target[len++] = namespace[i];
        }
    }
    target[len] = '\0';

    return target;
}
/* }}} */

/** {{{ int *splclassloader_register_namespace_single(splclassloader_object *obj, char *namespace, int ns_len TSRMLS_DC)
 */
static int splclassloader_register_namespace_single(splclassloader_object *obj, char *namespace, int ns_len TSRMLS_DC)
{
    char *target;
    uint len, flag = 0;

    if (strstr(namespace, ",")) {
        target = namespace_dir_separator(namespace, ns_len TSRMLS_CC);
        len = strlen(target);
        flag = 1;
    } else {
        target = namespace;
        len = ns_len;
    }

    /* should duplicate check ?
    if (obj->ns && strstr(obj->ns, target)) {
        if (flag) {
            efree(target);
        }
        return 0;
    }
    */

    if (obj->ns) {
        obj->ns = erealloc(obj->ns, obj->ns_len + 1 + len + 1);
        snprintf(obj->ns + obj->ns_len, len + 2, "%c%s", DEFAULT_DIR_SEPARATOR, target);
        obj->ns_len = obj->ns_len + len + 1;
    } else {
        obj->ns = estrndup(target, len);
        obj->ns_len = len;
    }

    if (flag) {
        efree(target);
    }

    return 1;
}
/* }}} */

/** {{{ proto void SplClassLoader::__construct([string $namespace [, string $include_path]])
       Constructor
*/
PHP_METHOD(SplClassLoader, __construct)
{
    char *ns = NULL;
    int  ns_len = 0;
    char *inc_path = NULL;
    int  inc_path_len = 0;
    splclassloader_object *obj;

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ss", &ns, &ns_len, &inc_path, &inc_path_len)) {
        return; /* should throw ? */
    }

    obj = (splclassloader_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

    if (ns_len) {
        char *target = namespace_dir_separator(ns, ns_len TSRMLS_CC);
        uint len = strlen(target);
        obj->ns = estrndup(target, len);
        obj->ns_len = len;
        efree(target);
    }
    if (inc_path_len) {
        obj->inc_path = estrndup(inc_path, inc_path_len);
        obj->inc_path_len = inc_path_len;
    }
}
/* }}} */


/** {{{ proto bool SplClassLoader::register()
   Installs this class loader on the SPL autoload stack
*/
PHP_METHOD(SplClassLoader, register)
{
    zval *arr;
    zval *retval = NULL;
    zval *pthis  = getThis();
    int res = 0;

    MAKE_STD_ZVAL(arr);
    array_init(arr);
    Z_ADDREF_P(pthis);

    add_next_index_zval(arr, pthis);
    add_next_index_string(arr, estrndup("loadClass", sizeof("loadClass")-1), 0);

    zend_call_method_with_1_params(NULL, NULL, NULL, "spl_autoload_register", &retval, arr);

    zval_ptr_dtor(&arr);

    if (retval) {
        res = i_zend_is_true(retval);
        zval_ptr_dtor(&retval);
    }
    RETURN_BOOL(res);
}
/* }}} */


/** {{{ proto bool SplClassLoader::unregister()
   Uninstalls this class loader from the SPL autoloader stack
*/
PHP_METHOD(SplClassLoader, unregister)
{
    zval *arr;
    zval *retval = NULL;
    zval *pthis = getThis();
    int res = 0;

    MAKE_STD_ZVAL(arr);
    array_init(arr);
    Z_ADDREF_P(pthis);

    add_next_index_zval(arr, pthis);
    add_next_index_string(arr, estrndup("loadClass", sizeof("loadClass")-1), 0);

    zend_call_method_with_1_params(NULL, NULL, NULL, "spl_autoload_unregister", &retval, arr);

    zval_ptr_dtor(&arr);

    if (retval) {
        res = i_zend_is_true(retval);
        zval_ptr_dtor(&retval);
    }
    RETURN_BOOL(res);
}
/* }}} */


/** {{{ proto bool SplClassLoader::loadClass(string $class_name)
   Loads the given class or interface
*/
PHP_METHOD(SplClassLoader, loadClass)
{
    char *cl;
    int  cl_len = 0;
    char filename[MAXPATHLEN];
    char realpath[MAXPATHLEN];
    int  len = 0, status = 0;
    splclassloader_object *obj;
    
    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s/", &cl, &cl_len) /* zval separation is required */
        || !cl_len) {
        RETURN_FALSE;
    }
   
    obj = (splclassloader_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

    len = get_filename(obj, cl, cl_len, filename TSRMLS_CC);

    if (len) {
        do {
            if (!VCWD_REALPATH(filename, realpath)) {
                status = 1;
                break;
            }

            zend_file_handle file_handle;
            zend_op_array   *op_array;
            file_handle.filename = filename;
            file_handle.opened_path = NULL;
            file_handle.free_filename = 0;
            file_handle.type = ZEND_HANDLE_FILENAME;

            op_array = zend_compile_file(&file_handle, ZEND_INCLUDE TSRMLS_CC);

            if (op_array && file_handle.handle.stream.handle) {
                int dummy = 1;

                if (!file_handle.opened_path) {
                    file_handle.opened_path = filename;
                }

                zend_hash_add(&EG(included_files), file_handle.opened_path, strlen(file_handle.opened_path)+1, (void *)&dummy, sizeof(int), NULL);
            }
            zend_destroy_file_handle(&file_handle TSRMLS_CC);

            if (op_array) {
                zval *result = NULL;

                SPLCLASSLOADER_STORE_EG_ENVIRON();

                EG(return_value_ptr_ptr) = &result;
                EG(active_op_array)      = op_array;

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)) || (PHP_MAJOR_VERSION > 5)
                if (!EG(active_symbol_table)) {
                    zend_rebuild_symbol_table(TSRMLS_C);
                }
#endif
                zend_execute(op_array TSRMLS_CC);

                destroy_op_array(op_array TSRMLS_CC);
                efree(op_array);
                if (!EG(exception)) {
                    if (EG(return_value_ptr_ptr) && *EG(return_value_ptr_ptr)) {
                        zval_ptr_dtor(EG(return_value_ptr_ptr));
                    }
                }
                SPLCLASSLOADER_RESTORE_EG_ENVIRON();
            } else {
                status = 1;
                break;
            }
        } while(0);

        if (status) {
           php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed opening script %s: %s", filename, strerror(errno));
        } else {
            char *lc_classname = zend_str_tolower_dup(cl, cl_len);

            if (zend_hash_exists(EG(class_table), lc_classname, cl_len + 1)) {
                efree(lc_classname);
                RETURN_TRUE;
            } else {
                efree(lc_classname);
                php_error_docref(NULL TSRMLS_CC, E_STRICT, "Could not find class %s in %s", cl, filename);
            }
        }
    }
    RETURN_FALSE;
}
/* }}} */


/** {{{ proto bool SplClassLoader::setIncludePath(string $include_path)
   Sets the base include path for all class files in the namespace of this class loader
*/
PHP_METHOD(SplClassLoader, setLibraryPath)
{
    char *inc_path;
    int  inc_path_len;
    splclassloader_object *obj;
    zend_bool global = 0;
    
    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|b", &inc_path, &inc_path_len, &global)) {
        return ;
    }
    
    obj = (splclassloader_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

    if (global) {
        if (obj->global_path) {
            efree(obj->global_path);
            obj->global_path = NULL;
        }
        if (inc_path_len) {
            obj->global_path = estrndup(inc_path, inc_path_len);
        }
        obj->global_path_len = inc_path_len;
    } else {
        if (obj->inc_path) {
            efree(obj->inc_path);
            obj->inc_path = NULL;
        }
        if (inc_path_len) {
            obj->inc_path = estrndup(inc_path, inc_path_len);
        }
        obj->inc_path_len = inc_path_len;
    }
    
    RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */


/** {{{ proto string SplClassLoader::getIncludePath()
   Gets the base include path for all class files in the namespace of this class loader
*/
PHP_METHOD(SplClassLoader, getLibraryPath)
{
    zend_bool global = 0;
    splclassloader_object *obj = (splclassloader_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
    
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &global) == FAILURE) {
        return;
    }

    if (global) {
        if (obj->global_path) {
            RETURN_STRINGL(obj->global_path, obj->global_path_len, 1);
        }
    } else {
        if (obj->inc_path) {
            RETURN_STRINGL(obj->inc_path, obj->inc_path_len, 1);
        }
    }

    RETURN_EMPTY_STRING();
}
/* }}} */


/** {{{ proto bool SplClassLoader::setFileExtension(string $file_extension)
   Sets the file extension of class files in the namespace of this class loader
*/
PHP_METHOD(SplClassLoader, setFileExtension)
{
    char *ext;
    int  ext_len;
    splclassloader_object *obj;
    
    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &ext, &ext_len)) {
        RETURN_FALSE;
    }
    
    obj = (splclassloader_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
    
    if (obj->file_ext) {
        efree(obj->file_ext);
        obj->file_ext = NULL;
    }
    if (ext_len) {
        obj->file_ext = estrndup(ext, ext_len);    
    }
    obj->file_ext_len = ext_len;

    RETURN_ZVAL(getThis(), 1, 0);
} /* }}} */


/** {{{ proto string SplClassLoader::getFileExtension()
   Gets the file extension of class files in the namespace of this class loader
*/
PHP_METHOD(SplClassLoader, getFileExtension)
{
    splclassloader_object *obj = (splclassloader_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
    
    if (obj->file_ext) {
        RETURN_STRINGL(obj->file_ext, obj->file_ext_len, 1);
    }
    RETURN_EMPTY_STRING();
}
/* }}} */


/** {{{ proto mixed SplClassLoader::getPath(string $class_name)
   Gets the path for the given class. Does not load anything. This is just a handy method
*/
PHP_METHOD(SplClassLoader, getPath)
{
    char *cl;
    int  cl_len = 0;
    char filename[MAXPATHLEN];
    int  len = 0;
    splclassloader_object *obj;
    
    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &cl, &cl_len)
        || !cl_len) {
        RETURN_FALSE;
    }
    
    obj = (splclassloader_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
    
    len = get_filename(obj, cl, cl_len, filename TSRMLS_CC);
    if (len) {
        RETURN_STRINGL(filename, len, 1);
    }
    RETURN_FALSE;
}
/* }}} */

/** {{{ proto mixed SplClassLoader::getLocalNamespace()
 */
PHP_METHOD(SplClassLoader, getLocalNamespace)
{
    splclassloader_object *obj;
    obj = (splclassloader_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
    if (obj->ns) {
        RETURN_STRINGL(obj->ns, obj->ns_len, 1);
    }
    RETURN_EMPTY_STRING();
}
/* }}} */

/** {{{ proto mixed SplClassLoader::registerLocalNamespace(mixed $namespace)
 */
PHP_METHOD(SplClassLoader, registerLocalNamespace)
{
    zval *namespace;
    splclassloader_object *obj;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &namespace) == FAILURE) {
        return ;
    }

    obj = (splclassloader_object *) zend_object_store_get_object(this_ptr TSRMLS_CC);
    if (IS_STRING == Z_TYPE_P(namespace) && Z_STRLEN_P(namespace)) {
        if (splclassloader_register_namespace_single(obj, Z_STRVAL_P(namespace), Z_STRLEN_P(namespace) TSRMLS_CC)) {
            RETURN_ZVAL(getThis(), 1, 0);
        }
    }

    if (IS_ARRAY == Z_TYPE_P(namespace)) {
        zval **ppval;
        HashTable *ht;

        ht = Z_ARRVAL_P(namespace);
        for (zend_hash_internal_pointer_reset(ht);
                zend_hash_has_more_elements(ht) == SUCCESS;
                zend_hash_move_forward(ht)) {
            if (zend_hash_get_current_data(ht, (void **)&ppval) == FAILURE) {
                continue;
            } else if (IS_STRING == Z_TYPE_PP(ppval)) {
                splclassloader_register_namespace_single(obj, Z_STRVAL_PP(ppval), Z_STRLEN_PP(ppval) TSRMLS_CC);
            }

        }
        RETURN_ZVAL(getThis(), 1, 0);
    }

    RETURN_FALSE;
}
/* }}} */

/** {{{ proto public SplClassLoader::isLocalNamespace(string $name)
 */
PHP_METHOD(SplClassLoader, isLocalNamespace)
{
    zval *name;
    splclassloader_object *obj;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &name) == FAILURE) {
        return ;
    }

    if (IS_STRING != Z_TYPE_P(name)) {
        RETURN_FALSE;
    }

    obj = (splclassloader_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

    RETURN_BOOL(splclassloader_is_local_namespace(obj->ns, Z_STRVAL_P(name), Z_STRLEN_P(name) TSRMLS_CC));
}
/* }}} */

/** {{{ proto public SplClassLoader::clearLocalNamespace(void)
 */
PHP_METHOD(SplClassLoader, clearLocalNamespace)
{
    splclassloader_object *obj;

    obj = (splclassloader_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    efree(obj->ns);
    obj->ns = NULL;
    obj->ns_len = 0;

    RETURN_TRUE;
}
/* }}} */

/** {{{ splclassloader_methods
*/
zend_function_entry splclassloader_methods[] = {
    PHP_ME(SplClassLoader, __construct, splclassloader_construct_arginfo, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(SplClassLoader, register, splclassloader_void_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(SplClassLoader, unregister, splclassloader_void_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(SplClassLoader, loadClass, splclassloader_loadclass_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(SplClassLoader, setLibraryPath, splclassloader_set_library_path_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(SplClassLoader, getLibraryPath, splclassloader_get_library_path_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(SplClassLoader, setFileExtension, splclassloader_get_set_file_extension_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(SplClassLoader, getFileExtension, splclassloader_void_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(SplClassLoader, getPath, splclassloader_get_path_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(SplClassLoader, getLocalNamespace, splclassloader_void_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(SplClassLoader, registerLocalNamespace, splclassloader_register_local_ns_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(SplClassLoader, isLocalNamespace, splclassloader_is_local_ns_arginfo, ZEND_ACC_PUBLIC)
    PHP_ME(SplClassLoader, clearLocalNamespace, splclassloader_void_arginfo, ZEND_ACC_PUBLIC)
    { NULL, NULL, NULL }
};
/* }}} */

/* {{{ splclassloader_functions[]
*/
zend_function_entry splclassloader_functions[] = {
    {NULL, NULL, NULL}
};
/* }}} */

/** {{{ PHP_MINIT_FUNCTION
*/
PHP_MINIT_FUNCTION(splclassloader)
{   
    REGISTER_INI_ENTRIES();

    zend_class_entry ce;
    INIT_CLASS_ENTRY(ce, "SplClassLoader", splclassloader_methods);
    splclassloader_ce = zend_register_internal_class(&ce TSRMLS_CC);
    splclassloader_ce->create_object = splclassloader_create_object;

    memcpy(&splclassloader_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    splclassloader_object_handlers.clone_obj = NULL; /* no cloning! */

    return SUCCESS;
}
/* }}} */

/** {{{ PHP_MSHUTDOWN_FUNCTION
*/
PHP_MSHUTDOWN_FUNCTION(splclassloader)
{
    UNREGISTER_INI_ENTRIES();

    return SUCCESS;
}
/* }}} */

/** {{{ PHP_RINIT_FUNCTION
*/
PHP_RINIT_FUNCTION(splclassloader)
{
    return SUCCESS;
}
/* }}} */

/** {{{ PHP_RSHUTDOWN_FUNCTION
*/
PHP_RSHUTDOWN_FUNCTION(splclassloader)
{
    return SUCCESS;
}
/* }}} */

/** {{{ PHP_MINFO_FUNCTION
*/
PHP_MINFO_FUNCTION(splclassloader)
{
    php_info_print_table_start ();
    php_info_print_table_header(2, "SplClassLoader support", "enabled");
    php_info_print_table_row   (2, "Conformance", "PSR-0");
    php_info_print_table_row   (2, "Version", SPLCLASSLOADER_VERSION);
    php_info_print_table_end   ();

    DISPLAY_INI_ENTRIES();
}
/* }}} */

/** {{{ DL support
 */
#ifdef COMPILE_DL_SPLCLASSLOADER
ZEND_GET_MODULE(splclassloader)
#endif
/* }}} */

/** {{{ module depends
 */
#if ZEND_MODULE_API_NO >= 20050922
zend_module_dep splclassloader_deps[] = {
    ZEND_MOD_REQUIRED("spl")
    {NULL, NULL, NULL}
};
#endif
/* }}} */


/** {{{ splclassloader_module_entry
*/
zend_module_entry splclassloader_module_entry = {
#if ZEND_MODULE_API_NO >= 20050922
    STANDARD_MODULE_HEADER_EX, NULL,
    splclassloader_deps,
#else
    STANDARD_MODULE_HEADER,
#endif
    "SplClassLoader",
    splclassloader_functions,
    PHP_MINIT(splclassloader),
    PHP_MSHUTDOWN(splclassloader),
    PHP_RINIT(splclassloader),
    PHP_RSHUTDOWN(splclassloader),
    PHP_MINFO(splclassloader),
    SPLCLASSLOADER_VERSION,
    PHP_MODULE_GLOBALS(splclassloader),
    PHP_GINIT(splclassloader),
    NULL,
    NULL,
    STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */
