//=============================================================================
// Destroy FX GUI objects by Marc Poirier
// --------------------------------------
// * CHorizontalSliderChunky
// * CVerticalSliderChunky
// * CHorizontalAnimSlider
// * CVerticalAnimSlider
// * CNumberBox
// * CMultiToggle
// * CWebLink
// * CFineTuneButton
//=============================================================================

#ifndef __dfxgui
#define __dfxgui

#ifndef __vstgui__
#include "vstgui.h"
#endif


//-----------------------------------------------------------------------------
//    Chunky Horizontal Slider
// a CHorizontalSlider replacement that "clicks" into each of its value positions
//-----------------------------------------------------------------------------
class CHorizontalSliderChunky : public CHorizontalSlider
{
public:
	CHorizontalSliderChunky(const CRect &size, CControlListener *listener, long tag, 
                       long    iMinXpos,    // min X position in pixel
                       long    iMaxXpos,    // max X position in pixel
                       long    numSteps,	// the number of value positions (chunky)
                       CBitmap *handle,     // bitmap slider
                       CBitmap *background, // bitmap background	
                       CPoint  &offset,
                       long    style = kLeft); // style (kRight, kLeft));
  
	virtual ~CHorizontalSliderChunky();
  
	virtual void draw(CDrawContext*);

protected:
	// these are sort of dummy variables, they are doubles of 
	// variables that are in CHorizontalSlider, but they have 
	// different names in VSTGUI 2.2 than they do in earlier 
	// versions, so I've made my own versions to avoid problems
	// (in VSTGUI, they are called iMinPos & iMaxPos, but in 
	// earlier VSTGUI versions they are called iMinXPos & iMaxXPos)
	long iMinXpos;	// min X position in pixel
	long iMaxXpos;	// max X position in pixel

	long numSteps;
};


//-----------------------------------------------------------------------------
//    Chunky Vertical Slider
// a CVerticalSlider replacement that "clicks" into each of its value positions
//-----------------------------------------------------------------------------
class CVerticalSliderChunky : public CVerticalSlider
{
public:
	CVerticalSliderChunky(const CRect &size, CControlListener *listener, long tag, 
                       long    iMinYpos,    // min Y position in pixel
                       long    iMaxYpos,    // max Y position in pixel
                       long    numSteps,	// the number of value positions (chunky)
                       CBitmap *handle,     // bitmap slider
                       CBitmap *background, // bitmap background	
                       CPoint  &offset,
                       long    style = kBottom); // style (kTop, kBottom));
  
	virtual ~CVerticalSliderChunky();
  
	virtual void draw(CDrawContext*);

protected:
	// these are sort of dummy variables, they are doubles of 
	// variables that are in CVerticalSlider, but they have 
	// different names in VSTGUI 2.2 than they do in earlier 
	// versions, so I've made my own versions to avoid problems
	// (in VSTGUI, they are called iMinPos & iMaxPos, but in 
	// earlier VSTGUI versions they are called iMinYPos & iMaxYPos)
	long iMinYpos;	// min Y position in pixel
	long iMaxYpos;	// max Y position in pixel

	long numSteps;
};



//-----------------------------------------------------------------------------
//    Horizontal Animated Slider
// a CHorizontalSlider replacement that displays a bitmap sequence rather than 
// a bitmap handle moving across a background
//-----------------------------------------------------------------------------
class CHorizontalAnimSlider : public CHorizontalSlider
{
public:
	CHorizontalAnimSlider(const CRect &size, CControlListener *listener, long tag, 
                       long    subPixmaps,	// number of subpixmaps
                       long    heightOfOneImage, // pixel 
                       long    iMinXPos,    // min X position in pixel
                       long    iMaxXPos,    // max X position in pixel
                       CBitmap *background, // bitmap background	
                       CPoint  &offset,
                       long    style = kLeft); // style (kRight, kLeft));
  
	virtual ~CHorizontalAnimSlider();
  
	virtual void draw(CDrawContext*);
//	virtual void mouse(CDrawContext *pContext, CPoint &where);

protected:
	long heightOfOneImage;
	long subPixmaps;
};


//-----------------------------------------------------------------------------
//    Vertical Animated Slider
// a CVerticalSlider replacement that displays a bitmap sequence rather than 
// a bitmap handle moving across a background
//-----------------------------------------------------------------------------
class CVerticalAnimSlider : public CVerticalSlider
{
public:
	CVerticalAnimSlider(const CRect &size, CControlListener *listener, long tag, 
                       long    subPixmaps,	// number of subpixmaps
                       long    heightOfOneImage, // pixel 
                       long    iMinYPos,    // min Y position in pixel
                       long    iMaxYPos,    // max Y position in pixel
                       CBitmap *background, // bitmap background	
                       CPoint  &offset,
                       long    style = kBottom); // style (kBottom, kTop));
  
	virtual ~CVerticalAnimSlider();
  
	virtual void draw(CDrawContext*);
//	virtual void mouse(CDrawContext *pContext, CPoint &where);

protected:
	long heightOfOneImage;
	long subPixmaps;
};



//-----------------------------------------------------------------------------
//    Number Box
// a CParamDisplay that you can click on & drag up & down or left & right to 
// change the associated parameter's value (like the number boxes in Max/MSP)
//-----------------------------------------------------------------------------
class CNumberBox : public CControl
{
public:
	CNumberBox(const CRect &size, CControlListener *listener, long tag, 
						CBitmap *background = 0, 
						long textStyle = 0,	// style (kShadowText, k3DIn, k3DOut, kNoTextStyle, kNoDrawStyle)
						long controlStyle = kVertical);	// style (kHorizontal, kVertical, or both)
	virtual ~CNumberBox();
	
	virtual void setFont(CFont fontID);
	CFont getFont() { return fontID; }

	virtual void setFontColor(CColor color);
	CColor getFontColor() { return fontColor; }

	virtual void setBackColor(CColor color);
	CColor getBackColor() { return backColor; }

	virtual void setFrameColor(CColor color);
	CColor getFrameColor() { return frameColor; }

	virtual void setShadowColor(CColor color);
	CColor getShadowColor() { return shadowColor; }

	virtual void setHoriAlign(CHoriTxtAlign hAlign);
	virtual void setBackOffset(CPoint &offset);

	virtual void setStringConvert(void (*convert) (float value, char *string));
	virtual void setStringConvert(void (*convert) (float value, char *string, void *userDta),
									void *userData);
	virtual void setString2FloatConvert(void (*convert) (char *string, float &output));

	virtual void setStyle(long val);
	long getStyle() { return textStyle; }
	virtual void setControlStyle(long val) { controlStyle = val; }
	virtual long getControlStyle() { return controlStyle; }

	virtual void setTxtFace(CTxtFace val);
	CTxtFace getTxtFace() { return txtFace; }

	virtual void draw(CDrawContext *pContext);
	virtual void mouse(CDrawContext *pContext, CPoint &where);

	virtual void setTextTransparency(bool val) { bTextTransparencyEnabled = val; }
	bool getTextTransparency() { return bTextTransparencyEnabled; }

	virtual void  setZoomFactor(float val) { zoomFactor = val; }
	virtual float getZoomFactor() { return zoomFactor; }

protected:
	void drawText(CDrawContext *pContext, char *string, CBitmap *newBack = 0);

	void (*stringConvert) (float value, char *string);
	void (*stringConvert2) (float value, char *string, void *userData);
	void (*string2FloatConvert) (char *string, float &output);
	void  *userData;

	CHoriTxtAlign horiTxtAlign;
	long    textStyle;	// style of text rendering
	long    controlStyle;	// orientation of mouse control

	CFont   fontID;
	CTxtFace txtFace;
	CColor  fontColor;
	CColor  backColor;
	CColor  frameColor;
	CColor  shadowColor;
	CPoint  offset;
	bool    bTextTransparencyEnabled;

	float zoomFactor;
};



//-----------------------------------------------------------------------------
//    Multi Toggle
// like a COnOffButton except that it can have more than 2 states, 
// & you flip through the states incrementally each time you click on it
//-----------------------------------------------------------------------------
class CMultiToggle : public CHorizontalSwitch
{
public:
	CMultiToggle(const CRect &size, CControlListener *listener, long tag, 
                       long subPixmaps,        // number of subPixmaps
                       long heightOfOneImage,  // pixel
                       CBitmap *background,
                       CPoint &offset);
	virtual	~CMultiToggle();

	virtual void mouse(CDrawContext *pContext, CPoint &where);

protected:
};



//-----------------------------------------------------------------------------
//    Web Page Link
// click in its space & the default browser is sent an URL
// can be an on/off style button or just a clickable area
//-----------------------------------------------------------------------------
class CWebLink : public CControl
{
public:
	CWebLink(const CRect &size, CControlListener *listener, long tag, 
	              char *URL, CBitmap *background = 0);
	virtual ~CWebLink();

	virtual void draw(CDrawContext *pContext);
	virtual void mouse(CDrawContext *pContext, CPoint &where);
	virtual char * getURL() { return URL; }
	virtual void setURL(char *newURL) { strcpy(URL, newURL); }
	virtual long getError() { return error; }
	virtual bool mouseIsDown() { return mouseDown; }

protected:
	void openTheURL();

	bool mouseDown;
	char *URL;
	char *tempURL;
	long error;
};



//-----------------------------------------------------------------------------
//    Fine Tune Button
// like a CKickButton, but it increases or decreases its parameter 
// by a very small amount, either up or down
//-----------------------------------------------------------------------------

enum	// fine tuning directions
{
	kFineUp,
	kFineDown
};

class CFineTuneButton : public CKickButton
{
public:
	CFineTuneButton(const CRect &size, CControlListener *listener, long tag, 
                    long heightOfOneImage,  // pixel
                    CBitmap *background,
                    CPoint &offset, 
                    long style);
	virtual	~CFineTuneButton();

	virtual void mouse(CDrawContext *pContext, CPoint &where);
	virtual void draw(CDrawContext *pContext);

	virtual void setIncrement(float newIncrement);
	virtual float getIncrement() { return increment; }
	virtual void setStyle(long newStyle);
	virtual long getStyle() { return style; }

protected:
	float increment;
	long style;
	bool mouseDown;
};


#endif
