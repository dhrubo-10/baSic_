#ifndef WIND
#define WIND

#include <kernel/events.h>
#include <kernel/gfxstream.h>

struct win_d * nw_create_default();
struct win_d * nw_create_child( struct win_d *parent, int x, int y, int width, int height );
struct win_d * nw_create_from_fd( int fd );

int nw_width( struct win_d *w );
int nw_height( struct win_d *w );

char nw_getchar( struct win_d *w, int blocking );

int nw_next_event( struct win_d *w, struct event *e );
int nw_read_events( struct win_d *w, struct event *e, int count, int timeout );
int nw_post_events( struct win_d *w, const struct event *e, int count );

int nw_move( struct win_d *w, int x, int y );
int nw_resize( struct win_d *w, int width, int height );
int nw_fd( struct win_d *w );

void nw_fgcolor( struct win_d *w, int r, int g, int b );
void nw_bgcolor( struct win_d *w, int r, int g, int b );
void nw_clear  ( struct win_d *w, int x, int y, int width, int height );
void nw_line   ( struct win_d *w, int x, int y, int width, int height );
void nw_rect   ( struct win_d *w, int x, int y, int width, int height );
void nw_char   ( struct win_d *w, int x, int y, char c );
void nw_string ( struct win_d *w, int x, int y, const char *s );
void nw_flush  ( struct win_d *w );



#endif
