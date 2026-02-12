#include "testrunnerswitcher.h"
#include "c_pal/gballoc_hl.h"

BEGIN_TEST_SUITE(exempted_module_int)
TEST_SUITE_INITIALIZE(suite_init) // no-timed-test-suite
{
}
TEST_SUITE_CLEANUP(suite_cleanup)
{
}
TEST_FUNCTION(test1)
{
    ASSERT_IS_TRUE(1);
}
END_TEST_SUITE(exempted_module_int)
