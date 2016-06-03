#pragma once
#include "stdafx.h"
#include "APU.h"
#include "IMemoryHandler.h"
#include "ApuEnvelope.h"

class TriangleChannel : public ApuLengthCounter
{
private:
	const vector<uint8_t> _sequence = { { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 } };

	uint8_t _linearCounter = 0;
	uint8_t _linearCounterReload = 0;
	bool _linearReloadFlag = false;
	bool _linearControlFlag = false;

	uint8_t _sequencePosition = 0;

protected:
	void Clock()
	{
		//The sequencer is clocked by the timer as long as both the linear counter and the length counter are nonzero. 
		if(_lengthCounter > 0 && _linearCounter > 0) {
			_sequencePosition = (_sequencePosition + 1) & 0x1F;

			AddOutput(_sequence[_sequencePosition]);
		}
	}

public:
	TriangleChannel(AudioChannel channel, SoundMixer* mixer) : ApuLengthCounter(channel, mixer)
	{
	}

	virtual void Reset(bool softReset)
	{
		ApuLengthCounter::Reset(softReset);
		
		_linearCounter = 0;
		_linearCounterReload = 0;
		_linearReloadFlag = false;
		_linearControlFlag = false;

		_sequencePosition = 0;
	}

	virtual void StreamState(bool saving)
	{
		ApuLengthCounter::StreamState(saving);

		Stream(_linearCounter, _linearCounterReload, _linearReloadFlag, _linearControlFlag, _sequencePosition);
	}

	void GetMemoryRanges(MemoryRanges &ranges)
	{
		ranges.AddHandler(MemoryOperation::Write, 0x4008, 0x400B);
	}

	void WriteRAM(uint16_t addr, uint8_t value)
	{
		APU::StaticRun();
		switch(addr & 0x03) {
			case 0:		//4008
				_linearControlFlag = (value & 0x80) == 0x80;
				_linearCounterReload = value & 0x7F;

				InitializeLengthCounter(_linearControlFlag);
				break;

			case 2:		//400A
				_period &= ~0x00FF;
				_period |= value;
				break;

			case 3:		//400B
				LoadLengthCounter(value >> 3);

				_period &= ~0xFF00;
				_period |= (value & 0x07) << 8;

				//Side effects 	Sets the linear counter reload flag 
				_linearReloadFlag = true;
				break;
		}
	}

	void TickLinearCounter()
	{
		if(_linearReloadFlag) {
			_linearCounter = _linearCounterReload;
		} else if(_linearCounter > 0) {
			_linearCounter--;
		}

		if(!_linearControlFlag) {
			_linearReloadFlag = false;
		}
	}
};