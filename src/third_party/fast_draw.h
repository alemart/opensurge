#ifndef FAST_DRAW_CACHE_H
#define FAST_DRAW_CACHE_H

#include <allegro5/allegro.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct FAST_DRAW_CACHE FAST_DRAW_CACHE;

// When using buffers, initial_size is in fact the maximum size, so set it appropriately.
// Otherwise, the cache will be resizes as necessary.
FAST_DRAW_CACHE* fd_create_cache(size_t inital_size, bool use_indices, bool use_buffers);
void fd_destroy_cache(FAST_DRAW_CACHE* cache);
void fd_flush_cache(FAST_DRAW_CACHE* cache);

void fd_draw_bitmap(FAST_DRAW_CACHE* cache, ALLEGRO_BITMAP* bmp, float x, float y);
void fd_draw_bitmap_region(FAST_DRAW_CACHE* cache, ALLEGRO_BITMAP* bmp, float sx, float sy, float sw, float sh, float dx, float dy);
void fd_draw_scaled_bitmap(FAST_DRAW_CACHE* cache, ALLEGRO_BITMAP* bmp, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh);
void fd_draw_tinted_bitmap(FAST_DRAW_CACHE* cache, ALLEGRO_BITMAP* bmp, ALLEGRO_COLOR tint, float x, float y);
void fd_draw_tinted_bitmap_region(FAST_DRAW_CACHE* cache, ALLEGRO_BITMAP* bmp, ALLEGRO_COLOR tint, float sx, float sy, float sw, float sh, float dx, float dy);
void fd_draw_tinted_scaled_bitmap(FAST_DRAW_CACHE* cache, ALLEGRO_BITMAP* bmp, ALLEGRO_COLOR tint, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh);

#ifdef __cplusplus
}
#endif

#endif
