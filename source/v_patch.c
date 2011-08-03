// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2005 James Haley
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// DESCRIPTION:
//  Functions to draw patches (by post)
//
//-----------------------------------------------------------------------------

#include "v_video.h"

// patch rendering globals -- like dc_ in r_draw.c
static cb_patch_column_t patchcol;
static int ytop;


//
// V_DrawPatchColumn
//
// Draws a plain patch column with no remappings.
//
static void V_DrawPatchColumn(void) 
{ 
   int              count;
   register byte    *dest;    // killough
   register fixed_t frac;     // killough
   fixed_t          fracstep;
   
   if((count = patchcol.y2 - patchcol.y1 + 1) <= 0)
      return; // Zero length, column does not exceed a pixel.
                                 
#ifdef RANGECHECK 
   if((unsigned int)patchcol.x  >= (unsigned int)patchcol.buffer->width || 
      (unsigned int)patchcol.y1 >= (unsigned int)patchcol.buffer->height) 
      I_Error("V_DrawPatchColumn: %i to %i at %i\n", patchcol.y1, patchcol.y2, patchcol.x); 
#endif 

   dest = patchcol.buffer->ylut[patchcol.y1] + patchcol.buffer->xlut[patchcol.x];

   // Determine scaling, which is the only mapping to be done.
   fracstep = patchcol.step;
   frac = patchcol.frac + ((patchcol.y1 * fracstep) & 0xFFFF);

   // Inner loop that does the actual texture mapping,
   //  e.g. a DDA-lile scaling.
   // This is as fast as it gets.       (Yeah, right!!! -- killough)
   //
   // killough 2/1/98: more performance tuning
   // haleyjd 06/21/06: rewrote and specialized for screen patches

   {
      register const byte *source = patchcol.source;
            
      while((count -= 2) >= 0)
      {
         *dest = source[frac >> FRACBITS];
         dest += patchcol.buffer->pitch;
         frac += fracstep;
         *dest = source[frac >> FRACBITS];
         dest += patchcol.buffer->pitch;
         frac += fracstep;
      }
      if(count & 1)
         *dest = source[frac >> FRACBITS];
   }
} 

//
// V_DrawPatchColumnTR
//
// Draws a plain patch column with color translation.
//
static void V_DrawPatchColumnTR(void) 
{ 
   int              count;
   register byte    *dest;    // killough
   register fixed_t frac;     // killough
   fixed_t          fracstep;
   
   if((count = patchcol.y2 - patchcol.y1 + 1) <= 0)
      return; // Zero length, column does not exceed a pixel.
                                 
#ifdef RANGECHECK 
   if((unsigned int)patchcol.x  >= (unsigned int)patchcol.buffer->width || 
      (unsigned int)patchcol.y1 >= (unsigned int)patchcol.buffer->height) 
      I_Error("V_DrawPatchColumnTR: %i to %i at %i\n", patchcol.y1, patchcol.y2, patchcol.x); 
#endif 

   dest = patchcol.buffer->ylut[patchcol.y1] + patchcol.buffer->xlut[patchcol.x];

   // Determine scaling, which is the only mapping to be done.
   fracstep = patchcol.step;
   frac = patchcol.frac + ((patchcol.y1 * fracstep) & 0xFFFF);

   // Inner loop that does the actual texture mapping,
   //  e.g. a DDA-lile scaling.
   // This is as fast as it gets.       (Yeah, right!!! -- killough)
   //
   // killough 2/1/98: more performance tuning
   // haleyjd 06/21/06: rewrote and specialized for screen patches

   {
      register const byte *source = patchcol.source;
            
      while((count -= 2) >= 0)
      {
         *dest = patchcol.translation[source[frac >> FRACBITS]];
         dest += patchcol.buffer->pitch;
         frac += fracstep;
         *dest = patchcol.translation[source[frac >> FRACBITS]];
         dest += patchcol.buffer->pitch;
         frac += fracstep;
      }
      if(count & 1)
         *dest = patchcol.translation[source[frac >> FRACBITS]];
   }
} 

#define DO_COLOR_BLEND()                       \
   fg = patchcol.fg2rgb[source[frac >> FRACBITS]];    \
   bg = patchcol.bg2rgb[*dest];                       \
   fg = (fg + bg) | 0x1f07c1f;                 \
   *dest = RGB32k[0][0][fg & (fg >> 15)]

//
// V_DrawPatchColumnTL
//
// Draws a translucent patch column. The DosDoom/zdoom-style
// translucency lookups must be set before getting here.
//
void V_DrawPatchColumnTL(void)
{ 
   int              count; 
   register byte    *dest;           // killough
   register fixed_t frac;            // killough
   fixed_t          fracstep;
   unsigned int     fg, bg;
   
   if((count = patchcol.y2 - patchcol.y1 + 1) <= 0)
      return; // Zero length, column does not exceed a pixel.
                                 
#ifdef RANGECHECK 
   if((unsigned int)patchcol.x  >= (unsigned int)patchcol.buffer->width || 
      (unsigned int)patchcol.y1 >= (unsigned int)patchcol.buffer->height) 
      I_Error("V_DrawPatchColumnTL: %i to %i at %i\n", patchcol.y1, patchcol.y2, patchcol.x); 
#endif 

   dest = patchcol.buffer->ylut[patchcol.y1] + patchcol.buffer->xlut[patchcol.x];

   // Determine scaling, which is the only mapping to be done.
   fracstep = patchcol.step;
   frac = patchcol.frac + ((patchcol.y1 * fracstep) & 0xFFFF);

   // haleyjd 06/21/06: rewrote and specialized for screen patches
   {
      register const byte *source = patchcol.source;
            
      while((count -= 2) >= 0)
      {
         DO_COLOR_BLEND();

         dest += patchcol.buffer->pitch;
         frac += fracstep;

         DO_COLOR_BLEND();

         dest += patchcol.buffer->pitch;
         frac += fracstep;
      }
      if(count & 1)
      {
         DO_COLOR_BLEND();
      }
   }
}

#undef DO_COLOR_BLEND

#define DO_COLOR_BLEND()          \
   fg = patchcol.fg2rgb[patchcol.translation[source[frac >> FRACBITS]]]; \
   bg = patchcol.bg2rgb[*dest];          \
   fg = (fg + bg) | 0x1f07c1f;    \
   *dest = RGB32k[0][0][fg & (fg >> 15)]

//
// V_DrawPatchColumnTRTL
//
// Draws translated translucent patch columns.
// Requires both translucency lookups and a translation table.
//
void V_DrawPatchColumnTRTL(void)
{ 
   int              count; 
   register byte    *dest;           // killough
   register fixed_t frac;            // killough
   fixed_t          fracstep;
   unsigned int     fg, bg;
   
   if((count = patchcol.y2 - patchcol.y1 + 1) <= 0)
      return; // Zero length, column does not exceed a pixel.
                                 
#ifdef RANGECHECK 
   if((unsigned int)patchcol.x  >= (unsigned int)patchcol.buffer->width || 
      (unsigned int)patchcol.y1 >= (unsigned int)patchcol.buffer->height) 
      I_Error("V_DrawPatchColumnTRTL: %i to %i at %i\n", patchcol.y1, patchcol.y2, patchcol.x); 
#endif 

   dest = patchcol.buffer->ylut[patchcol.y1] + patchcol.buffer->xlut[patchcol.x];

   // Determine scaling, which is the only mapping to be done.
   fracstep = patchcol.step; 
   frac = patchcol.frac + ((patchcol.y1 * fracstep) & 0xFFFF);

   // haleyjd 06/21/06: rewrote and specialized for screen patches
   {
      register const byte *source = patchcol.source;
      
      while((count -= 2) >= 0)
      {
         DO_COLOR_BLEND();

         dest += patchcol.buffer->pitch;
         frac += fracstep;

         DO_COLOR_BLEND();

         dest += patchcol.buffer->pitch;
         frac += fracstep;
      }
      if(count & 1)
      {
         DO_COLOR_BLEND();
      }
   }
}

#undef DO_COLOR_BLEND

#define DO_COLOR_BLEND() \
   /* mask out LSBs in green and red to allow overflow */ \
   a = patchcol.fg2rgb[source[frac >> FRACBITS]] + patchcol.bg2rgb[*dest]; \
   b = a; \
   a |= 0x01f07c1f; \
   b &= 0x40100400; \
   a &= 0x3fffffff; \
   b  = b - (b >> 5); \
   a |= b; \
   *dest = RGB32k[0][0][a & (a >> 15)]


//
// V_DrawPatchColumnAdd
//
// Draws a patch column with additive translucency.
//
void V_DrawPatchColumnAdd(void)
{ 
   int              count; 
   register byte    *dest;           // killough
   register fixed_t frac;            // killough
   fixed_t          fracstep;
   unsigned int     a, b;
   
   if((count = patchcol.y2 - patchcol.y1 + 1) <= 0)
      return; // Zero length, column does not exceed a pixel.
                                 
#ifdef RANGECHECK 
   if((unsigned int)patchcol.x  >= (unsigned int)patchcol.buffer->width || 
      (unsigned int)patchcol.y1 >= (unsigned int)patchcol.buffer->height) 
      I_Error("V_DrawPatchColumnAdd: %i to %i at %i\n", patchcol.y1, patchcol.y2, patchcol.x); 
#endif 

   dest = patchcol.buffer->ylut[patchcol.y1] + patchcol.buffer->xlut[patchcol.x];

   // Determine scaling, which is the only mapping to be done.
   fracstep = patchcol.step; 
   frac = patchcol.frac + ((patchcol.y1 * fracstep) & 0xFFFF);

   // haleyjd 06/21/06: rewrote and specialized for screen patches
   {
      register const byte *source = patchcol.source;
      
      while((count -= 2) >= 0)
      {
         DO_COLOR_BLEND();

         dest += patchcol.buffer->pitch;
         frac += fracstep;

         DO_COLOR_BLEND();

         dest += patchcol.buffer->pitch;
         frac += fracstep;
      }
      if(count & 1)
      {
         DO_COLOR_BLEND();
      }
   }
}

#undef DO_COLOR_BLEND

#define DO_COLOR_BLEND() \
   /* mask out LSBs in green and red to allow overflow */ \
   a = patchcol.fg2rgb[patchcol.translation[source[frac >> FRACBITS]]] + patchcol.bg2rgb[*dest]; \
   b = a; \
   a |= 0x01f07c1f; \
   b &= 0x40100400; \
   a &= 0x3fffffff; \
   b  = b - (b >> 5); \
   a |= b; \
   *dest = RGB32k[0][0][a & (a >> 15)]

//
// V_DrawPatchColumnAddTR
//
// Draws a patch column with additive translucency and
// translation.
//
void V_DrawPatchColumnAddTR(void)
{ 
   int              count; 
   register byte    *dest;           // killough
   register fixed_t frac;            // killough
   fixed_t          fracstep;
   unsigned int     a, b;
   
   if((count = patchcol.y2 - patchcol.y1 + 1) <= 0)
      return; // Zero length, column does not exceed a pixel.
                                 
#ifdef RANGECHECK 
   if((unsigned int)patchcol.x  >= (unsigned int)patchcol.buffer->width || 
      (unsigned int)patchcol.y1 >= (unsigned int)patchcol.buffer->height) 
      I_Error("V_DrawPatchColumnAddTR: %i to %i at %i\n", patchcol.y1, patchcol.y2, patchcol.x); 
#endif 

   dest = patchcol.buffer->ylut[patchcol.y1] + patchcol.buffer->xlut[patchcol.x];

   // Determine scaling, which is the only mapping to be done.
   fracstep = patchcol.step; 
   frac = patchcol.frac + ((patchcol.y1 * fracstep) & 0xFFFF);

   // haleyjd 06/21/06: rewrote and specialized for screen patches
   {
      register const byte *source = patchcol.source;
      
      while((count -= 2) >= 0)
      {
         DO_COLOR_BLEND();

         dest += patchcol.buffer->pitch;
         frac += fracstep;
         
         DO_COLOR_BLEND();
         
         dest += patchcol.buffer->pitch;
         frac += fracstep;
      }
      if(count & 1)
      {
         DO_COLOR_BLEND();
      }
   }
}

#undef DO_COLOR_BLEND



static void V_DrawMaskedColumn(column_t *column)
{
   for(;column->topdelta != 0xff; column = (column_t *)((byte *)column + column->length + 4))
   {
      // calculate unclipped screen coordinates for post

      int columntop = ytop + column->topdelta;

      if(columntop >= 0)
      {
         // SoM: Make sure the lut is never referenced out of range
         if(columntop >= patchcol.buffer->unscaledh)
            return;
            
         patchcol.y1 = patchcol.buffer->y1lookup[columntop];
         patchcol.frac = 0;
      }
      else
      {
         patchcol.frac = (-columntop) << FRACBITS;
         patchcol.y1 = 0;
      }

      if(columntop + column->length - 1 < 0)
         continue;
      if(columntop + column->length - 1 < patchcol.buffer->unscaledh)
         patchcol.y2 = patchcol.buffer->y2lookup[columntop + column->length - 1];
      else
         patchcol.y2 = patchcol.buffer->y2lookup[patchcol.buffer->unscaledh - 1];

      // SoM: The failsafes should be completely redundant now...
      // haleyjd 05/13/08: fix clipping; y2lookup not clamped properly
      if((column->length > 0 && patchcol.y2 < patchcol.y1) ||
         patchcol.y2 >= patchcol.buffer->height)
         patchcol.y2 = patchcol.buffer->height - 1;
      
      // killough 3/2/98, 3/27/98: Failsafe against overflow/crash:
      if(patchcol.y1 <= patchcol.y2 && patchcol.y2 < patchcol.buffer->height)
      {
         patchcol.source = (byte *)column + 3;
         patchcol.colfunc();
      }
   }
}



static void V_DrawMaskedColumnUnscaled(column_t *column)
{
   for(;column->topdelta != 0xff; column = (column_t *)((byte *)column + column->length + 4))
   {
      // calculate unclipped screen coordinates for post

      int columntop = ytop + column->topdelta;

      if(columntop >= 0)
      {
         patchcol.y1 = columntop;
         patchcol.frac = 0;
      }
      else
      {
         patchcol.frac = (-columntop) << FRACBITS;
         patchcol.y1 = 0;
      }

      if(columntop + column->length - 1 < patchcol.buffer->height)
         patchcol.y2 = columntop + column->length - 1;
      else
         patchcol.y2 = patchcol.buffer->height - 1;

      // killough 3/2/98, 3/27/98: Failsafe against overflow/crash:
      if(patchcol.y1 <= patchcol.y2 && patchcol.y2 < patchcol.buffer->height)
      {
         patchcol.source = (byte *)column + 3;
         patchcol.colfunc();
      }
   }
}


//
// V_DrawPatchInt
//
// Draws patches to the screen via the same vissprite-style scaling
// and clipping used to draw player gun sprites. This results in
// more clean and uniform scaling even in arbitrary resolutions, and
// eliminates mostly unnecessary special cases for certain resolutions
// like 640x400.
//
void V_DrawPatchInt(PatchInfo *pi, VBuffer *buffer)
{
   int        x1, x2, w;
   fixed_t    iscale, xiscale, startfrac = 0;
   patch_t    *patch = pi->patch;
   int        maxw;
   void       (*maskcolfunc)(column_t *);

   w = SwapShort(patch->width); // haleyjd: holy crap, stop calling this 800 times
   
   patchcol.buffer = buffer;

   // calculate edges of the shape
   if(pi->flipped)
   {
      // If flipped, then offsets are flipped as well which means they 
      // technically offset from the right side of the patch (x2)
      x2 = pi->x + SwapShort(patch->leftoffset);
      x1 = x2 - (w - 1);
   }
   else
   {
      x1 = pi->x - SwapShort(patch->leftoffset);
      x2 = x1 + w - 1;
   }

   // haleyjd 08/16/08: scale and step values must come from the VBuffer, NOT
   // the Cardboard video structure...

   if(buffer->scaled)
   {      
      iscale        = buffer->ixscale;
      patchcol.step = buffer->iyscale;
      maxw          = buffer->unscaledw;
      maskcolfunc   = V_DrawMaskedColumn;
   }
   else
   {
      iscale = patchcol.step = FRACUNIT;
      maxw = buffer->width;
      maskcolfunc   = V_DrawMaskedColumnUnscaled;
   }

   // off the left or right side?
   if(x2 < 0 || x1 >= maxw)
      return;

   xiscale = pi->flipped ? -iscale : iscale;

   if(buffer->scaled)
   {
      // haleyjd 10/10/08: must handle coordinates outside the screen buffer
      // very carefully here.
      if(x1 >= 0)
         x1 = buffer->x1lookup[x1];
      else
         x1 = -buffer->x2lookup[-x1 - 1];

      if(x2 < buffer->unscaledw)
         x2 = buffer->x2lookup[x2];
      else
         x2 = buffer->x2lookup[buffer->unscaledw - 1];
   }

   patchcol.x  = x1 < 0 ? 0 : x1;
   
   // SoM: Any time clipping occurs on screen coords, the resulting clipped 
   // coords should be checked to make sure we are still on screen.
   if(x2 < x1)
      return;

   // SoM: Ok, so the startfrac should ALWAYS be the last post of the patch 
   // when the patch is flipped minus the fractional "bump" from the screen
   // scaling, then the patchcol.x to x1 clipping will place the frac in the
   // correct column no matter what. This also ensures that scaling will be
   // uniform. If the resolution is 320x2X0 the iscale will be 65537 which
   // will create some fractional bump down, so it is safe to assume this 
   // puts us just below patch->width << 16
   if(pi->flipped)
      startfrac = (w << 16) - ((x1 * iscale) & 0xffff) - 1;
   else
      startfrac = (x1 * iscale) & 0xffff;

   if(patchcol.x > x1)
      startfrac += xiscale * (patchcol.x - x1);

   {
      column_t *column;
      int      texturecolumn;
      
      switch(pi->drawstyle)
      {
      case PSTYLE_NORMAL:
         patchcol.colfunc = V_DrawPatchColumn;
         break;
      case PSTYLE_TLATED:
         patchcol.colfunc = V_DrawPatchColumnTR;
         break;
      case PSTYLE_TRANSLUC:
         patchcol.colfunc = V_DrawPatchColumnTL;
         break;
      case PSTYLE_TLTRANSLUC:
         patchcol.colfunc = V_DrawPatchColumnTRTL;
         break;
      case PSTYLE_ADD:
         patchcol.colfunc = V_DrawPatchColumnAdd;
         break;
      case PSTYLE_TLADD:
         patchcol.colfunc = V_DrawPatchColumnAddTR;
         break;
      default:
         I_Error("V_DrawPatchInt: unknown patch drawstyle %d\n", pi->drawstyle);
      }

      ytop = pi->y - SwapShort(patch->topoffset);
      
      for(; patchcol.x <= x2; patchcol.x++, startfrac += xiscale)
      {
         texturecolumn = startfrac >> FRACBITS;
         
#ifdef RANGECHECK
         if(texturecolumn < 0 || texturecolumn >= w)
            I_Error("V_DrawPatchInt: bad texturecolumn %d\n", texturecolumn);
#endif
         
         column = (column_t *)((byte *)patch +
            SwapLong(patch->columnofs[texturecolumn]));
         maskcolfunc(column);
      }
   }
}

void V_SetPatchColrng(byte *colrng)
{
   patchcol.translation = colrng;
}

void V_SetPatchTL(unsigned int *fg, unsigned int *bg)
{
   patchcol.fg2rgb = fg;
   patchcol.bg2rgb = bg;
}

//
// V_SetupBufferFuncs
//
// VBuffer setup function
//
// Call to determine the type of drawing your VBuffer object will have
//
void V_SetupBufferFuncs(VBuffer *buffer, int drawtype)
{
   // call other setting functions
   V_SetBlockFuncs(buffer, drawtype);
}

//=============================================================================
//
// Conversion Routines
//
// haleyjd: this stuff turns other graphic formats into patches :)
//

//
// V_PatchToLinear
//
// haleyjd 11/02/08: converts a patch into a linear buffer
//
byte *V_PatchToLinear(patch_t *patch, boolean flipped, byte fillcolor,
                      int *width, int *height)
{
   int w = SwapShort(patch->width);
   int h = SwapShort(patch->height);
   int col = w - 1, colstop = -1, colstep = -1;

   byte *desttop;
   byte *buffer = malloc(w * h);

   memset(buffer, fillcolor, w * h);
  
   if(!flipped)
      col = 0, colstop = w, colstep = 1;

   desttop = buffer;
      
   for(; col != colstop; col += colstep, ++desttop)
   {
      const column_t *column = 
         (const column_t *)((byte *)patch + SwapLong(patch->columnofs[col]));
      
      // step through the posts in a column
      while(column->topdelta != 0xff)
      {
         // killough 2/21/98: Unrolled and performance-tuned         
         register const byte *source = (byte *)column + 3;
         register byte *dest = desttop + column->topdelta * w;
         register int count = column->length;
         int diff;

         // haleyjd 11/02/08: clip to indicated height
         if((diff = (count + column->topdelta) - h) > 0)
            count -= diff;

         // haleyjd: make sure there's something left to draw
         if(count <= 0)
            break;
         
         if((count -= 4) >= 0)
         {
            do
            {
               register byte s0, s1;
               s0 = source[0];
               s1 = source[1];
               dest[0] = s0;
               dest[w] = s1;
               dest += w * 2;
               s0 = source[2];
               s1 = source[3];
               source += 4;
               dest[0] = s0;
               dest[w] = s1;
               dest += w * 2;
            }
            while((count -= 4) >= 0);
         }
         if(count += 4)
         {
            do
            {
               *dest = *source++;
               dest += w;
            }
            while(--count);
         }
         column = (column_t *)(source + 1); // killough 2/21/98 even faster
      }
   }

   // optionally return width and height
   if(width)
      *width = w;
   if(height)
      *height = h;

   // voila!
   return buffer;
}


//
// V_LinearToPatch
//
// haleyjd 07/07/07: converts a linear graphic to a patch
//
patch_t *V_LinearToPatch(byte *linear, int w, int h, size_t *memsize)
{
   int      x, y;
   patch_t  *p;
   column_t *c;
   int      *columnofs;
   byte     *src, *dest;

   // 1. no source bytes are lost (no transparency)
   // 2. patch_t header is 4 shorts plus width * int for columnofs array
   // 3. one post per vertical slice plus 2 padding bytes and 1 byte for
   //    a -1 post to cap the column are required
   size_t total_size = 
      4 * sizeof(int16_t) + w * (h + sizeof(int32_t) + sizeof(column_t) + 3);
   
   byte *out = malloc(total_size);

   p = (patch_t *)out;

   // set basic header information
   p->width      = w;
   p->height     = h;
   p->topoffset  = 0;
   p->leftoffset = 0;

   // get pointer to columnofs table
   columnofs = (int *)(out + 4 * sizeof(int16_t));

   // skip past columnofs table
   dest = out + 4 * sizeof(int16_t) + w * sizeof(int32_t);

   // convert columns of linear graphic into true columns
   for(x = 0; x < w; ++x)
   {
      // set entry in columnofs table
      columnofs[x] = dest - out;

      // set basic column properties
      c = (column_t *)dest;
      c->length   = h;
      c->topdelta = 0;

      // skip past column header
      dest += sizeof(column_t) + 1;

      // copy bytes
      for(y = 0, src = linear + x; y < h; ++y, src += w)
         *dest++ = *src;

      // create end post
      *(dest + 1) = 255;

      // skip to next column location 
      dest += 2;
   }

   // allow returning size of allocation in *memsize
   if(memsize)
      *memsize = total_size;

   // voila!
   return p;
}

// EOF
