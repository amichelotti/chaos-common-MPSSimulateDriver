/*
MultiPSSim.h
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
#ifndef __MultiChannelPowerSupply__MultiPSSim__
#define __MultiChannelPowerSupply__MultiPSSim__
#include <common/MultiChannelPowerSupply/core/AbstractMultiChannelPowerSupply.h>
#include <common/powersupply/core/AbstractPowerSupply.h>
#ifdef CHAOS
#include <chaos/common/data/CDataWrapper.h>
#endif
namespace common {
	namespace multichannelpowersupply {
		namespace models {
			class MultiPSSim: public AbstractMultiChannelPowerSupply {
				public:
				MultiPSSim(const std::string Parameters);
				#ifdef CHAOS
				MultiPSSim(const chaos::common::data::CDataWrapper& config);
				#endif
				~MultiPSSim();
				int UpdateHV(std::string& crateData);
				int getSlotConfiguration(std::vector<int32_t>& slotNumberList,std::vector<int32_t>& channelsPerSlot);
				int setChannelVoltage(int32_t slot,int32_t channel,double voltage);
				int setChannelCurrent(int32_t slot,int32_t channel,double current);
				int getChannelParametersDescription(std::string& outJSONString);
				int setChannelParameter(int32_t slot,int32_t channel,std::string paramName,std::string paramValue);
				int PowerOn(int32_t slot,int32_t channel,int32_t onState);
				int getChanNum(int32_t slot, int32_t chan);
				void* getExtraPointer(std::string name, int32_t progChannel);
				private:
				class SimChannel {
				public:
					int slot;
					int chNum;
					double V0Set;
					double V1Set;
					double I0Set;
					double I1Set;
					double SVMax;
					double Trip;
					double RUp;
					double RDWn;
					double VMon;
					double IMon;
					long status;
					int PDWn;
					int POn;
					int Pw;
					public: SimChannel() {}; 
				};
				std::vector<int16_t> listOfSlots;
				std::vector<int16_t> channelsEachSlot;
				std::vector<std::string> toShow;
				std::vector<SimChannel> canali;
				bool voltageGenerator;
				double AdditiveNoise;
				
			};//end class
		}//end namespace models
	}//end namespace multichannelpowersupply
}//end namespace common
#endif //__MultiChannelPowerSupply__MultiPSSim__
