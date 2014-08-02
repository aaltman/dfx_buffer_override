//=============================================================================
// Destroy FX multiple-parameter GUI objects by Marc Poirier
// ----------------------------------------------------------
// * CMultiControl --- (the parent class for the rest of the multi-controls)
// * XYbox
// * CHorizontalRangeSlider
// * CVerticalRangeSlider
//=============================================================================

#ifndef __dfxguiMultiControls
#define __dfxguiMultiControls

#ifndef __vstgui__
#include "vstgui.h"
#endif


//-----------------------------------------------------------------------------
// * CMultiControl * multiple-parameter control object class
//-----------------------------------------------------------------------------
class CMultiControl : public CControl
{
public:
	CMultiControl(const CRect &size, CControlListener *listener = 0, 
				long numControlParams = 0, long *tags = 0, CBitmap *pBackground = 0);
	virtual ~CMultiControl();

	virtual void  setValue(float val) { values[0] = val; }
	virtual void  setValueTagged(long tag, float val) { values[findIndex(tag)] = val; }
	virtual void  setValueIndexed(long index, float val) { values[index] = val; }
	virtual float getValue() { return values[0]; };
	virtual float getValueTagged(long tag) { return values[findIndex(tag)]; };
	virtual float getValueIndexed(long index) { return values[index]; };

	virtual void  setMin(float val) { vmins[0] = val; }
	virtual void  setMinTagged(long tag, float val) { vmins[findIndex(tag)] = val; }
	virtual float getMin() { return vmins[0]; }
	virtual float getMinTagged(long tag) { return vmins[findIndex(tag)]; }
	virtual void  setMax(float val) { vmaxs[0] = val; }
	virtual void  setMaxTagged(long tag, float val) { vmaxs[findIndex(tag)] = val; }
	virtual float getMax() { return vmaxs[0]; }
	virtual float getMaxTagged(long tag) { return vmaxs[findIndex(tag)]; }

	virtual void  setOldValue(float val) { oldValues[0] = val; }
	virtual void  setOldValueTagged(long tag, float val) { oldValues[findIndex(tag)] = val; }
	virtual	float getOldValue() { return oldValues[0]; }
	virtual	float getOldValueTagged(long tag) { return oldValues[findIndex(tag)]; }
	virtual void  setDefaultValue(float val) { defaultValues[0] = val; }
	virtual void  setDefaultValueTagged(long tag, float val) { defaultValues[findIndex(tag)] = val; }
	virtual	float getDefaultValue() { return defaultValues[0]; }
	virtual	float getDefaultValueTagged(long tag) { return defaultValues[findIndex(tag)]; }

	virtual void  setTag(long newTag) { tags[0] = newTag;	CControl::setTag(newTag); }
	virtual void  setTagTagged(long oldTag, long newTag) { tags[findIndex(oldTag)] = newTag; }
	virtual void  setTagIndexed(long index, long newTag) { tags[index] = newTag;
										if (index == 0) CControl::setTag(newTag); }
	inline  long  getTag() { return tags[0]; }
	virtual long  getTagIndexed(long index) { return tags[index]; }
	virtual long* getTags() { return tags; }
	virtual long  getNumControls() { return numControlParams; }

	virtual bool  isDirty();
	virtual void  setDirty(const bool val = true);

	virtual void bounceValue();
	virtual void bounceValueTagged(long tag);

protected:
	long findIndex(long tag);

	long numControlParams;
	long *tags, *pTags;
	float *oldValues;
	float *defaultValues;
	float *values;
	float *vmins;
	float *vmaxs;
};



//-----------------------------------------------------------------------------
// XY box
//-----------------------------------------------------------------------------
// X-Y box that controls 2 parameters in a 2-D plane
class XYbox : public CMultiControl
{
public:
	XYbox(const CRect &size,   CControlListener *listener, 
          long    tagX,        // X-coordinate parameter tag
          long    tagY,        // Y-coordinate parameter tag
          long    iMinXPos,    // min X position in pixel
          long    iMaxXPos,    // max X position in pixel
          long    iMinYPos,    // min Y position in pixel
          long    iMaxYPos,    // max Y position in pixel
          CBitmap *handle,     // bitmap slider
          CBitmap *background, // bitmap background	
          CPoint  &offset,
          long    styleX = kLeft, // style (kRight, kLeft));
          long    styleY = kBottom); // style (kBottom, kTop))
  
	virtual ~XYbox();

//	virtual bool attached(CView *parent);	// 2.2
//	virtual bool removed(CView *parent);	// 2.2
	virtual void draw(CDrawContext*);
	virtual void mouse(CDrawContext *pContext, CPoint &where);
	virtual bool onWheel(CDrawContext *pContext, const CPoint &where, float distance);

	virtual void setDrawTransparentHandle(bool val) { bDrawTransparentEnabled = val; }
	virtual void setFreeClick(bool val) { bFreeClick = val; }
	virtual bool getFreeClick() { return bFreeClick; }
	virtual void setOffsetHandle(CPoint &val) { offsetHandle = val; }

	virtual void     setHandle(CBitmap* newHandle);
	virtual CBitmap *getHandle() { return pHandle; }

	virtual void  setZoomFactor(float val) { zoomFactor = val; }
	virtual float getZoomFactor() { return zoomFactor; }

	virtual bool mouseIsDown() { return mouseDown; }

protected:
	CPoint   offset; 
 	CPoint   offsetHandle;

	CBitmap *pHandle;
//	COffscreenContext *pOScreen;

	long tagX;
	long tagY;

	long     controlWidth;	// size of the entire control (background size)
	long     controlHeight;
	long     handleWidth;	// size of the handle
	long     handleHeight;
 
	long     iMinXPos;	// min X position in pixel
	long     iMaxXPos;	// max X position in pixel
	long     iMinYPos;	// min Y position in pixel
	long     iMaxYPos;	// max Y position in pixel
	long     styleX;
	long     styleY;

	long     actualXPos;	// not in 2.2b2
	long     actualYPos;	// not in 2.2b2

/*	long     rangeHandleX;	// 2.2b2 stuff
	long     rangeHandleY;
	long     minPosX;
	long     minPosY;
*/
	long     minTmpX;
	long     maxTmpX;
	long     minTmpY;
	long     maxTmpY;
	float    zoomFactor;

	bool     bDrawTransparentEnabled;
	bool     bFreeClick;
	bool     mouseDown;
};



//-----------------------------------------------------------------------------
// Horizontal Range Slider
//-----------------------------------------------------------------------------

enum	// range slider pushing styles
{
	kNeitherCanPush,
	kBothCanPush,
	kMinCanPush,
	kMaxCanPush
};

class CHorizontalRangeSlider : public CMultiControl
{
public:
	CHorizontalRangeSlider(const CRect &size,   CControlListener *listener, 
          long    minTag,      // lower value parameter tag
          long    maxTag,      // higher value parameter tag
          long    iMinXPos,    // min X position in pixel
          long    iMaxXPos,    // max X position in pixel
          CBitmap *handle,     // bitmap selected area
          CBitmap *background, // bitmap background	
          CPoint  &offset,
          long    directionStyle = kLeft,	// style (kRight, kLeft)
          long    pushStyle = kNeitherCanPush,	// pushing policy
          CBitmap *handle2 = 0);	// a second upper handle for the 2-handle mode

	virtual ~CHorizontalRangeSlider();

//	virtual bool attached(CView *parent);	// 2.2
//	virtual bool removed(CView *parent);	// 2.2
	virtual void draw(CDrawContext*);
	virtual void mouse(CDrawContext *pContext, CPoint &where);
	virtual bool onWheel(CDrawContext *pContext, const CPoint &where, float distance);

	virtual long getTag() {	if (currentTag) return *currentTag;	else return tags[0];	}

	virtual void setDrawTransparentHandle(bool val) { bDrawTransparentEnabled = val; }
	virtual void setFreeClick(bool val) { bFreeClick = val; }
	virtual bool getFreeClick() { return bFreeClick; }

	virtual void     setHandle(CBitmap* newHandle);
	virtual void     setHandle2(CBitmap* newHandle2);
	virtual CBitmap *getHandle() { return pHandle; }
	virtual CBitmap *getHandle2() { return pHandle2; }
	virtual void setOffsetHandle(CPoint &val) { offsetHandle = val; }
	virtual void setOffsetHandle2(CPoint &val) { offsetHandle2 = val; }

	virtual void  setZoomFactor(float val) { zoomFactor = val; }
	virtual float getZoomFactor() { return zoomFactor; }
	virtual void  setConvergeFactor(float val) { convergeFactor = val; }
	virtual float getConvergeFactor() { return convergeFactor; }
	virtual void setPushStyle(long newPushStyle) { pushStyle = newPushStyle; }
	virtual long getPushStyle() { return pushStyle; }
	virtual void setOvershoot(long newOvershoot) { overshoot = newOvershoot; }
	virtual long getOvershoot() { return overshoot; }
	virtual bool getClickBetween() { return clickBetween; }

protected:
	CPoint   offset; 
 	CPoint   offsetHandle;
 	CPoint   offsetHandle2;

	CBitmap *pHandle;
	CBitmap *pHandle2;
//	COffscreenContext *pOScreen;

	long minTag;
	long maxTag;
	long *currentTag;

	long controlWidth;	// size of the entire control (background size)
	long controlHeight;
 
	long iMinXPos;	// min X position in pixel
	long iMaxXPos;	// max X position in pixel
	long directionStyle;
	long pushStyle;

	long actualXPosMin;	// not in 2.2b2
	long actualXPosMax;	// not in 2.2b2

/*	long rangeHandle;	// 2.2b2 stuff
	long minPos;
*/
	long minTmpX;
	long maxTmpX;

	float zoomFactor;	// the factor of hi-res mode (when holding shift)
	float convergeFactor;	// the Y-pixel -> X-pixel adjustment ratio for convergence mode

	// how far you can click outside of being in between the points & 
	// still be close enough to be considered in between
	long overshoot;
	bool clickBetween;	// whether or not the mouse click was between the 2 points

	bool bDrawTransparentEnabled;
	bool bFreeClick;
};


//-----------------------------------------------------------------------------
// Vertical Range Slider
//-----------------------------------------------------------------------------
class CVerticalRangeSlider : public CMultiControl
{
public:
	CVerticalRangeSlider(const CRect &size,   CControlListener *listener, 
          long    minTag,      // lower value parameter tag
          long    maxTag,      // higher value parameter tag
          long    iMinYPos,    // min Y position in pixel
          long    iMaxYPos,    // max Y position in pixel
          CBitmap *handle,     // bitmap selected area
          CBitmap *background, // bitmap background	
          CPoint  &offset,
          long    directionStyle = kBottom,	// style (kBottom, kTop))
          long    pushStyle = kNeitherCanPush,	// pushing policy
          CBitmap *handle2 = 0);	// a second upper handle for the 2-handle mode

	virtual ~CVerticalRangeSlider();

//	virtual bool attached(CView *parent);	// 2.2
//	virtual bool removed(CView *parent);	// 2.2
	virtual void draw(CDrawContext*);
	virtual void mouse(CDrawContext *pContext, CPoint &where);
	virtual bool onWheel(CDrawContext *pContext, const CPoint &where, float distance);

	virtual long getTag() {	if (currentTag) return *currentTag;	else return tags[0];	}

	virtual void setDrawTransparentHandle(bool val) { bDrawTransparentEnabled = val; }
	virtual void setFreeClick(bool val) { bFreeClick = val; }
	virtual bool getFreeClick() { return bFreeClick; }

	virtual void     setHandle(CBitmap* newHandle);
	virtual void     setHandle2(CBitmap* newHandle2);
	virtual CBitmap *getHandle() { return pHandle; }
	virtual CBitmap *getHandle2() { return pHandle2; }
	virtual void setOffsetHandle(CPoint &val) { offsetHandle = val; }
	virtual void setOffsetHandle2(CPoint &val) { offsetHandle2 = val; }

	virtual void  setZoomFactor(float val) { zoomFactor = val; }
	virtual float getZoomFactor() { return zoomFactor; }
	virtual void  setConvergeFactor(float val) { convergeFactor = val; }
	virtual float getConvergeFactor() { return convergeFactor; }
	virtual void setPushStyle(long newPushStyle) { pushStyle = newPushStyle; }
	virtual long getPushStyle() { return pushStyle; }
	virtual void setOvershoot(long newOvershoot) { overshoot = newOvershoot; }
	virtual long getOvershoot() { return overshoot; }
	virtual bool getClickBetween() { return clickBetween; }

protected:
	CPoint   offset; 
 	CPoint   offsetHandle;
 	CPoint   offsetHandle2;

	CBitmap *pHandle;
	CBitmap *pHandle2;
//	COffscreenContext *pOScreen;

	long minTag;
	long maxTag;
	long *currentTag;

	long controlWidth;	// size of the entire control (background size)
	long controlHeight;
 
	long iMinYPos;	// min Y position in pixel
	long iMaxYPos;	// max Y position in pixel
	long directionStyle;
	long pushStyle;

	long actualYPosMin;	// not in 2.2b2
	long actualYPosMax;	// not in 2.2b2

/*	long rangeHandle;	// 2.2b2 stuff
	long minPos;
*/
	long minTmpY;
	long maxTmpY;

	float zoomFactor;	// the factor of hi-res mode (when holding shift)
	float convergeFactor;	// the X-pixel -> Y-pixel adjustment ratio for convergence mode

	// how far you can click outside of being in between the points & 
	// still be close enough to be considered in between
	long overshoot;
	bool clickBetween;	// whether or not the mouse click was between the 2 points

	bool bDrawTransparentEnabled;
	bool bFreeClick;
};


#endif
