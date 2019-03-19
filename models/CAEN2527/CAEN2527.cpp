/*
CAEN2527.cpp
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
#include <common/debug/core/debug.h>
#include <vector>
#ifdef CHAOS
#include <chaos/common/data/CDataWrapper.h>
#endif
#include "CAEN2527.h"
using namespace common::multichannelpowersupply;
using namespace common::multichannelpowersupply::models;
CAEN2527::CAEN2527(const std::string Parameters) {
}
#ifdef CHAOS
CAEN2527::CAEN2527(const chaos::common::data::CDataWrapper &config) { 
}
#endif
CAEN2527::~CAEN2527() {
}
int CAEN2527::UpdateHV(std::string& crateData) {
	return 0;
}
int CAEN2527::getSlotConfiguration(std::vector<int32_t>& slotNumberList,std::vector<int32_t>& channelsPerSlot) {
	return 0;
}
int CAEN2527::setChannelVoltage(int32_t slot,int32_t channel,double voltage) {
	return 0;
}
int CAEN2527::setChannelCurrent(int32_t slot,int32_t channel,double current) {
	return 0;
}
int CAEN2527::getChannelParametersDescription(std::string& outJSONString) {
	return 0;
}
int CAEN2527::setChannelParameter(int32_t slot,int32_t channel,std::string paramName,std::string paramValue) {
	return 0;
}
int CAEN2527::PowerOn(int32_t slot,int32_t channel,int32_t onState) {
	return 0;
}
int CAEN2527::MainUnitPowerOn(int32_t on_state) {
	return 0;
}
int CAEN2527::getMainStatus(int32_t& status,std::string& descr) {
	return 0;
}
int CAEN2527::getMainAlarms(int64_t& alarms,std::string& descr) {
	return 0;
}
