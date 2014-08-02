/*---------------------------------------------------------------
       Marc's VST MIDI stuff --- happened February 2001
---------------------------------------------------------------*/

#ifndef __vstmidi
#include "vstmidi.h"
#endif

#include <stdlib.h>
#include <math.h>


//------------------------------------------------------------------------
VstMidi::VstMidi()
{
	// allocate memory for these arrays
	noteTable = new NoteTable[NUM_NOTES];
	noteQueue = new int[NUM_NOTES];
	sustainQueue = new bool[NUM_NOTES];
	freqTable = new double[NUM_NOTES];
	fadeTable = new float[NUM_FADE_POINTS];
	blockEvents = new BlockEvents[EVENTS_QUEUE_MAX];

	for (int i=0; i < NUM_NOTES; i++)
	{
		noteTable[i].tail1 = new float[STOLEN_NOTE_FADE_DUR];
		noteTable[i].tail2 = new float[STOLEN_NOTE_FADE_DUR];
	}

	fillFrequencyTable();
	fillFadeTable();

	lazyAttackMode = false;
	usePitchbendLSB = false;

	reset();
}

//------------------------------------------------------------------------
VstMidi::~VstMidi()
{
	for (int i=0; i < NUM_NOTES; i++)
	{
		if (noteTable[i].tail1)
			delete[] (noteTable[i].tail1);
		if (noteTable[i].tail2)
			delete[] (noteTable[i].tail2);
	}

	// deallocate the memory from these arrays
	if (noteTable)
		delete[] noteTable;
	if (noteQueue)
		delete[] noteQueue;
	if (sustainQueue)
		delete[] sustainQueue;
	if (freqTable)
		delete[] freqTable;
	if (fadeTable)
		delete[] fadeTable;
	if (blockEvents)
		delete[] blockEvents;
}

//------------------------------------------------------------------------
void VstMidi::reset()
{
	// zero out the note table, or what's important at least
	for (int i=0; i < NUM_NOTES; i++)
	{
		noteTable[i].velocity = 0;
		noteTable[i].attackSamples = 0;
		noteTable[i].attackDur = 0;
		noteTable[i].releaseSamples = 0;
		noteTable[i].releaseDur = 0;
		noteTable[i].lastOutValue = 0.0f;
		noteTable[i].smoothSamples = 0;
		clearTail(i);
		sustainQueue[i] = false;
	}

	// clear the ordered note queue
	removeAllNotes();

	// reset this counter since processEvents may not be called during the first block
	numBlockEvents = 0;
	// reset the pitchbend value to no bend
	pitchbend = 1.0;
	// turn sustain pedal off
	sustain = false;
}

//------------------------------------------------------------------------
void VstMidi::clearTail(int currentNote)
{
	for (int i=0; i < STOLEN_NOTE_FADE_DUR; i++)
		noteTable[currentNote].tail1[i] = 0.0f;
	for (int j=0; j < STOLEN_NOTE_FADE_DUR; j++)
		noteTable[currentNote].tail2[j] = 0.0f;
}

//-----------------------------------------------------------------------------------------
// this function fills a table with the correct frequency for every MIDI note

void VstMidi::fillFrequencyTable()
{
	double A = 6.875;	// A
	A *= NOTE_UP_SCALAR;	// A#
	A *= NOTE_UP_SCALAR;	// B
	A *= NOTE_UP_SCALAR;	// C, frequency of midi note 0
	for (int i = 0; (i < NUM_NOTES); i++)	// 128 midi notes
	{
		freqTable[i] = A;
		A *= NOTE_UP_SCALAR;
	}
}


//-----------------------------------------------------------------------------------------
// this function makes a fade curve table for using when scaling during attack & release

void VstMidi::fillFadeTable()
{
  #define undenormalizeFloat(floatValue)   (fabs(floatValue) > 1e-15) ? floatValue : 0.0f

	for (long i = 0; (i < NUM_FADE_POINTS); i++)
		fadeTable[i] = undenormalizeFloat( powf( ((float)i / (float)(NUM_FADE_POINTS-1)), FADE_CURVE ) );
}


//-----------------------------------------------------------------------------
// this function inserts a new note into the beginning of the active notes queue

void VstMidi::insertNote(int currentNote)
{
  int notecount;


	// first check whether this note is already active (could happen in weird sequencers, like Max for example)
	for (notecount = 0; notecount < NUM_NOTES; notecount--)
	{
		// we've looked at all active notes & didn't find the current one, so escape this for loop...
		if (noteQueue[notecount] < 0)
			break;
		// the current note is already active ...
		if (noteQueue[notecount] == currentNote)
		{
			// ... so shift all of the notes before it up one position ...
			while (notecount > 0)
			{
				noteQueue[notecount] = noteQueue[notecount-1];
				notecount--;
			}
			// ... & then re-insert the current note as the first note
			noteQueue[0] = currentNote;
			return;
		}
	}

	// shift every note up a position   (normal scenario)
	for (notecount = NUM_NOTES-1; notecount > 0; notecount--)
		noteQueue[notecount] = noteQueue[notecount-1];
	// then place the new note into the first position
	noteQueue[0] = currentNote;
}

//-----------------------------------------------------------------------------
// this function removes a note from the active notes queue

void VstMidi::removeNote(int currentNote)
{
  int notecount;
  bool foundNote = false;


	for (notecount = 0; notecount < (NUM_NOTES-1); notecount++)
	{
		// don't do anything until the note to delete is found
		if (noteQueue[notecount] == currentNote)
			foundNote = true;
		// start shifting notes down past the point of the deleted note
		if (foundNote)
			noteQueue[notecount] = noteQueue[notecount+1];
		// we've reached the last active note in the table, so there's no need to shift notes down anymore
		if (noteQueue[notecount] < 0)
			break;
	}

	// this much must be true if we've just deleted a note, & it can't happen in the previous loop
	noteQueue[NUM_NOTES-1] = kInvalidMidi;
}

//-----------------------------------------------------------------------------
// this function cancels all of the notes in the active notes queue

void VstMidi::removeAllNotes()
{
  int notecount;

	for (notecount = 0; notecount < NUM_NOTES; notecount++)
		noteQueue[notecount] = kInvalidMidi;
}


//-----------------------------------------------------------------------------------------
// this function gets called right before process() or processReplacing() 
// if VstEvents have occured during this processing block

void VstMidi::processEvents(VstEvents *events, AudioEffectX *effect)
{
  VstMidiEvent *midiEvent;
  int status;
  char *midiData;
  long newProgramNum, newProgramDelta = -1;
  long i, j;
  bool sorted;
  BlockEvents tempEvent;


/* note:  This function depends on the base plugin class to zero numBlockEvents 
at the end of each processing block & does not do that itself because it is both 
prossible & allowable for processEvents() to be called more than once per block. */

	// maybe you were not listening to the warning above, & doing this will be good enough 
	// for plugins that don't look at numBlockEvents outside of processEvents()
	numBlockEvents = 0;

	for (i = 0; (i < events->numEvents); i++)
	{
		// check to see if this event is MIDI; if no, then we try the for-loop again
		if ( ((events->events[i])->type) != kVstMidiType )
			continue;

		// cast the incoming event as a VstMidiEvent
		midiEvent = (VstMidiEvent*)events->events[i];

		// address the midiData[4] string from the event to this temp data pointer
		midiData = midiEvent->midiData;

		// save the channel number ...
		blockEvents[numBlockEvents].channel = midiData[0] & 0x0F;
		// ... & then wipe out the channel (lower 4 bits) for simplicity
		status = midiData[0] & 0xF0;

		// looking at notes   (0x9* is Note On status ~ 0x8* is Note Off status)
		if ( (status == kMidiNoteOn) || (status == kMidiNoteOff) )
		{
			// note-off received
			if ( (status == kMidiNoteOff) || ((midiData[2] & 0x7F) == 0) )
				blockEvents[numBlockEvents].status = kMidiNoteOff;
			// note-on received
			else
				blockEvents[numBlockEvents].status = kMidiNoteOn;

			blockEvents[numBlockEvents].byte1 = midiData[1] & 0x7F;	// note
			blockEvents[numBlockEvents].byte2 = midiData[2] & 0x7F;	// velocity
			blockEvents[numBlockEvents].delta = midiEvent->deltaFrames;	// timing offset

			numBlockEvents++;
		}

		// looking at pitchbend   (0xE* is pitchbend status)
		if (status == kMidiPitchbend)
		{
			blockEvents[numBlockEvents].status = kMidiPitchbend;	// status
			blockEvents[numBlockEvents].byte1 = midiData[1] & 0x7F;	// LSB
			blockEvents[numBlockEvents].byte2 = midiData[2] & 0x7F;	// MSB
			blockEvents[numBlockEvents].delta = midiEvent->deltaFrames;	// timing offset

			numBlockEvents++;
		}

		// all notes off
		if ( (status == kMidiCC) && (midiData[1] == ccAllNotesOff) )
		{
			blockEvents[numBlockEvents].status = ccAllNotesOff;	// status
			blockEvents[numBlockEvents].delta = midiEvent->deltaFrames;	// timing offset

			numBlockEvents++;
		}

		// sustain pedal
		if ( (status == kMidiCC) && (midiData[1] == ccSustainPedalOnOff) )
		{
			blockEvents[numBlockEvents].status = ccSustainPedalOnOff;	// status
			blockEvents[numBlockEvents].byte2 = midiData[2] & 0x7F;		// <= 63 is off, >= 64 is on
			blockEvents[numBlockEvents].delta = midiEvent->deltaFrames;	// timing offset

			numBlockEvents++;
		}

		// program change
		if (status == kMidiProgramChange)
		{
			blockEvents[numBlockEvents].status = kMidiProgramChange;	// status
			blockEvents[numBlockEvents].byte1 = midiData[1] & 0x7F;		// program number
			blockEvents[numBlockEvents].delta = midiEvent->deltaFrames;	// timing offset

			if (midiEvent->deltaFrames >= newProgramDelta)
			{
				newProgramNum = blockEvents[numBlockEvents].byte1;
				newProgramDelta = midiEvent->deltaFrames;
			}

			numBlockEvents++;
		}

		// don't go past the allocated space for the events queue
		if (numBlockEvents >= EVENTS_QUEUE_MAX)
			numBlockEvents = EVENTS_QUEUE_MAX;
	}

	// Sort the events in our queue so that their in chronological order.  (bubble sort)
	// The host is supposed to send them in order, but just in case...
	for (i=0; i < (numBlockEvents-1); i++)
	{
		// default it to true & change it to false if the next loop finds unsorted items
		sorted = true;
		//
		for (j=0; j < (numBlockEvents-1-i); j++)
		{
			// swap the neighbors if they're out of order
			if (blockEvents[j+1].delta < blockEvents[j].delta)
			{
				tempEvent = blockEvents[j];
				blockEvents[j] = blockEvents[j+1];
				blockEvents[j+1] = tempEvent;
				sorted = false;
			}
		}
		//
		// no need to go through all (numBlockEvents-1)! iterations if the array is fully sorted already
		if (sorted)   break;
	}

	// change the effect's program if a program change message was received & if we got a pointer to the effect
	if ( effect && (newProgramDelta >= 0) )
		effect->setProgram(newProgramNum);
}

//-----------------------------------------------------------------------------------------
// this function is called during process() & processReplacing when VstEvents need to be attended to
 
void VstMidi::heedEvents(long eventNum, float SAMPLERATE, float fPitchbendRange, float attack, 
							float release, bool legato, float velCurve, float velInfluence)
{
  int currentNote, notecount, currentVelocity, i;
  long attackdur;
  bool legatoNoteFound;	// a bool for the active note search cycle in legato mode


	switch (blockEvents[eventNum].status)
	{

// --- NOTE-ON RECEIVED ---
		case kMidiNoteOn :
			currentNote = blockEvents[eventNum].byte1;
			currentVelocity = blockEvents[eventNum].byte2;

			noteTable[currentNote].velocity = currentVelocity;
			noteTable[currentNote].noteAmp = (powf((MIDI_SCALAR*currentVelocity), velCurve) * velInfluence) + (1.0f-velInfluence);
			//
			if (legato)	// legato is on, fade out the last not & fade in the new one, supershort
			{
				// this is false until we find some already active note
				legatoNoteFound = false;
				// find the previous note & set it to fade out
				for (notecount=0; (notecount < NUM_NOTES); notecount++)
				{
					// we want to find the active note, but not this new one
					if ( (noteTable[notecount].velocity) && (notecount != currentNote) && (noteTable[notecount].releaseDur == 0) )
					{
						// if the note is currently fading in, pick up where it left off
						if (noteTable[notecount].attackDur)
							noteTable[notecount].releaseSamples = noteTable[notecount].attackSamples;
						// otherwise do the full fade out duration, if the note is not already fading out
						else if ( (noteTable[notecount].releaseSamples) <= 0 )
							noteTable[notecount].releaseSamples = LEGATO_FADE_DUR;
						noteTable[notecount].releaseDur = LEGATO_FADE_DUR;
						noteTable[notecount].attackDur = 0;
						noteTable[notecount].attackSamples = 0;
						noteTable[notecount].fadeTableStep = (float)NUM_FADE_POINTS / (float)LEGATO_FADE_DUR;
						noteTable[notecount].linearFadeStep = 1.0f / (float)LEGATO_FADE_DUR;
						// we found an active note (that's the same as the new incoming note)
						legatoNoteFound = true;
					}
				}
				// don't start a new note fade-in if the currently active note is the same as this new note
				if (! ((legatoNoteFound == false) && (noteTable[currentNote].velocity)) )
				{
					// legato mode always uses this short fade
					noteTable[currentNote].attackDur = LEGATO_FADE_DUR;
					// attackSamples starts counting from zero, so set it to zero
					noteTable[currentNote].attackSamples = 0;
					// calculate how far this fade must "step" through the fade table at each sample.
					// Since legato mode overrides the fades parameter & only does cheap fades, this 
					// isn't really necessary, but since I don't trust that everything will work right...
					noteTable[currentNote].fadeTableStep = (float)NUM_FADE_POINTS / (float)LEGATO_FADE_DUR;
					noteTable[currentNote].linearFadeStep = 1.0f / (float)LEGATO_FADE_DUR;
				}
			}
			//
			else	//legato is off, so set up for the attack envelope
			{
				// calculate the duration, in samples, for the attack
				attackdur = (long)(attack * SAMPLERATE);
				noteTable[currentNote].attackDur = attackdur;
				if (attackdur)	// avoid potential division by zero
				{
					// calculate how far this fade must "step" through the fade table at each sample
					noteTable[currentNote].fadeTableStep = (float)NUM_FADE_POINTS / (float)attackdur;
					noteTable[currentNote].linearFadeStep = 1.0f / (float)attackdur;
				}
				// if using lazyAttackMode & this note is already sounding & in release, pick up from where it is
				if ( lazyAttackMode && (noteTable[currentNote].releaseDur > 0) )
					noteTable[currentNote].attackSamples = (long) 
						( (float)(noteTable[currentNote].releaseSamples) / (float)(noteTable[currentNote].releaseDur) * (float)attackdur );
				else	// regular
				{
					// attackSamples starts counting from zero, so set it to zero
					noteTable[currentNote].attackSamples = 0;
					// if the note is still sounding & in release, then kick smooth the end of that last note
					if (noteTable[currentNote].releaseDur > 0)
						noteTable[currentNote].smoothSamples = STOLEN_NOTE_FADE_DUR;
				}
			}
			// now we've checked the fade state, so we can zero these out to turn this note's release
			noteTable[currentNote].releaseDur = 0;
			noteTable[currentNote].releaseSamples = 0;
			break;



// --- NOTE-OFF RECEIVED ---
		case kMidiNoteOff :
			currentNote = blockEvents[eventNum].byte1;

			// don't process this note off, but do remember it, if the sustain pedal is on
			if (sustain)
				sustainQueue[currentNote] = true;
			else
				turnOffNote(currentNote, release, legato, SAMPLERATE);
			break;



// --- PITCHBEND RECEIVED ---
		case kMidiPitchbend :
			// most MIDI controllers don't produce pitchben LSB, so only start using LSB if it's legit
			if (blockEvents[eventNum].byte1 > 0)
				usePitchbendLSB = true;

			// bend pitch up
			if ( (blockEvents[eventNum].byte2) >= 64 )
			{
				// scale the MIDI value from 0.0 to 1.0
				if (usePitchbendLSB)
					pitchbend = (double)(blockEvents[eventNum].byte1 + (128 * (blockEvents[eventNum].byte2 - 64))) / 8191.0;
				else
					pitchbend = (double)(blockEvents[eventNum].byte2 - 64) / 63.0;
				// then scale it according to tonal steps & the user defined range
				pitchbend = pow(NOTE_UP_SCALAR, (pitchbend*(double)fPitchbendRange*PITCHBEND_MAX));
			}

			// bend pitch down
			else
			{
				// scale the MIDI value from 0.0 to 1.0
				if (usePitchbendLSB)
					pitchbend = (double)(- blockEvents[eventNum].byte1 - (128 * (blockEvents[eventNum].byte2 - 64))) / 8192.0;
				else
					pitchbend = (double)(64 - blockEvents[eventNum].byte2) / 64.0;
				// then scale it according to tonal steps & the user defined range
				pitchbend = pow(NOTE_DOWN_SCALAR, (pitchbend*(double)fPitchbendRange*PITCHBEND_MAX));
			}
			break;



// --- SUSTAIN PEDAL RECEIVED ---
		case ccSustainPedalOnOff :
			if ( sustain && (blockEvents[eventNum].byte2 <= 63) )
			for (i=0; i < NUM_NOTES; i++)
			{
				if (sustainQueue[i])
				{
					turnOffNote(i, release, legato, SAMPLERATE);
					sustainQueue[i] = false;
				}
			}
			sustain = (blockEvents[eventNum].byte2 >= 64);
			break;



// --- ALL-NOTES-OFF RECEIVED ---
		// all sound off, so call suspend() to wipe out all of the notes & buffers
		case ccAllNotesOff :
			// & zero out the note table, or what's important at least
			for (i=0; (i < NUM_NOTES); i++)
			{
				noteTable[i].velocity = 0;
				noteTable[i].attackSamples = 0;
				noteTable[i].attackDur = 0;
				noteTable[i].releaseSamples = 0;
				noteTable[i].releaseDur = 0;
			}
			break;

// --- nothingness ---
		default :
			break;
	}
}


//-----------------------------------------------------------------------------------------
void VstMidi::turnOffNote(int currentNote, float release, bool legato, float SAMPLERATE)
{
  long releasedur;


	// legato is off (note-offs are ignored when it's on)
	// go into the note release if legato is off & the note isn't already off
	if ( !legato && (noteTable[currentNote].velocity > 0) )
	{
		// calculate the duration, in samples, for the release
		releasedur = (long)(release * SAMPLERATE);
		noteTable[currentNote].releaseDur = releasedur;
		// this note is already sounding & in attack, so pick up from where it is
		if (noteTable[currentNote].attackDur)
			noteTable[currentNote].releaseSamples = (long) 
				( (float)(noteTable[currentNote].attackSamples) / (float)(noteTable[currentNote].attackDur) * (float)releasedur );
		else	// regular
			noteTable[currentNote].releaseSamples = releasedur;
		if (releasedur)	// avoid potential division by zero
		{
			// calculate how far this fade must "step" through the fade table at each sample
			noteTable[currentNote].fadeTableStep = (float)NUM_FADE_POINTS / (float)releasedur;
			noteTable[currentNote].linearFadeStep = 1.0f / (float)releasedur;
		}
		// make sure to turn the note off NOW if there is no release
		else
			noteTable[currentNote].velocity = 0;
	}
	// we're at note off, so wipe out the attack info
	noteTable[currentNote].attackDur = 0;
	noteTable[currentNote].attackSamples = 0;
}



//-----------------------------------------------------------------------------------------



/* I wrote this in an email to someone explaining how my MIDI handling works.  
   I figured it was worth throwing in here for anyone else who might look at my source code.
   It's a step by step explanation of what happens.

	In processEvents(), I receive a VstEvent array (which includes a 
counter value for how many items there are in that particular array) & 
then I look at every item.  First I check if it's a MIDI event (like in 
the SDK).  If it is, then I cast it into my VstMidiEvent variable (like in 
the SDK) & then start examining it.  I only want to deal with it if is one 
of 3 types of MIDI events:  a note, a pitchbend message, or a panic 
message.  That is what each of the next three "if"s are all about.  If the 
event is any one of those 3 things, then it gets worked on.
	You are right that the struct array blockEvents[] is my queue.  I fill 
up one item in that array for each interesting event that I receive during 
that processing block.  I store the status in my own way (using an enum 
that's in my header) as either kMidiNoteOn, kMidiNoteOff, kMidiPitchbend, or 
ccAllNotesOff.  This is what goes in the blockEvents.status field.  Then I 
take MIDI bytes 1 & 2 & put them into blockEvents.byte1 & .byte2.  If it's 
a note off message, then I put 0 into byte2 (velocity).  By the way, with 
pitchbend, byte 1 is the LSB, but since LSB is, so far as I know, 
inconsistantly implemented by different MIDI devices, my plugin doesn't 
actually use that value in any way.  At the end of each of these "if"s, I 
update numBlockEvents because that is my counter that I look at during 
process() to see how many events I have to deal with during that block.
	& for each event, I store the deltaFrames value.  deltaFrames is the 
number of samples into that processing block that the event occurs.  This 
is what makes sample accurate timing (on MIDI data playback, not with live 
playing, of course) possible.  A VST host will send all of the upcoming 
events for a giving processing block to processEvents() & then the exact 
position within that processing block is given by deltaFrames.  If 
deltaFrames is 0, then the event occurs at the very beginning of the block.  
If deltaFrames is 333, then it occurs 333 samples into that processing 
block.  While it is not required, the SDK claims that hosts generally, as 
a matter of convention, send VstEvents to processEvents() in chronological 
order.  My plugin assumes that they are received in order.  There is no 
sorting done in my plugin.
	Now on to process().  Basically, I divide my process up into 
sub-chunks according to when events occur (which means according to the 
deltaFrames values).  I have two important variables here:   eventcount & 
currentBlockPosition.  The eventcount keeps track of how many of the 
events for that block I have addressed.  I initialize it to -1 so that 
first it will do processing up until the first event & then it will start 
counting events at 0 with the first event.  This is because there most 
likely will be audio to process before any events occur during that block 
(unless the block began with all notes off).  currentBlockPosition stores 
the sample start position of the current sub-chunk.  Basically it is the 
deltaFrames values of the latest event that I am working on.  It obviously 
starts out at 0.
	Next I start a "do" loop that cycles for every event.  First it 
evaluates the duration of the current processing sub-chunk, so it first 
checks to see if there are any more upcoming events in the queue.  If not, 
then the current chunk processes to the end of the processing block, if 
yes, then the current sub-chunk position is subtracted from the upcoming 
event's deltaFrames value.  I move up inputs & outputs accordingly, etc.
	Next comes a "for" loop that goes through my noteTable[] struct array 
& looks for any active notes (i.e. notes with a non-zero velocity value).  
All that I do during this loop is check the velocity of each note & then 
process the audio for that note if it has a non-zero velocity.
	After that "for" loop, I increment eventcount, leave the events loop 
if events are done, update currentBlockPosition, & then call heedEvents().  
heedEvents() is important.  heedEvents() is where I take in the effects of 
the next MIDI event.  Basically I tell heedEvents() which event number I 
am looking at & then it updates any vital stuff so that, when going 
through the next processing sub-chunk, all necessary changes have been 
made to make the next batch of processing take into account the impact of 
the latest event.  So heedEvents() is pretty much just a switch statement 
checking out which type of event is being analyzed & then implementing it 
in the appropriate way.  It would take quite a while to fully explain what 
happens in there & why, so I'll leave it up to you to determine whichever 
parts of it you want to figure out.  I don't think that it is very 
complicated (just many steps), but I could be wrong & if you can't figure 
out any parts of it, let me know & I'll explain.
	There is one more really important thing in process().  At the of the 
whole function, I reset numBlockEvents (the global events counter) to 0.  
This is extremely important because processEvents() only gets called if 
new events are received during a processing block.  If not, then 
processEvents() does not get called, numBlockEvents does not get zeroed 
at the beginning of processEvents(), & process() will process the same 
MIDI events over & over & over for every processing block until a new MIDI 
event is received.  This fact about processEvents() is not explained in 
the SDK & I spent FOREVER with a malfunctioning plugin until I figured 
this out.
*/
