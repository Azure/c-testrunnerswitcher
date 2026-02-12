#include "testrunnerswitcher.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/timed_test_suite.h"

BEGIN_TEST_SUITE(multi_arg_int)
TIMED_TEST_SUITE_INITIALIZE(suite_init, TIMED_TEST_DEFAULT_TIMEOUT_MS, setup_gballoc)
{
}
TIMED_TEST_SUITE_CLEANUP(suite_cleanup, cleanup_gballoc)
{
}
TEST_FUNCTION(test1)
{
    ASSERT_IS_TRUE(1);
}
END_TEST_SUITE(multi_arg_int)
