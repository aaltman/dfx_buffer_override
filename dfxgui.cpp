#ifndef __dfxgui
#include "dfxgui.h"
#endif

#include <stdio.h>
#include <math.h>

// this is all for the internet stuff
#if MAC
	#ifndef __InternetConfig
	#include <internetconfig.h>
	#endif
	#ifndef __Gestalt
	#include <gestalt.h>
	#endif
	#ifndef __CodeFragments
	#include <codefragments.h>
	#endif
#endif
#if WIN32
	#ifndef __shlobj
	#include <shlobj.h>
	#endif
	#ifndef __shellapi
	#include <shellapi.h>
	#endif
#endif


//-----------------------------------------------------------------------------
// Chunky Horizontal Slider
//-----------------------------------------------------------------------------
CHorizontalSliderChunky::CHorizontalSliderChunky(const CRect &size,
                          CControlListener *listener,
                          long     tag,  		// child window identifier (tag)
                          long     iMinXpos,    // min X position in pixel
                          long     iMaxXpos,    // max X position in pixel
                          long     numSteps,	// the number of value positions (chunky)
                          CBitmap  *handle,     // bitmap of slider
                          CBitmap  *background, // bitmap of background
                          CPoint   &offset,     // offset in the background
                          long     style)       // style (kLeft, kRight)
:	CHorizontalSlider(size, listener, tag, iMinXpos, iMaxXpos, handle, background, offset, style), 
	numSteps(numSteps), iMinXpos(iMinXpos), iMaxXpos(iMaxXpos)
{
	// it's all done in CHorizontalSlider()
}

//------------------------------------------------------------------------
CHorizontalSliderChunky::~CHorizontalSliderChunky()
{
	// nud
}

//------------------------------------------------------------------------
void CHorizontalSliderChunky::draw(CDrawContext *pContext)
{
  float fValue, fChunkRange = (float)(numSteps - 1);

	
	if (style & kLeft)
		fValue = value;
	else 
		fValue = 1.f - value;

	// make the value chunky
	fValue = floorf(fValue*fChunkRange) / fChunkRange;
	
	COffscreenContext *pOScreen = new COffscreenContext (pParent, widthControl, heightControl, kBlackCColor);
  
	// (re)draw background
	CRect rect (0, 0, widthControl, heightControl);
	if (pBackground)
	{
		if (bTransparencyEnabled)
			pBackground->drawTransparent(pOScreen, rect, offset);
		else
			pBackground->draw(pOScreen, rect, offset);
	}

	// calc new coords of slider
	float   fEffRange = (float)(iMaxXpos - iMinXpos);
	CRect   rectNew;

	rectNew.top    = offsetHandle.v;
	rectNew.bottom = rectNew.top + heightOfSlider;	

	rectNew.left   = offsetHandle.h + (int)(fValue * fEffRange);
	rectNew.left   = (rectNew.left < minTmp) ? minTmp : rectNew.left;

	rectNew.right  = rectNew.left + widthOfSlider;
	rectNew.right  = (rectNew.right > maxTmp) ? maxTmp : rectNew.right;

	// draw slider at new position
	if (pHandle)
	{
		if (bDrawTransparentEnabled)
			pHandle->drawTransparent(pOScreen, rectNew);
		else 
			pHandle->draw(pOScreen, rectNew);
	}

	pOScreen->copyFrom(pContext, size);
	delete pOScreen;

//	actualXPos = rectNew.left + size.left;	// in VSTGUI 2.1, but pointless?

	setDirty(false);
}



//-----------------------------------------------------------------------------
// Chunky Vertical Slider
//-----------------------------------------------------------------------------
CVerticalSliderChunky::CVerticalSliderChunky(const CRect &size,
                          CControlListener *listener,
                          long     tag,  		// child window identifier (tag)
                          long     iMinYpos,    // min Y position in pixel
                          long     iMaxYpos,    // max Y position in pixel
                          long     numSteps,	// the number of value positions (chunky)
                          CBitmap  *handle,     // bitmap of slider
                          CBitmap  *background, // bitmap of background
                          CPoint   &offset,     // offset in the background
                          long     style)       // style (kBottom, kTop)
:	CVerticalSlider(size, listener, tag, iMinYpos, iMaxYpos, handle, background, offset, style), 
	numSteps(numSteps), iMinYpos(iMinYpos), iMaxYpos(iMaxYpos)
{
	// it's all done in CVerticalSlider()
}

//------------------------------------------------------------------------
CVerticalSliderChunky::~CVerticalSliderChunky()
{
	// nud
}

//------------------------------------------------------------------------
void CVerticalSliderChunky::draw(CDrawContext *pContext)

{
  float fValue, fChunkRange = (float)(numSteps - 1);

	
	if (style & kTop)
		fValue = value;
	else 
		fValue = 1.f - value;

	// make the value chunky
	fValue = floorf(fValue*fChunkRange) / fChunkRange;
	
	COffscreenContext *pOScreen = new COffscreenContext (pParent, widthControl, heightControl, kBlackCColor);
  
	// (re)draw background
	CRect rect (0, 0, widthControl, heightControl);
	if (pBackground)
	{
		if (bTransparencyEnabled)
			pBackground->drawTransparent(pOScreen, rect, offset);
		else
			pBackground->draw(pOScreen, rect, offset);
	}
	
	// calc new coords of slider
	float   fEffRange = (float)(iMaxYpos - iMinYpos);
	CRect   rectNew;

	rectNew.left   = offsetHandle.h;
	rectNew.right  = rectNew.left + widthOfSlider;	

	rectNew.top    = offsetHandle.v + (int)(fValue * fEffRange);
	rectNew.top    = (rectNew.top < minTmp) ? minTmp : rectNew.top;

	rectNew.bottom = rectNew.top + heightOfSlider;
	rectNew.bottom = (rectNew.bottom > maxTmp) ? maxTmp : rectNew.bottom;

	// draw slider at new position
	if (pHandle)
	{
		if (bDrawTransparentEnabled)
			pHandle->drawTransparent(pOScreen, rectNew);
		else 
			pHandle->draw(pOScreen, rectNew);
	}

	pOScreen->copyFrom(pContext, size);
	delete pOScreen;

//	actualYPos = rectNew.top + size.top;	// in VSTGUI 2.1, but pointless?

	setDirty(false);
}






//------------------------------------------------------------------------
// CHorizontalAnimSlider
//------------------------------------------------------------------------
 CHorizontalAnimSlider::CHorizontalAnimSlider(const CRect &size,
                          CControlListener *listener,
                          long     tag,			// child window identifier (tag)
                          long     subPixmaps,	// number of subpixmaps
                          long     heightOfOneImage, // pixel
                          long     iMinXPos,	// min X position in pixel
                          long     iMaxXPos,	// max X position in pixel
                          CBitmap  *background,	// bitmap of background
                          CPoint   &offset,		// offset in the background
                          long     style)		// style (kLeft, kRight)
:	CHorizontalSlider(size, listener, tag, iMinXPos, iMaxXPos, 0, background, offset, style), 
	subPixmaps(subPixmaps), heightOfOneImage(heightOfOneImage)
{
	// nud
}

//------------------------------------------------------------------------
CHorizontalAnimSlider::~CHorizontalAnimSlider()
{
	// nud
}

//------------------------------------------------------------------------
void CHorizontalAnimSlider::draw(CDrawContext *pContext)
{
	CPoint where (offset.h, offset.v);

	if (value > 1.0f)
		value = 1.0f;

 	if (value > 0.0f)
		where.v += heightOfOneImage * (int)(value * (float)(subPixmaps - 1) + 0.5f);

	if (pBackground)
	{
		if (bTransparencyEnabled)
			pBackground->drawTransparent(pContext, size, where);
		else
			pBackground->draw(pContext, size, where);
	}

	setDirty(false);
}



//------------------------------------------------------------------------
// CVerticalAnimSlider
//------------------------------------------------------------------------
 CVerticalAnimSlider::CVerticalAnimSlider(const CRect &size,
                          CControlListener *listener,
                          long     tag,			// child window identifier (tag)
                          long     subPixmaps,	// number of subpixmaps
                          long     heightOfOneImage, // pixel
                          long     iMinYPos,	// min Y position in pixel
                          long     iMaxYPos,	// max Y position in pixel
                          CBitmap  *background,	// bitmap of background
                          CPoint   &offset,		// offset in the background
                          long     style)		// style (kBottom, kTop)
:	CVerticalSlider(size, listener, tag, iMinYPos, iMaxYPos, 0, background, offset, style), 
	subPixmaps(subPixmaps), heightOfOneImage(heightOfOneImage)
{
	// nud
}

//------------------------------------------------------------------------
CVerticalAnimSlider::~CVerticalAnimSlider()
{
	// nud
}

//------------------------------------------------------------------------
void CVerticalAnimSlider::draw(CDrawContext *pContext)
{
	CPoint where (offset.h, offset.v);

	if (value > 1.0f)
		value = 1.0f;

 	if (value > 0.0f)
		where.v += heightOfOneImage * (int)(value * (float)(subPixmaps - 1) + 0.5f);

	if (pBackground)
	{
		if (bTransparencyEnabled)
			pBackground->drawTransparent(pContext, size, where);
		else
			pBackground->draw(pContext, size, where);
	}

	setDirty(false);
}






//------------------------------------------------------------------------
// CNumberBox
//------------------------------------------------------------------------
CNumberBox::CNumberBox(const CRect &size, CControlListener *listener, long tag, 
							CBitmap *background, long textStyle, long controlStyle)
:	CControl(size, listener, tag, background), 
 horiTxtAlign(kCenterText), textStyle(textStyle), controlStyle(controlStyle), 
 stringConvert(0), stringConvert2(0), string2FloatConvert(0), bTextTransparencyEnabled(true)
{
	offset (0, 0);

	fontID      = kNormalFont;
	txtFace     = kNormalFace;
	fontColor   = kWhiteCColor;
	backColor   = kBlackCColor;
	frameColor  = kBlackCColor;
	shadowColor = kRedCColor;
	userData    = 0;
	if (textStyle & kNoDrawStyle)
		setDirty(false);

	zoomFactor = 10.0f;
}

//------------------------------------------------------------------------
CNumberBox::~CNumberBox()
{
	// nud
}

//------------------------------------------------------------------------
void CNumberBox::setStyle(long val)
{
	if (textStyle != val)
	{
		textStyle = val;
		setDirty();
	}
}

//------------------------------------------------------------------------
void CNumberBox::draw(CDrawContext *pContext)
{
	char string[256];

	if (stringConvert2)
	{
		string[0] = 0;
		stringConvert2(value, string, userData);
	}
	else if (stringConvert)
	{
		string[0] = 0;
		stringConvert(value, string);
	}
	else
		sprintf(string, "%2.2f", value);

	drawText(pContext, string);
}

//------------------------------------------------------------------------
void CNumberBox::drawText(CDrawContext *pContext, char *string, CBitmap *newBack)
{
	if (textStyle & kNoDrawStyle)
	{
		setDirty(false);
		return;
	}

	// draw the background
	if (newBack)
	{
		if (bTransparencyEnabled)
			newBack->drawTransparent(pContext, size, offset);
		else
			newBack->draw(pContext, size, offset);
	}
	else if (pBackground)
	{
		if (bTransparencyEnabled)
			pBackground->drawTransparent(pContext, size, offset);
		else
			pBackground->draw(pContext, size, offset);
	}
	else
	{
		if (!bTransparencyEnabled)
		{
			pContext->setFillColor(backColor);
			pContext->fillRect(size);

			if (!(textStyle & (k3DIn|k3DOut))) 
			{
				pContext->setFrameColor(frameColor);
				pContext->drawRect(size);
			}
		}
	}
	// draw the frame for the 3D effect
	if (textStyle & (k3DIn|k3DOut)) 
	{
		if (textStyle & k3DIn)
			pContext->setFrameColor(backColor);
		else
			pContext->setFrameColor(frameColor);
		CPoint p;
		pContext->moveTo(p (size.left, size.bottom));
		pContext->lineTo(p (size.left, size.top));
		pContext->lineTo(p (size.right + 1, size.top));

		if (textStyle & k3DIn)
			pContext->setFrameColor(frameColor);
		else
			pContext->setFrameColor(backColor);
		pContext->moveTo(p (size.right, size.top + 1));
		pContext->lineTo(p (size.right, size.bottom));
		pContext->lineTo(p (size.left, size.bottom));
	}

	if (!(textStyle & kNoTextStyle))
	{
//		pContext->setFont(fontID);				// v2.1
		pContext->setFont(fontID, 0, txtFace);	// v2.2
	
		// draw darker text (as shadow)
		if (textStyle & kShadowText) 
		{
			CRect newSize (size);
			newSize.offset (1, 1);
			pContext->setFontColor(shadowColor);
			pContext->drawString(string, newSize, !bTextTransparencyEnabled, horiTxtAlign);
		}
		pContext->setFontColor(fontColor);
		pContext->drawString(string, size, !bTextTransparencyEnabled, horiTxtAlign);
	}

	setDirty(false);
}

//------------------------------------------------------------------------
void CNumberBox::mouse(CDrawContext *pContext, CPoint &where)
{
	if (!bMouseEnabled)
		return;

	long button = pContext->getMouseButtons();
	if (!(button & kLButton))
		return;

	// set the default value
	if (button == (kControl|kLButton))
	{
		value = getDefaultValue();
		if (isDirty())
			listener->valueChanged(pContext, this);
		return;
	}

	float old = oldValue;
	CPoint firstPoint;
	float fEntryState = value;
	float middle = (vmax - vmin) / 2.f;
	float range = 200.f;
	float coef = (vmax - vmin) / range;
	long  oldButton = button;

	if (button & kShift)
		range *= zoomFactor;
	firstPoint = where;
	coef = (vmax - vmin) / range;

	CPoint oldWhere (-1, -1);

	// begin of edit parameter
	getParent()->beginEdit(tag);
	do
	{
		button = pContext->getMouseButtons();
		if (where != oldWhere)
		{
			oldWhere = where;

			long diff;
			if ( (controlStyle & kHorizontal) && (controlStyle & kVertical) )
				diff = (firstPoint.v - where.v) + (where.h - firstPoint.h);
			else
			{
				if (controlStyle & kHorizontal)
					diff = where.h - firstPoint.h;
				else
					diff = firstPoint.v - where.v;
			}
			if (button != oldButton)
			{
				range = 200.f;
				if (button & kShift)
					range *= zoomFactor;
	
				float coef2 = (vmax - vmin) / range;
				fEntryState += diff * (coef - coef2);
				coef = coef2;
				oldButton = button;
			}
			value = fEntryState + diff * coef;
			bounceValue();

			if (isDirty())
				listener->valueChanged(pContext, this);
		}

		pContext->getMouseLocation(where);
		doIdleStuff();
	
	} while (button & kLButton);

	// end of edit parameter
	getParent()->endEdit(tag);
}

//------------------------------------------------------------------------
void CNumberBox::setFont(CFont fontID)
{
	// to force the redraw
	if (this->fontID != fontID)
		setDirty();
	this->fontID = fontID;
}

//------------------------------------------------------------------------
void CNumberBox::setTxtFace(CTxtFace txtFace)
{
	// to force the redraw
	if (this->txtFace != txtFace)
		setDirty();
	this->txtFace = txtFace;
}

//------------------------------------------------------------------------
void CNumberBox::setFontColor(CColor color)
{
	// to force the redraw
	if (fontColor != color)
		setDirty();
	fontColor = color;
}

//------------------------------------------------------------------------
void CNumberBox::setBackColor(CColor color)
{
	// to force the redraw
	if (backColor != color)
		setDirty();
	backColor = color;
}

//------------------------------------------------------------------------
void CNumberBox::setFrameColor(CColor color)
{
	// to force the redraw
	if (frameColor != color)
		setDirty();
	frameColor = color;
}

//------------------------------------------------------------------------
void CNumberBox::setShadowColor(CColor color)
{
	// to force the redraw
	if (shadowColor != color)
		setDirty();
	shadowColor = color;
}

//------------------------------------------------------------------------
void CNumberBox::setHoriAlign(CHoriTxtAlign hAlign)
{
	// to force the redraw
	if (horiTxtAlign != hAlign)
		setDirty();
	horiTxtAlign = hAlign;
}

//------------------------------------------------------------------------
void CNumberBox::setBackOffset(CPoint &offset)
{
	this->offset = offset;
}

//------------------------------------------------------------------------
void CNumberBox::setStringConvert(void (*convert) (float value, char *string))
{
	stringConvert = convert;
}

//------------------------------------------------------------------------
void CNumberBox::setStringConvert(void (*convert) (float value, char *string,
									  void *userDta), void *userData)
{
	stringConvert2 = convert;
	this->userData = userData;
}

//------------------------------------------------------------------------
void CNumberBox::setString2FloatConvert(void (*convert) (char *string, float &output))
{
	string2FloatConvert = convert;
}






//------------------------------------------------------------------------
// CMultiToggle
//------------------------------------------------------------------------
CMultiToggle::CMultiToggle(const CRect &size, CControlListener *listener, long tag,
                                 long subPixmaps,   // number of subPixmaps
                                 long heightOfOneImage, // height of one image in pixel
                                 CBitmap *background,
                                 CPoint &offset)
: CHorizontalSwitch(size, listener, tag, subPixmaps, heightOfOneImage, 
					subPixmaps, background, offset)
{
	// nud
}

//------------------------------------------------------------------------
CMultiToggle::~CMultiToggle()
{
	// nud
}

//------------------------------------------------------------------------
void CMultiToggle::mouse(CDrawContext *pContext, CPoint &where)
{
	if (!bMouseEnabled)
		return;

	long button = pContext->getMouseButtons();
	long direction;
#if MAC
	// rightclick or ctrl+leftclick
	if ( (button & kRButton) || 
		 ((button & kLButton) && (button & kApple)) )
#else
	if (button & kRButton)	// right click reverses direction
#endif
		direction = -1;
	else if (button & kLButton)
		direction = 1;
	else
		return;
	
	// set the default value
	if (button == (kControl|kLButton))
	{
		value = getDefaultValue();
		if (isDirty())
			listener->valueChanged(pContext, this);
		return;
	}
	else
	{
		value = value + ((float)direction / (float)(subPixmaps-1));
		if (value > 1.0f)
			value = 0.0f;
		else if (value < 0.0f)
			value = 1.0f;

		if (isDirty())
			listener->valueChanged (pContext, this);
	}
}






//------------------------------------------------------------------------
// CWebLink
//------------------------------------------------------------------------
CWebLink::CWebLink(const CRect &size, CControlListener *listener, long tag, 
                            char *tempURL, CBitmap *background)
:	CControl(size, listener, tag, background), tempURL(tempURL)
{
	URL = 0;
	URL = new char[256];
	if (tempURL && URL)
		strcpy(URL, tempURL);

	mouseDown = false;
	value = 0.0f;
	error = 0;
}

//------------------------------------------------------------------------
CWebLink::~CWebLink()
{
	if (URL)
		delete[] URL;
}

//------------------------------------------------------------------------
void CWebLink::draw(CDrawContext *pContext)
{
	long offsetY;
	if (mouseDown && pBackground)
		offsetY = pBackground->getHeight() / 2;
	else
		offsetY = 0;

	if (pBackground)
	{
		if (bTransparencyEnabled)
			pBackground->drawTransparent(pContext, size, CPoint(0, offsetY));
		else
			pBackground->draw(pContext, size, CPoint(0, offsetY));
	}

	setDirty(false);
}

//------------------------------------------------------------------------
void CWebLink::mouse(CDrawContext *pContext, CPoint &where)
{
	mouseDown = true;

	if (!bMouseEnabled)
	{
		mouseDown = false;
		return;
	}

 	long button = pContext->getMouseButtons();
	if (!(button & kLButton))
	{
		mouseDown = false;
		return;
	}

	value = 1.0f;
	draw(pContext);
	openTheURL();
	listener->valueChanged(pContext, this);

	do
	{
		button = pContext->getMouseButtons();
		doIdleStuff();
	} while (button & kLButton);

	mouseDown = false;
	value = 0.0f;
	draw(pContext);
	listener->valueChanged(pContext, this);
}

//------------------------------------------------------------------------
void CWebLink::openTheURL()
{
#if MAC
  ICInstance ICconnection;
  long urlStart = 1, urlEnd, urlLength;
	#if CALL_NOT_IN_CARBON
	long gestaltResponse;
	error = Gestalt('ICAp', &gestaltResponse);
	if (error == noErr)
	#endif
		error = ICStart(&ICconnection, '????');
	if ( (error == noErr) && (ICconnection == (void*)kUnresolvedCFragSymbolAddress) )
		error = noErr + 3;
	#if CALL_NOT_IN_CARBON
	if (error == noErr)
		error = ICFindConfigFile(ICconnection, 0, nil);
	#endif
	if (error == noErr)
	{
		// get this info for the ICLaunchURL function
		urlEnd = urlLength = (long)strlen(URL) + urlStart;
		if (urlLength > 255)
			urlEnd = urlLength = 255;
		// convert the URL string into a Pascal string for the ICLaunchURL function
		char *pascalURL;
		pascalURL = new char[256];
		for (int i = 1; i < urlLength; i++)
			pascalURL[i] = URL[i-1];	// move each char up one spot in the string array...
		pascalURL[0] = (char)urlLength;	// ... & set the Pascal string length byte
		//
		error = ICLaunchURL(ICconnection, "\phttp", pascalURL, urlLength, &urlStart, &urlEnd);
		delete[] pascalURL;
	}
	if (error == noErr)
		error = ICStop(ICconnection);
#endif
#if WIN32
	ShellExecute(NULL, "open", URL, NULL, NULL, SW_SHOWNORMAL);
#endif
}






//-----------------------------------------------------------------------------
// Fine Tune Button
//-----------------------------------------------------------------------------
CFineTuneButton::CFineTuneButton(const CRect &size, CControlListener *listener, long tag,
                                 long heightOfOneImage, // height of one image in pixel
                                 CBitmap *background,
                                 CPoint &offset, 
                                 long style)
:	CKickButton(size, listener, tag, heightOfOneImage, background, offset), 
	style(style)
{
	increment = 0.0001f;
	if (style == kFineDown)
		increment = -increment;

	value = 0.0f;
	mouseDown = false;
}

//------------------------------------------------------------------------
CFineTuneButton::~CFineTuneButton()
{
	// nud
}

//------------------------------------------------------------------------
void CFineTuneButton::draw(CDrawContext *pContext)
{
	if (pBackground)
	{
		CPoint where (offset.h, offset.v);
		if (mouseDown)
			where.v += pBackground->getHeight() / 2;

		if (bTransparencyEnabled)
			pBackground->drawTransparent(pContext, size, where);
		else
			pBackground->draw(pContext, size, where);
	}

	setDirty(false);
}

//------------------------------------------------------------------------
void CFineTuneButton::mouse(CDrawContext *pContext, CPoint &where)
{
	mouseDown = true;

	if (!bMouseEnabled)
	{
		mouseDown = false;
		return;
	}
	
	long button = pContext->getMouseButtons();
	if (!(button & kLButton))
	{
		mouseDown = false;
		return;
	}

	value += increment;
	bounceValue();
	listener->valueChanged(pContext, this);

	do
	{
		if ( (where.h >= size.left && where.v >= size.top) &&
		     (where.h <= size.right && where.v <= size.bottom) )
			mouseDown = true;
		else
			mouseDown = false;

		pContext->getMouseLocation(where);
		button = pContext->getMouseButtons();
		doIdleStuff();
	} while (button & kLButton);

	mouseDown = false;
	draw(pContext);
	listener->valueChanged(pContext, this);
}

//------------------------------------------------------------------------
void CFineTuneButton::setIncrement(float newIncrement)
{
	increment = fabsf(newIncrement);
	if (style == kFineDown)
		increment = -increment;
}

//------------------------------------------------------------------------
void CFineTuneButton::setStyle(long newStyle)
{
	style = newStyle;
	increment = fabsf(increment);
	if (style == kFineDown)
		increment = -increment;
}
