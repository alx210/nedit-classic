/*******************************************************************************
*
* font.h -- Xft font support shim
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

#ifndef FONT_H
#define FONT_H

#include <X11/Xlib.h>

#ifdef USE_XRENDER
#include <X11/Xft/Xft.h>

typedef struct {
	XftFont *xft;
	XCharStruct min_bounds;
	XCharStruct max_bounds;
	int ascent;
	int descent;
} FontStruct;
#else
typedef XFontStruct FontStruct;
#endif /* XFT_FONTS */

FontStruct* LoadFont(const char *fontName);
void UnloadFont(FontStruct *font);

#endif /* FONT_H */
