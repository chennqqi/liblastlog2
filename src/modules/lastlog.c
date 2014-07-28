#include <assert.h>

#ifdef HAVE_THREADS
#include <pthread.h>

static pthread_mutex_t _init_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

typedef enum {
    A,
    B
} retcode_t;

/* Prototypes */
static retcode_t putlstlogent();
static retcode_t getlstlogent();
static retcode_t dellstlogent();

/* Jump table */
static struct jump_tbl {
    retcode_t (*putlstlogent_p)();
    retcode_t (*getlstlogent_p)();
    retcode_t (*dellstlogent_p)();
    retcode_t (*
} private_jump_tbl = {
    putlstlogent_p: &putlstlogent,
    getlstlogent_p: &getlstlogent,
    dellstlogent_p: &dellstlogent,
};


static retcode_t putlstlogent()
{

}

static retcode_t getlstlogent()
{

}

static retcode_t dellstlogent()
{

}

static int init_lastlog_backend (const struct jump_tbl *jump_tbl_p)
{
    assert (jump_tbl_p != NULL);
    jump_tbl_p = &private_jump_tbl;
}

int main ()
{
}
