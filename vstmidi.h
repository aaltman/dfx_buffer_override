/*---------------------------------------------------------------
       Marc's VST MIDI stuff --- happened February 2001
---------------------------------------------------------------*/

#ifndef __vstmidi
#define __vstmidi

#ifndef __audioeffectx__
#include "audioeffectx.h"
#endif


//----------------------------------------------------------------------------- 
// enums

// these are the MIDI event status types
enum
{
	kMidiNoteOff = 0x80,
	kMidiNoteOn = 0x90,
	kMidiPolyphonicAftertouch = 0xA0,
	kMidiCC = 0xB0,
	kMidiProgramChange = 0xC0,
	kMidiChannelAftertouch = 0xD0,
	kMidiPitchbend = 0xE0,

	kMidiSysEx = 0xF0,
	kMidiTimeCode = 0xF1,
	kMidiSongPositionPointer = 0xF2,
	kMidiSongSelect = 0xF3,
	kMidiTuneRequest = 0xF6,
	kMidiEndOfSysex = 0xF7,
	kMidiTimingClock = 0xF8,
	kMidiStart = 0xFA,
	kMidiContinue = 0xFB,
	kMidiStop = 0xFC,
	kMidiActiveSensing = 0xFE,
	kMidiSystemReset = 0xFF,

	kInvalidMidi = -3	// for whatever
};

// these are the MIDI continuous controller messages (CCs)
enum
{
	ccBankSelect = 0x00,
	ccModWheel = 0x01,
	ccBreathControl = 0x02,
	ccFootControl = 0x04,
	ccPortamentoTime = 0x05,
	ccDataEntry = 0x06,
	ccChannelVolume = 0x07,
	ccBalance = 0x08,
	ccPan = 0x0A,
	ccExpressionController = 0x0B,
	ccEffectControl1 = 0x0C,
	ccEffectControl2 = 0x0D,
	ccGeneralPurposeController1 = 0x10,
	ccGeneralPurposeController2 = 0x11,
	ccGeneralPurposeController3 = 0x12,
	ccGeneralPurposeController4 = 0x13,

	// on/off CCs   ( <= 63 is off, >= 64 is on )
	ccSustainPedalOnOff = 0x40,
	ccPortamentoOnOff = 0x41,
	ccSustenutoOnOff = 0x42,
	ccSoftPedalOnOff = 0x43,
	ccLegatoFootswitch = 0x44,
	ccHold2 = 0x45,

	ccSoundController1_soundVariation = 0x46,
	ccSoundController2_timbre = 0x47,
	ccSoundController3_releaseTime = 0x48,
	ccSoundController4_attackTime = 0x49,
	ccSoundController5_brightness = 0x4A,
	ccSoundController6 = 0x4B,
	ccSoundController7 = 0x4C,
	ccSoundController8 = 0x4D,
	ccSoundController9 = 0x4E,
	ccSoundController10 = 0x4F,
	ccGeneralPurposeController5 = 0x50,
	ccGeneralPurposeController6 = 0x51,
	ccGeneralPurposeController7 = 0x52,
	ccGeneralPurposeController8 = 0x53,
	ccPortamentoControl = 0x54,
	ccEffects1Depth = 0x5B,
	ccEffects2Depth = 0x5C,
	ccEffects3Depth = 0x5D,
	ccEffects4Depth = 0x5E,
	ccEffects5Depth = 0x5F,
	ccDataEntryPlus1 = 0x60,
	ccDataEntryMinus1 = 0x61,

	// sepcial commands
	ccAllSoundOff = 0x78,	// 0 only
	ccResetAllControllers = 0x79,	// 0 only
	ccLocalControlOnOff = 0x7A,	// 0 = off, 127 = on
	ccAllNotesOff = 0x7B,	// 0 only
	ccOmniModeOff = 0x7C,	// 0 only
	ccOmniModeOn = 0x7D,	// 0 only
	ccPolyModeOnOff = 0x7E,
	ccPolyModeOn = 0x7F	// 0 only
};

//----------------------------------------------------------------------------- 
// constants & macros

#define NUM_FADE_POINTS 30000
#define FADE_CURVE 2.7f

#define PITCHBEND_MAX 36.0

// 128 midi notes
#define NUM_NOTES 128
// 12th root of 2
#define NOTE_UP_SCALAR   1.059463094359295264561825294946
#define NOTE_DOWN_SCALAR   0.94387431268169349664191315666792
const float MIDI_SCALAR = 1.0f / 127.0f;

#define STOLEN_NOTE_FADE_DUR 48
const float STOLEN_NOTE_FADE_STEP = 1.0f / (float)STOLEN_NOTE_FADE_DUR;
#define LEGATO_FADE_DUR 39

#define EVENTS_QUEUE_MAX 12000

#define isNote(A)   ( ((A) == kMidiNoteOn) || ((A) == kMidiNoteOff) )


//----------------------------------------------------------------------------- 
// types

// this holds MIDI event information
struct BlockEvents {
	int status;	// the event status MIDI byte
	int byte1;	// the first MIDI data byte
	int byte2;	// the second MIDI data byte
	long delta;	// the delta offset (the sample position in the current block where the event occurs)
	int channel;	// the MIDI channel
};

// this holds information for each MIDI note
struct NoteTable {
	int velocity;	// note velocity - 7-bit MIDI value
	float noteAmp;	// the gain for the note, scaled with velocity, curve, & influence
	long attackDur;	// duration, in samples, of the attack phase
	long attackSamples;	// current position in the attack phase
	long releaseDur;	// duration, in samples, of the release phase
	long releaseSamples;	// current position in the release phase
	float fadeTableStep;	// the gain increment for each envelope step using the fade table
	float linearFadeStep;	// the gain increment for each linear envelope step
	float lastOutValue;	// capture the most recent output value for smoothing, if necessary
	long smoothSamples;	// counter for quickly fading cut-off notes, for smoothity
	float *tail1;	// a little buffer of output samples for smoothing a cut-off note (left channel)
	float *tail2;	// (right channel)
};


//-----------------------------------------------------------------------------

class VstMidi
{
public:
	VstMidi();
	~VstMidi();

	void reset();	// resets the variables
	void clearTail(int currentNote);	// zero out a note's tail buffers
	void setLazyAttack(bool newMode=true) { lazyAttackMode = newMode; };

	// this is an implementation of the processEvents() method for VST
	void processEvents(VstEvents *events, AudioEffectX *effect = 0);
	// this is where new MIDI events are reckoned with during audio processing
	void heedEvents(long eventNum, float SAMPLERATE, float fPitchbendRange, float attack, float release, 
					bool legato, float velCurve, float velInfluence);

	// these are for manage the ordered queue of active MIDI notes
	void insertNote(int currentNote);
	void removeNote(int currentNote);
	void removeAllNotes();

	// public variables
	NoteTable *noteTable;	// a table with important data about each note
	BlockEvents *blockEvents;	// the new MIDI events for a given processing block
	long numBlockEvents;	// the number of new MIDI events in a given processing block
	int *noteQueue;		// a chronologically ordered queue of all active notes
	double *freqTable;	// a table of the frequency corresponding to each MIDI note
	double pitchbend;		// a frequency scalar value for the current pitchbend setting

	//-------------------------------------------------------------------------
	// this function calculates fade scalars if attack or release are happening
	float processEnvelope(bool fades, int currentNote)
	{
	  long attackcount, releasecount;
	  NoteTable *note = &noteTable[currentNote];


		// if attack is in progress
		if (note->attackDur)
		{
			attackcount = (note->attackSamples)++;
			// zero things out if the attack is over so we won't do this fade calculation next time
			if ( attackcount >= (note->attackDur) )
			{
				note->attackDur = 0;
				return 1.0f;
			}

			if (fades)	// use nice, exponential fading
				return (fadeTable[(long) ((float)attackcount*(note->fadeTableStep))]);
			else	// bad, linear fade
				return ( (float)attackcount * note->linearFadeStep );
				// exponential sine fade (stupendously inefficient)
//				envAmp = pow( (( sin((envAmp*PI)-(PI/2.0)) + 1.0 ) / 2.0), 2.0 );
		}

		// if release is in progress
		else if (note->releaseDur)
		{
			releasecount = --(note->releaseSamples);
			// zero things out if the release is over so we won't do this fade calculation next time
			// & turn this note off
			if (releasecount <= 0)
			{
				note->releaseDur = 0;
				note->velocity = 0;
				return 0.0f;
			}

			if (fades)	// use nice, exponential fading
				return (fadeTable[(long) ((float)releasecount*(note->fadeTableStep))]);
			else	// use bad fade
				return ( (float)releasecount * note->linearFadeStep );
				// exponential sine fade
//				envAmp = powf( (( sinf((envAmp*PI)-(PI/2.0f)) + 1.0f ) / 2.0f), 2.0f );
		}

		// since it's possible for the release to end & the note to turn off 
		// during this process chunk, we have to check for that & then return 0.0
		else if ( (note->velocity) == 0 )
			return 0.0f;

		// just send 1.0 no fades or note-offs are happening
		return 1.0f;
	}

	//-------------------------------------------------------------------------
	// this function writes the audio output for smoothing the tips of cut-off notes
	// by sloping down from the last sample outputted by the note
	void processSmoothingOutputSample(float *out, long sampleFrames, int currentNote)
	{
		for (long samplecount=0; (samplecount < sampleFrames); samplecount++)
		{
			// add the latest sample to the output collection, scaled by the note envelope & user gain
			out[samplecount] += noteTable[currentNote].lastOutValue * 
								(float)noteTable[currentNote].smoothSamples * STOLEN_NOTE_FADE_STEP;
			// decrement the smoothing counter
			(noteTable[currentNote].smoothSamples)--;
			// exit this function if we've done all of the smoothing necessary
			if (noteTable[currentNote].smoothSamples <= 0)
				return;
		}
	}

	//-------------------------------------------------------------------------
	// this function writes the audio output for smoothing the tips of cut-off notes
	// by fading out the samples stored in the tail buffers
	void processSmoothingOutputBuffer(float *out, long sampleFrames, int currentNote, int channel)
	{
	  float *tail;
	  long *smoothsamples = &(noteTable[currentNote].smoothSamples);
	  
		if (channel == 1)
			tail = (noteTable[currentNote].tail1);
		else
			tail = (noteTable[currentNote].tail2);
		for (long samplecount=0; (samplecount < sampleFrames); samplecount++)
		{
//			out[samplecount] += tail[(*smoothsamples)-1] * 
			out[samplecount] += tail[STOLEN_NOTE_FADE_DUR-(*smoothsamples)] * 
								(float)(*smoothsamples) * STOLEN_NOTE_FADE_STEP;
			(*smoothsamples)--;
			if (*smoothsamples <= 0)
				return;
		}
	}


protected:
	// initializations
	void fillFrequencyTable();
	void fillFadeTable();

	void turnOffNote(int currentNote, float release, bool legato, float SAMPLERATE);

	bool *sustainQueue;	// a queue of note-offs for when the sustain pedal is active
	float *fadeTable;	// an exponentially curved gain envelope

	bool usePitchbendLSB;	// if the MIDI hardware uses it
	bool lazyAttackMode;	// pick up where the release left off, if it's still releasing
	bool sustain;			// sustain pedal is active
};

#endif
