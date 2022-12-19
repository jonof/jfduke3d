#include "duke3d.h"

void osdfunc_onshowosd(int shown)
{
    if (numplayers == 1)
        if ((shown && !ud.pause_on) || (!shown && ud.pause_on))
            KB_KeyDown[sc_Pause] = 1;
}

#define BGTILE 1141 // BIGHOLE
#define BGSHADE 12
#define BORDTILE 3250   // VIEWBORDER
#define BORDSHADE 0
#define BITSTH 1+32+8+16    // high translucency
#define BITSTL 1+8+16   // low translucency
#define BITS 8+16+64        // solid
#define PALETTE 0
void osdfunc_clearbackground(int c, int r)
{
    int x, y, xsiz, ysiz, tx2, ty2;
    int daydim, bits;
    (void)c;

    if (!POLYMOST_RENDERMODE_POLYGL()) bits = BITS; else bits = BITSTL;

    daydim = r*14+4;

    xsiz = tilesizx[BGTILE]; tx2 = xdim/xsiz;
    ysiz = tilesizy[BGTILE]; ty2 = daydim/ysiz;

    for(x=0;x<=tx2;x++)
        for(y=0;y<=ty2;y++)
            rotatesprite(x*xsiz<<16,y*ysiz<<16,65536L,0,BGTILE,BGSHADE,PALETTE,bits,0,0,xdim,daydim);

    xsiz = tilesizy[BORDTILE]; tx2 = xdim/xsiz;
    ysiz = tilesizx[BORDTILE];
    
    for(x=0;x<=tx2;x++)
        rotatesprite(x*xsiz<<16,(daydim+ysiz+1)<<16,65536L,1536,BORDTILE,BORDSHADE,PALETTE,BITS,0,0,xdim,daydim+ysiz+1);
}   

