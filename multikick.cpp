/*------------ by Tom Murphy 7  ][  October 2001 ------------*/
/* see multikick.hpp for documentation */

#include "multikick.hpp"

MultiKick::MultiKick (const CRect &size,
                      CControlListener *listener,
                      long tag,
                      int ns,
                      long heightOfOneImage,
                      CBitmap *background,
                      CPoint  &offset,
                      long style,
		      int as) : CControl (size, listener, tag, background),
				numstates(ns),
				offset (offset),
				heightOfOneImage (heightOfOneImage), 
				style(style),
				buttondown(false), 
				obdown(false), 
				actualstate(0), 
				oactualstate(0) {
  if (as == 0) numactualstates = numstates;
  else numactualstates = as;
  setDirty(true); 
}

MultiKick::~MultiKick () {}

/* return the value. Unlike everywhere else, use
   the 'numactualstates' so that we scale to the right
   range. */
float MultiKick::getValue() {
  if (numactualstates == 1 || actualstate <= 0)
    return 0.0f;
  if (actualstate >= numactualstates)
    return 1.0f;

  return ((float)actualstate)/((float)(numactualstates-1)) + .0001f;
}

void MultiKick::setValue(float f) {
  actualstate = (int) ((f + .0001f) * (numactualstates-1));
  if (actualstate >= numstates) actualstate = numstates - 1;
}

bool MultiKick::isDirty() {
  return ((actualstate != oactualstate) 
	  || (buttondown != obdown)
	  || CView::isDirty());
}

void MultiKick::setDirty(const bool val) {
  if (val) oactualstate = -1;
  else {
    oactualstate = actualstate;
    obdown = buttondown;
  }
  CView::setDirty(val);
}

void MultiKick::draw (CDrawContext *pContext) {
  CPoint where (offset.h, offset.v);

  if (style == kKickPairs) {
    where.v += heightOfOneImage * ((actualstate<<1) / 2);
    if (buttondown)
      where.h += (size.right - size.left);
  } else {
    where.v += heightOfOneImage * (actualstate<<1);
    if (buttondown)
      where.v += heightOfOneImage;
  }

  if (pBackground) {
    if (bTransparencyEnabled)
      pBackground->drawTransparent (pContext, size, where);
    else
      pBackground->draw (pContext, size, where);
  }

  setDirty (false);
}

//------------------------------------------------------------------------
void MultiKick::mouse (CDrawContext *pContext, CPoint &where) {
  if (!bMouseEnabled) { buttondown = false; return; }

  long button = pContext->getMouseButtons ();
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
  else {
    buttondown = false;
    return;
  }

  /* save old value in case the mouse is dragged off while the
     button is still held down. */

  int entrystate = actualstate;

  if (pContext->getMouseButtons ()) {

    // begin of edit parameter
    getParent()->beginEdit (tag);
    do {
      if (where.h >= size.left && where.v >= size.top  &&
          where.h <= size.right && where.v <= size.bottom) {
        setState((entrystate+direction+numstates) % numstates);
        setMouseDown(true) /* 1 */;
      } else {
        setState(entrystate);
        setMouseDown(false);
      }
      
      if (isDirty())
        listener->valueChanged(pContext, this);

      pContext->getMouseLocation(where);

      doIdleStuff();
//      draw(pContext);
      oactualstate = actualstate;
      obdown = buttondown;
    } while (pContext->getMouseButtons() & (kLButton|kRButton));

    setDirty(true);
    // end of edit parameter
    getParent()->endEdit (tag);
  } else {
    setState((actualstate+direction+numstates) % numstates);
  }
  draw(pContext);
  setMouseDown(false);
  listener->valueChanged (pContext, this);
}
