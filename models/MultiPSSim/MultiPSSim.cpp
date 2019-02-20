/*
MultiPSSim.cpp
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
//#define CHAOS
#ifdef CHAOS
#include <common/misc/driver/ConfigDriverMacro.h>
#include <chaos/common/data/CDataWrapper.h>
#endif
#include "MultiPSSim.h"

using namespace common::multichannelpowersupply;
using namespace common::multichannelpowersupply::models;
using namespace std;
inline std::string trim_right_copy(const std::string& s, 
								  const std::string& delimiters=" \f\n\r\t\v")
								  {
									  return s.substr(0,s.find_last_not_of(delimiters)+1);
								  }
inline std::string trim_left_copy(const std::string &s,
 								  const std::string& delimiters=" \f\n\r\t\v")
								   {
									   return s.substr(s.find_first_not_of(delimiters));
								   }
std::string trim_copy(const std::string &s, const std::string& delimiters=" \f\n\r\t\v")
{
	return trim_left_copy(trim_right_copy(s,delimiters),delimiters);
}



MultiPSSim::MultiPSSim(const std::string Parameters) {

}

#ifdef CHAOS
MultiPSSim::MultiPSSim(const chaos::common::data::CDataWrapper &config) { 
   this->listOfSlots.clear();
   this->listOfSlots.push_back(2);
   this->listOfSlots.push_back(4);
   this->listOfSlots.push_back(5);
   this->channelsEachSlot.clear();
   this->channelsEachSlot.push_back(24);
   this->channelsEachSlot.push_back(24);
   this->channelsEachSlot.push_back(20);
   this->canali.clear();
   for (int i=0; i < this->listOfSlots.size(); i++)
   {
	   int chanThisSlot=this->channelsEachSlot[i];
	   for (int ch=0; ch < chanThisSlot; ch++)
	   {
		   SimChannel nuovo;
		   nuovo.chNum=ch;
		   nuovo.slot=this->listOfSlots[i];
		   nuovo.Pw=0;
		   nuovo.RUp=1.0;
		   nuovo.RDWn=1;
		   nuovo.V0Set=0;
		   nuovo.V1Set=0;
		   nuovo.status= ::common::powersupply::POWER_SUPPLY_STATE_STANDBY; //Status POWER SUPPLY OFF on start
		   nuovo.alarm=0;
			 nuovo.I0Set=0;
		   nuovo.I1Set=0;
		   nuovo.SVMax=2.0;
		   nuovo.IMon= (float) ch/4;
		   nuovo.VMon=(float) ch/4;
		   this->canali.push_back(nuovo);
			 this->voltageGenerator=true;
			 this->AdditiveNoise=0;
			 this->MainPowerOn=true;
	   }
   }
    //DPRINT("received init parameter %s ",config.getJSONString().c_str());
	GET_PARAMETER_TREE((&config), device_param)
	{
		GET_PARAMETER(device_param, SHOW, string, 0);
		if (!SHOW.empty())
		{
			size_t current, next=-1;
			do
			{
				current=next+1;
				next=SHOW.find_first_of(",",current);
				std::string parola=SHOW.substr(current,next-current);
				std::string trimmedStr=trim_copy(parola);

				this->toShow.push_back(trimmedStr);
			} while (next != string::npos);
		}
		GET_PARAMETER(device_param, Behaviour, int32_t, 0);
		if (Behaviour == ::common::multichannelpowersupply::MPS_CURRENT_GENERATOR)
		{
			this->voltageGenerator=false;
		}
		GET_PARAMETER(device_param, Noise, double, 0);
		if (Noise > 0)
		{
			this->AdditiveNoise=Noise;
		}
	}
}
#endif
MultiPSSim::~MultiPSSim() {
}
int MultiPSSim::UpdateHV(std::string& crateData) {
	char converted[32];
	std::string toPrint;
	crateData.clear();
	crateData="[";
	int prog=0;
	bool first=true;
	for (int i=0;i < this->listOfSlots.size();i++)
	{
		for (int ch=0;ch < this->channelsEachSlot[i]; ch++)
		{
			if (first)
				first=false;
			else
			{
				crateData+=",";
			}
			crateData+="{";
			if (CHECKMASK(this->canali[prog].status,common::powersupply::POWER_SUPPLY_STATE_ON))
			{// rand : RAND_MAX = noiseLevel : AdditiveNoise
				
			  double noiseLevel= (this->AdditiveNoise*std::rand())/ RAND_MAX;
				snprintf(converted,32,"%6.3f",this->canali[prog].VMon + noiseLevel);toPrint=std::string(converted);
				crateData+= "\"VMon\":"+toPrint +",";
				noiseLevel= (this->AdditiveNoise*std::rand())/ RAND_MAX;
				snprintf(converted,32,"%6.3f",this->canali[prog].IMon + noiseLevel);toPrint=std::string(converted);
				crateData+= "\"IMon\":"+toPrint +",";
			}
			else 
			{
				crateData+= "\"VMon\":0,";
				crateData+= "\"IMon\":0,";
			}
			snprintf(converted,32,"%ld",this->canali[prog].alarm);toPrint=std::string(converted);
			crateData+= "\"alarm\":"+toPrint+",";
			snprintf(converted,32,"%ld",this->canali[prog].status);toPrint=std::string(converted);
			crateData+= "\"status\":"+toPrint;
			for (int i=0; i < this->toShow.size(); i++)
			{
				double *pt;
				pt=(double*)getExtraPointer(this->toShow[i],prog);
				crateData+=",";
				snprintf(converted,32,"%6.3f",*pt);toPrint=std::string(converted);
				crateData+= "\""+this->toShow[i]+"\":"+toPrint;

			}
			crateData+="}";
			prog++;
		}
	}
	crateData+="]";

	return 0;
}
int MultiPSSim::getSlotConfiguration(std::vector<int32_t>& slotNumberList,std::vector<int32_t>& channelsPerSlot) {
	slotNumberList.clear();
	for (int i=0; i < this->listOfSlots.size(); i++)
	{
		slotNumberList.push_back(this->listOfSlots[i]);
	}
	channelsPerSlot.clear();
	for (int i=0; i < this->channelsEachSlot.size(); i++)
	{
		channelsPerSlot.push_back(this->channelsEachSlot[i]);
	}
	return 0;
}
int MultiPSSim::setChannelVoltage(int32_t slot,int32_t channel,double voltage) {
	int chanNum=getChanNum(slot,channel);
	if (chanNum < 0)
		return -1;
	else
	{
		this->canali[chanNum].V0Set=voltage;
		if (this->voltageGenerator)
			this->canali[chanNum].VMon=voltage;
	}
		
	return 0;
}
int MultiPSSim::setChannelCurrent(int32_t slot,int32_t channel,double current) {
	int chanNum=getChanNum(slot,channel);
	if (chanNum < 0)
		return -1;
	else
	{
		this->canali[chanNum].I0Set=current;
		if (!this->voltageGenerator)
		  this->canali[chanNum].IMon=current;
	}
		
	return 0;
}
int MultiPSSim::getChannelParametersDescription(std::string& outJSONString) {
	outJSONString.clear();
	outJSONString+="{";
	outJSONString+="\"V1Set\":\"double\",";
	outJSONString+="\"I1Set\":\"double\",";
	outJSONString+="\"SVMax\":\"double\",";
	outJSONString+="\"Trip\":\"double\",";
	outJSONString+="\"RUp\":\"double\",";
	outJSONString+="\"RDWn\":\"double\",";
	outJSONString+="\"PDWn\":\"double\",";
	outJSONString+="\"POn\":\"double\"";
	outJSONString+="}";

	return 0;
}
int MultiPSSim::setChannelParameter(int32_t slot,int32_t channel,std::string paramName,std::string paramValue) {
	for (int i=0; i < this->listOfSlots.size();i++)
	{
		int slotToWork=-1;
		if (slot==-1)
		{
			slotToWork=this->listOfSlots[i];
		}
		else
		{
			if (this->listOfSlots[i] == slot)
			slotToWork=this->listOfSlots[i];
 		}
		if (slotToWork!= -1)
		{
			for (int ch=0; ch < this->channelsEachSlot[i];ch++)
			{
				if ((channel == -1) || (channel==ch))
				{
					int chanToSet=this->getChanNum(slotToWork,ch);
					if (chanToSet>=0)
					{
						double* pt=(double*)this->getExtraPointer(paramName,chanToSet);
						*pt=atof(paramValue.c_str());
					}
				}
				
			}
		}


	}
	return 0;
}
int MultiPSSim::PowerOn(int32_t slot,int32_t channel,int32_t onState) {
	if ((slot !=-1) && (channel != -1))
	{
		int chanNum=getChanNum(slot,channel);
		if (chanNum < 0)
			return -1;
		else
		{
			if (!onState)
			{
				UPMASK(this->canali[chanNum].status,common::powersupply::POWER_SUPPLY_STATE_STANDBY);
				DOWNMASK(this->canali[chanNum].status,common::powersupply::POWER_SUPPLY_STATE_ON);
			}
				
			else
			{
				DOWNMASK(this->canali[chanNum].status,common::powersupply::POWER_SUPPLY_STATE_OFF);
				UPMASK(this->canali[chanNum].status,common::powersupply::POWER_SUPPLY_STATE_ON);
			}
			
		}
	}
	else
	{
		if (slot == -1)
		{
			for (int i=0; i < this->listOfSlots.size(); i++)
			{
				int sl=this->listOfSlots[i];
				int maxChan=this->channelsEachSlot[i];
				
				if (channel != -1)
				{
					int chanNum=getChanNum(sl,channel);
					if (chanNum < 0)
						return -1;
					else
					{
						if (!onState)
						{
							UPMASK(this->canali[chanNum].status,common::powersupply::POWER_SUPPLY_STATE_OFF);
							DOWNMASK(this->canali[chanNum].status,common::powersupply::POWER_SUPPLY_STATE_ON);
						}
							
						else
						{
							DOWNMASK(this->canali[chanNum].status,common::powersupply::POWER_SUPPLY_STATE_OFF);
							UPMASK(this->canali[chanNum].status,common::powersupply::POWER_SUPPLY_STATE_ON);
						}
						
					}
				}
				else
				{
					for (int j=0;j < maxChan;j++)
					{
						int chanNum=getChanNum(sl,j);
						if (chanNum < 0)
							return -1;
						else
						{
							if (!onState)
							{
								UPMASK(this->canali[chanNum].status,common::powersupply::POWER_SUPPLY_STATE_OFF);
								DOWNMASK(this->canali[chanNum].status,common::powersupply::POWER_SUPPLY_STATE_ON);
							}
								
							else
							{
								DOWNMASK(this->canali[chanNum].status,common::powersupply::POWER_SUPPLY_STATE_OFF);
								UPMASK(this->canali[chanNum].status,common::powersupply::POWER_SUPPLY_STATE_ON);
							}
							
						}

					}
				}
				
				
					
				
			}
		}
		else //channel è sicuramente -1
		{
			int i;
			/* code */
			//finding correct slot
			for ( i=0; i < this->listOfSlots.size(); i++)
			{
				if (slot == this->listOfSlots[i])
					break;

			}
			if (i == this->listOfSlots.size())
				return -1;
			int maxChan=this->channelsEachSlot[i];
			for (int j=0;j < maxChan;j++)
			{
				int chanNum=getChanNum(slot,j);
				if (chanNum < 0)
					return -1;
				else
				{
					if (!onState)
					{
						UPMASK(this->canali[chanNum].status,common::powersupply::POWER_SUPPLY_STATE_OFF);
						DOWNMASK(this->canali[chanNum].status,common::powersupply::POWER_SUPPLY_STATE_ON);
					}
						
					else
					{
						DOWNMASK(this->canali[chanNum].status,common::powersupply::POWER_SUPPLY_STATE_OFF);
						UPMASK(this->canali[chanNum].status,common::powersupply::POWER_SUPPLY_STATE_ON);
					}
					
				}

			}
		}
		
	}
	
	
	return 0;
}
int MultiPSSim::MainUnitPowerOn(int32_t on_state) {
				if (on_state==0)
				{
					this->MainPowerOn=false;
					this->PowerOn(-1,-1,0);
				}
				else
				{
					this->MainPowerOn=true;
				}
				
        return 0;
}
int MultiPSSim::getMainStatus(int32_t& status, std::string& desc) {
				status=0;
				desc.clear();
				if (this->MainPowerOn)
				{
					UPMASK(status,::common::powersupply::POWER_SUPPLY_STATE_ON);
					desc+="Main Unit ON.";
				}
				else
				{
					UPMASK(status,::common::powersupply::POWER_SUPPLY_STATE_MAINUNIT_NOT_ON);
					desc+="Main Unit OFF";
				}
        return 0;
}
int MultiPSSim::getMainAlarms(int64_t& alarms, std::string& desc ) {
				alarms=0;
				desc.clear();
        return 0;
}




int MultiPSSim::getChanNum(int32_t slot, int32_t chan) {
	int i;
	int chanToRet=0;
	int channelChosenSlot;
	//finding correct slot
	for ( i=0; i < this->listOfSlots.size(); i++)
	{
		if (slot == this->listOfSlots[i])
			break;

	}
	if (i == this->listOfSlots.size())
		return -1;
	//checking enough channel in the chosen slot
	channelChosenSlot=this->channelsEachSlot[i];
	if (chan >= channelChosenSlot)
		return -2;
    //counting  channels of previous slots
	for (int pastSlots=0; pastSlots < i ; pastSlots++)
	{
		chanToRet+=this->channelsEachSlot[pastSlots];
	}
	chanToRet+=chan;
	return chanToRet;	
}
void* MultiPSSim::getExtraPointer(std::string name, int32_t progChannel) {
	#define QUESTO  this->canali[progChannel]
	if (name=="RUp")
		//return this->canali[progChannel].RUp;
		return &QUESTO.RUp;
	if (name=="RDWn")
		return &QUESTO.RDWn;
	if (name=="SVMax")
		return &QUESTO.SVMax;
	if (name=="I1Set")
		return &QUESTO.I1Set;
	if (name=="V1Set")
		return &QUESTO.V1Set;
	if (name=="Trip")
		return &QUESTO.Trip;
	if (name=="PDWn")
		return &QUESTO.PDWn;
	if (name=="POn")
		return &QUESTO.POn;
	return NULL;
	
}

