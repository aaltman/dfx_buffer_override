#include <math.h>

#ifndef __dfxguiMultiControls
#include "dfxguiMultiControls.h"
#endif


//------------------------------------------------------------------------
// CMultiControl
//------------------------------------------------------------------------
CMultiControl::CMultiControl(const CRect &size, CControlListener *listener, 
 long numControlParams, long *pTags, CBitmap *pBackground)
:	CControl (size, listener, 0, pBackground), 
	pTags(pTags), numControlParams(numControlParams)
{
  long i;

	tags = 0;
	values = 0;
	oldValues = 0;
	defaultValues = 0;
	vmins = 0;
	vmaxs = 0;
	tags = new long[numControlParams];
	values = new float[numControlParams];
	oldValues = new float[numControlParams];
	defaultValues = new float[numControlParams];
	vmins = new float[numControlParams];
	vmaxs = new float[numControlParams];

	if (pTags)
	{
		for (i=0; i < numControlParams; i++)
			tags[i] = pTags[i];
		CControl::setTag(tags[0]);
	}
	for (i=0; i < numControlParams; i++)
	{
		values[i] = 0.0f;
		oldValues[i] = 1.0f;
		defaultValues[i] = 0.5f;
		vmins[i] = 0.0f;
		vmaxs[i] = 1.0f;
	}
}

//------------------------------------------------------------------------
CMultiControl::~CMultiControl()
{
	if (tags)
		delete[] tags;
	if (values)
		delete[] values;
	if (oldValues)
		delete[] oldValues;
	if (defaultValues)
		delete[] defaultValues;
	if (vmins)
		delete[] vmins;
	if (vmaxs)
		delete[] vmaxs;
}

//------------------------------------------------------------------------
bool CMultiControl::isDirty()
{
	if (CView::isDirty())
		return true;
	for (long i=0; i < numControlParams; i++)
	{
		if (oldValues[i] != values[i])
			return true;
	}
	return false;
}

//------------------------------------------------------------------------
void CMultiControl::setDirty(const bool val)
{
  long i;

	CView::setDirty(val);
	if (val)
	{
		for (i=0; i < numControlParams; i++)
		{
			if (values[i] != -1.0f)
				oldValues[i] = -1.0f;
			else
				oldValues[i] = 0.0f;
		}
	}
	else
	{
		for (i=0; i < numControlParams; i++)
			oldValues[i] = values[i];
	}
}

//------------------------------------------------------------------------
void CMultiControl::bounceValue()
{
	if (values[0] > vmaxs[0])
		values[0] = vmaxs[0];
	else if (values[0] < vmins[0])
		values[0] = vmins[0];
}

//------------------------------------------------------------------------
void CMultiControl::bounceValueTagged(long tag)
{
	long i = findIndex(tag);

	if (values[i] > vmaxs[i])
		values[i] = vmaxs[i];
	else if (values[i] < vmins[i])
		values[i] = vmins[i];
}

//------------------------------------------------------------------------
long CMultiControl::findIndex(long tag)
{
	for (long i=0; i < numControlParams; i++)
	{
		if (tags[i] == tag)
			return i;
	}
	return 0;
}









//------------------------------------------------------------------------
// XYbox
//------------------------------------------------------------------------
XYbox::XYbox(const CRect &size, 
             CControlListener *listener,
             long     tagX,        // X-coordinate parameter tag
             long     tagY,        // Y-coordinate parameter tag
             long     iMinXPos,    // min X position in pixel
             long     iMaxXPos,    // max X position in pixel
             long     iMinYPos,    // min Y position in pixel
             long     iMaxYPos,    // max Y position in pixel
             CBitmap  *handle,     // bitmap of slider
             CBitmap  *background, // bitmap of background
             CPoint   &offset,     // offset in the background
             long     styleX,      // style (kLeft, kRight)
             long     styleY)      // style (kBottom, kTop)
:	CMultiControl (size, listener, 2, 0, background), 
    pHandle(handle), offset(offset), tagX(tagX), tagY(tagY), 
    iMinXPos(iMinXPos), iMaxXPos(iMaxXPos), iMinYPos(iMinYPos), iMaxYPos(iMaxYPos), 
    styleX(styleX), styleY(styleY), bFreeClick(true)//, pOScreen (0)	// 2.2b2
{
	setTagIndexed(0, tagX);
	setTagIndexed(1, tagY);

	setDrawTransparentHandle(true);

	if (pHandle)
	{
		pHandle->remember();
		handleWidth  = pHandle->getWidth();
		handleHeight = pHandle->getHeight();
	}
	else
	{
		handleWidth = 1; 
		handleHeight = 1;
	}

//	controlWidth  = size.width();
//	controlHeight = size.height();
	controlWidth  = size.right - size.left;
	controlHeight = size.bottom - size.top;

	actualXPos = iMaxXPos;	// not in 2.2b2
	actualYPos = iMaxYPos;	// not in 2.2b2

	offsetHandle (0, 0);	// not in 2.2b2

	minTmpX = iMinXPos - size.left;					// not
	maxTmpX = iMaxXPos + handleWidth - size.left;	// in
	minTmpY = iMinYPos - size.top;					// 2.2
	maxTmpY = iMaxYPos + handleHeight - size.top;	// b2

/*	minPosX = iMinPosX - size.left;		// 2.2b2 version of the above
	rangeHandleX = iMaxPosX - iMinPosX;
	minPosY = iMinPosY - size.top;
	rangeHandleY = iMaxPosY - iMinPosY;
	setOffsetHandle( CPoint (0, 0) );	*/

/*	minPosX = 0;							// or this 2.2b2 version
	minPosY = 0;
	rangeHandleX = _rangeHandleX - handleWidth;
	rangeHandleY = _rangeHandleY - handleHeight;
	setOffsetHandle (offsetHandle);	*/

	zoomFactor = 10.0f;
	mouseDown = false;
}

//------------------------------------------------------------------------
XYbox::~XYbox()
{
	if (pHandle)
		pHandle->forget();
}

// 2.2b2 things
/*
//------------------------------------------------------------------------
void CSlider::setOffsetHandle(CPoint &val)
{
	offsetHandle = val;

	minTmpX = offsetHandle.h + minPosX;
	maxTmpX = minTmpX + rangeHandleX + handleWidth;
	minTmpY = offsetHandle.v + minPosY;
	maxTmpY = minTmpY + rangeHandleY + handleHeight;
}

//-----------------------------------------------------------------------------
bool CSlider::attached(CView *parent)
{
	if (pOScreen)
		delete pOScreen;

	pOScreen = new COffscreenContext(getParent(), widthControl, heightControl, kBlackCColor);
	
	return CControl::attached(parent);
}

!!!//-----------------------------------------------------------------------------
bool CSlider::removed(CView *parent)
{
	if (pOScreen)
	{
		delete pOScreen;
		pOScreen = 0;
	}
	return CControl::removed(parent);
}
*/

//------------------------------------------------------------------------
void XYbox::draw(CDrawContext *pContext)
{
  float fValueX, fValueY;

	if (styleX & kLeft)
		fValueX = values[0];
	else 
		fValueX = 1.0f - values[0];
	if (styleY & kTop)
		fValueY = values[1];
	else 
		fValueY = 1.0f - values[1];
	
	// not in 2.2b2
	COffscreenContext *pOScreen = new COffscreenContext(getParent(), controlWidth, controlHeight, kBlackCColor);
  
	// (re)draw background
	CRect rect(0, 0, controlWidth, controlHeight);
	if (pBackground)
	{
		if (bTransparencyEnabled)
			pBackground->drawTransparent(pOScreen, rect, offset);
		else
			pBackground->draw(pOScreen, rect, offset);
	}

	// calculate new coordinates of box
	float fEffRangeX = (float)(iMaxXPos - iMinXPos);	// is rangeHandle in 2.2b2
	float fEffRangeY = (float)(iMaxYPos - iMinYPos);	// is rangeHandle in 2.2b2
	CRect rectNew;

//	rectNew.top    = offsetHandle.v;
//	rectNew.bottom = rectNew.top + handleHeight;	

	rectNew.left   = offsetHandle.h + (int)(fValueX * fEffRangeX);
	rectNew.left   = (rectNew.left < minTmpX) ? minTmpX : rectNew.left;

	rectNew.right  = rectNew.left + handleWidth;
	rectNew.right  = (rectNew.right > maxTmpX) ? maxTmpX : rectNew.right;

//	rectNew.left   = offsetHandle.h;
//	rectNew.right  = rectNew.left + handleWidth;	

	rectNew.top    = offsetHandle.v + (int)(fValueY * fEffRangeY);
	rectNew.top    = (rectNew.top < minTmpY) ? minTmpY : rectNew.top;

	rectNew.bottom = rectNew.top + handleHeight;
	rectNew.bottom = (rectNew.bottom > maxTmpY) ? maxTmpY : rectNew.bottom;

	// draw slider at new position
	if (pHandle)
	{
		if (bDrawTransparentEnabled)
			pHandle->drawTransparent(pOScreen, rectNew);
		else 
			pHandle->draw(pOScreen, rectNew);
	}

	pOScreen->copyFrom(pContext, size);
	delete pOScreen;	// not in 2.2b2

	actualXPos = rectNew.left + size.left;	// not in 2.2b2
	actualYPos = rectNew.top + size.top;

	setDirty(false);
}

//------------------------------------------------------------------------
void XYbox::mouse(CDrawContext *pContext, CPoint &where)
{
	mouseDown = true;

	if (!bMouseEnabled)
	{
		mouseDown = false;
		return;
	}

	long button = pContext->getMouseButtons();

/* Mac --> kControl is command, kAlt is option, kApple is control, & kShift is indeed shift */

	// set the default value
#if MAC
 	if (button == (kControl|kLButton))
#else
 	if (button == (kControl|kAlt|kLButton))
#endif
	{
		values[0] = getDefaultValueTagged(tagX);
		values[1] = getDefaultValueTagged(tagY);
		if (isDirty())
			listener->valueChanged(pContext, this);
		return;
	}

	// allow left mouse button only
	if ( !(button & kLButton) )
	{
		return;
		mouseDown = false;
	}

	long xDelta = iMinXPos;
	long yDelta = iMinYPos;
//	long xDelta = size.left + offsetHandle.h;	// 2.2b2
//	long yDelta = size.top  + offsetHandle.v;	// 2.2b2
	if (!bFreeClick)
	{
/*		float fValueX, fValueY;		// 2.2b2 stuff
		if (style & kLeft)
			fValueX = values[0];
		else 
			fValueX = 1.0f - values[0];
		if (style & kTop)
			fValueY = values[1];
		else 
			fValueY = 1.0f - values[1];
*/
		CRect rect;

//		long actualXPos = offsetHandle.h + (int)(fValueX * rangeHandle) + size.left;	// 2.2b2
//		long actualYPos = offsetHandle.v + (int)(fValueY * rangeHandle) + size.top;		// 2.2b2

		rect.left   = actualXPos;
//		rect.top    = size.top;
//		rect.left   = size.left;
		rect.top    = actualYPos;
		rect.right  = rect.left + handleWidth;
		rect.bottom = rect.top  + handleHeight;
///		rect.top    = size.top  + offsetHandle.v;	// 2.2b2
///		rect.left   = size.left  + offsetHandle.h;	// 2.2b2

		if (!where.isInside(rect))
			return;
		else
		{
			xDelta += where.h - actualXPos;
			yDelta += where.v - actualYPos;
		}
	}
	else
	{
		xDelta += handleWidth / 2;// - 1;
		yDelta += handleHeight / 2;// - 1;
	}

	float fEffRangeX = (float)(iMaxXPos - iMinXPos);	// is (float)rangeHandle in 2.2b2
	float fEffRangeY = (float)(iMaxYPos - iMinYPos);	// is (float)rangeHandle in 2.2b2
	float oldValX    = values[0];
	float oldValY    = values[1];
	long  oldButton  = button;

	// the following stuff allows you click within a handle & have the value not "jump" at all
	long clickOffsetX = 0, clickOffsetY = 0;
	if (bFreeClick)
	{
		float fHandleWidth = (float)(pHandle->getWidth()) / fEffRangeX;
		float fHandleHeight = (float)(pHandle->getHeight()) / fEffRangeY;
		float startValueX = (float)(where.h - xDelta) / fEffRangeX;
		if (styleX & kRight)
			startValueX = 1.0f - startValueX;
		float startValueY = (float)(where.v - yDelta) / fEffRangeY;
		if (styleY & kBottom)
			startValueY = 1.0f - startValueY;
		float startDiffX = startValueX - values[0];
		float startDiffY = startValueY - values[1];
		//
		if ( fabsf(startDiffX) < (fHandleWidth*0.5f) )
			clickOffsetX = (long)(startDiffX * fEffRangeX);
		if (styleX & kLeft)
			clickOffsetX *= -1;
		if ( fabsf(startDiffY) < (fHandleHeight*0.5f) )
			clickOffsetY = (long)(startDiffY * fEffRangeY);
		if (styleY & kTop)
			clickOffsetY *= -1;
	}

	// begin of edit parameter
	getParent()->beginEdit(tagX);
	getParent()->beginEdit(tagY);
	while (true)
	{
		button = pContext->getMouseButtons();
		if (!(button & kLButton))
			break;

		if ( (oldButton != button) && (button & kShift) )
		{
			oldValX = values[0];
			oldValY = values[1];
			oldButton = button;
		}
		else if (!(button & kShift))
		{
			oldValX = values[0];
			oldValY = values[1];
		}

		// option locks the X axis, so only adjust the X value if option is not being pressed 
		// (alt on PCs)
		if ( !(button & kAlt) )
		{
			values[0] = (float)(where.h - xDelta + clickOffsetX) / fEffRangeX;
			if (styleX & kRight)
				values[0] = 1.0f - values[0];
			if (button & kShift)
				values[0] = oldValX + ((values[0] - oldValX) / zoomFactor);
		}

		// control locks the Y axis, so only adjust the Y value if control is not being pressed
	#if MAC
		if ( !(button & kApple) )
	#else
		if ( !(button & kControl) )
	#endif
		{
			values[1] = (float)(where.v - yDelta + clickOffsetY) / fEffRangeY;
			if (styleY & kBottom)
				values[1] = 1.0f - values[1];
			if (button & kShift)
				values[1] = oldValY + ((values[1] - oldValY) / zoomFactor);
		}
	
		bounceValueTagged(tagX);
		bounceValueTagged(tagY);
		
		if (isDirty())
			listener->valueChanged(pContext, this);

		pContext->getMouseLocation(where);

		doIdleStuff();
	}
//	while (button == kLButton || (button == (kShift|kLButton)));	// 2.1

	mouseDown = false;

	// end of edit parameter
	getParent()->endEdit(tagX);
	getParent()->endEdit(tagY);
}

//------------------------------------------------------------------------
// it doesn't seem to make sense to support this for a 2-dimensional, 2-parameter control
bool XYbox::onWheel(CDrawContext *pContext, const CPoint &where, float distance)
{
//	if (!bMouseEnabled)
		return false;
/*
	long buttons = pContext->getMouseButtons();
	if (buttons & kShift)
		value += 0.1f * distance * wheelInc;
	else
		value += distance * wheelInc;
	bounceValue();

	if (isDirty())
		listener->valueChanged(pContext, this);
	return true;
*/}

//------------------------------------------------------------------------
void XYbox::setHandle(CBitmap *newHandle)
{
	if (pHandle)
		pHandle->forget();
	pHandle = newHandle;
	if (pHandle)
	{
		pHandle->remember();
		handleWidth  = pHandle->getWidth();
		handleHeight = pHandle->getHeight();
	}
}






//------------------------------------------------------------------------
// CHorizontalRangeSlider
//------------------------------------------------------------------------
CHorizontalRangeSlider::CHorizontalRangeSlider(const CRect &size, 
             CControlListener *listener,
             long     minTag,      // lower value parameter tag
             long     maxTag,      // higher value parameter tag
             long     iMinXPos,    // min X position in pixel
             long     iMaxXPos,    // max X position in pixel
//             long     iMinYPos,    // min Y position in pixel
//             long     iMaxYPos,    // max Y position in pixel
             CBitmap  *handle,     // bitmap of slider
             CBitmap  *background, // bitmap of background
             CPoint   &offset,     // offset in the background
             long     directionStyle, // style (kLeft, kRight)
             long     pushStyle,   // pushing policy
             CBitmap  *handle2)    // a second upper handle for the 2-handle mode
:	CMultiControl (size, listener, 2, 0, background), 
    pHandle(handle), offset(offset), minTag(minTag), maxTag(maxTag), 
    iMinXPos(iMinXPos), iMaxXPos(iMaxXPos), pHandle2(handle2), 
    directionStyle(directionStyle), pushStyle(pushStyle), bFreeClick(true) //, pOScreen(0)	// 2.2b2
{
	setTagIndexed(0, minTag);
	setTagIndexed(1, maxTag);

	setDrawTransparentHandle(true);

	if (pHandle)
		pHandle->remember();
	if (pHandle2)
		pHandle2->remember();

//	controlWidth  = size.width();
//	controlHeight = size.height();
	controlWidth  = size.right - size.left;
	controlHeight = size.bottom - size.top;

	if (pHandle2)
	{
		if (directionStyle & kLeft)
			actualXPosMin = actualXPosMax = iMaxXPos - pHandle2->getWidth();	// not in 2.2b2
		else
			actualXPosMin = actualXPosMax = iMaxXPos - pHandle->getWidth();	// not in 2.2b2
	}
	else
		actualXPosMin = actualXPosMax = iMaxXPos;	// not in 2.2b2

	offsetHandle (0, 0);	// not in 2.2b2
	offsetHandle2 (0, 0);	// not in 2.2b2

	minTmpX = iMinXPos - size.left;	// not
	maxTmpX = iMaxXPos - size.left;	// in

/*	minPosX = iMinPosX - size.left;		// 2.2b2 version of the above
	rangeHandle = iMaxPosX - iMinPosX;
	setOffsetHandle( CPoint (0, 0) );	*/

/*	minPosX = 0;							// or this 2.2b2 version
	rangeHandle = _rangeHandle;
	setOffsetHandle (offsetHandle);	*/

	currentTag = tags;
	zoomFactor = 10.0f;
	convergeFactor = 1.0f;
	overshoot = 1;	// default to 1 pixel
	clickBetween = false;
}

//------------------------------------------------------------------------
CHorizontalRangeSlider::~CHorizontalRangeSlider()
{
	if (pHandle)
		pHandle->forget();
	if (pHandle2)
		pHandle2->forget();
}

// 2.2b2 things
/*
//------------------------------------------------------------------------
void CSlider::setOffsetHandle(CPoint &val)
{
	offsetHandle = val;

	minTmpX = offsetHandle.h + minPosX;
	maxTmpX = minTmpX + rangeHandleX + handleWidth;
}

//-----------------------------------------------------------------------------
bool CSlider::attached(CView *parent)
{
	if (pOScreen)
		delete pOScreen;

	pOScreen = new COffscreenContext(getParent(), widthControl, heightControl, kBlackCColor);
	
	return CControl::attached(parent);
}

!!!//-----------------------------------------------------------------------------
bool CSlider::removed(CView *parent)
{
	if (pOScreen)
	{
		delete pOScreen;
		pOScreen = 0;
	}
	return CControl::removed(parent);
}
*/

//------------------------------------------------------------------------
void CHorizontalRangeSlider::draw(CDrawContext *pContext)
{
  float fMinValue, fMaxValue;
  long minXpos = iMinXPos, maxXpos = iMaxXPos;


	if (directionStyle & kLeft)
	{
		fMinValue = values[0];
		fMaxValue = values[1];
		if (pHandle2)
		{
			minXpos = iMinXPos + pHandle->getWidth();
			maxXpos = iMaxXPos - pHandle2->getWidth();
		}
	}
	else
	{
		fMinValue = 1.0f - values[0];
		fMaxValue = 1.0f - values[1];
		if (pHandle2)
		{
			minXpos = iMinXPos + pHandle2->getWidth();
			maxXpos = iMaxXPos - pHandle->getWidth();
		}
	}

	// not in 2.2b2
	COffscreenContext *pOScreen = new COffscreenContext(getParent(), controlWidth, controlHeight, kBlackCColor);
  
	// (re)draw background
	CRect rect(0, 0, controlWidth, controlHeight);
	if (pBackground)
	{
		if (bTransparencyEnabled)
			pBackground->drawTransparent(pOScreen, rect, offset);
		else
			pBackground->draw(pOScreen, rect, offset);
	}

	// calculate new coordinates of box
	float fEffRange = (float)(maxXpos - minXpos);	// is rangeHandle in 2.2b2
	CRect rect1, rect2;
	CPoint where(0, 0);
	int minPixelPos = (int)(fMinValue * fEffRange);
	int maxPixelPos = (int)(fMaxValue * fEffRange);
	if ( ((directionStyle & kLeft) && (maxPixelPos <= minPixelPos)) && (!pHandle2) )
	{
		if (maxPixelPos <= 0)
		{
			maxPixelPos = 1;
			minPixelPos = 0;
		}
		else
			minPixelPos = maxPixelPos - 1;
	}
	else if ( ((directionStyle & kRight) && (minPixelPos <= maxPixelPos)) && (!pHandle2) )
	{
		if (minPixelPos <= 0)
		{
			minPixelPos = 1;
			maxPixelPos = 0;
		}
		else
			maxPixelPos = minPixelPos - 1;
	}

	rect1.top = rect2.top = offsetHandle.v;
	rect1.bottom = rect2.bottom = offsetHandle.v + controlHeight;

	if (pHandle2)
	{
		if (directionStyle & kLeft)
		{
			rect1.left = offsetHandle.h + minPixelPos;
			rect2.left = offsetHandle.h + maxPixelPos + pHandle->getWidth();
		}
		else
		{
			rect1.left = offsetHandle.h + minPixelPos + pHandle2->getWidth();
			rect2.left = offsetHandle.h + maxPixelPos;
		}
		rect1.right = rect1.left + pHandle->getWidth();
		rect2.right = rect2.left + pHandle2->getWidth();
		where (0, 0);
	}
	else
	{
		if (directionStyle & kLeft)
		{
			where.h = minPixelPos;
			rect1.left = offsetHandle.h + minPixelPos;
			rect1.right = offsetHandle.h + maxPixelPos;
		}
		else
		{
			where.h = maxPixelPos;
			rect1.left = offsetHandle.h + maxPixelPos;
			rect1.right = offsetHandle.h + minPixelPos;
		}
	}

	// draw handle(s) at new position(s)
	if (pHandle)
	{
		if (bDrawTransparentEnabled)
			pHandle->drawTransparent(pOScreen, rect1, where);
		else
			pHandle->draw(pOScreen, rect1, where);
	}
	if (pHandle2)
	{
		if (bDrawTransparentEnabled)
			pHandle2->drawTransparent(pOScreen, rect2);
		else
			pHandle2->draw(pOScreen, rect2);
	}

	pOScreen->copyFrom(pContext, size);
	delete pOScreen;	// not in 2.2b2

/*	if (directionStyle & kLeft)
	{
		actualXPosMin = rect1.left + size.left;	// not in 2.2b2
		actualXPosMax = rect1.right + size.left;	// not in 2.2b2
	}
	else
	{
		actualXPosMin = rect1.right + size.left;	// not in 2.2b2
		actualXPosMax = rect1.left + size.left;	// not in 2.2b2
	}
*/

	setDirty(false);
}

//------------------------------------------------------------------------
void CHorizontalRangeSlider::mouse(CDrawContext *pContext, CPoint &where)
{
	if (!bMouseEnabled)
		return;

	long minXpos = iMinXPos;
	long maxXpos = iMaxXPos;
	if (pHandle2)
	{
		if (directionStyle & kLeft)
		{
			minXpos = iMinXPos + pHandle->getWidth();
			maxXpos = iMaxXPos - pHandle2->getWidth();
		}
		else
		{
			minXpos = iMinXPos + pHandle2->getWidth();
			maxXpos = iMaxXPos - pHandle->getWidth();
		}
	}

	long button = pContext->getMouseButtons();

	// allow left mouse button only
	if ( !(button & kLButton) )
		return;

	// set the default value
#if MAC
 	if (button == (kControl|kLButton))	// command-click for Macs
#else
// 	if (button == (kControl|kLButton|kAlt))	// ctrl+alt for PCs
 	if (button & kRButton)	// right-click for PCs
#endif
	{
		values[0] = getDefaultValueTagged(minTag);
		values[1] = getDefaultValueTagged(maxTag);
		clickBetween = true;
		if (isDirty())
			listener->valueChanged(pContext, this);
		return;
	}

	long xDelta = minXpos;
//	long xDelta = size.left + offsetHandle.h;	// 2.2b2
	if (!bFreeClick)
	{
		float fMinValue, fMaxValue;		// 2.2b2 stuff
		if (directionStyle & kLeft)
		{
			fMinValue = values[0];
			fMaxValue = values[1];
		}
		else
		{
			fMinValue = 1.0f - values[0];
			fMaxValue = 1.0f - values[1];
		}

		actualXPosMin = offsetHandle.h + (int)(fMinValue * (float)(maxXpos - minXpos)) + size.left;	// 2.2b2
		actualXPosMax = offsetHandle.h + (int)(fMaxValue * (float)(maxXpos - minXpos)) + size.left;	// 2.2b2
//		long actualXPosMin = offsetHandle.h + (int)(fMinValue * (float)rangeHandle) + size.left;	// 2.2b2
//		long actualXPosMax = offsetHandle.h + (int)(fMaxValue * (float)rangeHandle) + size.left;	// 2.2b2

		CRect rect;

//		if (directionStyle & kLeft)
			rect.left = actualXPosMin;
//		else
//			rect.left = actualXPosMax;
		rect.right  = rect.left + controlWidth;
		rect.top    = size.top;
		rect.bottom = rect.top  + controlHeight;

///		rect.top    = size.top  + offsetHandle.v;	// 2.2b2
///		rect.left   = size.left  + offsetHandle.h;	// 2.2b2

		if (!where.isInside(rect))
			return;
		else
			xDelta += where.h - actualXPosMin;
	}
//	else	// this crap doesn't seem to work
//		xDelta += (controlWidth / 2) - 1;

	float fEffRange = (float)(maxXpos - minXpos);	// is (float)rangeHandle in 2.2b2
	float oldMinVal = values[0], startMinValue = values[0];
	float oldMaxVal = values[1], startMaxValue = values[1];
	long  oldButton = button;
	float startValue = (float)(where.h - xDelta) / fEffRange;
	long currentYpos = where.v, oldYpos = where.v;
	if (directionStyle & kRight)
		startValue = 1.0f - startValue;
	float anchorValue;	// the start value of the anchored parameter
	long anchorParam;	// the parameter index of the anchored parameter
//	float difference1 = fabsf(values[0] - startValue);
//	float difference2 = fabsf(values[1] - startValue);
	float difference1 = fabsf(startValue - values[0]);
	float difference2 = fabsf(startValue - values[1]);
	if (difference1 < difference2)
		anchorParam = 1;
	else if (difference1 == difference2)
	{
		if (startValue < values[0])
			anchorParam = 1;
		else
			anchorParam = 0;
	}
	else
		anchorParam = 0;
	anchorValue = values[anchorParam];
	currentTag = &(tags[(anchorParam==0) ? 1 : 0]);

	// this determines if the mouse click was between the 2 points (or close, by 1 pixel)
	float pixelValue = (float)overshoot / fEffRange;
	clickBetween = false;
	if ( (startValue >= (values[0]-pixelValue)) && (startValue <= (values[1]+pixelValue)) )
		clickBetween = true;

	// the following stuff allows you click within a handle & have the value not "jump" at all
	long clickOffset = 0;
	float fClickOffset = 0.0f;
	if (pHandle2)
	{
		float fHandleWidth, startDiff;
		if (anchorParam == 0)
		{
			fHandleWidth = (float)(pHandle2->getWidth()) / fEffRange;
			startDiff = startValue - values[1];
			if ( (startDiff >= 0.0f) && (startDiff <= fHandleWidth) )
			{
				// add 0.5 to round rather than truncate
				clickOffset = (long) ((startDiff * fEffRange) + 0.5f);
				fClickOffset = startDiff;
			}
			if (directionStyle == kLeft)
			{
				clickOffset *= (-1);
				fClickOffset *= (-1.0f);
			}
		}
		else
		{
			fHandleWidth = (float)pHandle->getWidth() / fEffRange;
			startDiff = values[0] - startValue;
			if ( (startDiff >= 0.0f) && (startDiff <= fHandleWidth) )
			{
				// add 0.5 to round rather than truncate
				clickOffset = (long) ((startDiff * fEffRange) + 0.5f);
				fClickOffset = startDiff;
			}
			if (directionStyle == kRight)
			{
				clickOffset *= (-1);
				fClickOffset *= (-1.0f);
			}
		}
	}

	float oldValue, newValue;
	if (directionStyle == kRight)
		oldValue = newValue = startValue - fClickOffset;
	else
		oldValue = newValue = startValue + fClickOffset;

	// this is so that valueChanged() definitely gets called on the first click, 
	// because then the correct parameter will become a "learner" if my MIDI learn mode is on
	setDirty();
	// begin of edit parameter
	getParent()->beginEdit(minTag);
	getParent()->beginEdit(maxTag);
	while (true)
	{
		button = pContext->getMouseButtons();
		if (!(button & kLButton))
			break;

		if ( (oldButton != button) && (button & kShift) )
		{
			oldMinVal = values[0];
			oldMaxVal = values[1];
			oldValue = newValue;
			oldButton = button;
		}
		else if (!(button & kShift))
		{
			oldMinVal = values[0];
			oldMaxVal = values[1];
			oldValue = newValue;
		}

		newValue = (float)(where.h - xDelta + clickOffset) / fEffRange;
		if (directionStyle & kRight)
			newValue = 1.0f - newValue;
		if (button & kShift)
			newValue = oldValue + ((newValue - oldValue) / zoomFactor);

	#if MAC
		// move both parameters to the new value
		if ( (button & kApple) && !(button & kAlt) )	// ctrl for Macs
	#else
		if ( (button & kControl) && !(button & kAlt) )	// ctrl for PCs
	#endif
			values[0] = values[1] = newValue;
/*		// ............................................................................
	#if MAC
		// the anchor is by default at the initial parameter value 
		// that was furthest from the initial new value (the first click), 
		// but ctrl+option on Mac OS switches the anchor to where 
		// the first click was, like how Max range sliders work
		// (PC keyboards don't have enough modifier keys for this additional option)
		else if ( (button & kApple) && (button & kAlt) )
		{
			if (newValue < startValue)
			{
				values[0] = newValue;
				values[1] = startValue;
			}
			else
			{
				values[0] = startValue;
				values[1] = newValue;
			}
		}
	#endif
*/
		// ............................................................................
	#if MAC
		// reverso axis convergence/divergence mode
		else if ( (button & kApple) && (button & kAlt) )	// control+option for Macs
	#else
		else if ( (button & kControl) && (button & kAlt) )	// ctrl+alt for PCs
	#endif
		{
//			long diff = currentXpos - oldXpos;
			long diff = oldYpos - currentYpos;
			float changeAmount = ((float)diff / fEffRange) * convergeFactor;
			if (button & kShift)
				changeAmount /= zoomFactor;
			values[0] -= changeAmount;
			values[1] += changeAmount;
			// prevent the max from crossing below the min & vice versa
			if (values[0] > values[1])
				values[0] = values[1] = (values[0] + values[1]) / 2.0f;
		}
		// ............................................................................
	#if MAC
		// move both parameters, preserving relationship
		else if ( ((button & kAlt) && !(button & kApple)) 	// option for Macs
	#else
		else if ( ((button & kAlt) && !(button & kControl)) 	// alt for PCs
	#endif
					|| (clickBetween) )
		{
			// the click offset stuff is annoying in this mode, so remove it
			float valueChange = newValue - startValue - fClickOffset;
			values[0] = startMinValue + valueChange;
			values[1] = startMaxValue + valueChange;
		}
		// ............................................................................
		// no key command moves both parameters to the new value & then 
		// moves the appropriate parameter after that point, anchoring at the new value
		else
		{
			if (anchorParam == 0)
			{
				if (newValue < anchorValue)
				{
					if ( (pushStyle == kMaxCanPush) || (pushStyle == kBothCanPush) )
						values[0] = values[1] = newValue;
					else
						values[0] = values[1] = anchorValue;
				}
				else
				{
					values[0] = anchorValue;
					values[1] = newValue;
				}
			}
			else
			{
				if (newValue > anchorValue)
				{
					if ( (pushStyle == kMinCanPush) || (pushStyle == kBothCanPush) )
						values[0] = values[1] = newValue;
					else
						values[0] = values[1] = anchorValue;
				}
				else
				{
					values[0] = newValue;
					values[1] = anchorValue;
				}
			}
		}

		bounceValueTagged(minTag);
		bounceValueTagged(maxTag);
		
		if (isDirty())
			listener->valueChanged(pContext, this);

		pContext->getMouseLocation(where);
		oldYpos = currentYpos;
		currentYpos = where.v;

		doIdleStuff();
	}
//	while (button == kLButton || (button == (kShift|kLButton)));	// 2.1

	// end of edit parameter
	getParent()->endEdit(minTag);
	getParent()->endEdit(maxTag);
}

//------------------------------------------------------------------------
bool CHorizontalRangeSlider::onWheel(CDrawContext *pContext, const CPoint &where, float distance)
{
	if (!bMouseEnabled)
		return false;

	long buttons = pContext->getMouseButtons();
	if (buttons & kShift)
	{
		values[0] += zoomFactor * distance * wheelInc;
		values[1] += zoomFactor * distance * wheelInc;
	}
	else
	{
		values[0] += distance * wheelInc;
		values[1] += distance * wheelInc;
	}
	bounceValueTagged(minTag);
	bounceValueTagged(maxTag);

	if (isDirty())
		listener->valueChanged(pContext, this);
	return true;
}

//------------------------------------------------------------------------
void CHorizontalRangeSlider::setHandle(CBitmap *newHandle)
{
	if (pHandle)
		pHandle->forget();
	pHandle = newHandle;
	if (pHandle)
		pHandle->remember();
}

//------------------------------------------------------------------------
void CHorizontalRangeSlider::setHandle2(CBitmap *newHandle2)
{
	if (pHandle2)
		pHandle2->forget();
	pHandle2 = newHandle2;
	if (pHandle2)
		pHandle2->remember();
}



//------------------------------------------------------------------------
// CVerticalRangeSlider
//------------------------------------------------------------------------
CVerticalRangeSlider::CVerticalRangeSlider(const CRect &size, 
             CControlListener *listener,
             long     minTag,      // lower value parameter tag
             long     maxTag,      // higher value parameter tag
             long     iMinYPos,    // min Y position in pixel
             long     iMaxYPos,    // max Y position in pixel
             CBitmap  *handle,     // bitmap of slider
             CBitmap  *background, // bitmap of background
             CPoint   &offset,     // offset in the background
             long     directionStyle, // style (kBottom, kTop)
             long     pushStyle,   // pushing policy
             CBitmap  *handle2)    // a second upper handle for the 2-handle mode
:	CMultiControl (size, listener, 2, 0, background), 
    pHandle(handle), offset(offset), minTag(minTag), maxTag(maxTag), 
    iMinYPos(iMinYPos), iMaxYPos(iMaxYPos), pHandle2(handle2), 
    directionStyle(directionStyle), pushStyle(pushStyle), bFreeClick(true) //, pOScreen(0)	// 2.2b2
{
	setTagIndexed(0, minTag);
	setTagIndexed(1, maxTag);

	setDrawTransparentHandle(true);

	if (pHandle)
		pHandle->remember();
	if (pHandle2)
		pHandle2->remember();

//	controlWidth  = size.width();
//	controlHeight = size.height();
	controlWidth  = size.right - size.left;
	controlHeight = size.bottom - size.top;

	if (pHandle2)
	{
		if (directionStyle & kBottom)
			actualYPosMin = actualYPosMax = iMaxYPos - pHandle->getHeight();	// not in 2.2b2
		else
			actualYPosMin = actualYPosMax = iMaxYPos - pHandle2->getHeight();	// not in 2.2b2
	}
	else
		actualYPosMin = actualYPosMax = iMaxYPos;	// not in 2.2b2


	offsetHandle (0, 0);	// not in 2.2b2
	offsetHandle2 (0, 0);	// not in 2.2b2

	minTmpY = iMinYPos - size.top;	// 2.2
	maxTmpY = iMaxYPos - size.top;	// b2

/*	minPosY = iMinPosY - size.top;
	rangeHandle = iMaxPosY - iMinPosY;
	setOffsetHandle( CPoint (0, 0) );	*/

/*	minPosY = 0;
	rangeHandle = _rangeHandle;
	setOffsetHandle (offsetHandle);	*/

	currentTag = tags;
	zoomFactor = 10.0f;
	convergeFactor = 1.0f;
	overshoot = 1;	// default to 1 pixel
	clickBetween = false;
}

//------------------------------------------------------------------------
CVerticalRangeSlider::~CVerticalRangeSlider()
{
	if (pHandle)
		pHandle->forget();
	if (pHandle2)
		pHandle2->forget();
}

// 2.2b2 things
/*
//------------------------------------------------------------------------
void CSlider::setOffsetHandle(CPoint &val)
{
	offsetHandle = val;

	minTmpY = offsetHandle.v + minPosY;
	maxTmpY = minTmpY + rangeHandleY + handleHeight;
}

//-----------------------------------------------------------------------------
bool CSlider::attached(CView *parent)
{
	if (pOScreen)
		delete pOScreen;

	pOScreen = new COffscreenContext(getParent(), widthControl, heightControl, kBlackCColor);
	
	return CControl::attached(parent);
}

!!!//-----------------------------------------------------------------------------
bool CSlider::removed(CView *parent)
{
	if (pOScreen)
	{
		delete pOScreen;
		pOScreen = 0;
	}
	return CControl::removed(parent);
}
*/

//------------------------------------------------------------------------
void CVerticalRangeSlider::draw(CDrawContext *pContext)
{
  float fMinValue, fMaxValue;
  long minYpos = iMinYPos, maxYpos = iMaxYPos;


	if (directionStyle & kTop)
	{
		fMinValue = values[0];
		fMaxValue = values[1];
		if (pHandle2)
		{
			minYpos = iMinYPos + pHandle->getHeight();
			maxYpos = iMaxYPos - pHandle2->getHeight();
		}
	}
	else
	{
		fMinValue = 1.0f - values[0];
		fMaxValue = 1.0f - values[1];
		if (pHandle2)
		{
			minYpos = iMinYPos + pHandle2->getHeight();
			maxYpos = iMaxYPos - pHandle->getHeight();
		}
	}

	// not in 2.2b2
	COffscreenContext *pOScreen = new COffscreenContext(getParent(), controlWidth, controlHeight, kBlackCColor);
  
	// (re)draw background
	CRect rect(0, 0, controlWidth, controlHeight);
	if (pBackground)
	{
		if (bTransparencyEnabled)
			pBackground->drawTransparent(pOScreen, rect, offset);
		else
			pBackground->draw(pOScreen, rect, offset);
	}

	// calculate new coordinates of box
	float fEffRange = (float)(maxYpos - minYpos);	// is rangeHandle in 2.2b2
	CRect rect1, rect2;
	CPoint where(0, 0);
	int minPixelPos = (int)(fMinValue * fEffRange);
	int maxPixelPos = (int)(fMaxValue * fEffRange);
	if ( ((directionStyle & kTop) && (maxPixelPos <= minPixelPos)) && (!pHandle2) )
	{
		if (maxPixelPos <= 0)
		{
			maxPixelPos = 1;
			minPixelPos = 0;
		}
		else
			minPixelPos = maxPixelPos - 1;
	}
	else if ( ((directionStyle & kBottom) && (minPixelPos <= maxPixelPos)) && (!pHandle2) )
	{
		if (minPixelPos <= 0)
		{
			minPixelPos = 1;
			maxPixelPos = 0;
		}
		else
			maxPixelPos = minPixelPos - 1;
	}

	rect1.left = rect2.left = offsetHandle.h;
	rect1.right = rect2.right = offsetHandle.h + controlWidth;

	if (pHandle2)
	{
		if (directionStyle & kTop)
		{
			rect1.top = offsetHandle.v + minPixelPos;
			rect2.top = offsetHandle.v + maxPixelPos + pHandle->getHeight();
		}
		else
		{
			rect1.top = offsetHandle.v + minPixelPos + pHandle2->getHeight();
			rect2.top = offsetHandle.v + maxPixelPos;
		}
		rect1.bottom = rect1.top + pHandle->getHeight();
		rect2.bottom = rect2.top + pHandle2->getHeight();
		rect2.bottom = offsetHandle.v + maxPixelPos - pHandle2->getHeight();
		rect1.bottom = offsetHandle.v + minPixelPos;
		where (0, 0);
	}
	else
	{
		if (directionStyle & kTop)
		{
			where.v = minPixelPos;
			rect1.top = offsetHandle.v + minPixelPos;
			rect1.bottom = offsetHandle.v + maxPixelPos;
		}
		else
		{
			where.v = maxPixelPos;
			rect1.top = offsetHandle.v + maxPixelPos;
			rect1.bottom = offsetHandle.v + minPixelPos;
		}
	}

	// draw handle(s) at new position(s)
	if (pHandle)
	{
		if (bDrawTransparentEnabled)
			pHandle->drawTransparent(pOScreen, rect1, where);
		else
			pHandle->draw(pOScreen, rect1, where);
	}
	if (pHandle2)
	{
		if (bDrawTransparentEnabled)
			pHandle2->drawTransparent(pOScreen, rect2);
		else
			pHandle2->draw(pOScreen, rect2);
	}

	pOScreen->copyFrom(pContext, size);
	delete pOScreen;	// not in 2.2b2

/*	if (directionStyle & kTop)
	{
		actualYPosMin = rectNew.top + size.top;		// not in 2.2b2
		actualYPosMax = rectNew.bottom + size.top;	// not in 2.2b2
	}
	else
	{
		actualYPosMin = rectNew.bottom + size.top;	// not in 2.2b2
		actualYPosMax = rectNew.top + size.top;		// not in 2.2b2
	}
*/
	setDirty(false);
}

//------------------------------------------------------------------------
void CVerticalRangeSlider::mouse(CDrawContext *pContext, CPoint &where)
{
	if (!bMouseEnabled)
		return;

	long minYpos = iMinYPos;
	long maxYpos = iMaxYPos;
	if (pHandle2)
	{
		if (directionStyle & kTop)
		{
			minYpos = iMinYPos + pHandle->getHeight();
			maxYpos = iMaxYPos - pHandle2->getHeight();
		}
		else
		{
			minYpos = iMinYPos + pHandle2->getHeight();
			maxYpos = iMaxYPos - pHandle->getHeight();
		}
	}

	long button = pContext->getMouseButtons();

	// allow left mouse button only
	if ( !(button & kLButton) )
		return;

	// set the default value
#if MAC
 	if (button == (kControl|kLButton))
#else
 	if (button == (kControl|kLButton|kAlt))	// ctrl+alt for PCs
#endif
	{
		values[0] = getDefaultValueTagged(minTag);
		values[1] = getDefaultValueTagged(maxTag);
		clickBetween = true;
		if (isDirty())
			listener->valueChanged(pContext, this);
		return;
	}

	long yDelta = minYpos;
//	long yDelta = size.top  + offsetHandle.v;	// 2.2b2
	if (!bFreeClick)
	{
		float fMinValue, fMaxValue;		// 2.2b2 stuff
		if (directionStyle & kTop)
		{
			fMinValue = values[0];
			fMaxValue = values[1];
		}
		else
		{
			fMinValue = 1.0f - values[0];
			fMaxValue = 1.0f - values[1];
		}

		actualYPosMin = offsetHandle.v + (int)(fMinValue * (float)(maxYpos - minYpos)) + size.top;	// 2.2b2
		actualYPosMax = offsetHandle.v + (int)(fMaxValue * (float)(maxYpos - minYpos)) + size.top;	// 2.2b2
//		long actualYPosMin = offsetHandle.v + (int)(fMinValue * (float)rangeHandle) + size.top;	// 2.2b2
//		long actualYPosMax = offsetHandle.v + (int)(fMaxValue * (float)rangeHandle) + size.top;	// 2.2b2

		CRect rect;

		rect.left   = size.left;
		rect.right  = rect.left + controlWidth;
//		if (directionStyle & kTop)
			rect.top = actualYPosMin;
//		else
//			rect.top = actualYPosMax;
		rect.bottom = rect.top  + controlHeight;

///		rect.top    = size.top  + offsetHandle.v;	// 2.2b2
///		rect.left   = size.left  + offsetHandle.h;	// 2.2b2

		if (!where.isInside(rect))
			return;
		else
			yDelta += where.v - actualYPosMin;
	}
//	else	// this crap doesn't seem to work
//		yDelta += (controlHeight / 2) - 1;

	float fEffRange = (float)(maxYpos - minYpos);	// is (float)rangeHandle in 2.2b2
	float oldMinVal = values[0], startMinValue = values[0];
	float oldMaxVal = values[1], startMaxValue = values[1];
	long  oldButton  = button;
	float startValue = (float)(where.v - yDelta) / fEffRange;
	long currentXpos = where.h, oldXpos = where.h;
	if (directionStyle & kBottom)
		startValue = 1.0f - startValue;
	float anchorValue;	// the start value of the anchored parameter
	long anchorParam;	// the parameter index of the anchored parameter
	float difference1 = fabsf(startValue - values[0]);
	float difference2 = fabsf(startValue - values[1]);
	if (difference1 < difference2)
		anchorParam = 1;
	else if (difference1 == difference2)
	{
		if (startValue < values[0])
			anchorParam = 1;
		else
			anchorParam = 0;
	}
	else
		anchorParam = 0;
	anchorValue = values[anchorParam];
	currentTag = &(tags[(anchorParam==0) ? 1 : 0]);

	// this determines if the mouse click was between the 2 points (or close, by 1 pixel)
	float pixelValue = (float)overshoot / fEffRange;
	clickBetween = false;
	if ( (startValue >= (values[0]-pixelValue)) && (startValue <= (values[1]+pixelValue)) )
		clickBetween = true;

	// the following stuff allows you click within a handle & have the value not "jump" at all
	long clickOffset = 0;	// offset within the clicked handle. in pixels
	float fClickOffset = 0.0f;	// offset within the clicked handle. in parameter 0-1 value
	if (pHandle2)
	{
		float fHandleWidth, startDiff;
		if (anchorParam == 0)
		{
			fHandleWidth = (float)(pHandle2->getWidth()) / fEffRange;
			startDiff = startValue - values[1];
			if ( (startDiff >= 0.0f) && (startDiff <= fHandleWidth) )
			{
				// add 0.5 to round rather than truncate
				clickOffset = (long) ((startDiff * fEffRange) + 0.5f);
				fClickOffset = startDiff;
			}
			if (directionStyle == kTop)
			{
				clickOffset *= (-1);
				fClickOffset *= (-1.0f);
			}
		}
		else
		{
			fHandleWidth = (float)pHandle->getWidth() / fEffRange;
			startDiff = values[0] - startValue;
			if ( (startDiff >= 0.0f) && (startDiff <= fHandleWidth) )
			{
				// add 0.5 to round rather than truncate
				clickOffset = (long) ((startDiff * fEffRange) + 0.5f);
				fClickOffset = startDiff;
			}
			if (directionStyle == kBottom)
			{
				clickOffset *= (-1);
				fClickOffset *= (-1.0f);
			}
		}
	}

	float oldValue, newValue;
	if (directionStyle == kBottom)
		oldValue = newValue = startValue - fClickOffset;
	else
		oldValue = newValue = startValue + fClickOffset;

	// this is so that valueChanged() definitely gets called on the first click, 
	// because then the correct parameter will become a "learner" if my MIDI learn mode is on
	setDirty();
	// begin of edit parameter
	getParent()->beginEdit(minTag);
	getParent()->beginEdit(maxTag);
	while (true)
	{
		button = pContext->getMouseButtons();
		if (!(button & kLButton))
			break;

		if ( (oldButton != button) && (button & kShift) )
		{
			oldMinVal = values[0];
			oldMaxVal = values[1];
			oldValue = newValue;
			oldButton = button;
		}
		else if (!(button & kShift))
		{
			oldMinVal = values[0];
			oldMaxVal = values[1];
			oldValue = newValue;
		}

		newValue = (float)(where.v - yDelta + clickOffset) / fEffRange;
		if (directionStyle & kBottom)
			newValue = 1.0f - newValue;
		if (button & kShift)
			newValue = oldValue + ((newValue - oldValue) / zoomFactor);

	#if MAC
		// & pressing control moves both parameters to the new value
		if ( (button & kApple) && !(button & kAlt) )	// ctrl for Macs
	#else
		if ( (button & kControl) && !(button & kAlt) )	// ctrl for PCs
	#endif
			values[0] = values[1] = newValue;
/*		// ............................................................................
	#if MAC
		// the anchor is by default at the initial parameter value 
		// that was furthest from the initial new value (the first click), 
		// but ctrl+option on Mac OS switches the anchor to where 
		// the first click was, like how Max range sliders work
		// (PC keyboards don't have enough modifier keys for this additional option)
		else if ( (button & kApple) && (button & kAlt) )
		{
			if (newValue < startValue)
			{
				values[0] = newValue;
				values[1] = startValue;
			}
			else
			{
				values[0] = startValue;
				values[1] = newValue;
			}
		}
	#endif
*/
		// ............................................................................
	#if MAC
		// reverso axis convergence/divergence mode
		else if ( (button & kApple) && (button & kAlt) )	// control+option for Macs
	#else
		else if ( (button & kControl) && (button & kAlt) )	// ctrl+alt for PCs
	#endif
		{
			long diff = currentXpos - oldXpos;
			float changeAmount = ((float)diff / fEffRange) * convergeFactor;
			if (button & kShift)
				changeAmount /= zoomFactor;
			values[0] -= changeAmount;
			values[1] += changeAmount;
			// prevent the max from crossing below the min & vice versa
			if (values[0] > values[1])
				values[0] = values[1] = (values[0] + values[1]) / 2.0f;
		}
		// ............................................................................
	#if MAC
		// move both parameters, preserving relationship
		else if ( ((button & kAlt) && !(button & kApple)) 	// option for Macs
	#else
		else if ( ((button & kAlt) && !(button & kControl)) 	// alt for PCs
	#endif
					|| (clickBetween) )
		{
			// the click offset stuff is annoying in this mode, so remove it
			float valueChange = newValue - startValue - fClickOffset;
			values[0] = startMinValue + valueChange;
			values[1] = startMaxValue + valueChange;
		}
		// ............................................................................
		// no key command moves both parameters to the new value & then 
		// moves the appropriate parameter after that point, anchoring at the new value
		else
		{
			if (anchorParam == 0)
			{
				if (newValue < anchorValue)
				{
					if ( (pushStyle == kMaxCanPush) || (pushStyle == kBothCanPush) )
						values[0] = values[1] = newValue;
					else
						values[0] = values[1] = anchorValue;
				}
				else
				{
					values[0] = anchorValue;
					values[1] = newValue;
				}
			}
			else
			{
				if (newValue > anchorValue)
				{
					if ( (pushStyle == kMinCanPush) || (pushStyle == kBothCanPush) )
						values[0] = values[1] = newValue;
					else
						values[0] = values[1] = anchorValue;
				}
				else
				{
					values[0] = newValue;
					values[1] = anchorValue;
				}
			}
		}

		bounceValueTagged(minTag);
		bounceValueTagged(maxTag);
		
		if (isDirty())
			listener->valueChanged(pContext, this);

		pContext->getMouseLocation(where);
		oldXpos = currentXpos;
		currentXpos = where.h;

		doIdleStuff();
	}
//	while (button == kLButton || (button == (kShift|kLButton)));	// 2.1

	// end of edit parameter
	getParent()->endEdit(minTag);
	getParent()->endEdit(maxTag);
}

//------------------------------------------------------------------------
bool CVerticalRangeSlider::onWheel(CDrawContext *pContext, const CPoint &where, float distance)
{
	if (!bMouseEnabled)
		return false;

	long buttons = pContext->getMouseButtons();
	if (buttons & kShift)
	{
		values[0] += zoomFactor * distance * wheelInc;
		values[1] += zoomFactor * distance * wheelInc;
	}
	else
	{
		values[0] += distance * wheelInc;
		values[1] += distance * wheelInc;
	}
	bounceValueTagged(minTag);
	bounceValueTagged(maxTag);

	if (isDirty())
		listener->valueChanged(pContext, this);
	return true;
}

//------------------------------------------------------------------------
void CVerticalRangeSlider::setHandle(CBitmap *newHandle)
{
	if (pHandle)
		pHandle->forget();
	pHandle = newHandle;
	if (pHandle)
		pHandle->remember();
}

//------------------------------------------------------------------------
void CVerticalRangeSlider::setHandle2(CBitmap *newHandle2)
{
	if (pHandle2)
		pHandle2->forget();
	pHandle2 = newHandle2;
	if (pHandle2)
		pHandle2->remember();
}
