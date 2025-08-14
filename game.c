#include <stdio.h>
#include "utils.h"
#include "platform.h"

init_function(init)
{
    fprintf(stderr, "init\n");
}

update_function(update)
{
    fprintf(stderr, "update\n");
}

render_function(render)
{
    fprintf(stderr, "render\n");
}
