/* Wrapper for argon Ruby bindings
 * lolware.net
 * Much of this code is based on run.c from the reference implementation
 */

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../phc-winner-argon2/src/argon2.h"
#include "../phc-winner-argon2/src/core.h"
#include "../phc-winner-argon2/src/encoding.h"

#define T_COST_DEF 3
#define LOG_M_COST_DEF 12 /* 2^12 = 4 MiB */
#define LANES_DEF 1
#define THREADS_DEF 1
#define OUT_LEN 32
#define SALT_LEN 16
#define ENCODE_LEN 108

int argon2_compare(const uint8_t *b1, const uint8_t *b2, size_t len);

unsigned int argon2_wrap(char *out, const char *pwd, size_t pwd_length,
        uint8_t *salt, uint32_t t_cost, uint32_t m_cost, uint32_t lanes, 
        uint8_t *secret, size_t secretlen)
{
    uint8_t hash[OUT_LEN];
    argon2_context context;

    if (!pwd) {
        return ARGON2_PWD_PTR_MISMATCH;
    }

    if (!salt) {
        return ARGON2_PWD_PTR_MISMATCH;
    }

    pwd_length = strlen(pwd);

    context.out = hash;
    context.outlen = OUT_LEN;
    context.pwd = (uint8_t *)pwd;
    context.pwdlen = pwd_length;
    context.salt = salt;
    context.saltlen = SALT_LEN;
    context.secret = secret;
    context.secretlen = secretlen;
    context.ad = NULL;
    context.adlen = 0;
    context.t_cost = t_cost;
    context.m_cost = m_cost;
    context.lanes = lanes;
    context.threads = lanes;
    context.allocate_cbk = NULL;
    context.free_cbk = NULL;
    context.flags = 0;

    int result = argon2i(&context);
    if (result != ARGON2_OK)
        return result;

    encode_string(out, ENCODE_LEN, &context, Argon2_i);
    return ARGON2_OK;
}
 
int wrap_argon2_verify(const char *encoded, const char *pwd,
    const size_t pwdlen,
    uint8_t *secret, size_t secretlen)
{
    argon2_context ctx;
    int ret;
    char out[ENCODE_LEN];
    memset(&ctx, 0, sizeof(argon2_context));

    /* max values, to be updated in decode_string */
    ctx.adlen = 512;
    ctx.saltlen = 512;
    ctx.outlen = 512;

    ctx.ad = malloc(ctx.adlen);
    ctx.salt = malloc(ctx.saltlen);
    ctx.out = malloc(ctx.outlen);
    if (!ctx.out || !ctx.salt || !ctx.ad) {
        free(ctx.ad);
        free(ctx.salt);
        free(ctx.out);
        return ARGON2_MEMORY_ALLOCATION_ERROR;
    }

    if(decode_string(&ctx, encoded, Argon2_i) != 1) {
        free(ctx.ad);
        free(ctx.salt);
        free(ctx.out);
        return ARGON2_DECODING_FAIL;
    } 

    ret = argon2_wrap(out, pwd, pwdlen, ctx.salt, ctx.t_cost, 
           ctx.m_cost, ctx.lanes, secret, secretlen);

    free(ctx.ad);
    free(ctx.salt);

    if (ret != ARGON2_OK || argon2_compare((uint8_t*)out, (uint8_t*)encoded, 
                strlen(encoded))) {
        free(ctx.out);
        return ARGON2_DECODING_FAIL;
    }
    free(ctx.out);

    return ARGON2_OK;
}

