#include "include/myproject/swim_fd.h"

int main()
{
    SWIM swim = {"127.0.0.1", 2, {}};
    swim.run();
}