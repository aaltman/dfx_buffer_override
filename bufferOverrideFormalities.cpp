/*------------------- by Marc Poirier  ][  March 2001 -------------------*/

#ifndef __bufferOverride
#include "bufferOverride.hpp"
#endif

START_NAMESPACE_DISTRHO

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#pragma mark _________init_________

//-----------------------------------------------------------------------------
// initializations & such

BufferOverride::BufferOverride()
	: Plugin(paramCount, NUM_PROGRAMS, NUM_PARAMETERS) // 16 programs, 17 parameters
{
	// set default values
	d_setProgram(0);

	// reset
	d_deactivate();

	buffer1 = NULL;
#ifdef BUFFEROVERRIDE_STEREO
	buffer2 = NULL;
#endif
	// default this to something, for the sake of getTailSize()
	SUPER_MAX_BUFFER = (long) ((44100.0f / MIN_ALLOWABLE_BPS) * 4.0f);

#ifdef BUFFEROVERRIDE_STEREO
	setNumInputs(2);	// stereo inputs; not a synth
	canMono();	// it's okay to feed both inputs with the same signal
#else
	setNumInputs(1);	// mono inputs; not a synth
#endif
#endif

#ifdef BUFFEROVERRIDE_STEREO
	setNumOutputs(2);	// stereo out
	setUniqueID(PLUGIN_ID);	// identify Buffer Override stereo   'bufS'
#else
	setNumOutputs(1);	// mono out
	setUniqueID('bufM');	// identify Buffer Override mono
#endif
	canProcessReplacing();	// supports both accumulating and replacing output

	// allocate memory for these structures
	tempoRateTable = new TempoRateTable;
	divisorLFO = new LFO;
	bufferLFO = new LFO;

	chunk = new VstChunk(NUM_PARAMETERS, NUM_PROGRAMS, PLUGIN_ID, this);
	programs = new BufferOverrideProgram[NUM_PROGRAMS];
	setProgram(0);
	strcpy(programs[0].name, "self-determined");	// default program name
	initPresets();

	suspend();

	// check to see if the host supports sending tempo & time information to VST plugins
	hostCanDoTempo = canHostDo("sendVstTimeInfo");
	// default the tempo to something more reasonable than 39 bmp
	// also give currentTempoBPS a value in case that's useful for a freshly opened GUI
	if (hostCanDoTempo == 1)
		currentTempoBPS = (float)tempoAt(0) / 600000.0f;
	if ( (hostCanDoTempo != 1) || (currentTempoBPS <= 0.0f) ) {
		setParameter( kTempo, tempoUnscaled(120.0f) );
		currentTempoBPS = tempoScaled(fTempo) / 60.0f;
	}

	// Set up the programs.
	int i = 1;
	programs[i].param[kDivisor] = bufferDivisorUnscaled(4.0f);
	programs[i].param[kBuffer] = paramSteppedUnscaled(8.7f, NUM_TEMPO_RATES);
	programs[i].param[kBufferTempoSync] = 1.0f;
	programs[i].param[kSmooth] = 0.09f;
	programs[i].param[kDryWetMix] = 1.0f;
	programs[i].param[kMidiMode] = 0.0f;

	programs[i].param[kDivisor] = bufferDivisorUnscaled(37.0f);
	programs[i].param[kBuffer] = forcedBufferSizeUnscaled(444.0f);
	programs[i].param[kBufferTempoSync] = 0.0f;
	programs[i].param[kBufferInterrupt] = 1.0f;
	programs[i].param[kDivisorLFOrate] = LFOrateUnscaled(0.3f);
	programs[i].param[kDivisorLFOdepth] = 0.72f;
	programs[i].param[kDivisorLFOshape] = LFOshapeUnscaled(kSawLFO);
	programs[i].param[kDivisorLFOtempoSync] = 0.0f;
	programs[i].param[kBufferLFOrate] = LFOrateUnscaled(0.27f);
	programs[i].param[kBufferLFOdepth] = 0.63f;
	programs[i].param[kBufferLFOshape] = LFOshapeUnscaled(kSawLFO);
	programs[i].param[kBufferLFOtempoSync] = 0.0f;
	programs[i].param[kSmooth] = 0.042f;
	programs[i].param[kDryWetMix] = 1.0f;
	programs[i].param[kMidiMode] = 0.0f;
	i++;

	programs[i].param[kDivisor] = bufferDivisorUnscaled(170.0f);
	programs[i].param[kBuffer] = forcedBufferSizeUnscaled(128.0f);
	programs[i].param[kBufferTempoSync] = 0.0f;
	programs[i].param[kBufferInterrupt] = 1.0f;
	programs[i].param[kDivisorLFOrate] = LFOrateUnscaled(9.0f);
	programs[i].param[kDivisorLFOdepth] = 0.87f;
	programs[i].param[kDivisorLFOshape] = LFOshapeUnscaled(kThornLFO);
	programs[i].param[kDivisorLFOtempoSync] = 0.0f;
	programs[i].param[kBufferLFOrate] = LFOrateUnscaled(5.55f);
	programs[i].param[kBufferLFOdepth] = 0.69f;
	programs[i].param[kBufferLFOshape] = LFOshapeUnscaled(kReverseSawLFO);
	programs[i].param[kBufferLFOtempoSync] = 0.0f;
	programs[i].param[kSmooth] = 0.201f;
	programs[i].param[kDryWetMix] = 1.0f;
	programs[i].param[kMidiMode] = 0.0f;
	i++;

	programs[i].param[kDivisor] = bufferDivisorUnscaled(42.0f);
	programs[i].param[kBuffer] = forcedBufferSizeUnscaled(210.0f);
	programs[i].param[kBufferTempoSync] = 0.0f;
	programs[i].param[kBufferInterrupt] = 1.0f;
	programs[i].param[kDivisorLFOrate] = LFOrateUnscaled(3.78f);
	programs[i].param[kDivisorLFOdepth] = 0.9f;
	programs[i].param[kDivisorLFOshape] = LFOshapeUnscaled(kRandomLFO);
	programs[i].param[kDivisorLFOtempoSync] = 0.0f;
	programs[i].param[kBufferLFOdepth] = 0.0f;
	programs[i].param[kSmooth] = 0.039f;
	programs[i].param[kDryWetMix] = 1.0f;
	programs[i].param[kMidiMode] = 0.0f;
	i++;

	programs[i].param[kDivisor] = bufferDivisorUnscaled(9.0f);
	programs[i].param[kBuffer] = forcedBufferSizeUnscaled(747.0f);
	programs[i].param[kBufferTempoSync] = 0.0f;
	programs[i].param[kDivisorLFOrate] = 0.0f;
	programs[i].param[kDivisorLFOdepth] = 0.0f;
	programs[i].param[kDivisorLFOshape] = LFOshapeUnscaled(kTriangleLFO);
	programs[i].param[kDivisorLFOtempoSync] = 0.0f;
	programs[i].param[kBufferLFOrate] = LFOrateUnscaled(0.174f);
	programs[i].param[kBufferLFOdepth] = 0.21f;
	programs[i].param[kBufferLFOshape] = LFOshapeUnscaled(kTriangleLFO);
	programs[i].param[kBufferLFOtempoSync] = 0.0f;
	programs[i].param[kSmooth] = 0.081f;
	programs[i].param[kDryWetMix] = 1.0f;
	programs[i].param[kMidiMode] = 0.0f;
	i++;

	programs[i].param[kDivisor] = bufferDivisorUnscaled(2.001f);
	programs[i].param[kBuffer] = forcedBufferSizeUnscaled(603.0f);
	programs[i].param[kBufferTempoSync] = 0.0f;
	programs[i].param[kDivisorLFOdepth] = 0.0f;
	programs[i].param[kBufferLFOdepth] = 0.0f;
	programs[i].param[kSmooth] = 1.0f;
	programs[i].param[kDryWetMix] = 1.0f;
	programs[i].param[kMidiMode] = 0.0f;
	i++;

	programs[i].param[kDivisor] = bufferDivisorUnscaled(27.0f);
	programs[i].param[kBuffer] = forcedBufferSizeUnscaled(81.0f);
	programs[i].param[kBufferTempoSync] = 0.0f;
	programs[i].param[kBufferInterrupt] = 1.0f;
	programs[i].param[kDivisorLFOrate] = paramSteppedUnscaled(6.6f, NUM_TEMPO_RATES);
	programs[i].param[kDivisorLFOdepth] = 0.333f;
	programs[i].param[kDivisorLFOshape] = LFOshapeUnscaled(kSineLFO);
	programs[i].param[kDivisorLFOtempoSync] = 1.0f;
	programs[i].param[kBufferLFOrate] = 0.0f;
	programs[i].param[kBufferLFOdepth] = 0.06f;
	programs[i].param[kBufferLFOshape] = LFOshapeUnscaled(kSawLFO);
	programs[i].param[kBufferLFOtempoSync] = 1.0f;
	programs[i].param[kSmooth] = 0.06f;
	programs[i].param[kDryWetMix] = 1.0f;
	programs[i].param[kMidiMode] = 0.0f;
	programs[i].param[kTempo] = 0.0f;
}

//-------------------------------------------------------------------------
BufferOverride::~BufferOverride()
{
	if (programs)
		delete[] programs;
	if (chunk)
		delete chunk;

	// deallocate the memory from these arrays
	if (buffer1)
		delete[] buffer1;
#ifdef BUFFEROVERRIDE_STEREO
	if (buffer2)
		delete[] buffer2;
#endif
	if (midistuff)
		delete midistuff;
	if (tempoRateTable)
		delete tempoRateTable;
	if (divisorLFO)
		delete divisorLFO;
	if (bufferLFO)
		delete bufferLFO;
}

//-------------------------------------------------------------------------
void BufferOverride::d_deactivate()
{
	// setting the values like this will restart the forced buffer in the next process()
	currentForcedBufferSize = 1;
	writePos = readPos = 1;
	minibufferSize = 1;
	prevMinibufferSize = 0;
	smoothcount = smoothDur = 0;
	sqrtFadeIn = sqrtFadeOut = 1.0f;

	divisorLFO->reset();
	bufferLFO->reset();

	oldNote = false;
	lastNoteOn = kInvalidMidi;
	lastPitchbend = kInvalidMidi;
	pitchbend = 1.0;
	oldPitchbend = 1.0;
	divisorWasChangedByMIDI = divisorWasChangedByHand = false;
	midistuff->reset();
}

//-----------------------------------------------------------------------------
// this gets called when the plugin is activated
void BufferOverride::d_activate()
{
	needResync = true;	// some hosts may call resume when restarting playback
	wantEvents();

	createAudioBuffers();
}

//-------------------------------------------------------------------------
void BufferOverride::d_sampleRateChanged(double newSampleRate)
{
	// update the sample rate value
	SAMPLERATE = newSampleRate;
	// just in case the host responds with something wacky
	if (SAMPLERATE <= 0.0f)
		SAMPLERATE = 44100.0f;
	long oldMax = SUPER_MAX_BUFFER;
	SUPER_MAX_BUFFER = (long) ((SAMPLERATE / MIN_ALLOWABLE_BPS) * 4.0f);

	// if the sampling rate (& therefore the max buffer size) has changed,
	// then delete & reallocate the buffers according to the sampling rate
	if (SUPER_MAX_BUFFER != oldMax) {
		if (buffer1 != NULL)
			delete[] buffer1;
		buffer1 = NULL;
#ifdef BUFFEROVERRIDE_STEREO
		if (buffer2 != NULL)
			delete[] buffer2;
		buffer2 = NULL;
#endif
	}
	if (buffer1 == NULL)
		buffer1 = new float[SUPER_MAX_BUFFER];
#ifdef BUFFEROVERRIDE_STEREO
	if (buffer2 == NULL)
		buffer2 = new float[SUPER_MAX_BUFFER];
#endif
}


#pragma mark _________programs_________

//-----------------------------------------------------------------------------
BufferOverrideProgram::BufferOverrideProgram()
{
	name = new char[32];
	param = new float[NUM_PARAMETERS];

	param[kDivisor] = 0.0f;
	param[kBuffer] = forcedBufferSizeUnscaled(90.0f);
	param[kBufferTempoSync] = 0.0f;	// default to no tempo sync
	param[kBufferInterrupt] = 1.0f;	// default to on, use new forced buffer behaviour
	param[kDivisorLFOrate] = LFOrateUnscaled(0.3f);
	param[kDivisorLFOdepth] = 0.0f;
	param[kDivisorLFOshape] = 0.0f;
	param[kDivisorLFOtempoSync] = 0.0f;
	param[kBufferLFOrate] = LFOrateUnscaled(3.0f);
	param[kBufferLFOdepth] = 0.0f;
	param[kBufferLFOshape] = 0.0f;
	param[kBufferLFOtempoSync] = 0.0f;
	param[kSmooth] = 0.09f;
	param[kDryWetMix] = 1.0f;	// default to all wet
	param[kPitchbend] = 6.0f / (float)PITCHBEND_MAX;
	param[kMidiMode] = 0.0f;	// default to "nudge" mode
	param[kTempo] = 0.0f;	// default to "auto" (i.e. get it from the host)
	strcpy(name, "default");
}

//-----------------------------------------------------------------------------
BufferOverrideProgram::~BufferOverrideProgram()
{
	if (name)
		delete[] name;
	if (param)
		delete[] param;
}

void BufferOverride::d_initProgramName(uint32_t index, d_string& programName)
{
	switch (index) {
		case 1:	
			programName = "drum roll";
			break;
		case 2:
			programName = "arpeggio";
			break;
		case 3:
			programName = "laser";
			break;
		case 4:
			programName = "sour melodies";
			break;
		case 5:
			programName = "rerun";
			break;
		case 6:
			programName = "\"echo\"";
			break;
		case 7:
			programName = "squeegee";
			break;
	}
}

//-----------------------------------------------------------------------------
void BufferOverride::d_setProgram(uint32_t programNum)
{
	if ( (programNum < NUM_PROGRAMS) && (programNum >= 0) ) {
		for (int i=0; i < NUM_PARAMETERS; i++) {
			this->d_setParameterValue(i, programs[programNum].param[i]);
		}
	}
}

#pragma mark _________parameters_________

//-------------------------------------------------------------------------
void BufferOverride::d_setParameterValue(uint32_t index, float value)
{
	switch (index) {
	case kDivisor :
		fDivisor = value;
		// tell MIDI trigger mode to respect this change
		divisorWasChangedByHand = true;
		break;

	case kBuffer:
		// make sure the cycles match up if the tempo rate has changed
		if (tempoRateTable->getScalar(fBuffer) != tempoRateTable->getScalar(value))
			needResync = true;
		fBuffer = value;
		break;

	case kBufferTempoSync :
		// set needResync true if tempo sync mode has just been switched on
		if ( onOffTest(value) && !onOffTest(fBufferTempoSync) )
			needResync = true;
		fBufferTempoSync = value;
		break;

	case kBufferInterrupt     :
		fBufferInterrupt = value;
		break;
	case kDivisorLFOrate      :
		divisorLFO->fRate = value;
		break;
	case kDivisorLFOdepth     :
		divisorLFO->fDepth = value;
		break;
	case kDivisorLFOshape     :
		divisorLFO->fShape = value;
		break;
	case kDivisorLFOtempoSync :
		divisorLFO->fTempoSync = value;
		break;
	case kBufferLFOrate       :
		bufferLFO->fRate = value;
		break;
	case kBufferLFOdepth      :
		bufferLFO->fDepth = value;
		break;
	case kBufferLFOshape      :
		bufferLFO->fShape = value;
		break;
	case kBufferLFOtempoSync  :
		bufferLFO->fTempoSync = value;
		break;
	case kSmooth              :
		fSmooth = value;
		break;
	case kDryWetMix           :
		fDryWetMix = value;
		break;
	case kPitchbend           :
		fPitchbend = value;
		break;
	case kMidiMode :
		// reset all notes to off if we're switching into MIDI trigger mode
		if ( (onOffTest(value) == true) && (onOffTest(fMidiMode) == false) ) {
			midistuff->removeAllNotes();
			divisorWasChangedByHand = false;
		}
		fMidiMode = value;
		break;
	case kTempo               :
		fTempo = value;
		break;

	default :
		break;
	}

	if ( (index >= 0) && (index < NUM_PARAMETERS) )
		programs[curProgram].param[index] = value;
}

//-------------------------------------------------------------------------
float BufferOverride::d_getParameterValue(uint32_t index)
{
	switch (index) {
	default:
	case kDivisor             :
		return fDivisor;
	case kBuffer              :
		return fBuffer;
	case kBufferTempoSync     :
		return fBufferTempoSync;
	case kBufferInterrupt     :
		return fBufferInterrupt;
	case kDivisorLFOrate      :
		return divisorLFO->fRate;
	case kDivisorLFOdepth     :
		return divisorLFO->fDepth;
	case kDivisorLFOshape     :
		return divisorLFO->fShape;
	case kDivisorLFOtempoSync :
		return divisorLFO->fTempoSync;
	case kBufferLFOrate       :
		return bufferLFO->fRate;
	case kBufferLFOdepth      :
		return bufferLFO->fDepth;
	case kBufferLFOshape      :
		return bufferLFO->fShape;
	case kBufferLFOtempoSync  :
		return bufferLFO->fTempoSync;
	case kSmooth              :
		return fSmooth;
	case kDryWetMix           :
		return fDryWetMix;
	case kPitchbend           :
		return fPitchbend;
	case kMidiMode            :
		return fMidiMode;
	case kTempo               :
		return fTempo;
}

//-------------------------------------------------------------------------
// titles of each parameter

void BufferOverride::getParameterName(long index, char *label)
{
	switch (index) {
	case kDivisor             :
		strcpy(label, "buffer divisor");
		break;
	case kBuffer              :
		strcpy(label, "forced buffer size");
		break;
	case kBufferTempoSync     :
		strcpy(label, "forced buffer tempo sync");
		break;
	case kBufferInterrupt     :
		strcpy(label, "stuck buffer");
		break;
	case kDivisorLFOrate      :
		strcpy(label, "divisor LFO rate");
		break;
	case kDivisorLFOdepth     :
		strcpy(label, "divisor LFO depth");
		break;
	case kDivisorLFOshape     :
		strcpy(label, "divisor LFO shape");
		break;
	case kDivisorLFOtempoSync :
		strcpy(label, "divisor LFO tempo sync");
		break;
	case kBufferLFOrate       :
		strcpy(label, "buffer LFO rate");
		break;
	case kBufferLFOdepth      :
		strcpy(label, "buffer LFO depth");
		break;
	case kBufferLFOshape      :
		strcpy(label, "buffer LFO shape");
		break;
	case kBufferLFOtempoSync  :
		strcpy(label, "buffer LFO tempo sync");
		break;
	case kSmooth              :
		strcpy(label, "smooth");
		break;
	case kDryWetMix           :
		strcpy(label, "dry/wet mix");
		break;
	case kPitchbend           :
		strcpy(label, "pitchbend");
		break;
	case kMidiMode            :
		strcpy(label, "MIDI mode");
		break;
	case kTempo               :
		strcpy(label, "tempo");
		break;
	}
}

//-------------------------------------------------------------------------
// numerical display of each parameter's gradiations

void BufferOverride::getParameterDisplay(long index, char *text)
{
	switch (index) {
	case kDivisor :
		if (bufferDivisorScaled(fDivisor) < 2.0f)
			sprintf(text, "%.3f", 1.0f);
		else
			sprintf(text, "%.3f", bufferDivisorScaled(fDivisor));
		break;
	case kBuffer :
		if (onOffTest(fBufferTempoSync))
			strcpy(text, tempoRateTable->getDisplay(fBuffer));
		else
			sprintf(text, "%.1f", forcedBufferSizeScaled(fBuffer));
		break;
	case kBufferTempoSync :
		if (onOffTest(fBufferTempoSync))
			strcpy(text, "yes");
		else
			strcpy(text, "no");
		break;
	case kBufferInterrupt :
		if (onOffTest(fBufferInterrupt))
			strcpy(text, "yes");
		else
			strcpy(text, "no");
		break;
	case kDivisorLFOrate :
		if (onOffTest(divisorLFO->fTempoSync))
			strcpy(text, tempoRateTable->getDisplay(divisorLFO->fRate));
		else
			sprintf(text, "%.1f", LFOrateScaled(divisorLFO->fRate));
		break;
	case kDivisorLFOdepth :
		sprintf(text, "%ld %%", (long)(divisorLFO->fDepth * 100.0f));
		break;
	case kDivisorLFOshape :
		divisorLFO->getShapeName(text);
		break;
	case kDivisorLFOtempoSync :
		if (onOffTest(divisorLFO->fTempoSync))
			strcpy(text, "yes");
		else
			strcpy(text, "no");
		break;
	case kBufferLFOrate :
		if (onOffTest(bufferLFO->fTempoSync))
			strcpy(text, tempoRateTable->getDisplay(bufferLFO->fRate));
		else
			sprintf(text, "%.1f", LFOrateScaled(bufferLFO->fRate));
		break;
	case kBufferLFOdepth :
		sprintf(text, "%ld %%", (long)(bufferLFO->fDepth * 100.0f));
		break;
	case kBufferLFOshape :
		bufferLFO->getShapeName(text);
		break;
	case kBufferLFOtempoSync :
		if (onOffTest(bufferLFO->fTempoSync))
			strcpy(text, "yes");
		else
			strcpy(text, "no");
		break;
	case kSmooth :
		sprintf(text, "%.1f %%", (fSmooth*100.0f));
		break;
	case kDryWetMix :
		sprintf(text, "%ld %%", (long)(fDryWetMix*100.0f));
		break;
	case kPitchbend :
		sprintf(text, "\xB1%.2f", fPitchbend*PITCHBEND_MAX);
		break;
	case kMidiMode :
		if (onOffTest(fMidiMode))
			sprintf(text, "trigger");
		else
			strcpy(text, "nudge");
		break;
	case kTempo :
		if ( (fTempo > 0.0f) || (hostCanDoTempo != 1) )
			sprintf(text, "%.3f", tempoScaled(fTempo));
		else
			strcpy(text, "auto");
		break;
	}
}

void BufferOverride::getParameterLabel(long index, char *label)
{
	switch (index) {
	case kDivisor         :
		strcpy(label, " ");
		break;
	case kBuffer :
		if (onOffTest(fBufferTempoSync))
			strcpy(label, "buffers/beat");
		else
			strcpy(label, "samples");
		break;
	case kBufferTempoSync     :
		strcpy(label, " ");
		break;
	case kBufferInterrupt     :
		strcpy(label, " ");
		break;
	case kDivisorLFOrate      :
		strcpy(label, "Hz");
		break;
	case kDivisorLFOdepth     :
		strcpy(label, " ");
		break;
	case kDivisorLFOshape     :
		strcpy(label, " ");
		break;
	case kDivisorLFOtempoSync :
		strcpy(label, " ");
		break;
	case kBufferLFOrate       :
		strcpy(label, "Hz");
		break;
	case kBufferLFOdepth      :
		strcpy(label, " ");
		break;
	case kBufferLFOshape      :
		strcpy(label, " ");
		break;
	case kBufferLFOtempoSync  :
		strcpy(label, " ");
		break;
	case kSmooth              :
		strcpy(label, " ");
		break;
	case kDryWetMix           :
		strcpy(label, " ");
		break;
	case kPitchbend           :
		strcpy(label, "semitones");
		break;
	case kMidiMode            :
		strcpy(label, " ");
		break;
	case kTempo               :
		strcpy(label, "bpm");
		break;
	default:
		strcpy(label, " ");
		break;
	}
}

Plugin* createPlugin()
{
	return new DistrhoPlugin3BandEQ();
}

END_NAMESPACE_DISTRHO
