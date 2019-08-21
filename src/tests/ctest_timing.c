/* Icecast
 *
 * This program is distributed under the GNU General Public License, version 2.
 * A copy of this license is included with this source.
 *
 * Copyright 2018,      Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>,
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "ctest_lib.h"

#include <igloo/timing.h>

static void test_get_time(void)
{
    uint64_t a, b;

    a = igloo_timing_get_time();
    b = igloo_timing_get_time();
    ctest_test("Time increases", a <= b);
}

static void test_sleep(void)
{
    uint64_t a, b;

    a = igloo_timing_get_time();
    igloo_timing_sleep(128);
    b = igloo_timing_get_time();
    ctest_test("Slept", (a < b) && ((a + 256) > b));
}

int main (void)
{
    ctest_init();

    test_get_time();
    test_sleep();

    ctest_fin();

    return 0;
}
