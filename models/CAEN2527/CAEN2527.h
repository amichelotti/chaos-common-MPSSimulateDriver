/*
CAEN2527.h
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
#ifndef __MultiChannelPowerSupply__CAEN2527__
#define __MultiChannelPowerSupply__CAEN2527__
#include <common/MultiChannelPowerSupply/core/AbstractMultiChannelPowerSupply.h>
#ifdef CHAOS
#include <chaos/common/data/CDataWrapper.h>
#endif
namespace common {
	namespace multichannelpowersupply {
		namespace models {
			class CAEN2527: public AbstractMultiChannelPowerSupply {
				public:
				CAEN2527(const std::string Parameters);
				#ifdef CHAOS
				CAEN2527(const chaos::common::data::CDataWrapper& config);
				#endif
				~CAEN2527();
				int UpdateHV(std::string& crateData);
				int getSlotConfiguration(std::vector<int32_t>& slotNumberList,std::vector<int32_t>& channelsPerSlot);
				int setChannelVoltage(int32_t slot,int32_t channel,double voltage);
				int setChannelCurrent(int32_t slot,int32_t channel,double current);
				int getChannelParametersDescription(std::string& outJSONString);
				int setChannelParameter(int32_t slot,int32_t channel,std::string paramName,std::string paramValue);
				int PowerOn(int32_t slot,int32_t channel,int32_t onState);
				int MainUnitPowerOn(int32_t on_state);
				int getMainStatus(int32_t& status,std::string& descr);
				int getMainAlarms(int64_t& alarms,std::string& descr);
			};//end class
		}//end namespace models
	}//end namespace multichannelpowersupply
}//end namespace common
#endif //__MultiChannelPowerSupply__CAEN2527__
