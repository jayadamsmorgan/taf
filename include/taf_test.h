#ifndef TAF_TEST_H
#define TAF_TEST_H

#include "test_case.h"

void taf_test_mark_current_test_failed();

void taf_mark_test_failed();

test_case_t *taf_test_get_current_test();

int taf_test();

#endif // TAF_TEST_H
