#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "platform.h"
#include "file_reader_cached.h"


// Cached File Reader
typedef struct cfr_s
{
    // super class
    file_reader_t super;
    // Private fields
    FILE* fd;
    uint8_t* cache;
    uint32_t cache_size;
    uint32_t cache_data_length;
    uint32_t cache_offset;
#ifdef CFR_MEASURE_CACHE_PERFORMACE
    unsigned long long cache_hit;
    unsigned long long cache_miss;
#endif
} cfr_t;



static uint32_t read_and_cache(cfr_t *ctx, uint8_t *out, uint32_t offset, uint32_t length)
{
    uint32_t read = 0, total = 0;
    fseek(ctx->fd, offset, SEEK_SET);
    while (length > ctx->cache_size)
    {
        read = fread(out, 1, ctx->cache_size, ctx->fd);
        if (0 == read) break;
        out += read;
        total += read;
        offset += read;
        length -= read;
    }
    if (length > 0)
    {
        read = fread(ctx->cache, 1, ctx->cache_size, ctx->fd);
        if (read > 0)
        {
            ctx->cache_offset = offset;
            ctx->cache_data_length = read;
            memcpy(out, ctx->cache, length);
            total += length;
        }
    }
    return total;
}


/*                                                  off    len   o_c_s  o_c_l   c_s
 *  cache off=4,len=3          |4|5|6|                                     
 *  read (1, 3)          |1|2|3|                     1      3            
 *  read (1, 4)          |1|2|3|4|                   1      4      3      1      0     
 *  read (4, 1)                |4|                   4      1      0      1      0 
 *  read (6, 1)                    |6|               6      1      0      1      2
 *  read (4, 3)                |4|5|6|               4      3      0      3      0
 *  read (2, 6)            |2|3|4|5|6|7|             2      6      2      3      0
 *  read (6, 1)                    |6|               6      1      0      1      2
 *  read (6, 3)                    |6|7|8|           6      3      0      1      2
 *  read (7, 3)                      |7|8|9|         7      3      
 */

static uint32_t read(file_reader_t *self, uint8_t *out, uint32_t offset, uint32_t length)
{
    cfr_t *ctx = (cfr_t *)self;
    uint32_t o_c_s; // out_cached_start
    uint32_t o_c_l; // out_cached_length
    uint32_t c_s;   // cached_start
    uint32_t total = 0;

    if (length == 0)
        return 0;


    VGM_PRINTF("Read from %d, len = %d\n", offset, length);

    // Check if reqest data is already in cache (or part of)
    if ((ctx->cache_data_length > 0) && (offset + length > ctx->cache_offset) && (offset < ctx->cache_offset + ctx->cache_data_length))
    {
        o_c_s = (offset > ctx->cache_offset) ? 0 : (ctx->cache_offset - offset);
        c_s = offset + o_c_s - ctx->cache_offset;
        if (length - o_c_s > ctx->cache_data_length - c_s)
            o_c_l = ctx->cache_data_length - c_s;
        else
            o_c_l = length - o_c_s;
        // transfer from catch to output
        VGM_PRINTF("Copy cache[%d] > out[%d], len = %d\n", c_s, o_c_s, o_c_l);
        memcpy(out + o_c_s, ctx->cache + c_s, o_c_l);
        total += o_c_l;
    }
    else
    {
        // fetch fresh data
        VGM_PRINTF("Fetch whole buffer\n");
        return read_and_cache(ctx, out, offset, length);
    }
    // fetch data before cached region
    if (o_c_s > 0)
    {
        VGM_PRINTF("Fetch to out[0], len = %d\n", o_c_s);
        total += read_and_cache(ctx, out, offset, o_c_s);
    }
    // fetch data after cached region
    if (length > o_c_s + o_c_l)
    {
        VGM_PRINTF("Fetch to out[%d], len = %d\n", o_c_s + o_c_l, length - o_c_s - o_c_l);
        total += read_and_cache(ctx, out + o_c_s + o_c_l, ctx->cache_offset + ctx->cache_data_length, length - o_c_s - o_c_l);
    }
    
    return total;
}


static uint32_t size(file_reader_t *self)
{
    cfr_t *ctx = (cfr_t*)self;
    if (ctx && ctx->fd)
    {
        fseek(ctx->fd, 0, SEEK_END);
        size_t len = ftell(ctx->fd);
        return (uint32_t)len;
    }
    return 0;
}



file_reader_t * cfr_create(const char* fn, uint32_t cache_size)
{
    FILE *fd = 0;
    cfr_t *ctx = 0;
    
    do
    {
        fd = fopen(fn, "rb");
        if (0 == fd)
            break;

        ctx = (cfr_t*)VGM_MALLOC(sizeof(cfr_t));
        if (0 == ctx)
            break;

        ctx->cache = (uint8_t*)VGM_MALLOC(cache_size);
        if (0 == ctx->cache)
            break;

        ctx->fd = fd;
        ctx->cache_size = cache_size;
        ctx->cache_data_length = 0;
        ctx->cache_offset = 0;

        ctx->super.self = (file_reader_t*)ctx;
        ctx->super.read = read;
        ctx->super.size = size;

#ifdef NFR_MEASURE_CACHE_PERFORMACE
        ctx->cache_hit = 0;
        ctx->cache_miss = 0;
#endif
        return (file_reader_t*)ctx;

    } while (0);

    if (ctx && ctx->cache)
        VGM_FREE(ctx->cache);
    if (ctx)
        VGM_FREE(ctx);
    if (fd)
        fclose(fd);
    return 0;
}


void cfr_destroy(file_reader_t *cfr)
{
    cfr_t* ctx = (cfr_t*)cfr;
    if (0 == ctx)
        return;
    if (ctx->cache)
        VGM_FREE(ctx->cache);
    if (ctx->fd)
        fclose(ctx->fd);
    VGM_FREE(ctx);
}


#ifdef CFR_MEASURE_CACHE_PERFORMACE

void cfr_show_cache_status(file_reader_t* cfr)
{
    cfr_t *ctx = (cfr_t *)cfr;
    VGM_PRINTF("Cache Status: (%llu/%llu), hit %.1f%%\n", ctx->cache_hit, ctx->cache_hit + ctx->cache_miss, (ctx->cache_hit * 100.0f) / (ctx->cache_hit + ctx->cache_miss));
}

#endif