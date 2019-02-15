/*
AbstractMultiChannelPowerSupply.h
!CHAOS
Created by CUGenerator

Copyright 2013 INFN, National Institute of Nuclear Physics
Licensed under the Apache License, Version 2.0 (the "License")
you may not use this file except in compliance with the License.
      You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include <inttypes.h>
#include <string>
#ifndef __common_AbstractMultiChannelPowerSupply_h__
#define __common_AbstractMultiChannelPowerSupply_h__
namespace common {
	namespace multichannelpowersupply {
		class AbstractMultiChannelPowerSupply {
		  public:
			AbstractMultiChannelPowerSupply() {};
			virtual ~AbstractMultiChannelPowerSupply() {};
			virtual int UpdateHV(std::string& crateData)=0;
			virtual int getSlotConfiguration(std::vector<int32_t>& slotNumberList,std::vector<int32_t>& channelsPerSlot)=0;
			virtual int setChannelVoltage(int32_t slot,int32_t channel,double voltage)=0;
			virtual int setChannelCurrent(int32_t slot,int32_t channel,double current)=0;
			virtual int getChannelParametersDescription(std::string& outJSONString)=0;
			virtual int setChannelParameter(int32_t slot,int32_t channel,std::string paramName,std::string paramValue)=0;
			virtual int PowerOn(int32_t slot,int32_t channel,int32_t onState)=0;
		};
	}
#define UPMASK(bitmask,powerofTwo) \
bitmask |= powerofTwo 

#define DOWNMASK(bitmask,powerofTwo) \
bitmask=bitmask & (~powerofTwo) 

#define CHECKMASK(bitmask,powerofTwo) \
((bitmask & powerofTwo) != 0)

#define RAISEBIT(bitmask,bitToRaise) \
bitmask |= (1 << bitToRaise) 


#define DOWNBIT(bitmask,bitToLower) \
{ int mask = ~(1 << bitToLower); \
  bitmask=bitmask & mask; }

#define CHECKBIT(bitmask,bitToCheck)\
((bitmask & (1 << bitToCheck)) != 0)
}//common
#endif
