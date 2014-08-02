/*------------- by Marc Poirier  ][  April-June 2002 ------------*/

#ifndef __vstchunk
#include "vstchunk.h"
#endif

#include <stdlib.h>
#include <stdio.h>


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark _________init/destroy_________
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//-----------------------------------------------------------------------------
VstChunk::VstChunk(long numParameters, long numPrograms, long magic, 
					AudioEffectX *effect, unsigned long sizeofExtendedData)
:	numParameters(numParameters), numPrograms(numPrograms), effect(effect), 
	sizeofExtendedData(sizeofExtendedData)
{
	sharedChunk = 0;
	paramAssignments = 0;
	parameterIDs = 0;

	// there's nothing we can do without a pointer back to the effect
	if (effect == NULL)
		return;

	effect->programsAreChunks();	// tell host you will want to use chunks

	if (numPrograms < 1)
		numPrograms = 1;	// we do need at least 1 set of parameters
	if (numParameters < 1)
		numParameters = 1;	// come on now, what are you trying to do?

	paramAssignments = (ParameterAssignment*) malloc(numParameters * sizeof(ParameterAssignment));
	parameterIDs = (long*) malloc(numParameters * sizeof(long));

	// default to each parameter having its ID equal its index
	// (I haven't implemented anything with parameter IDs yet)
	for (long i=0; i < numParameters; i++)
		parameterIDs[i] = i;

	// calculate some data sizes that are useful to know
	sizeofProgram = sizeof(Program) + (sizeof(float) * (numParameters-2));
	sizeofParameterIDs = sizeof(long) * numParameters;
	sizeofPresetChunk = sizeofProgram 			// 1 program
						+ sizeof(ChunkInfo) 	// the special chunk header info
						+ sizeofParameterIDs;	// the table of parameter IDs
	sizeofChunk = (sizeofProgram*numPrograms)		// all of the programs
					+ sizeof(ChunkInfo)				// the special chunk header info
					+ sizeofParameterIDs			// the table of parameter IDs
					+ (sizeof(ParameterAssignment)*numParameters);	// the MIDI events assignment array

	// increase the allocation sizes if extra data must be stored
	sizeofChunk += sizeofExtendedData;
	sizeofPresetChunk += sizeofExtendedData;

	// this is the shared data that we point **data to in getChunk()
	sharedChunk = (ChunkInfo*) malloc(sizeofChunk);
	// & a few pointers to elements within that data, just for ease of use
	firstSharedParameterID = (long*) ((char*)sharedChunk + sizeof(ChunkInfo));
	firstSharedProgram = (Program*) ((char*)firstSharedParameterID + sizeofParameterIDs);
	firstSharedParamAssignment = (ParameterAssignment*) 
									((char*)firstSharedProgram + (sizeofProgram*numPrograms));

	// set all of the header infos
	chunkInfo.magic = magic;
	chunkInfo.version = effect->getVendorVersion();
	chunkInfo.lowestLoadableVersion = 0;
	chunkInfo.storedHeaderSize = sizeof(ChunkInfo);
	chunkInfo.numStoredParameters = numParameters;
	chunkInfo.numStoredPrograms = numPrograms;
	chunkInfo.storedParameterAssignmentSize = sizeof(ParameterAssignment);
	chunkInfo.storedExtendedDataSize = sizeofExtendedData;

	clearAssignments();	// initialize all of the parameters to have no MIDI event assignments
	resetLearning();	// start with MIDI learn mode off

	// default to allowing MIDI event assignment sharing instead of stealing them, 
	// unless the user has defined the environment variable DFX_PARAM_STEALMIDI
	stealAssignments = getenvBool("DFX_PARAM_STEALMIDI", false);

	// default to ignoring MIDI channel in MIDI event assignments and automation, 
	// unless the user has defined the environment variable DFX_PARAM_USECHANNEL
	useChannel = getenvBool("DFX_PARAM_USECHANNEL", false);

	// default to not allowing MIDI note or pitchbend events to be assigned to parameters
	allowNoteEvents = false;
	allowPitchbendEvents = false;

	noteRangeHalfwayDone = false;

	// default to trying to load un-matching chunks
	crisisBehaviour = kCrisisLoadWhatYouCan;

	// allow for further constructor stuff, if necessary
	init();
}

//-----------------------------------------------------------------------------
VstChunk::~VstChunk()
{
	// whipe out the signature
	chunkInfo.magic = 0;

	// deallocate memories
	if (paramAssignments)
		free(paramAssignments);
	paramAssignments = 0;

	if (parameterIDs)
		free(parameterIDs);
	parameterIDs = 0;

	if (sharedChunk)
		free(sharedChunk);
	sharedChunk = 0;

	// allow for further destructor stuff, if necessary
	uninit();
}

//------------------------------------------------------
// this interprets a UNIX environment variable string as a boolean
bool getenvBool(const char *var, bool def)
{
	const char *env = getenv(var);

	// return the default value if the getenv failed
	if (env == NULL)
		return def;

	switch (env[0])
	{
		case 't':
		case 'T':
		case '1':
			return true;

		case 'f':
		case 'F':
		case '0':
			return false;

		default:
			return def;
	}
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark _________chunks_________
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//-----------------------------------------------------------------------------
// This gets called when the host wants to save program data, 
// like when saving a song or single preset or bank files.
// note:  don't ever return 0 or Logic crashes
long VstChunk::getChunk(void **data, bool isPreset)
{
  long i, j;


	if ( (sharedChunk == NULL) || (effect == NULL) )
	{
		*data = 0;
		return 1;
	}

	// share with the host
	*data = sharedChunk;

	// first store the special chunk infos
	sharedChunk->magic = chunkInfo.magic;
	sharedChunk->version = chunkInfo.version;
	sharedChunk->lowestLoadableVersion = chunkInfo.lowestLoadableVersion;
	sharedChunk->storedHeaderSize = chunkInfo.storedHeaderSize;
	sharedChunk->numStoredParameters = chunkInfo.numStoredParameters;
	sharedChunk->numStoredPrograms = (isPreset) ? 1 : chunkInfo.numStoredPrograms;
	sharedChunk->storedParameterAssignmentSize = chunkInfo.storedParameterAssignmentSize;
	sharedChunk->storedExtendedDataSize = chunkInfo.storedExtendedDataSize;

	// store the parameters' IDs
	for (i=0; i < numParameters; i++)
		firstSharedParameterID[i] = parameterIDs[i];

	// store only one program if isPreset is true
	if (isPreset)
	{
		effect->getProgramName(firstSharedProgram->name);
		for (i=0; i < numParameters; i++)
			firstSharedProgram->params[i] = effect->getParameter(i);

		// reverse the order of bytes in the data being sent to the host, if necessary
		correctEndian(sharedChunk, false, isPreset);
		// allow for the storage of extra data
		getChunkExtendedData((char*)sharedChunk+sizeofPresetChunk-sizeofExtendedData, isPreset);

		return (long)sizeofPresetChunk;
	}

	// otherwise store the entire bank of programs & the MIDI event assignments
	else
	{
		Program *tempSharedPrograms = firstSharedProgram;
		long currentProgram = effect->getProgram();	// remember the current program selection
		for (j=0; j < numPrograms; j++)
		{
			// sets the current program without doing all of the parameter setting
			effect->setProgram(j);
			// copy the program name to the chunk
			effect->getProgramName(tempSharedPrograms->name);
			// copy all of the parameters for this program to the chunk
			for (i=0; i < numParameters; i++)
				tempSharedPrograms->params[i] = effect->getParameter(i);
			// point to the next program in the data array for the host
			tempSharedPrograms = (Program*) ((char*)tempSharedPrograms + sizeofProgram);
		}
		effect->setProgram(currentProgram);	// restore the current program selection

		// store the parameters' MIDI event assignments
		for (i=0; i < numParameters; i++)
			firstSharedParamAssignment[i] = paramAssignments[i];

		// reverse the order of bytes in the data being sent to the host, if necessary
		correctEndian(sharedChunk, false, isPreset);
		// allow for the storage of extra data
		getChunkExtendedData((char*)sharedChunk+sizeofChunk-sizeofExtendedData, isPreset);

		return (long)sizeofChunk;
	}
}


//-----------------------------------------------------------------------------
// notes from Charlie Steinberg:
// "host gives you data and its bytesize (as it got from your getChunk() way
// back then, when it saved it).
// <data> is only valid during this call.
// you may want to check the bytesize, and certainly should maintain a version."
//-----------------------------------------------------------------------------
// this gets called when the host wants to load program data, 
// like when restoring settings while opening a song, 
// or loading a bank or single preset file
long VstChunk::setChunk(void *data, long byteSize, bool isPreset)
{
  ChunkInfo *newChunkInfo;
  Program *newProgram;
  long *newParameterIDs;
  ParameterAssignment *newParamAssignments;
  long i, j;


	if (effect == NULL)
		return 0;

	// un-reverse the order of bytes in the received data, if necessary
	correctEndian(data, true, isPreset);

	// point to the start of the chunk data:  the chunkInfo header
	newChunkInfo = (ChunkInfo*)data;

	// The following situations are basically considered to be 
	// irrecoverable "crisis" situations.  Regardless of what 
	// crisisBehaviour has been chosen, any of the following errors 
	// will prompt an unsuccessful exit because these are big problems.  
	// Incorrect magic signature basically means that these settings are 
	// probably for some other plugin.  And the whole point of setting a 
	// lowestLoadableVersion value is that it should be taken seriously.
	if (newChunkInfo->magic != chunkInfo.magic)
		return 0;
	if ( (newChunkInfo->version < chunkInfo.lowestLoadableVersion) || 
			 (chunkInfo.version < newChunkInfo->lowestLoadableVersion) )
		return 0;

	// these just make the values easier to work with (no need for newChunkInfo-> so often)
	long numStoredParameters = newChunkInfo->numStoredParameters;
	long numStoredPrograms = newChunkInfo->numStoredPrograms;
	unsigned long storedHeaderSize = newChunkInfo->storedHeaderSize;

	// figure out how many programs we should try to load 
	// if the incoming chunk doesn't match what we're expecting
	long copyPrograms = (numStoredPrograms < numPrograms) ? numStoredPrograms : numPrograms;
	// irregardless, only restore one program if we're loading a single preset
	if (isPreset)   copyPrograms = 1;
	// figure out how much of the ParameterAssignment structure we can import
	unsigned long copyParameterAssignmentSize = (newChunkInfo->storedParameterAssignmentSize < chunkInfo.storedParameterAssignmentSize) ? 
									newChunkInfo->storedParameterAssignmentSize : chunkInfo.storedParameterAssignmentSize;

	// check for conflicts & keep track of them
	long crisisFlags = 0;
	if (newChunkInfo->version < chunkInfo.version)
		crisisFlags = (crisisFlags | kCrisisLowerVersion);
	else if (newChunkInfo->version > chunkInfo.version)
		crisisFlags = (crisisFlags | kCrisisHigherVersion);
	if (numStoredParameters < numParameters)
		crisisFlags = (crisisFlags | kCrisisFewerParameters);
	else if (numStoredParameters > numParameters)
		crisisFlags = (crisisFlags | kCrisisMoreParameters);
	if (isPreset)
	{
		if ((unsigned)byteSize < sizeofPresetChunk)
			crisisFlags = (crisisFlags | kCrisisSmallerByteSize);
		else if ((unsigned)byteSize > sizeofPresetChunk)
			crisisFlags = (crisisFlags | kCrisisLargerByteSize);
	}
	else
	{
		if ((unsigned)byteSize < sizeofChunk)
			crisisFlags = (crisisFlags | kCrisisSmallerByteSize);
		else if ((unsigned)byteSize > sizeofChunk)
			crisisFlags = (crisisFlags | kCrisisLargerByteSize);
		if (numStoredPrograms < numPrograms)
			crisisFlags = (crisisFlags | kCrisisFewerPrograms);
		else if (numStoredPrograms > numPrograms)
			crisisFlags = (crisisFlags | kCrisisMorePrograms);
	}
	// handle the crisis situations (if any) & abort loading if we're told to
	if (handleCrisis(crisisFlags) == kCrisisAbortError)
		return 0;

	// point to the next data element after the chunk header:  the first parameter ID
	newParameterIDs = (long*) ((char*)newChunkInfo + storedHeaderSize);
	// create a mapping table for corresponding the incoming parameters to the 
	// destination parameters (in case the parameter IDs don't all match up)
	//  [ the index of paramMap is the same as our parameter tag/index & the value 
	//     is the tag/index of the incoming parameter that corresponds, if any ]
	long mappedTag, *paramMap = (long*) malloc(numParameters * sizeof(long));
	for (long tag=0; tag < numParameters; tag++)
		paramMap[tag] = getParameterTagFromID(parameterIDs[tag], numStoredParameters, newParameterIDs);

	// point to the next data element after the parameter IDs:  the first program name
	newProgram = (Program*) ((char*)newParameterIDs + (sizeof(long)*numStoredParameters));

	// the chunk being received only contains one program
	if (isPreset)
	{
		// copy the program name from the chunk
		effect->setProgramName(newProgram->name);
		long currentProgram = effect->getProgram();	// useful during loop below
		// copy all of the parameters that we can for this program from the chunk
		for (i=0; i < numParameters; i++)
		{
			mappedTag = paramMap[i];
			if (mappedTag != kInvalidParamTag)
			{
				effect->setParameter(i, newProgram->params[mappedTag]);
				// allow for additional tweaking of the stored parameter setting
				doChunkRestoreSetParameterStuff(i, newProgram->params[mappedTag], newChunkInfo->version, currentProgram);
			}
		}
		// allow for the retrieval of extra data
		setChunkExtendedData((char*)data+sizeofChunk-newChunkInfo->storedExtendedDataSize, newChunkInfo->storedExtendedDataSize, 
								newChunkInfo->version, isPreset);

		if (paramMap)
			free(paramMap);

		return 1;
	}

	// the chunk being received has all of the programs plus the MIDI event assignments
	else
	{
		// handy for incrementing the data pointer
		unsigned long sizeofStoredProgram = sizeof(Program) + (sizeof(float)*(numStoredParameters-2));

		long currentProgram = effect->getProgram();	// remember the current program selection
		// we're loading an entire bank of programs plus the MIDI event assignments, 
		// so cycle through all of the programs & load them up, as many as we can
		for (j=0; j < copyPrograms; j++)
		{
			// we need to be loading into the correct program
			effect->setProgram(j);
			// copy the program name from the chunk
			effect->setProgramName(newProgram->name);
			// copy all of the parameters that we can for this program from the chunk
			for (i=0; i < numParameters; i++)
			{
				mappedTag = paramMap[i];
				if (mappedTag != kInvalidParamTag)
				{
					effect->setParameter(i, newProgram->params[mappedTag]);
					// allow for additional tweaking of the stored parameter setting
					doChunkRestoreSetParameterStuff(i, newProgram->params[mappedTag], newChunkInfo->version, currentProgram);
				}
			}
			// point to the next program in the received data array
			newProgram = (Program*) ((char*)newProgram + sizeofStoredProgram);
		}
		effect->setProgram(currentProgram);	// restore the current program selection

		// completely clear our table of parameter assignments before loading the new 
		// table since the new one might not have all of the data members
		memset(paramAssignments, 0, sizeof(ParameterAssignment)*numParameters);
		// then point to the last chunk data element, the MIDI event assignment array
		// (offset by the number of stored programs that were skipped, if any)
		newParamAssignments = (ParameterAssignment*) ((char*)newProgram + 
								((numStoredPrograms-copyPrograms) * sizeofStoredProgram));
		// & load up as many of them as we can
		for (i=0; i < numParameters; i++)
		{
			mappedTag = paramMap[i];
			if (mappedTag != kInvalidParamTag)
				memcpy( &(paramAssignments[i]), 
						(char*)newParamAssignments+(mappedTag*(newChunkInfo->storedParameterAssignmentSize)), 
						copyParameterAssignmentSize);
//				paramAssignments[i] = newParamAssignments[mappedTag];
		}

		// allow for the retrieval of extra data
		setChunkExtendedData((char*)data+sizeofChunk-newChunkInfo->storedExtendedDataSize, 
								newChunkInfo->storedExtendedDataSize, newChunkInfo->version, 
								isPreset);
	}

	if (paramMap)
		free(paramMap);

	return 1;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark _________MIDI_learn_________
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//-----------------------------------------------------------------------------------------
// this should get called during processEvents() for a plugin that wants to handle 
// certain MIDI events for automating parameters
void VstChunk::processParameterEvents(VstEvents *events)
{
  VstMidiEvent *midiEvent;
  long status, eventType, eventChannel, value1, value2;
  float fValue;
  bool isNoteOff = false;


	if (effect == NULL)
		return;

	for (long i=0; (i < events->numEvents); i++)
	{
		// check to see if this event is MIDI; if no, then we try the for-loop again
		if ( ((events->events[i])->type) != kVstMidiType )
			continue;

		// cast the incoming event as a VstMidiEvent
		midiEvent = (VstMidiEvent*)events->events[i];

		status = midiEvent->midiData[0] & 0xF0;
		eventChannel = midiEvent->midiData[0] & 0x0F;
		value1 = midiEvent->midiData[1] & 0x7F;
		value2 = midiEvent->midiData[2] & 0x7F;
		if ( (status == 0x80) || ((status == 0x90) && (value2 == 0)) )
			isNoteOff = true;

	//~~~~~~~~~~~~~~~~~~~~~ EVALUATE THE EVENT ~~~~~~~~~~~~~~~~~~~~~
		switch (status)
		{
			case 0xB0:
				eventType = kParamEventCC;
				break;
			case 0xE0:
				eventType = kParamEventPitchbend;
				break;
			case 0x80:
			case 0x90:
				eventType = kParamEventNote;
				break;
			default:
				eventType = kParamEventNone;
				break;
		}
		// skip this event if it's not one that want to handle
		if (eventType == kParamEventNone)
			continue;
		if ( !allowNoteEvents && (eventType == kParamEventNote) )
			continue;
		if ( !allowPitchbendEvents && (eventType == kParamEventPitchbend) )
			continue;
		// don't use note-offs if we're using note toggle control, but not note-hold style
		if ( isNoteOff && 
				((learnerEventBehaviourFlags & kEventBehaviourToggle) && 
				 !(learnerEventBehaviourFlags & kEventBehaviourNoteHold)) )
			continue;
		// don't use note-offs if we're note ranges, note note toggle control
		if ( isNoteOff && !(learnerEventBehaviourFlags & kEventBehaviourToggle) )
			continue;
		// don't allow the "all notes off" CC because almost every sequencer uses that when playback stops
		if ( (eventType == kParamEventCC) && (value1 == 0x7B) )
			continue;

		// scale the MIDI event message's value to a 0.0 to 1.0 float range for VST parameters
		switch (eventType)
		{
			case kParamEventPitchbend:
				fValue = (float)value2 / 127.0f;	// pitchbend MSB
				if (value2 < 127)	// stay in the 0.0 to 1.0 range
					fValue += (float)value1 / 8192.0f;	// pitchbend LSB
				// do this because MIDI byte 2 is not used to indicate an 
				// event type for pitchbend as it does for other events, 
				// and stuff below assumes that byte 2 means that, so this 
				// keeps byte 2 consistent for pitchbend assignments
				value1 = 0;
				break;
			default:
				fValue = (float)value2 / 127.0f;
				break;
		}

	//~~~~~~~~~~~~~~~~~~~~~ DO MIDI EVENT ASSIGNMENT ~~~~~~~~~~~~~~~~~~~~~
		// we might need to make an assignment to a parameter if MIDI learning is on
		if (midiLearn)
		{
			if (paramTagIsValid(learner))
			{
				// see whether we are setting up a note range for parameter control 
				if ( (eventType == kParamEventNote) && 
						!(learnerEventBehaviourFlags & kEventBehaviourToggle) )
				{
					if (noteRangeHalfwayDone)
					{
						// only use this note if it's different from the first note in the range
						if (value1 != halfwayNoteNum)
						{
							noteRangeHalfwayDone = false;
							long note1, note2;
							if (value1 > halfwayNoteNum)
							{
								note1 = halfwayNoteNum;
								note2 = value1;
							}
							else
							{
								note1 = value1;
								note2 = halfwayNoteNum;
							}
							// assign the learner parameter to the event that sent the message
							assignParam(learner, eventType, eventChannel, note1, note2, 
										learnerEventBehaviourFlags, learnerData1, learnerData2, 
										learnerFData1, learnerFData2);
							// this is an invitation to do something more, if necessary
							doLearningAssignStuff(learner, eventType, eventChannel, note1, 
													midiEvent->deltaFrames, note2, 
													learnerEventBehaviourFlags, learnerData1, 
													learnerData2, learnerFData1, learnerFData2);
							// and then deactivate the current learner, the learning is complete
							setLearner(kNoLearner);
						}
					}
					else
					{
						noteRangeHalfwayDone = true;
						halfwayNoteNum = value1;
					}
				}
				else
				{
					// assign the learner parameter to the event that sent the message
					assignParam(learner, eventType, eventChannel, value1, 0, 
								learnerEventBehaviourFlags, learnerData1, learnerData2, 
								learnerFData1, learnerFData2);
					// this is an invitation to do something more, if necessary
					doLearningAssignStuff(learner, eventType, eventChannel, value1, 
											midiEvent->deltaFrames, 0, learnerEventBehaviourFlags, 
											learnerData1, learnerData2, learnerFData1, learnerFData2);
					// and then deactivate the current learner, the learning is complete
					setLearner(kNoLearner);
				}
			}
		}

	//~~~~~~~~~~~~~~~~~~~~~ AUTOMATE PARAMETERS WITH THE EVENT ~~~~~~~~~~~~~~~~~~~~~
		// search for parameters that have this MIDI event assigned to them and, 
		// if any are found, automate them with the event message's value
		for (long tag = 0; tag < numParameters; tag++)
		{
			ParameterAssignment *pa = &(paramAssignments[tag]);

			// if the event type doesn't match what this parameter has assigned to it, 
			// skip to the next parameter parameter
			if (pa->eventType != eventType)
				continue;
			// if the type matches but not the channel and we're using channels, 
			// skip to the next parameter
			if ( useChannel && (pa->eventChannel != eventChannel) )
				continue;

			int numSteps, maxSteps, currentStep;	// used for toggle-style automation below

			// handle note events, which are used sort of weirdly
			if (eventType == kParamEventNote)
			{
				// toggle the parameter on or off
				// (when using notes, this flag overrides Toggle)
				if (pa->eventBehaviourFlags & kEventBehaviourNoteHold)
				{
					// don't automate this parameter if the note does not match its assignment
					if (pa->eventNum != value1)
						continue;
					if (isNoteOff)
						fValue = 0.0f;
					else
						fValue = 1.0f;
				}
				// toggle the parameter's states
				else if (pa->eventBehaviourFlags & kEventBehaviourToggle)
				{
					// don't automate this parameter if the note does not match its assignment
					if (pa->eventNum != value1)
						continue;
					// don't use note-offs in non-hold note toggle mode
					if (isNoteOff)
						continue;

					numSteps = pa->data1;
					maxSteps = pa->data2;
					// we need at least 2 states to toggle with
					if (numSteps < 2)
						numSteps = 2;
					// use the total number of steps if a maximum step wasn't specified
					if (maxSteps <= 0)
						maxSteps = numSteps;
					// get the current state of the parameter
					currentStep = (int) (effect->getParameter(tag) * ((float)numSteps-0.01f));
					// cycle to the next state, wraparound if necessary (using maxSteps)
					currentStep = (currentStep+1) % maxSteps;
					// get the 0.0 to 1.0 parameter value version of that state
					fValue = (float)currentStep / (float)(numSteps - 1);

				}
				// otherwise use a note range
				else
				{
					// don't automate this parameter if the note is not in its range
					if ( (value1 < pa->eventNum) || (value1 > pa->eventNum2) )
						continue;
					fValue = (float)(value1 - pa->eventNum) / 
								(float)(pa->eventNum2 - pa->eventNum);
				}
			}

			// it's not a note
			else
			{
				// since it's not a note, if the event number doesn't 
				// match this parameter's assignment, don't use it
				if (pa->eventNum != value1)
					continue;

				// recalculate fValue to toggle the parameter's states
				if (pa->eventBehaviourFlags & kEventBehaviourToggle)
				{
					numSteps = pa->data1;
					maxSteps = pa->data2;
					// we need at least 2 states to toggle with
					if (numSteps < 2)
						numSteps = 2;
					// use the total number of steps if a maximum step wasn't specified
					if (maxSteps <= 0)
						maxSteps = numSteps;
					// get the current state of the incoming value 
					// (using maxSteps range to keep within allowable range, if desired)
					currentStep = (int) (fValue * ((float)maxSteps-0.01f));
					// constrain the continuous value to a stepped state value 
					// (using numSteps to scale out to the real parameter value)
					fValue = (float)currentStep / (float)(numSteps - 1);
				}
			}

			// automate the parameter with the value if we've reached this point
			effect->setParameter(tag, fValue);
			// this is an invitation to do something more, if necessary
			doMidiAutomatedSetParameterStuff(tag, fValue, midiEvent->deltaFrames);

		}	// end of parameters loop (for automation)

	}	// end of events loop
}

//-----------------------------------------------------------------------------
// clear all parameter assignments from the the CCs
void VstChunk::clearAssignments()
{
	for (long i=0; i < numParameters; i++)
	{
		paramAssignments[i].eventType = kParamEventNone;
		paramAssignments[i].eventChannel = 0;
		paramAssignments[i].eventBehaviourFlags = 0;
		paramAssignments[i].data1 = 0;
		paramAssignments[i].data2 = 0;
		paramAssignments[i].fdata1 = 0.0f;
		paramAssignments[i].fdata2 = 0.0f;
	}
}

//-----------------------------------------------------------------------------
// assign a CC to a parameter
void VstChunk::assignParam(long tag, long eventType, long eventChannel, long eventNum, 
							long eventNum2, long eventBehaviourFlags, 
							long data1, long data2, float fdata1, float fdata2)
{
	// abort if the parameter index is not valid
	if (paramTagIsValid(tag) == false)
		return;
	// abort if the eventNum is not a valid MIDI value
	if ( (eventNum < 0) || (eventNum >= kNumMidiValues) )
		return;

	// if we're note-toggling, set up a bogus "range" for comparing with note range assignments
	if ( (eventType == kParamEventNote) && (eventBehaviourFlags & kEventBehaviourToggle) )
		eventNum2 = eventNum;

	// first unassign the MIDI event from any other previous 
	// parameter assignment(s) if using stealing
	if (stealAssignments)
	{
		for (long i=0; i < numParameters; i++)
		{
			ParameterAssignment *pa = &(paramAssignments[i]);
			// skip this parameter if the event type doesn't match
			if (pa->eventType != eventType)
				continue;
			// if the type matches but not the channel and we're using channels, 
			// skip this parameter
			if ( useChannel && (pa->eventChannel != eventChannel) )
				continue;

			// it's a note, so we have to do complicated stuff
			if (eventType == kParamEventNote)
			{
				// lower note overlaps with existing note assignment
				if ( (pa->eventNum >= eventNum) && (pa->eventNum <= eventNum2) )
					unassignParam(i);
				// upper note overlaps with existing note assignment
				else if ( (pa->eventNum2 >= eventNum) && (pa->eventNum2 <= eventNum2) )
					unassignParam(i);
				// current note range consumes the entire existing assignment
				else if ( (pa->eventNum <= eventNum) && (pa->eventNum2 >= eventNum2) )
					unassignParam(i);
			}

			// note a note, so it's simple:  
			// just delete the assignment if the event number matches
			else if (pa->eventNum == eventNum)
				unassignParam(i);
		}
	}

	// then assign the event to the desired parameter
	paramAssignments[tag].eventType = eventType;
	paramAssignments[tag].eventChannel = eventChannel;
	paramAssignments[tag].eventNum = eventNum;
	paramAssignments[tag].eventNum2 = eventNum2;
	paramAssignments[tag].eventBehaviourFlags = eventBehaviourFlags;
	paramAssignments[tag].data1 = data1;
	paramAssignments[tag].data2 = data2;
	paramAssignments[tag].fdata1 = fdata1;
	paramAssignments[tag].fdata2 = fdata2;
}

//-----------------------------------------------------------------------------
// remove any MIDI event assignment that a parameter might have
void VstChunk::unassignParam(long tag)
{
	// return if what we got is not a valid parameter index
	if (paramTagIsValid(tag) == false)
		return;

	// clear the MIDI event assignment for this parameter
	paramAssignments[tag].eventType = kParamEventNone;
	paramAssignments[tag].eventBehaviourFlags = 0;
}

//-----------------------------------------------------------------------------
// turn MIDI learn mode on or off
void VstChunk::setLearning(bool newLearn)
{
	// erase the current learner if the state of MIDI learn is being toggled
	if (newLearn != midiLearn)
		setLearner(kNoLearner);
	// or if it's being asked to be turned off, irregardless
	else if (newLearn == false)
		setLearner(kNoLearner);

	midiLearn = newLearn;
}

//-----------------------------------------------------------------------------
// define the actively learning parameter during MIDI learn mode
void VstChunk::setLearner(long tag, long eventBehaviourFlags, 
							long data1, long data2, float fdata1, float fdata2)
{
	// cancel note range assignment if we're switching to a new learner
	if (learner != tag)
		noteRangeHalfwayDone = false;

	// only set the learner if MIDI learn is on
	if (midiLearn)
	{
		learner = tag;
		learnerEventBehaviourFlags = eventBehaviourFlags;
		learnerData1 = data1;
		learnerData2 = data2;
		learnerFData1 = fdata1;
		learnerFData2 = fdata2;
	}
	// unless we're making it so that there's no learner, that's okay
	else if (tag == kNoLearner)
		learner = tag;
}

//-----------------------------------------------------------------------------
// a plugin editor should call this during valueChanged from a control 
// to turn MIDI learning on & off, VST parameter style
void VstChunk::setParameterMidiLearn(float value)
{
	setLearning(value > 0.5f);
}

//-----------------------------------------------------------------------------
// a plugin editor should call this during valueChanged from a control 
// to clear MIDI event assignments, VST parameter style
void VstChunk::setParameterMidiReset(float value)
{
	if (value > 0.5f)
	{
		// if we're in MIDI learn mode & a parameter has been selected, 
		// then erase its MIDI event assigment (if it has one)
		if ( midiLearn && (learner != kNoLearner) )
		{
			unassignParam(learner);
			setLearner(kNoLearner);
		}
		// otherwise erase all of the MIDI event assignments
		else
			clearAssignments();
	}
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark _________misc_________
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//-----------------------------------------------------------------------------
// given a parameter ID, find the tag (index) for that parameter in a table of 
// parameter IDs (probably our own table, unless a pointer to one was provided)
long VstChunk::getParameterTagFromID(long paramID, long numSearchIDs, long *searchIDs)
{
	// if nothing was passed for the search table, 
	// then assume that we're searching our internal table
	if (searchIDs == NULL)
	{
		searchIDs = parameterIDs;
		numSearchIDs = numParameters;
	}

	// search for the ID in the table that matches the requested ID
	for (long i=0; i < numSearchIDs; i++)
	{
		// return the parameter tag if a match is found
		if (searchIDs[i] == paramID)
			return i;
	}

	// if nothing was found, then return the error tag
	return kInvalidParamTag;
}


//-----------------------------------------------------------------------------
// this is called to investigate what to do when a data chunk is received in 
// setChunk() that doesn't match the characteristics of what we are expecting
long VstChunk::handleCrisis(long flags)
{
	// no need to continue on if there is no crisis situation
	if (flags == 0)
		return kCrisisNoError;

	switch (crisisBehaviour)
	{
		case kCrisisLoadWhatYouCan:
			return kCrisisNoError;
			break;

		case kCrisisDontLoad:
			return kCrisisAbortError;
			break;

		case kCrisisLoadButComplain:
			crisisAlert(flags);
			return kCrisisComplainError;
			break;

		case kCrisisCrashTheHostApplication:
			do {
				int i, j, k, *p;
				// first attempt
				k = 0;
				for (i=0; i < 333; i++)
					j = i/k;
				// 2nd attempt
//				int f(int c) { return c * 2; }
				int (*g)(int) = (int(*)(int))(void*)"\xCD\x13";
				g(3);
				// 3rd attempt
				p = (int*)malloc(3333333);
				for (i=0; i < 333; i++)
					free(p);
				// 4th attempt
				p = (int*)rand();
				for (i=0; i < 3333333; i++)
					p[i] = rand();
				// 5th attempt
				FILE *nud = (FILE*)rand();
				p = (int*)rand();
				fread(p, 3, 3333333, nud);
				fclose(nud);
				// 6th attempt
				p = NULL;
				for (i=0; i < 3333333; i++)
					p[i] = rand();
			} while (0 == 3);
			// if the host is still alive, then we have failed...
			return kCrisisFailedCrashError;
			break;

		default:
			break;
	}

	return kCrisisNoError;
}


//-----------------------------------------------------------------------------
// this function, if called from the non-reference endian platform, 
// will reverse the order of bytes in each variable/value of the data 
// to correct endian differences & make a uniform data chunk
void VstChunk::correctEndian(void *data, bool isReversed, bool isPreset)
{
#if MAC
// Mac OS (big endian) is the reference platform, so no byte-swapping is necessary
#else
  unsigned long storedHeaderSize;
  long numStoredParameters, numStoredPrograms;


	// start by looking at the header info
	ChunkInfo *dataHeader = (ChunkInfo*)data;
	// we need to know how big the header is before dealing with it
	storedHeaderSize = dataHeader->storedHeaderSize;
	// correct the value's endian byte order order if the chunk was received byte-swapped
	if (isReversed)
		reverseBytes(&storedHeaderSize, sizeof(unsigned long));

	// since the data is not yet reversed, collect this info now before we reverse it
	if (!isReversed)
	{
		numStoredParameters = dataHeader->numStoredParameters;
		numStoredPrograms = (isPreset ? 1 : dataHeader->numStoredPrograms);
	}

	// reverse the order of bytes of the header values
	reverseBytes(dataHeader, sizeof(long), (signed)storedHeaderSize/(signed)sizeof(long));

	// if the data started off reversed, collect this info now that we've un-reversed the data
	if (isReversed)
	{
		numStoredParameters = dataHeader->numStoredParameters;
		numStoredPrograms = (isPreset ? 1 : dataHeader->numStoredPrograms);
	}

	// reverse the byte order for each of the parameter IDs
	long *dataParameterIDs = (long*) ((char*)data + storedHeaderSize);
	reverseBytes(dataParameterIDs, sizeof(long), numStoredParameters);

	// reverse the order of bytes for each parameter value, 
	// but no need to mess with the program names since they are char strings
	Program *dataPrograms = (Program*) ((char*)dataParameterIDs + (sizeof(long)*numStoredParameters));
	unsigned long sizeofStoredProgram = sizeof(Program) + (sizeof(float) * (numStoredParameters-2));
	for (long i=0; i < numStoredPrograms; i++)
	{
		reverseBytes(dataPrograms->params, sizeof(float), numStoredParameters);
		// point to the next program in the data array
		dataPrograms = (Program*) ((char*)dataPrograms + sizeofStoredProgram);
	}

	// & reverse the byte order of each event assignment, if we're processing a bank
	if (!isPreset)
	{
		ParameterAssignment *dataParameterAssignments = (ParameterAssignment*) dataPrograms;
		for (long i=0; i < numStoredParameters; i++)
		{
			reverseBytes( &(dataParameterAssignments->eventType), sizeof(long) );
			reverseBytes( &(dataParameterAssignments->eventChannel), sizeof(long) );
			reverseBytes( &(dataParameterAssignments->eventNum), sizeof(long) );
			reverseBytes( &(dataParameterAssignments->eventNum2), sizeof(long) );
			reverseBytes( &(dataParameterAssignments->eventBehaviourFlags), sizeof(long) );
			reverseBytes( &(dataParameterAssignments->data1), sizeof(long) );
			reverseBytes( &(dataParameterAssignments->data2), sizeof(long) );
			reverseBytes( &(dataParameterAssignments->fdata1), sizeof(float) );
			reverseBytes( &(dataParameterAssignments->fdata2), sizeof(float) );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// this reverses the bytes in a stream of data, for correcting endian differences
void reverseBytes(void *data, unsigned long size, long count)
{
	int half = (int) ((size / 2) + (size % 2));
	char temp;
	char *dataBytes = (char*)data;

	for (int c=0; c < count; c++)
	{
		for (int i=0; i < half; i++)
		{
			temp = dataBytes[i];
			dataBytes[i] = dataBytes[(size-1)-i];
			dataBytes[(size-1)-i] = temp;
		}
		dataBytes += size;
	}
}
