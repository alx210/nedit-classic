/*******************************************************************************
*
* font.c -- Xft font support shim
*
* This is free software; you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; either version 2 of the License, or (at your option) any later
* version. In addition, you may distribute versions of this program linked to
* Motif or Open Motif. See README for details.                                 
*
* This software is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
* for more details.
*
* You should have received a copy of the GNU General Public License along with
* software; if not, write to the Free Software Foundation, Inc., 59 Temple
* Place, Suite 330, Boston, MA  02111-1307 USA
*
*****************************************************************************/

#include <stdlib.h>
#include "nedit.h"
#include "font.h"

FontStruct* LoadFont(const char *fontName)
{
	FontStruct *font;

	#ifdef USE_XRENDER
	font = malloc(sizeof(FontStruct));
	if(!font) return NULL;

	font->xft = XftFontOpenName(TheDisplay, TheScreen, fontName);
	if(font->xft) {
	    XGlyphInfo ext;
		int i;

		 /* Loop trough ASCII printables and figure out minimum extents */
		font->min_bounds.width = font->xft->max_advance_width;
		for(i = 0; i < 93; i++ ) {
			FT_UInt glyph = i + 33;
			XftGlyphExtents(TheDisplay, font->xft, &glyph, 1, &ext);
			if(ext.xOff < font->min_bounds.width)
				font->min_bounds.width = ext.xOff;
		}

		font->max_bounds.width = font->xft->max_advance_width;
		font->ascent = font->xft->ascent;
		font->descent = font->xft->descent;
	} else {
		free(font);
		return NULL;
	}
	
	#else
	font = XLoadQueryFont(TheDisplay, fontName);
	#endif /* XFT_FONTS */
	
	return font;
}

void UnloadFont(FontStruct *font)
{
	#ifdef USE_XRENDER
	XftFontClose(TheDisplay, font->xft);
	free(font);
	#else
	XFreeFont(TheDisplay, font);
	#endif /* XFT_FONTS */
}
