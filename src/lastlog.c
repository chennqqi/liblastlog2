#include <assert.h>
#include "backend.h"

int main (int argc, char **argv)
{
    ll_init (LL_LASTLOG2);
    llent_t ent = {
        .host = "xxx",
        .line = "2xxx2",
        .time = 12
    };

    ll_putent (&ent);
}
