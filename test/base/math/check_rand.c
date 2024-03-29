#include "m_config.h"
#include <stdlib.h> /* EXIT_SUCCESS, EXIT_FAILURE, srand, rand */
#include <check.h>

#include <mstdlib/mstdlib.h>

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static M_uint64 rand_10_firsts[] = {
	14010483323465047854ULL,
	1848569026263983419ULL,
	1204107001967172447ULL,
	12708102326677774558ULL,
	12422399073999995804ULL,
	4018829258401158403ULL,
	11227831433803194036ULL,
	2584988939181427326ULL,
	11074516100571463907ULL,
	7987106447217900757ULL
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

START_TEST(check_rand_10)
{
	M_rand_t *state;
	M_uint64 *seconds;
	size_t    i;
	size_t    len;

	len = sizeof(rand_10_firsts)/sizeof(*rand_10_firsts);

	state   = M_rand_create(10);
	seconds = M_malloc(len*sizeof(*seconds));

	for (i=0; i<len; i++) {
		seconds[i] = M_rand(state);
	}

	for (i=0; i<len; i++) {
		ck_assert_msg(seconds[i] == rand_10_firsts[i], "test %zu got %llu expect %llu", i, seconds[i], rand_10_firsts[i]);
	}

	M_free(seconds);
	M_rand_destroy(state);
}
END_TEST

START_TEST(check_rand_rand)
{
	M_rand_t *state;
	M_uint64 *seconds;
	size_t    i;
	size_t    len;

	len = sizeof(rand_10_firsts)/sizeof(*rand_10_firsts);

	/* 0 will use an internally generated seed. */
	state   = M_rand_create(0);
	seconds = M_malloc(len*sizeof(*seconds));

	for (i=0; i<len; i++) {
		seconds[i] = M_rand(state);
	}

	/* It's possible this wll fail if for some reason the internal seed was generated
 	 * as the same vale as the 10_firsts seed. However, it's unlikely the internal seed
	 * generator will generate a seen that small. If it does it should be considered a bug. */
	for (i=0; i<len; i++) {
		ck_assert_msg(seconds[i] != rand_10_firsts[i], "test %zu got %llu expect anything else", i, seconds[i]);
	}

	M_free(seconds);
	M_rand_destroy(state);
}
END_TEST

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static Suite *rand_suite(void)
{
	Suite *suite;
	TCase *tc_rand_10;
	TCase *tc_rand_rand;

	suite = suite_create("rand");

	tc_rand_10 = tcase_create("rand_10");
	tcase_add_test(tc_rand_10, check_rand_10);
	suite_add_tcase(suite, tc_rand_10);

	tc_rand_rand = tcase_create("rand_rand");
	tcase_add_test(tc_rand_rand, check_rand_rand);
	suite_add_tcase(suite, tc_rand_rand);

	return suite;
}

int main(int argc, char **argv)
{
	SRunner *sr;
	int      nf;

	(void)argc;
	(void)argv;

	sr = srunner_create(rand_suite());
	if (getenv("CK_LOG_FILE_NAME")==NULL) srunner_set_log(sr, "check_rand.log");

	srunner_run_all(sr, CK_NORMAL);
	nf = srunner_ntests_failed(sr);
	srunner_free(sr);

	return nf == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
