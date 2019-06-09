#include <janet.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include <curl/curl.h>


/* embedded source */

extern const unsigned char *src___curl_embed;
extern size_t src___curl_embed_size;

static int CURL_INITIALIZED = 0;

static Janet cfun_curl_init(int32_t argc, Janet *argv) {
	(void) argv;

	janet_fixarity(argc, 0);

	if(CURL_INITIALIZED == 1) {
		return janet_wrap_nil();
	}

	curl_global_init(CURL_GLOBAL_ALL);
	CURL_INITIALIZED = 1;

	return janet_wrap_nil();
}

typedef struct easy_handle_wrapper {
	CURL *easy_handle;
} easy_handle_wrapper_t;

static int easy_handle_gc(void *x, size_t s) {
	(void) s;
	easy_handle_wrapper_t *w = (easy_handle_wrapper_t *)x;

	if (NULL != w->easy_handle) {
		curl_easy_cleanup(w->easy_handle);
	}

	return 0;
}

static Janet easy_handle_get(void*, Janet);
static const JanetAbstractType easy_handle_type = {
	"curl/easy-handle",
	easy_handle_gc, // gc
	NULL, // gcmark
	easy_handle_get, // get
	NULL, // put
	NULL, // marshal
	NULL, // unmarshal
	NULL  // tostring
};

static Janet cfun_make_easy_handle(int32_t argc, Janet *argv) {
	(void) argv;

	janet_fixarity(argc, 0);
	// TODO: allow a fiber as argv[0]
	// janet_arity(argc, 0, 1);

	cfun_curl_init(0, NULL);

	CURL *easy_handle = curl_easy_init();
	if(NULL == easy_handle) {
		janet_panic("could not create CURL easy_handle");
	}

	easy_handle_wrapper_t *wrapper = (easy_handle_wrapper_t*)janet_abstract(&easy_handle_type, sizeof(*wrapper));

	wrapper->easy_handle = easy_handle;

	Janet val = janet_wrap_abstract(wrapper);
	janet_gcroot(val);

	return val;
}

static CURLoption easy_handle_parse_opt(const char *key) {
	if(strcmp(key, "url") == 0) {
		return CURLOPT_URL;
	}

	janet_panicf("unknown option, got %s", key);
}

static Janet cfun_easy_handle_reset(int32_t argc, Janet *argv) {
	janet_fixarity(argc, 1);

	easy_handle_wrapper_t *wrapper = janet_getabstract(argv, 0, &easy_handle_type);
	if(wrapper->easy_handle == NULL) {
		janet_panic("easy_handle has been destroyed and cannot be used");
	}

	curl_easy_reset(wrapper->easy_handle);

	return janet_wrap_nil();
}
static Janet cfun_easy_handle_setopt(int32_t argc, Janet *argv) {
	janet_fixarity(argc, 3);

	easy_handle_wrapper_t *wrapper = janet_getabstract(argv, 0, &easy_handle_type);
	if(wrapper->easy_handle == NULL) {
		janet_panic("easy_handle has been destroyed and cannot be used");
	}

	const char *opt_key = (const char *)janet_getkeyword(argv, 1);
	const char *opt_value = janet_getcstring(argv, 2);

	CURLoption opt = easy_handle_parse_opt(opt_key);

	CURLcode res = curl_easy_setopt(wrapper->easy_handle, opt, opt_value);
	if(CURLE_OK != res) {
		janet_panicf("failed to set option: %s", curl_easy_strerror(res));
	}

	return janet_wrap_nil();
}

static Janet cfun_easy_handle_perform(int32_t argc, Janet *argv) {
	janet_fixarity(argc, 1);

	easy_handle_wrapper_t *wrapper = janet_getabstract(argv, 0, &easy_handle_type);
	if(wrapper->easy_handle == NULL) {
		janet_panic("easy_handle has been destroyed and cannot be used");
	}

	CURLcode res = curl_easy_perform(wrapper->easy_handle);
	if(CURLE_OK != res) {
		janet_panicf("failed to perform request: %s", curl_easy_strerror(res));
	}

	return janet_wrap_nil();
}

static JanetMethod easy_handle_methods[] = {
	{"reset", cfun_easy_handle_reset},
	{"set-opt", cfun_easy_handle_setopt},
	{"perform", cfun_easy_handle_perform},
	{NULL, NULL}
};

static Janet easy_handle_get(void* p, Janet key) {
	(void) p;

	if(!janet_checktype(key, JANET_KEYWORD)) {
		janet_panicf("unexpected keyword, got %v", key);
	}

	return janet_getmethod(janet_unwrap_keyword(key), easy_handle_methods);
}

/* Module Entry */

static const JanetReg cfuns[] = {
	{"init", cfun_curl_init,
		"(curl/init)\n\n"
		"Initializes CURL, must be called once for the whole program."
	},
	{"make-easy-handle", cfun_make_easy_handle,
		"(curl/make-easy-handle)\n\n"
		"Returns a new CURL easy handle.\n"
	},
	{"set-opt", cfun_easy_handle_setopt,
		"(curl/set-opt handle key value)\n\n"
		"Sets CURL option `key` to `value` on `handle`."
	},
	{"perform", cfun_easy_handle_setopt,
		"(curl/perform handle)\n\n"
		"Perform a blocking file transfer."
	},
	{NULL, NULL, NULL},
};

JANET_MODULE_ENTRY(JanetTable *env) {
	janet_cfuns(env, "curl", cfuns);

	// Set a default curl-easy-handle
	janet_setdyn("curl-easy-handle", cfun_make_easy_handle(0, NULL));
	janet_dobytes(env, src___curl_embed, src___curl_embed_size, "[curl/curl.janet]", NULL);
}
