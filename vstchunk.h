// --------- VST chunks stuff by Marc Poirier  ][  April-July 2002 ---------
//
// This is a class, some functions, & other relevant stuff for dealing with 
// VST program/bank/parameter data "chunks."
//
// My basic purpose for using the chunk feature is for storing MIDI event 
// assignments for parameter modulation.  Rather than double the number of 
// regular float 0-1 VST parameters & have a MIDI event number parameter 
// that corresponds to each functional parameter, I decided to use chunks.  
//
// This has several benefits:
//	*	For one thing, you can store ints rather than using 0-1 floats & 
//		scaling & casting to int (which is fudgy).
//	*	Also, if you randomize parameters in your VST host, you won't 
//		randomize MIDI event assignments, as that would most likely not 
//		be a desirable result of randomization.
//	*	You can also assign ID numbers to each parameter so that if you 
//		add or remove or rearrange parameters in later versions of your 
//		plugin, you can still map the new parameters to the old set & 
//		therefore load old settings into new versions of your plugin & 
//		vice versa.
//
//
// If you have other stuff that you need to store as special chunk data 
// aside from MIDI event assignments, it is quite easy to extend this 
// class.  Supply a value for the optional VstChunk constructor argument 
// sizeofExtendedData.  This should be the size of the extra data 
// (in bytes) of your additional data.  Then implement or override 
// (if inheriting) the getChunkExtendedData() & setChunkExtendedData() 
// methods to take care of your extra data.
//
// If you need to do anything special when parameter settings are restored 
// from particular versions of your plugin (for example, remapping the 
// value to accomodate for a change in range or scaling of the parameter 
// in a more recent version of your plugin), I've provided a hook for 
// extending parameter restoration during setChunk().  Just take care of 
// your special cases in doChunkRestoreSetParameterStuff().
// 
// If you want to do any extra stuff when handling MIDI event assignment 
// and automation, I've provided a couple of hooks from the event 
// processing routine in order to make that easy:
// doLearningAssignStuff() is called during processParameterEvents() 
// whenever a MIDI event is assigned to a parameter via MIDI learn.
// doSetParameterStuff() is called during processParameterEvents() 
// whenever an incoming MIDI event automates a parameter to which it is 
// assigned.
//
//-------------------------------------------------------------------------
// NOTE July 3rd:  I've recently mucked with this code a lot and added new 
// features with notes, pitchbend, stepped control, limited ranges, etc.  
// I'll document this stuff, later...

#ifndef __vstchunk
#define __vstchunk

#include "audioeffectx.h"


//------------------------------------------------------
// some constants
enum
{
	// . . . MIDI learn stuff . . .

	kNoLearner = -3,	// for when no parameter is activated for learning
	kNumMidiValues = 128,	// the number of values for MIDI events

	// MIDI event types
	kParamEventNone = 0,	// for when you haven't assigned an event to a parameter
	kParamEventCC,	// CC assignment
	kParamEventPitchbend,	// pitchbend wheel assignment
	kParamEventNote,	// note assignment

	// parameter automation behaviour mode flags
	//
	// use MIDI events to toggle the associated parameter between 0.0 and 1.0 
	// if no data1 value was specified, 
	// (good for controlling on/off buttons)
	// but if data1 is greater than 2, then MIDI events will toggle between 
	// paramater's states.  note-ons will cycle through the values and 
	// continuous MIDI events will move through the values in steps
	// (good for switches)
	// (if using notes events and this flag is not set, and neither is NoteHold, 
	// then note ranges are used to control associated parameters)
	kEventBehaviourToggle = 1,
	// send 1.0 on note on and 0.0 on note off if this flag is on, 
	// otherwise toggle 1.0 and 0.0 at each note on
	// (only relevent when using notes events)
	// (overrides Toggle setting for notes events, but not other events)
	kEventBehaviourNoteHold = 1 << 1,
	// use a range other than 0.0 to 1.0 
	// the range is defined by fdata1 and fdata2
	kEventBehaviourRange = 1 << 2,



	// . . . load crisis stuff . . .

	// crisis behaviours (what to do when setChunk sends a foreign byteSize)
	kCrisisLoadWhatYouCan = 0,
	kCrisisDontLoad,
	kCrisisLoadButComplain,
	kCrisisCrashTheHostApplication,

	// crisis-handling errors
	kCrisisNoError = 0,
	kCrisisAbortError,
	kCrisisComplainError,
	kCrisisFailedCrashError,

	// crisis situation flags
	kCrisisMismatchedMagic		= 1,		// the magic signatures don't match
	kCrisisSmallerByteSize		= 1 << 1,	// the incoming chunk size is smaller
	kCrisisLargerByteSize		= 1 << 2,	// the incoming chunk size is larger
	kCrisisFewerParameters		= 1 << 3,	// the incoming chunk has fewer parameters
	kCrisisMoreParameters		= 1 << 4,	// the incoming chunk has more parameters
	kCrisisFewerPrograms		= 1 << 5,	// the incoming chunk has fewer programs
	kCrisisMorePrograms			= 1 << 6,	// the incoming chunk has more programs
	kCrisisLowerVersion			= 1 << 7,	// the incoming chunk has a lower version number
	kCrisisHigherVersion		= 1 << 8,	// the incoming chunk has a higher version number
	kCrisisVersionIsTooLow		= 1 << 9,	// the incoming chunk's version number is lower than our lowest loadable version number
	kCrisisVersionIsTooHigh		= 1 << 10,	// our version number is lower than the incoming chunk's lowest loadable version number
	kCrisisSmallerHeader		= 1 << 11,	// the incoming chunk's header size is smaller
	kCrisisLargerHeader			= 1 << 12,	// the incoming chunk's header size is larger



	// misc
	kInvalidParamTag = -1	// for when there is no parameter with a matching ID
};


//------------------------------------------------------
// structure of a VST program
struct Program
{
	char name[32];
	float params[2];	// can of course be more than 2
};


//------------------------------------------------------
// header information for the data chunks
// note:  correctEndian() assumes that the data is all of type long, 
// so if you change this structure & add different types, 
// then you will want to modify correctEndian() to handle that.
// It is also assumed that the first 6 items in this struct 
// (everything up through numStoredPrograms) will not change, 
// so if you alter this header structure at all, keep the 
// first six items in there & add anything else AFTER those.
struct ChunkInfo
{
	// this is a value that you should look at to check for authenticity 
	// of the data & as an identifier of the data's creator 
	// (the easiest thing is probably to use your VST plugin's "unique ID")
	long magic;
	// the version number of the plugin that is creating the chunk data
	long version;
	// this defaults to 0, but you can set it to be the earliest version 
	// number of your plugin that would be able to handle the chunk data
	// (then lower versions of the plugin will fail to load the chunk)
	long lowestLoadableVersion;
	// the size (in bytes) of the header data that the plugin 
	// will store in the chunk
	unsigned long storedHeaderSize;
	// the number of parameters that the plugin will store in the chunk
	long numStoredParameters;
	// the number of programs that the plugin will store in the chunk
	long numStoredPrograms;
	// the size (in bytes) of each parameter assignment struct 
	// that the plugin will store in the chunk
	unsigned long storedParameterAssignmentSize;
	// the size (in bytes) of the extra chunk data (if any)
	unsigned long storedExtendedDataSize;
};


//------------------------------------------------------
struct ParameterAssignment
{
	// the MIDI event type 
	//    (CC, note, pitchbend, etc.)
	long eventType;
	// the MIDI channel of the MIDI event assignment
	//    (so far, I'm not using channel information for anything)
	long eventChannel;
	// the number of the MIDI event assigned to the parameter 
	//    (CC number, note number, etc.)
	long eventNum;
	// a second MIDI event number for double-value assignments 
	//    (like 2 notes defining a note range)
	long eventNum2;
	// indicating the behaviour of the event, i.e. toggle vs. hold for notes, etc.
	long eventBehaviourFlags;
	// bonus data slots
	// (context-specific)
	// (like for the number of steps in an indexed toggle assignment)
	long data1;
	// (or the maximum step, within that range, to cycle up to)
	long data2;
	// (or the minimum point in a float range)
	float fdata1;
	// (or the maximum point in a float range)
	float fdata2;
};


//------------------------------------------------------
// this swaps bytes for endian data correction
void reverseBytes(void *data, unsigned long size, long count = 1);


//------------------------------------------------------
// this interprets a UNIX environment variable string as a boolean
bool getenvBool(const char *var, bool def);


//------------------------------------------------------
//------------------------------------------------------
class VstChunk
{
public:
	VstChunk(long numParameters, long numPrograms, long magic, 
			AudioEffectX *effect, unsigned long sizeofExtendedData = 0);
	~VstChunk();


	/* - - - - - - - - - VST methods - - - - - - - - - */

	// for adding to your base plugin class methods
	virtual long getChunk(void **data, bool isPreset);
	virtual long setChunk(void *data, long byteSize, bool isPreset);
	virtual void processParameterEvents(VstEvents *events);


	/* - - - - - - - - - MIDI learn - - - - - - - - - */

	// deactivate MIDI learn mode
	// call this when your editor window opens, or when it closes if you prefer
	virtual void resetLearning() { setLearning(false); }
	// remove MIDI event assignments from all parameters
	virtual void clearAssignments();
	// assign a MIDI event to a parameter
	virtual void assignParam(long tag, long eventType, long eventChannel, 
								long eventNum, long eventNum2 = 0, 
								long eventBehaviourFlags = 0, 
								long data1 = 0, long data2 = 0, 
								float fdata1 = 0.0f, float fdata2 = 0.0f);
	// remove a parameter's MIDI event assignment
	virtual void unassignParam(long tag);

	// define or report the actively learning parameter during MIDI learn mode
	virtual void setLearner(long tag, long eventBehaviourFlags = 0, 
							long data1 = 0, long data2 = 0, 
							float fdata1 = 0.0f, float fdata2 = 0.0f);
	virtual long getLearner() { return learner; }

	// turn MIDI learning on or off
	virtual void setLearning(bool newLearn);
	// report whether or not MIDI learn mode is active
	virtual bool isLearning() { return midiLearn; }

	// call these from valueChanged in the plugin editor
	virtual void setParameterMidiLearn(float value);
	virtual void setParameterMidiReset(float value);


	/* - - - - - - - - - version compatibility management - - - - - - - - - */

	// if you set this to something & a chunk is received during setChunk()
	// which has a version number in its header that's lower than this, 
	// then loading will abort
	virtual void setLowestLoadableVersion(long version)
		{ chunkInfo.lowestLoadableVersion = version; }
	virtual long getLowestLoadableVersion()
		{ return chunkInfo.lowestLoadableVersion; }

	// This stuff manages the parameter IDs.  Each parameter has an ID number 
	// so that, if you later add parameters & their tags are inserted within 
	// the old set (rather than tacked on at the end of the old order), 
	// or you remove parameters, or you switch around their order, 
	// then you can just make sure to keep the ID for each old parameter 
	// the same so that old versions of the plugin can load new settings & 
	// vice versa.  By default, a parameter ID table is constructed in which 
	// the ID for each parameter is the same as the parameter's tag/index.  
	// If your plugin's parameter IDs need to be different than that, then 
	// you use these functions to manage the IDs & change them if necessary.
	// If you make any changes, they should really be done immediately after 
	// creating the VstChunk object, or at least before your plugin's 
	// constructor returns, because you don't want any set or getChunk calls 
	// made before you have your parameter ID map finalized.
	virtual void setParameterID(long tag, long newID)
		{ if (paramTagIsValid(tag)) parameterIDs[tag] = newID; }
	virtual long getParameterID(long tag)
		{ if (paramTagIsValid(tag)) return parameterIDs[tag]; else return 0; }
	virtual long getParameterTagFromID(long paramID, long numSearchIDs=0, long *searchIDs=0);


	/* - - - - - - - - - optional settings - - - - - - - - - */

	// true means allowing a given MIDI event to be assigned to only one parameter; 
	// false means that a single event can be assigned to more than one parameter
	virtual void setSteal(bool newSteal) { stealAssignments = newSteal; }
	virtual bool getSteal() { return stealAssignments; }

	// true means that pitchbend events can be assigned to parameters and 
	// used to control those parameters; false means don't use pitchbend like that
	virtual void setAllowPitchbendEvents(bool newMode=true) { allowPitchbendEvents = newMode; }
	virtual bool getAllowPitchbendEvents() { return allowPitchbendEvents; }

	// true means that MIDI note events can be assigned to parameters and 
	// used to control those parameters; false means don't use notes like that
	virtual void setAllowNoteEvents(bool newMode=true) { allowNoteEvents = newMode; }
	virtual bool getAllowNoteEvents() { return allowNoteEvents; }

	// true means that MIDI channel in events and assignments matters; 
	// false means operate in MIDI omni mode
	virtual void setUseChannel(bool newMode=true) { useChannel = newMode; }
	virtual bool getUseChannel() { return useChannel; }

	// this tells VstChunk what you want it to do if a non-matching 
	// data chunk is received in setChunk()   (see the enum options above)
	virtual void setCrisisBehaviour(bool newCB) { crisisBehaviour = newCB; }
	virtual long getCrisisBehaviour() { return crisisBehaviour; }


	/* - - - - - - - - - hooks - - - - - - - - - */

	// these allow for additional constructor or destructor stuff, if necessary
	void init() {}
	void uninit() {}

	// these can be overridden to store & restore more data during 
	// getChunk() & setChunk() respectively
	// the data pointers point to the start of the extended data 
	// sections of the data chunks
	virtual void getChunkExtendedData(void *data, bool isPreset) {}
	virtual void setChunkExtendedData(void *data, unsigned long storedExtendedDataSize, 
										long chunkVersion, bool isPreset) {}

	// this can be overridden to react when parameter values are 
	// restored from settings that are loaded during setChunk()
	// for example, you might need to remap certain parameter values 
	// when loading settings stored by older versions of your plugin 
	// (that's why chunk version is one of the function arguments; 
	// it's the version number of the plugin that created the chunk)
	virtual void doChunkRestoreSetParameterStuff(long tag, float value, long chunkVersion, long programNum) {}

	// these can be overridden to do something & extend the MIDI event processing
	virtual void doLearningAssignStuff(long tag, long eventType, long eventChannel, 
										long eventNum, long delta, long eventNum2 = 0, 
										long eventBehaviourFlags = 0, 
										long data1 = 0, long data2 = 0, 
										float fdata1 = 0.0f, float fdata2 = 0.0f) {}
	virtual void doMidiAutomatedSetParameterStuff(long tag, float value, long delta) {}


protected:
	// reverse the byte order of data in a chunk
	void correctEndian(void *data, bool isReversed, bool isPreset=false);

	// investigates what to do when a data chunk is received in 
	// setChunk() that doesn't match what we are expecting
	long handleCrisis(long flags);
	// can be implemented to display an alert dialogue or something 
	// if kCrisisLoadButComplain crisis behaviour is being used
	void crisisAlert(long flags) {}

	// a simple but handy check to see if a parameter tag is valid
	bool paramTagIsValid(long tag)
		{ return ( (tag >= 0) && (tag < numParameters) ); }


	long numParameters, numPrograms;
	AudioEffectX *effect;

	bool midiLearn;	// switch value for MIDI learn mode
	long learner;	// the parameter currently selected for MIDI learning

	// size of one program (32-char program name + all parameter values)
	unsigned long sizeofProgram;
	// size of the table of parameter IDs (one for each parameter)
	unsigned long sizeofParameterIDs;
	// size of the single-program "preset" version of the data chunk
	unsigned long sizeofPresetChunk;
	// size of the entire data chunk (entire bank, not preset)
	unsigned long sizeofChunk;

	// This is how much larger (beyond the regular stuff) the data chunk 
	// memory allocation should be (for extending the basic data stuff).
	// It is set with an optional constructor argument (default it 0).
	unsigned long sizeofExtendedData;

	// the header info for the data chunk
	ChunkInfo chunkInfo;
	// an ordered table of IDs for each parameter stored in each program
	// (this is so that non-parameter-compatible plugin versions can load 
	// settings & know which stored parameters correspond to theirs)
	long *parameterIDs;
	// the array of which MIDI event, if any, is assigned to each parameter
	ParameterAssignment *paramAssignments;

	// this what we point the host to during getChunk()
	ChunkInfo *sharedChunk;
	// a few handy pointers into sections of our chunk
	long *firstSharedParameterID;
	Program *firstSharedProgram;
	ParameterAssignment *firstSharedParamAssignment;

	// whether to allow only one parameter assignment per MIDI event, or steal them
	bool stealAssignments;
	// whether to allow MIDI note events to be assigned to control parameters
	bool allowNoteEvents;
	// whether to allow pitchbend events to be assigned to control parameters
	bool allowPitchbendEvents;
	// what to do if setChunk() sends a data chunk with a mismatched byte size
	long crisisBehaviour;
	// whether to differentiate events and parameter assignments based 
	// on MIDI channel or whether to ignore channel (omni-style)
	bool useChannel;

	// this lets the plugin specify any MIDI control behaviour characterists 
	// for the current MIDI-learning parameter
	long learnerEventBehaviourFlags;
	// let's the plugin pass along an extra context-specific data bytes
	long learnerData1, learnerData2;
	float learnerFData1, learnerFData2;

	// if a note range is being learned for a parameter, this will be true
	bool noteRangeHalfwayDone;
	// the note that is the first part of the 2-note range being learned
	long halfwayNoteNum;
};

#endif



/*
Here's what you add to a plugin to make it use chunks & MIDI learn.  
In the header, add these includes:

#include "vstchunk.h"


Add declarations for these 3 virtual functions:

	virtual long getChunk(void **data, bool isPreset);
	virtual long setChunk(void *data, long byteSize, bool isPreset);
	virtual long processEvents(VstEvents* events);


Add a VstChunk pointer to your effect class:

	VstChunk *chunk;



Then in the source, make this call in the constructor:

	chunk = new VstChunk(NUM_PARAMETERS, NUM_PROGRAMS, PLUGIN_ID, this);


In the destructor add:

	if (chunk)
		delete chunk;


In canDo() add:
	if (strcmp(text, "receiveVstEvents") == 0)
		return 1;
	if (strcmp(text, "receiveVstMidiEvent") == 0)
		return 1;


Also add these functions somewhere, or if they're already implemented, 
add the following stuff into them:

long PLUGIN::getChunk(void **data, bool isPreset)
{	return chunk->getChunk(data, isPreset);	}

long PLUGIN::setChunk(void *data, long byteSize, bool isPreset)
{	return chunk->setChunk(data, byteSize, isPreset);	}

long PLUGIN::processEvents(VstEvents* events)
{
	chunk->processParameterEvents(events);
	return 1;
}


To resume(), add:

	wantEvents();

*/
