/* START TEST INIT */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>  /* INT_MIN, INT_MAX */
#include "duktape.h"

#define  TEST_SAFE_CALL(func)  do { \
	duk_ret_t _rc; \
	printf("*** %s (duk_safe_call)\n", #func); \
	fflush(stdout); \
	_rc = duk_safe_call(ctx, (func), NULL, 0 /*nargs*/, 1 /*nrets*/); \
	printf("==> rc=%d, result='%s'\n", (int) _rc, duk_safe_to_string(ctx, -1)); \
	fflush(stdout); \
	duk_pop(ctx); \
} while (0)

#define  TEST_PCALL(func)  do { \
	duk_ret_t _rc; \
	 printf("*** %s (duk_pcall)\n", #func); \
	fflush(stdout); \
	duk_push_c_function(ctx, (func), 0); \
	_rc = duk_pcall(ctx, 0); \
	printf("==> rc=%d, result='%s'\n", (int) _rc, duk_safe_to_string(ctx, -1)); \
	fflush(stdout); \
	duk_pop(ctx); \
} while (0)

#line 1  /* END TEST INIT */

/*
 *  Duktape 1.4.0 (and unpatched previous versions) had a bug in refcount
 *  finalizer handling: when the rescued object was queued back into the
 *  heap_allocated list, the "prev" pointer of the previous list head (which
 *  becomes the "next" of the inserted object) was not updated.
 *
 *  The "prev" pointer will be NULL which, while incorrect, is not a memory
 *  safety issue by itself.  However, when the object later participates in
 *  heap linked list operations, the NULL pointer may cause an unrelated
 *  object's "next" pointer to be left unupdated.  That "next" pointer may
 *  then be a non-NULL dangling pointer pointing to an object already freed.
 *
 *  This issue often goes unnoticed (before this test was added no previous
 *  finalizer test triggered any issues, even with valgrind enabled), but can
 *  cause odd side effects much later after the finalizer rescue.  For example,
 *  a previously freed object can be added back into the heap which then causes
 *  a read/write-after-free detected by valgrind or ASAN.
 *
 *  This test case is a result of debugging an issue which occurred with
 *  Frida Duktape integration.  https://github.com/oleavr was able to reduce
 *  the original issue into an independent repro case ("dukrecycle") which was
 *  then adapted into this test case.  The test case may seem a bit odd, but
 *  several (seemingly unrelated) heap linked list operations are needed to
 *  trigger the concrete memory safety issues caused by the bug.
 *
 *  The test case doesn't work without refcounts because it expects the
 *  finalizer to run immediately and make the rescued object available
 *  for reuse.
 */

/*===
*** test_badgers (duk_safe_call)
first call
* pushing badger1
> inspect_badger
* pushing badger2
* putting back badger2
< inspect_badger
* putting back badger1

second call
* pushing badger1
> inspect_badger
* pushing badger2
* putting back badger2
< inspect_badger
* putting back badger1


done
final top: 0
==> rc=0, result='undefined'
* putting back badger1
* putting back badger2
===*/

static void *create_badger(duk_context *ctx, const char *id);
static void push_badger(duk_context *ctx);
static duk_ret_t finalize_badger(duk_context *ctx);
static duk_ret_t inspect_badger(duk_context *ctx);

static void *cached_badger_1 = NULL;
static void *cached_badger_2 = NULL;

static duk_ret_t test_badgers(duk_context *ctx) {
	cached_badger_1 = create_badger(ctx, "badger1");
	cached_badger_2 = create_badger(ctx, "badger2");

	printf("first call\n");

	duk_push_c_function(ctx, inspect_badger, 1);
	duk_push_object(ctx);
	push_badger(ctx);
	duk_put_prop_string(ctx, -2, "animal");

	duk_pcall(ctx, 1);
	duk_pop(ctx);

	printf("\nsecond call\n");

	duk_push_c_function(ctx, inspect_badger, 1);

	duk_push_object(ctx);
	push_badger(ctx);
	duk_put_prop_string(ctx, -2, "animal");

	duk_pcall(ctx, 1);
	duk_pop(ctx);

	printf("\ndone\n");
	printf("final top: %ld\n", (long) duk_get_top(ctx));
	return 0;
}

static void *create_badger(duk_context *ctx, const char *id) {
	void *result;

	duk_push_object(ctx);

	duk_push_string(ctx, id);
	duk_put_prop_string(ctx, -2, "id");

	duk_push_c_function(ctx, finalize_badger, 2);
	duk_set_finalizer(ctx, -2);

	result = duk_get_heapptr(ctx, -1);

	duk_push_global_stash(ctx);
	duk_dup(ctx, -2);
	duk_put_prop_string(ctx, -2, id);

	duk_pop_2(ctx);

	return result;
}

static void push_badger(duk_context *ctx) {
	void **cached_badger;

	cached_badger = &cached_badger_1;
	if (*cached_badger != NULL) {
		printf("* pushing badger1\n");
	} else {
		cached_badger = &cached_badger_2;
		printf("* pushing badger2\n");
	}

	if (*cached_badger != NULL) {
		const char *id;

		duk_push_heapptr(ctx, *cached_badger);
		*cached_badger = NULL;

		duk_get_prop_string(ctx, -1, "id");
		id = duk_get_string(ctx, -1);

		duk_push_global_stash(ctx);
		duk_del_prop_string(ctx, -1, id);

		duk_pop_2(ctx);

		return;
	}

	printf("never here\n");
}

static duk_ret_t finalize_badger(duk_context *ctx) {
	const char *id;

	duk_get_prop_string(ctx, 0, "id");
	id = duk_get_string(ctx, -1);

	if (strcmp(id, "badger1") == 0) {
		printf("* putting back badger1\n");
		cached_badger_1 = duk_get_heapptr(ctx, 0);
	} else if (strcmp(id, "badger2") == 0) {
		printf("* putting back badger2\n");
		cached_badger_2 = duk_get_heapptr(ctx, 0);
	} else {
		printf("never here\n");
	}

	duk_push_global_stash(ctx);
	duk_dup(ctx, 0);
	duk_put_prop_string(ctx, -2, id);

	duk_pop_2(ctx);

	return 0;
}

static duk_ret_t inspect_badger(duk_context *ctx) {
	printf("> inspect_badger\n");

	push_badger(ctx);
	duk_pop(ctx);

	printf("< inspect_badger\n");

	return 0;
}

void test(duk_context *ctx) {
	TEST_SAFE_CALL(test_badgers);
}

/* START TEST MAIN */
static duk_ret_t runtests_print_alert_helper(duk_context *ctx, FILE *fh) {
	duk_idx_t nargs;
	const duk_uint8_t *buf;
	duk_size_t sz_buf;

	nargs = duk_get_top(ctx);
	if (nargs == 1 && duk_is_buffer(ctx, 0)) {
		buf = (const duk_uint8_t *) duk_get_buffer(ctx, 0, &sz_buf);
		fwrite((const void *) buf, 1, (size_t) sz_buf, fh);
	} else {
		duk_push_string(ctx, " ");
		duk_insert(ctx, 0);
		duk_join(ctx, nargs);
		fprintf(fh, "%s\n", duk_to_string(ctx, -1));
	}
	fflush(fh);
	return 0;
}
static duk_ret_t runtests_print(duk_context *ctx) {
	return runtests_print_alert_helper(ctx, stdout);
}
static duk_ret_t runtests_alert(duk_context *ctx) {
	return runtests_print_alert_helper(ctx, stderr);
}

int main(int argc, char *argv[]) {
	duk_context *ctx = NULL;
	(void) argc; (void) argv;

	ctx = duk_create_heap_default();
	if (!ctx) {
		fprintf(stderr, "cannot allocate heap for testcase\n");
		exit(1);
	}

	/* Minimal print() replacement; removed in Duktape 2.x. */
	duk_push_global_object(ctx);
	duk_push_string(ctx, "print");
	duk_push_c_function(ctx, runtests_print, DUK_VARARGS);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_WRITABLE | DUK_DEFPROP_SET_CONFIGURABLE);
	duk_push_string(ctx, "alert");
	duk_push_c_function(ctx, runtests_alert, DUK_VARARGS);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_WRITABLE | DUK_DEFPROP_SET_CONFIGURABLE);
	duk_pop(ctx);

	test(ctx);

	duk_destroy_heap(ctx);
}
/* END TEST MAIN */
