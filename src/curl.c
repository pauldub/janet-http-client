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

	if (CURL_INITIALIZED == 1) {
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

static Janet easy_handle_get(void *, Janet);

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

	cfun_curl_init(0, NULL);

	CURL *easy_handle = curl_easy_init();
	if (NULL == easy_handle) {
		janet_panic("could not create CURL easy_handle");
	}

	easy_handle_wrapper_t *wrapper = (easy_handle_wrapper_t *)janet_abstract(&easy_handle_type, sizeof(*wrapper));

	wrapper->easy_handle = easy_handle;

	Janet val = janet_wrap_abstract(wrapper);
	janet_gcroot(val);

	return val;
}

static CURLoption easy_handle_parse_opt(const char *key) {
	if (strcmp(key, "CURLOPT_URL") == 0) {
		return CURLOPT_URL;
	} else if (strcmp(key, "CURLOPT_PORT") == 0) {
		return CURLOPT_PORT;
	} else if (strcmp(key, "CURLOPT_READFUNCTION") == 0) {
		return CURLOPT_READFUNCTION;
	} else if (strcmp(key, "CURLOPT_READDATA") == 0) {
		return CURLOPT_READDATA;
	} else if (strcmp(key, "CURLOPT_WRITEFUNCTION") == 0) {
		return CURLOPT_WRITEFUNCTION;
	} else if (strcmp(key, "CURLOPT_WRITEDATA") == 0) {
		return CURLOPT_WRITEDATA;
	} else if (strcmp(key, "CURLOPT_HEADERFUNCTION") == 0) {
		return CURLOPT_HEADERFUNCTION;
	} else if (strcmp(key, "CURLOPT_HEADERDATA") == 0) {
		return CURLOPT_HEADERDATA;
	}

	janet_panicf("unknown option, got %s", key);
}

static Janet cfun_easy_handle_reset(int32_t argc, Janet *argv) {
	janet_fixarity(argc, 1);

	easy_handle_wrapper_t *wrapper = janet_getabstract(argv, 0, &easy_handle_type);
	if (wrapper->easy_handle == NULL) {
		janet_panic("easy_handle has been destroyed and cannot be used");
	}

	curl_easy_reset(wrapper->easy_handle);

	return janet_wrap_abstract(wrapper);
}

static size_t curl_setopt_wrap_janet_function(
		char *ptr, size_t size, size_t nitems, void *userdata
) {
	JanetFunction *func = (JanetFunction *)userdata;

	const uint8_t *buf = (const uint8_t *)ptr;

	Janet argv[3] = {
		janet_wrap_string(janet_string(buf, size * nitems)),
		janet_wrap_number(size),
		janet_wrap_number(nitems),
	};

	Janet res = janet_call(func, 3, argv);
	if (!janet_checktype(res, JANET_NUMBER)) {
		janet_panic_type(res, janet_type(res), JANET_NUMBER); 
	}

	return (size_t)janet_unwrap_number(res);
}

static Janet cfun_easy_handle_setopt(int32_t argc, Janet *argv) {
	janet_fixarity(argc, 3);

	easy_handle_wrapper_t *wrapper = janet_getabstract(argv, 0, &easy_handle_type);
	if (wrapper->easy_handle == NULL) {
		janet_panic("easy_handle has been destroyed and cannot be used");
	}

	const char *opt_key = (const char *)janet_getkeyword(argv, 1);
	Janet opt_value = argv[2];
	
	CURLcode res = CURLE_OK;
	CURLoption opt = easy_handle_parse_opt(opt_key);

	switch(janet_type(opt_value)) {
	case JANET_NIL:
		res = curl_easy_setopt(wrapper->easy_handle, opt, NULL);
		break;
	case JANET_NUMBER:
		res = curl_easy_setopt(wrapper->easy_handle, opt, (long)janet_unwrap_number(opt_value));
		break;
	case JANET_STRING:
		res = curl_easy_setopt(wrapper->easy_handle, opt, janet_unwrap_string(opt_value));
		break;
	case JANET_KEYWORD:
		res = curl_easy_setopt(wrapper->easy_handle, opt, janet_unwrap_keyword(opt_value));
		break;
	case JANET_POINTER:
		res = curl_easy_setopt(wrapper->easy_handle, opt, janet_unwrap_pointer(opt_value));
		break;
	case JANET_FUNCTION:
		res = curl_easy_setopt(wrapper->easy_handle, opt, curl_setopt_wrap_janet_function);
		break;
	default:
		janet_panic("unsupported value type");
	}


	if (CURLE_OK != res) {
		janet_panicf("failed to set option %s: %s", opt_key, curl_easy_strerror(res));
	}

	return janet_wrap_abstract(wrapper);
}

static Janet cfun_easy_handle_setopt_function(int32_t argc, Janet *argv) {
	janet_fixarity(argc, 4);

	easy_handle_wrapper_t *wrapper = janet_getabstract(argv, 0, &easy_handle_type);
	if (wrapper->easy_handle == NULL) {
		janet_panic("easy_handle has been destroyed and cannot be used");
	}

	JanetFunction *function = janet_getfunction(argv, 3);

	Janet function_argv[3] = {
		argv[0], argv[1], argv[3],
	};

	Janet data_argv[3] = {
		argv[0], argv[2], janet_wrap_pointer(function),
	};

	cfun_easy_handle_setopt(3, function_argv);
	cfun_easy_handle_setopt(3, data_argv);

	return janet_wrap_abstract(wrapper);
}

static Janet cfun_easy_handle_perform(int32_t argc, Janet *argv) {
	janet_fixarity(argc, 1);

	easy_handle_wrapper_t *wrapper = janet_getabstract(argv, 0, &easy_handle_type);
	if (wrapper->easy_handle == NULL) {
		janet_panic("easy_handle has been destroyed and cannot be used");
	}

	CURLcode res = curl_easy_perform(wrapper->easy_handle);
	if (CURLE_OK != res) {
		janet_panicf("failed to perform request: %s", curl_easy_strerror(res));
	}

	return janet_wrap_abstract(wrapper);
}

static JanetMethod easy_handle_methods[] = {
	{"reset", cfun_easy_handle_reset},
	{"set-opt", cfun_easy_handle_setopt},
	{"set-opt-function", cfun_easy_handle_setopt_function},
	{"perform", cfun_easy_handle_perform},
	{NULL, NULL}
};

static Janet easy_handle_get(void* p, Janet key) {
	(void) p;

	if (!janet_checktype(key, JANET_KEYWORD)) {
		janet_panic_type(key, janet_type(key), JANET_NUMBER); 
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
	{"set-opt-function", cfun_easy_handle_setopt_function,
		"(curl/set-opt-function handle key data-key value)\n\n"
		"Special case of `set-opt` to set function callbacks"
	},
	{"perform", cfun_easy_handle_perform,
		"(curl/perform handle)\n\n"
		"Perform a blocking file transfer."
	},
	{"reset", cfun_easy_handle_reset,
		"(curl/reset handle)\n\n"
		"Resets a easy handle."
	},
	{NULL, NULL, NULL},
};

JANET_MODULE_ENTRY(JanetTable *env) {
	janet_cfuns(env, "curl", cfuns);

	// Set a default curl-easy-handle
	janet_setdyn("curl-easy-handle", cfun_make_easy_handle(0, NULL));
	janet_dobytes(env, src___curl_embed, src___curl_embed_size, "[curl/curl.janet]", NULL);
}
