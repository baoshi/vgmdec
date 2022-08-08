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


/*                                                  off    len   o_c_s  o_c_l   c_s
 *  cache off=4,len=3          |4|5|6|                                     
 *  read (1, 3)          |1|2|3|                     1      3            
 *  read (1, 4)          |1|2|3|4|                   1      4      3      1      0     
 *  read (4, 1)                |4|                   4      1      0      1      0 
 *  read (6, 1)                    |6|               6      1      0      1      2
 *  read (4, 3)                |4|5|6|               4      3      0      3      0
 *  read (2, 6)            |2|3|4|5|6|7|8|           2      6      2      3      0
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

    if (length == 0)
        return 0;

    ctx->cache_data_length = 3;
    ctx->cache_offset = 4;

    // Check if reqest data is already in cache (or part of)
    if ((ctx->cache_data_length > 0) && (offset + length > ctx->cache_offset) && (offset < ctx->cache_offset + ctx->cache_data_length))
    {
        o_c_s = (offset > ctx->cache_offset) ? 0 : (ctx->cache_offset - offset);
        if (length - o_c_s > ctx->cache_data_length)
            o_c_l = ctx->cache_data_length;
        else
            o_c_l = length - o_c_s;
        c_s = offset + o_c_s - ctx->cache_offset;
        VGM_PRINTF("%d      %d      %d      %d      %d\n", offset, length, o_c_s, o_c_l, c_s);
    }
    else
    {
        VGM_PRINTF("%d      %d\n", offset, length);
    }

    return 0;
    

    

    if (((offset + length) <= ctx->cache_offset) || (offset >= ctx->cache_offset + ctx->cache_data_length))
    {
        // reading range is completely out side cache
        if (length <= ctx->cache_size)
        {
            fseek(ctx->fd, offset, SEEK_SET);
            ctx->cache_offset = offset;
            ctx->cache_data_length = (uint32_t)fread(ctx->cache, 1, ctx->cache_size, ctx->fd);
            length = (length > ctx->cache_data_length) ? length : ctx->cache_data_length;
            memcpy(out, ctx->cache, length);
#ifdef CFR_MEASURE_CACHE_PERFORMACE
            ctx->cache_miss += length;
#endif           
            return length;
        }
        else
        {

        }
    }

    if (length > 1)
    {
        fseek(ctx->fd, offset, SEEK_SET);
        return (uint32_t)(fread(out, 1, length, ctx->fd));
    }
    if ((offset >= ctx->cache_offset) && (offset < ctx->cache_offset + ctx->cache_data_length))
    {
        // Cache hit
#ifdef CFR_MEASURE_CACHE_PERFORMACE
        ++(ctx->cache_hit);
#endif
        * out = ctx->cache[offset - ctx->cache_offset];
        return 1;
    }
    // Cache miss
#ifdef CFR_MEASURE_CACHE_PERFORMACE
    ++(ctx->cache_miss);
#endif
    fseek(ctx->fd, offset, SEEK_SET);
    ctx->cache_offset = offset;
    ctx->cache_data_length = (uint32_t)fread(ctx->cache, 1, ctx->cache_size, ctx->fd);
    if (ctx->cache_data_length > 0)
    {
        *out = ctx->cache[0];
        return 1;
    }
    // Offset is out-of-bound
    return 0;
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