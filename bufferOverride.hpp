/*------------------- by Marc Poirier  ][  March 2001 -------------------*/


#ifndef __bufferOverride
#define __bufferOverride

#include "DistrhoPlugin.hpp"
START_NAMESPACE_DISTRHO

#include "dfxmisc.h"
#include "lfo.h"
#include "TempoRateTable.h"

//-----------------------------------------------------------------------------
// constants & macros

#define DIVISOR_MIN 1.92f
#define DIVISOR_MAX 222.0f
#define bufferDivisorScaled(A) ( paramRangeSquaredScaled((A), DIVISOR_MIN, DIVISOR_MAX) )
#define bufferDivisorUnscaled(A) ( paramRangeSquaredUnscaled((A), DIVISOR_MIN, DIVISOR_MAX) )

#define BUFFER_MIN 1.0f
#define BUFFER_MAX 999.0f
#define forcedBufferSizeScaled(A) ( paramRangeSquaredScaled((1.0f-(A)), BUFFER_MIN, BUFFER_MAX) )
#define forcedBufferSizeUnscaled(A) ( 1.0f - paramRangeSquaredUnscaled((A), BUFFER_MIN, BUFFER_MAX) )
#define forcedBufferSizeSamples(A) ( (long)(forcedBufferSizeScaled((A)) * SAMPLERATE * 0.001f) )

#define TEMPO_MIN 57.0f
#define TEMPO_MAX 480.0f
#define tempoScaled(A)   ( paramRangeScaled((A), TEMPO_MIN, TEMPO_MAX) )
#define tempoUnscaled(A)   ( paramRangeUnscaled((A), TEMPO_MIN, TEMPO_MAX) )

#define LFO_RATE_MIN 0.03f
#define LFO_RATE_MAX 21.0f
#define LFOrateScaled(A)   ( paramRangeSquaredScaled((A), LFO_RATE_MIN, LFO_RATE_MAX) )
#define LFOrateUnscaled(A)   ( paramRangeSquaredUnscaled((A), LFO_RATE_MIN, LFO_RATE_MAX) )

// you need this stuff to get some maximum buffer size & allocate for that
// this is 42 bpm - should be sufficient
#define MIN_ALLOWABLE_BPS 0.7f

#define NUM_PROGRAMS 16
#define PLUGIN_VERSION 2000
#define PLUGIN_ID 'bufS'


//-----------------------------------------------------------------------------
class BufferOverrideProgram
{
	friend class BufferOverride;
public:
	BufferOverrideProgram();
	~BufferOverrideProgram();
private:
	float *param;
	char *name;
};


//-----------------------------------------------------------------------------
class BufferOverride : public Plugin
{
	friend class BufferOverrideEditor;
public:
	BufferOverride();
	~BufferOverride();

	enum Parameters {
	    kDivisor = 0,
	    kBuffer,
	    kBufferTempoSync,
	    kBufferInterrupt,

	    kDivisorLFOrate,
	    kDivisorLFOdepth,
	    kDivisorLFOshape,
	    kDivisorLFOtempoSync,
	    kBufferLFOrate,
	    kBufferLFOdepth,
	    kBufferLFOshape,
	    kBufferLFOtempoSync,

	    kSmooth,
	    kDryWetMix,

	    kPitchbend,
	    kMidiMode,

	    kTempo,

	    NUM_PARAMETERS
	};

	virtual void d_deactivate();
	virtual void d_activate();

	virtual long processEvents(VstEvents* events);

	virtual long canDo(char* text);

protected:
	void d_run(float **inputs, float **outputs, long sampleFrames, bool replacing);
	void updateBuffer(long samplePos);

	void heedBufferOverrideEvents(long samplePos);
	float getDivisorParameterFromNote(int currentNote);
	float getDivisorParameterFromPitchbend(int pitchbendByte);

	void initPresets();
	void d_sampleRateChanged(double newSampleRate);

	// the parameters
	float fDivisor, fBuffer, fBufferTempoSync, fBufferInterrupt, fSmooth, fDryWetMix, fPitchbend, fMidiMode, fTempo;

	BufferOverrideProgram *programs;	// presets / program slots

	long currentForcedBufferSize;	// the size of the larger, imposed buffer
	// these store the forced buffer
	float *buffer1;
#ifdef BUFFEROVERRIDE_STEREO
	float *buffer2;
#endif
	long writePos;	// the current sample position within the forced buffer

	long minibufferSize;	// the current size of the divided "mini" buffer
	long prevMinibufferSize;	// the previous size
	long readPos;	// the current sample position within the minibuffer
	float currentBufferDivisor;	// the current value of the divisor with LFO possibly applied

	float numLFOpointsDivSR;	// the number of LFO table points divided by the sampling rate

	VstTimeInfo *timeInfo;
	float currentTempoBPS;	// tempo in beats per second
	TempoRateTable *tempoRateTable;	// a table of tempo rate values
	long hostCanDoTempo;	// my semi-booly dude who knows something about the host's VstTimeInfo implementation
	bool needResync;

	long SUPER_MAX_BUFFER;
	float SAMPLERATE;

	long smoothDur, smoothcount;	// total duration & sample counter for the minibuffer transition smoothing period
	float smoothStep;	// the gain increment for each sample "step" during the smoothing period
	float sqrtFadeIn, sqrtFadeOut;	// square root of the smoothing gains, for equal power crossfading
	float smoothFract;

	double pitchbend, oldPitchbend;	// pitchbending scalar values
	VstMidi *midistuff;	// all of the MIDI everythings
	bool oldNote;	// says if there was an old, unnatended note-on or note-off from a previous block
	int lastNoteOn, lastPitchbend;	// these carry over the last events from a previous processing block
	bool divisorWasChangedByHand;	// for MIDI trigger mode - tells us to respect the fDivisor value
	bool divisorWasChangedByMIDI;	// tells the GUI that the divisor displays need updating

	LFO *divisorLFO, *bufferLFO;

	float fadeOutGain, fadeInGain, realFadePart, imaginaryFadePart;	// for trig crossfading

	// Distrho plugin functions
	const char* d_getLabel() const noexcept override {
		return "DestroyFX Buffer Override";
	}

	const char* d_getMaker() const noexcept override {
		return "DestroyFX";
	}

	const char* d_getLicense() const noexcept override {
		return "GPL";
	}

	uint32_t d_getVersion() const noexcept override {
		return 0x1000;
	}

	long d_getUniqueId() const noexcept override {
		return d_cconst('D', 'X', 'B', 'O');
	}

	// -------------------------------------------------------------------
	// Init

	void d_initParameter(uint32_t index, Parameter& parameter) override;
	void d_initProgramName(uint32_t index, d_string& programName) override;

	// -------------------------------------------------------------------
	// Internal data

	float d_getParameterValue(uint32_t index) const override;
	void  d_setParameterValue(uint32_t index, float value) override;
	void  d_setProgram(uint32_t index) override;

	// -------------------------------------------------------------------
	// Process

	void d_activate() override;
	void d_deactivate() override;
	void d_run(const float** inputs, float** outputs, uint32_t frames) override;
};

END_NAMESPACE_DISTRHO

#endif
