static const char CVSID[] = "$Id: help.c,v 1.68 2001/12/10 04:57:59 edel Exp $";
/*******************************************************************************
*									       *
* help.c -- Nirvana Editor help display					       *
*									       *
* Copyright (C) 1999 Mark Edel						       *
*									       *
* This is free software; you can redistribute it and/or modify it under the    *
* terms of the GNU General Public License as published by the Free Software    *
* Foundation; either version 2 of the License, or (at your option) any later   *
* version.							               *
* 									       *
* This software is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License        *
* for more details.							       *
* 									       *
* You should have received a copy of the GNU General Public License along with *
* software; if not, write to the Free Software Foundation, Inc., 59 Temple     *
* Place, Suite 330, Boston, MA  02111-1307 USA		                       *
*									       *
* Nirvana Text Editor	    						       *
* September 10, 1991							       *
*									       *
* Written by Mark Edel, mostly rewritten by Steve Haehn for new help system,   *
* December, 2001   	    						       *
*									       *
*******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#ifdef VMS
#include "../util/VMSparam.h"
#else
#ifndef __MVS__
#include <sys/param.h>
#endif
#endif /*VMS*/

#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrollBar.h>
#include <Xm/PushB.h>

#include "../util/misc.h"
#include "../util/DialogF.h"
#include "../util/system.h"
#include "textBuf.h"
#include "text.h"

#include <Xm/XmP.h>         /* These are for applying style info to help text */
#include <Xm/PrimitiveP.h>
#include "textDisp.h"
#include "textP.h"

#include "textSel.h"
#include "nedit.h"
#include "search.h"
#include "window.h"
#include "preferences.h"
#include "help.h"
#include "help_data.h"
#include "file.h"
#include "highlight.h"

/*============================================================================*/
/*                              SYMBOL DEFINITIONS                            */
/*============================================================================*/

#define EOS '\0'              /* end-of-string character                  */

#define CLICK_THRESHOLD 5     /* number of pixels mouse may move from its */
                              /* pressed location for mouse-up to be      */
                              /* considered a valid click (as opposed to  */
                              /* a drag or mouse-pick error)              */

/*============================================================================*/
/*                             VARIABLE DECLARATIONS                          */
/*============================================================================*/

static Widget HelpWindows[NUM_TOPICS] = {NULL}; 
static Widget HelpTextPanes[NUM_TOPICS] = {NULL};
static textBuffer *HelpStyleBuffers[NUM_TOPICS] = {NULL};

/* Information on the last search for search-again */
static char LastSearchString[DF_MAX_PROMPT_LENGTH] = "";
static int LastSearchTopic = -1;
static int LastSearchPos = 0;
static int LastSearchWasAllTopics = False;

int StyleFonts[] =
{
    /* Normal (proportional) fonts, styles: 'A', 'B', 'C', 'D' */
    HELP_FONT, BOLD_HELP_FONT, ITALIC_HELP_FONT, BOLD_ITALIC_HELP_FONT,

    /* Underlined fonts, styles: 'E', 'F', 'G', 'H' */
    HELP_FONT, BOLD_HELP_FONT, ITALIC_HELP_FONT, BOLD_ITALIC_HELP_FONT,

    /* Fixed fonts, styles: 'I', 'J', 'K', 'L' */
    FIXED_HELP_FONT, BOLD_FIXED_HELP_FONT, BOLD_FIXED_HELP_FONT,
    BOLD_ITALIC_FIXED_HELP_FONT,

    /* Underlined fixed fonts, styles: 'M', 'N', 'O', 'P' */
    FIXED_HELP_FONT, BOLD_FIXED_HELP_FONT, BOLD_FIXED_HELP_FONT,
    BOLD_ITALIC_FIXED_HELP_FONT,

    /* Link font, style: 'Q' */
    HELP_FONT,

    /* Heading fonts, styles: 'R', 'S', 'T', 'U', 'V', 'W' */
    H1_HELP_FONT, H2_HELP_FONT,  H3_HELP_FONT, H4_HELP_FONT, H5_HELP_FONT,
    H6_HELP_FONT,
};

#define N_STYLES (sizeof( StyleFonts ) / sizeof( char * ))

static styleTableEntry HelpStyleInfo[ N_STYLES ];

/*============================================================================*/
/*                             PROGRAM PROTOTYPES                             */
/*============================================================================*/

static Widget createHelpPanel(Widget parent, int topic);
static void dismissCB(Widget w, XtPointer clientData, XtPointer callData);
static void searchHelpCB(Widget w, XtPointer clientData, XtPointer callData);
static void searchHelpAgainCB(Widget w, XtPointer clientData,
	XtPointer callData);
static void printCB(Widget w, XtPointer clientData, XtPointer callData);
static void hyperlinkEH(Widget w, XtPointer data, XEvent *event,
	Boolean *continueDispatch);
static void searchHelpText(Widget parent, int parentTopic, const char *searchFor,
	int allSections, int startPos, int startTopic);
static int findTopicFromShellWidget(Widget shellWidget);
static void loadFontsAndColors(Widget parent, int style);

/*============================================================================*/
/*================================= PROGRAMS =================================*/
/*============================================================================*/

static char *  getBuildInfo()
{
    char * bldFormat =
        "\n%s\n"
        "     Built on: %s, %s, %s\n"
        "     Built at: %s, %s\n"
        "   With Motif: %d [%s]\n"
        "Running Motif: %d\n"
        "       Server: %s %d\n"
        ;

    char * bldInfoString = XtMalloc( strlen( bldFormat ) + 1024);

    if( bldInfoString == NULL ) {
       fputs( "nedit: memory corrupted!\n", stderr );
       exit( EXIT_FAILURE);
    }

    sprintf(bldInfoString, bldFormat,
            NEditVersion,
            COMPILE_OS, COMPILE_MACHINE, COMPILE_COMPILER,
            linkdate, linktime,
            XmVersion, XmVERSION_STRING,
            xmUseVersion,
            ServerVendor(TheDisplay),
            VendorRelease(TheDisplay));

    return bldInfoString;
}

/*----------------------------------------------------------------------------*/

void initHelpStyles( Widget parent )
{
    static int styleTableInitialized = False;
    
    if( ! styleTableInitialized )
    {
        Pixel black = BlackPixelOfScreen(XtScreen(parent));
        int   styleIndex;
        char ** line;

        for( styleIndex = 0; styleIndex < STL_HD + MAX_HEADING; styleIndex++ )
        {
            HelpStyleInfo[ styleIndex ].color     = black;
            HelpStyleInfo[ styleIndex ].underline = FALSE;
            HelpStyleInfo[ styleIndex ].font      = NULL;
        }
        
        HelpStyleInfo[ STL_LINK ].underline = TRUE;  /* for hyperlinks */

        styleTableInitialized  = True;

        /*-------------------------------------------------------
        * Only attempt to add build information to version text
        * when string formatting symbols are present in the text.
        * This special case is needed to incorporate this 
        * dynamically created information into the static help.
        *-------------------------------------------------------*/
        for( line = HelpText[ HELP_VERSION ]; *line != NULL; *line++ )
        {
            /*--------------------------------------------------
            * If and when this printf format is found in the
            * version help text, replace that line with the
            * build information. Then stitching the help text
            * will have the final count of characters to use.
            *--------------------------------------------------*/
            if( strstr( *line, "%s" ) != NULL )
            {
                char * bldInfo  = getBuildInfo();
                char * text     = XtMalloc( strlen( *line ) + strlen( bldInfo ));
                sprintf( text, *line, bldInfo );
                *line = text;
                XtFree( bldInfo );
                break;
            }
        }
    }
}

/*-----------------------------------------------------------------------
* Help fonts are not loaded until they're actually needed.  This function
* checks if the style's font is loaded, and loads it if it's not.
*-----------------------------------------------------------------------*/
static void loadFontsAndColors(Widget parent, int style)
{
    XFontStruct *font;
    if (HelpStyleInfo[style - STYLE_PLAIN].font == NULL) {
	font = XLoadQueryFont(XtDisplay(parent),
		GetPrefHelpFontName(StyleFonts[style - STYLE_PLAIN]));
	if (font == NULL) {
	    fprintf(stderr, "NEdit: help font, %s, not available\n",
		    GetPrefHelpFontName(StyleFonts[style - STYLE_PLAIN]));
	    font = XLoadQueryFont(XtDisplay(parent), "fixed");
	    if (font == NULL) {
		fprintf(stderr, "NEdit: fallback help font, \"fixed\", not "
			"available, cannot continue\n");
		exit(EXIT_FAILURE);
		return;
	    }
	}
	HelpStyleInfo[style - STYLE_PLAIN].font = font;
	if (style == STL_NM_LINK)
	    HelpStyleInfo[style - STYLE_PLAIN].color =
		    AllocColor(parent, GetPrefHelpLinkColor());
    }
}

/*----------------------------------------------------------------------------*/

char * stitch( 

    Widget  parent, 	 /* used for dynamic font/color allocation */
    char ** string_list, /* given help strings to stitch together */
    char ** styleMap     /* NULL, or a place to store help styles */
)
{
    char  *  cp;
    char  *  section, * sp;       /* resulting help text section            */
    char  *  styleData, * sdp;    /* resulting style data for text          */
    char     style = STYLE_PLAIN; /* start off each section with this style */
    int      total_size = 0;      /* help text section size                 */
    char  ** crnt_line;

    /*----------------------------------------------------
    * How many characters are there going to be displayed?
    *----------------------------------------------------*/
    for( crnt_line = string_list; *crnt_line != NULL; crnt_line++ )
    {
        for( cp = *crnt_line; *cp != EOS; cp++ )
        {
            /*---------------------------------------------
            * The help text has embedded style information
            * consisting of the style marker and a single
            * character style, for a total of 2 characters.
            * This style information is not to be included
            * in the character counting below.
            *---------------------------------------------*/
            if( *cp != STYLE_MARKER ) {
                total_size++;
            }
            else {
                cp++;  /* skipping style marker, loop will handle style */
            }
        }
    }
    
    /*--------------------------------------------------------
    * Get the needed space, one area for the help text being
    * stitched together, another for the styles to be applied.
    *--------------------------------------------------------*/
    sp  = section   = XtMalloc( total_size +1 );
    sdp = styleData = (styleMap) ? XtMalloc( total_size +1 ) : NULL;
    *sp = EOS;
    
    /*--------------------------------------------
    * Fill in the newly acquired contiguous space
    * with help text and style information.
    *--------------------------------------------*/
    for( crnt_line = string_list; *crnt_line != NULL; crnt_line++ )
    {
        for( cp = *crnt_line; *cp != EOS; cp++ )
        {
            if( *cp == STYLE_MARKER ) {
                style = *(++cp);
		loadFontsAndColors(parent, style);
            } 
            else {
                *(sp++)  = *cp;
                
                if( styleMap )
                    *(sdp++) = style;
            }
        }
    }
    
    *sp = EOS;
    
    /*-----------------------------------------
    * Only deal with style map, when available.
    *-----------------------------------------*/
    if( styleMap ) {
        *styleMap = styleData;
        *sdp      = EOS;
    }

    return section;
}

/*----------------------------------------------------------------------------*/

void Help(Widget parent, enum HelpTopic topic)
{
    if (HelpWindows[topic] != NULL)
    	RaiseShellWindow(HelpWindows[topic]);
    else
    	HelpWindows[topic] = createHelpPanel(parent, topic);
}


static Widget createHelpPanel(Widget parent, int topic)
{
    Arg al[50];
    int ac;
    Widget appShell, form, btn, dismissBtn;
    Widget sw, hScrollBar, vScrollBar;
    XmString st1;
    char * helpText  = NULL;
    char * styleData = NULL;
    
    ac = 0;
    XtSetArg(al[ac], XmNtitle, HelpTitles[topic]); ac++;
    XtSetArg(al[ac], XmNdeleteResponse, XmDO_NOTHING); ac++;
    XtSetArg(al[ac], XmNiconName, HelpTitles[topic]); ac++;
    appShell = CreateShellWithBestVis(APP_NAME, APP_CLASS,
	    applicationShellWidgetClass, TheDisplay, al, ac);
    AddSmallIcon(appShell);
    /* With openmotif 2.1.30, a crash may occur when the text widget of the
       help window is (slowly) resized to a zero width. By imposing a 
       minimum _window_ width, we can work around this problem. The minimum 
       width should be larger than the width of the scrollbar. 50 is probably 
       a safe value; this leaves room for a few characters */
    XtVaSetValues(appShell, XtNminWidth, 50, NULL);
    form = XtVaCreateManagedWidget("helpForm", xmFormWidgetClass, appShell, NULL);
    XtVaSetValues(form, XmNshadowThickness, 0, NULL);
    
    btn = XtVaCreateManagedWidget("find", xmPushButtonWidgetClass, form,
    	    XmNlabelString, st1=XmStringCreateSimple("Find..."),
    	    XmNmnemonic, 'F',
    	    XmNhighlightThickness, 0,
    	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 3,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 25, NULL);
    XtAddCallback(btn, XmNactivateCallback, searchHelpCB, appShell);
    XmStringFree(st1);

    btn = XtVaCreateManagedWidget("findAgain", xmPushButtonWidgetClass, form,
    	    XmNlabelString, st1=XmStringCreateSimple("Find Again"),
    	    XmNmnemonic, 'A',
    	    XmNhighlightThickness, 0,
    	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 27,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 49, NULL);
    XtAddCallback(btn, XmNactivateCallback, searchHelpAgainCB, appShell);
    XmStringFree(st1);

    btn = XtVaCreateManagedWidget("print", xmPushButtonWidgetClass, form,
    	    XmNlabelString, st1=XmStringCreateSimple("Print..."),
    	    XmNmnemonic, 'P',
    	    XmNhighlightThickness, 0,
    	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 51,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 73, NULL);
    XtAddCallback(btn, XmNactivateCallback, printCB, appShell);
    XmStringFree(st1);

    dismissBtn = XtVaCreateManagedWidget("dismiss", xmPushButtonWidgetClass,
	    form, XmNlabelString, st1=XmStringCreateSimple("Dismiss"),
    	    XmNhighlightThickness, 0,
    	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 75,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 97, NULL);
    XtAddCallback(dismissBtn, XmNactivateCallback, dismissCB, appShell);
    XmStringFree(st1);
            
    /* Create a text widget inside of a scrolled window widget */
    sw = XtVaCreateManagedWidget("sw", xmScrolledWindowWidgetClass,
    	    form, XmNspacing, 0, XmNhighlightThickness, 0,
	    XmNshadowThickness, 2,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_FORM,
    	    XmNrightAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_WIDGET,
	    XmNbottomWidget, dismissBtn, NULL);
    hScrollBar = XtVaCreateManagedWidget("hScrollBar",
    	    xmScrollBarWidgetClass, sw, XmNorientation, XmHORIZONTAL, 
    	    XmNrepeatDelay, 10, NULL);
    vScrollBar = XtVaCreateManagedWidget("vScrollBar",
    	    xmScrollBarWidgetClass, sw, XmNorientation, XmVERTICAL,
    	    XmNrepeatDelay, 10, NULL);
    HelpTextPanes[topic] = XtVaCreateManagedWidget("helpText",
	    textWidgetClass, sw, textNrows, 30, textNcolumns, 65,
    	    textNhScrollBar, hScrollBar, textNvScrollBar, vScrollBar,
	    textNreadOnly, True, textNcontinuousWrap, True,
	    textNautoShowInsertPos, True, NULL);
    XtVaSetValues(sw, XmNworkWindow, HelpTextPanes[topic],
	    XmNhorizontalScrollBar, hScrollBar,
    	    XmNverticalScrollBar, vScrollBar, NULL);
            
    /* Initialize help style information, if it hasn't already been init'd */
    initHelpStyles( parent );
    
    /* Put together the text to display and separate it into parallel text
       and style data for display by the widget */
    helpText = stitch( parent, HelpText[topic], &styleData );
    
    /* Stuff the text into the widget's text buffer */
    BufSetAll( TextGetBuffer( HelpTextPanes[topic] ), helpText );
    XtFree( helpText );
    
    /* Create a style buffer for the text widget and fill it with the style
       data which was generated along with the text content */
    HelpStyleBuffers[topic] = BufCreate(); 
    BufSetAll(HelpStyleBuffers[topic], styleData);
    XtFree( styleData );
    TextDAttachHighlightData(((TextWidget)HelpTextPanes[topic])->text.textD,
    	    HelpStyleBuffers[topic], HelpStyleInfo, N_STYLES, '\0', NULL, NULL);
    
    /* This shouldn't be necessary (what's wrong in text.c?) */
    HandleXSelections(HelpTextPanes[topic]);
    
    /* Process dialog mnemonic keys */
    AddDialogMnemonicHandler(form, FALSE);
    
    /* Set the default button */
    XtVaSetValues(form, XmNdefaultButton, dismissBtn, NULL);
    XtVaSetValues(form, XmNcancelButton, dismissBtn, NULL);
    
    /* realize all of the widgets in the new window */
    RealizeWithoutForcingPosition(appShell);

    /* Set up an event handler to process mouse clicks on hyperlinks */
    XtAddEventHandler( HelpTextPanes[topic], ButtonPressMask | ButtonReleaseMask,
	    False, hyperlinkEH, (XtPointer) topic );

    /* Make close command in window menu gracefully prompt for close */
    AddMotifCloseCallback(appShell, (XtCallbackProc)dismissCB, appShell);
    
    return appShell;
}


static void dismissCB(Widget w, XtPointer clientData, XtPointer callData)
{
    int topic;
    
    if ((topic = findTopicFromShellWidget((Widget)clientData)) == -1)
    	return;
    
    /* I don't understand the mechanism by which this can be called with
       HelpWindows[topic] as NULL, but it has happened */
    XtDestroyWidget(HelpWindows[topic]);
    HelpWindows[topic] = NULL;
}


static void searchHelpCB(Widget w, XtPointer clientData, XtPointer callData)
{
    char promptText[DF_MAX_PROMPT_LENGTH];
    int response, topic;
    static char **searchHistory = NULL;
    static int nHistoryStrings = 0;
    
    if ((topic = findTopicFromShellWidget((Widget)clientData)) == -1)
    	return; /* shouldn't happen */
    SetDialogFPromptHistory(searchHistory, nHistoryStrings);
    response = DialogF(DF_PROMPT, HelpWindows[topic], 3,
	    "Search for:    (use up arrow key to recall previous)",
    	    promptText, "This Section", "All Sections", "Cancel");
    if (response == 3)
    	return;
    AddToHistoryList(promptText, &searchHistory, &nHistoryStrings);
    searchHelpText(HelpWindows[topic], topic, promptText, response == 2, 0, 0);
}


static void searchHelpAgainCB(Widget w, XtPointer clientData,
	XtPointer callData)
{
    int topic;
    
    if ((topic = findTopicFromShellWidget((Widget)clientData)) == -1)
    	return; /* shouldn't happen */
    searchHelpText(HelpWindows[topic], topic, LastSearchString,
	    LastSearchWasAllTopics, LastSearchPos, LastSearchTopic);
}


static void printCB(Widget w, XtPointer clientData, XtPointer callData)
{
    int topic, helpStringLen;
    char *helpString;
    
    if ((topic = findTopicFromShellWidget((Widget)clientData)) == -1)
    	return; /* shouldn't happen */
    helpString = TextGetWrapped(HelpTextPanes[topic], 0,
	    TextGetBuffer(HelpTextPanes[topic])->length, &helpStringLen);
    PrintString(helpString, helpStringLen, HelpWindows[topic],
	    HelpTitles[topic]);
}

/*----------------------------------------------------------------------------*/

int is_known_link( char * link_name, int * topic, Href ** target )
{
    Href * hypertext;
    
    /*------------------------------
    * Direct topic links found here.
    *------------------------------*/
    for( *topic=0; HelpTitles[*topic] != NULL; (*topic)++ )
    {
        if( strcmp( link_name, HelpTitles[*topic] ) == 0 )
        {
            *target = NULL;
            return 1;
        }
    }
    
    /*------------------------------------
    * Links internal to topics found here.
    *------------------------------------*/
    for( hypertext = &H_R[0]; hypertext != NULL; hypertext = hypertext->next )
    {
        if( strcmp( link_name, hypertext->source ) == 0 )
        {
            *topic  = hypertext->topic;
            *target = hypertext;
            return 1;
        }
    }

   return 0;
}

/*----------------------------------------------------------------------------*/

void follow_hyperlink( 

    textDisp * textD,
    int        charPosition,
    int        topic
)
{
    char * link_text;
    int    link_topic;
    Href * target  = NULL;
    int end        = charPosition;
    int begin      = charPosition;
    char whatStyle = BufGetCharacter(textD->styleBuffer, end);
    
    /*--------------------------------------------------
    * Locate beginning and ending of current text style.
    *--------------------------------------------------*/
    while( whatStyle == BufGetCharacter(textD->styleBuffer, ++end) );
    while( whatStyle == BufGetCharacter(textD->styleBuffer, begin-1) ) begin--;

    link_text = BufGetRange( textD->buffer, begin, end );
    
    if( is_known_link( link_text, &link_topic, &target ) )
    {
        Help( HelpWindows[topic], link_topic );
        
        if( target != NULL )
        {
            TextSetCursorPos(HelpTextPanes[link_topic], target->location);
        }
    }
    XtFree( link_text );
}

/*----------------------------------------------------------------------------*/

static void hyperlinkEH(Widget w, XtPointer data, XEvent *event,
	Boolean *continueDispatch)
{
    XButtonEvent *e = (XButtonEvent *)event;
    int topic = (int)data;
    textDisp *textD = ((TextWidget)HelpTextPanes[topic])->text.textD;
    int clickedPos;
    static int pressX=0, pressY=0;
    
    /* On initial button press, just record coordinates */
    if (e->type == ButtonPress && e->button == 1) {
	pressX = e->x;
	pressY = e->y;
	return;
    }
    
    /* On button release, within CLICK_THRESHOLD of button press,
       look for link */
    if (e->type != ButtonRelease || e->button != 1 ||
	    abs(pressX - e->x) > CLICK_THRESHOLD ||
	    abs(pressY - e->y) > CLICK_THRESHOLD)
	return;
    clickedPos = TextDXYToCharPos(textD, e->x, e->y);
    if (BufGetCharacter(textD->styleBuffer, clickedPos) != STL_NM_LINK)
    	return;
    
    /* Button was pressed on a hyperlink.  ... Lookup and dispatch */
    follow_hyperlink( textD, clickedPos, topic );
}

static void searchHelpText(Widget parent, int parentTopic, const char *searchFor,
	int allSections, int startPos, int startTopic)
{    
    int topic, beginMatch, endMatch;
    int found = False;
    char * helpText  = NULL;
    char * styleData = NULL;
    
    /* Search for the string */
    for (topic=startTopic; topic<NUM_TOPICS; topic++) {
	if (!allSections && topic != parentTopic)
	    continue;
        helpText = stitch( parent, HelpText[topic], &styleData );
	if (SearchString(helpText, searchFor, SEARCH_FORWARD,
		SEARCH_LITERAL, False, topic == startTopic ? startPos : 0,
		&beginMatch, &endMatch, NULL, GetPrefDelimiters())) {
	    found = True;
	    break;
	}
        XtFree( helpText );
        XtFree( styleData );
    }
    if (!found) {
	if (startPos != 0 || (allSections && startTopic != 0)) { /* Wrap search */
	    searchHelpText(parent, parentTopic, searchFor, allSections, 0, 0);
	    return;
    	}
	DialogF(DF_INF, parent, 1, "String Not Found", "Dismiss");
	return;
    }
    
    /* If the appropriate window is already up, bring it to the top, if not,
       make the parent window become this topic */
    if (HelpWindows[topic] == NULL) {
	XtVaSetValues(HelpWindows[parentTopic], XmNtitle, HelpTitles[topic],
		NULL);
	HelpWindows[topic] = HelpWindows[parentTopic];
	HelpWindows[parentTopic] = NULL;
	HelpStyleBuffers[topic] = HelpStyleBuffers[parentTopic];
	HelpStyleBuffers[parentTopic] = NULL;
 	HelpTextPanes[topic] = HelpTextPanes[parentTopic];
	HelpTextPanes[parentTopic] = NULL;
	TextDAttachHighlightData(((TextWidget)HelpTextPanes[topic])->text.textD,
    		NULL, NULL, 0, '\0', NULL, NULL);
	BufSetAll( TextGetBuffer( HelpTextPanes[topic] ), helpText );
	XtFree( helpText );
	BufSetAll(HelpStyleBuffers[topic], styleData);
	XtFree( styleData );
	TextDAttachHighlightData(((TextWidget)HelpTextPanes[topic])->text.textD,
    		HelpStyleBuffers[topic], HelpStyleInfo, N_STYLES, '\0',
		NULL, NULL);
   } else if (topic != parentTopic)
	RaiseShellWindow(HelpWindows[topic]);
    BufSelect(TextGetBuffer(HelpTextPanes[topic]), beginMatch, endMatch);
    TextSetCursorPos(HelpTextPanes[topic], endMatch);
    
    /* Save the search information for search-again */
    strcpy(LastSearchString, searchFor);
    LastSearchTopic = topic;
    LastSearchPos = endMatch;
    LastSearchWasAllTopics = allSections;
}


static int findTopicFromShellWidget(Widget shellWidget)
{
    int i;
    
    for (i=0; i<NUM_TOPICS; i++)
	if (shellWidget == HelpWindows[i])
	    return i;
    return -1;
}

#if XmVersion == 2000
/* amai: This function may be called before the Motif part
         is being initialized. The following, public interface
         is known to initialize at least xmUseVersion.
	 That interface is declared in <Xm/Xm.h> in Motif 1.2 only.
	 As for Motif 2.1 we don't need this call anymore.
	 This also holds for the Motif 2.1 version of LessTif
	 releases > 0.93.0. */
extern void XmRegisterConverters(void);
#endif

/* Print version info to stdout */
void PrintVersion(void) {

    char *text;
  
#if XmVersion < 2001
    XmRegisterConverters();  /* see comment above */
#endif
    text = getBuildInfo();
    puts( text );
    XtFree( text );
}
