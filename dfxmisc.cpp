#ifndef __dfxmisc
#include "dfxmisc.h"
#endif



//-----------------------------------------------------------------------------------------
// the calculates the number of samples until the next musical measure starts

long samplesToNextBar(VstTimeInfo *timeInfo)
{
  // default these values to something reasonable in case they are not available from the host
  double currentBarStartPos = 0.0, currentPPQpos = 0.0, meterNumerator = 4.0;
  double currentTempoBPS, numPPQ;
  long numSamples;

 
	// exit immediately if timeInfo got returned NULL - there's nothing we can do in that case
	if (timeInfo == NULL)
		return 0;
	if (kVstTempoValid & timeInfo->flags)
		currentTempoBPS = timeInfo->tempo / 60.0;
	// there's no point in going on with this if the host isn't supplying tempo
	else
		return 0;

	// get the song beat position of the beginning of the previous measure
	if (kVstBarsValid & timeInfo->flags)
		currentBarStartPos = timeInfo->barStartPos;

	// get the song beat position of our precise current location
	if (kVstPpqPosValid & timeInfo->flags)
		currentPPQpos = timeInfo->ppqPos;

	// get the numerator of the time signature - this is the number of beats per measure
	if (kVstTimeSigValid & timeInfo->flags)
		meterNumerator = (double) timeInfo->timeSigNumerator;
	// it will screw up the while loop below bigtime if timeSigNumerator isn't a positive number
	if (meterNumerator <= 0.0)
		meterNumerator = 4.0;

	// calculate the distance in beats to the upcoming measure beginning point
	if (currentBarStartPos == currentPPQpos)
		numPPQ = 0.0;
	else
		numPPQ = currentBarStartPos + meterNumerator - currentPPQpos;

	// do this stuff because some hosts (Cubase) give kind of wacky barStartPos sometimes
	while (numPPQ < 0.0)
		numPPQ += meterNumerator;
	while (numPPQ > meterNumerator)
		numPPQ -= meterNumerator;

	// convert the value for the distance to the next measure from beats to samples
	numSamples = (long) ( numPPQ * timeInfo->sampleRate / currentTempoBPS );

	// return the number of samples until the next measure
	if (numSamples < 0)	// just protecting again against wacky values
		return 0;
	else
		return numSamples;
}


//-----------------------------------------------------------------------------------------
// this should get called during processEvents() for a plugin that wants to handle 
// MIDI program change events, but not any other MIDI events

void processProgramChangeEvents(VstEvents *events, AudioEffectX *effect)
{
  VstMidiEvent *midiEvent;
  int programNumber = -1;
  long delta = 0;


	for (long i = 0; (i < events->numEvents); i++)
	{
		// check to see if this event is MIDI; if no, then we try the for-loop again
		if ( ((events->events[i])->type) != kVstMidiType )
			continue;

		// cast the incoming event as a VstMidiEvent
		midiEvent = (VstMidiEvent*)events->events[i];

		// program change
		if ( (midiEvent->midiData[0] & 0xF0) == 0xC0 )
		{
			if (midiEvent->deltaFrames >= delta)
			{
				programNumber = (midiEvent->midiData[1]) & 0x7F;	// program number
				delta = midiEvent->deltaFrames;	// timing offset
			}
		}
	}

	if (programNumber >= 0)
		effect->setProgram(programNumber);
}


//-----------------------------------------------------------------------------------------
// computes the principle branch of the Lambert W function
//    { LambertW(x) = W(x), where W(x) * exp(W(x)) = x }

double LambertW(double input)
{
  double x = fabs(input);

	if (x <= 500.0)
		return 0.665 * ( 1.0 + (0.0195 * log(x+1.0)) ) * log(x+1.0) + 0.04;
	else
		return log(x-4.0) - ( (1.0 - 1.0/log(x)) * log(log(x)) );
}
