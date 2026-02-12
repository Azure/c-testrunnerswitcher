#include "testrunnerswitcher.h"
#include "c_pal/gballoc_hl.h"

BEGIN_TEST_SUITE(multi_arg_int)
TEST_SUITE_INITIALIZE(suite_init, setup_gballoc)
{
}
TEST_SUITE_CLEANUP(suite_cleanup, cleanup_gballoc)
{
}
TEST_FUNCTION(test1)
{
    ASSERT_IS_TRUE(1);
}
END_TEST_SUITE(multi_arg_int)
