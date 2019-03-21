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
//#define CHAOS
#ifdef CHAOS
#include <common/misc/driver/ConfigDriverMacro.h>
#include <chaos/common/data/CDataWrapper.h>
#endif
#include <cstring>
#include "CAENHVWrapper.h"
#include "CAEN2527.h"

using namespace std;
using namespace common::multichannelpowersupply;
using namespace common::multichannelpowersupply::models;
CAEN2527::CAEN2527(const std::string Parameters) {
}
#ifdef CHAOS
CAEN2527::CAEN2527(const chaos::common::data::CDataWrapper &config) { 
	GET_PARAMETER_TREE((&config), driver_param)
	{
		GET_PARAMETER(driver_param, IP, string, 1);
		IPaddress.assign(IP);
		GET_PARAMETER(driver_param, Name, string, 1);
		CrateName.assign(Name);
	}
  unsigned short slotNum;
  unsigned short *chanList;

  unsigned short *SerNumList;
  char **ModelList;
  char **DescrList;
  unsigned char** FmwRelMinList;
  unsigned char** FmwRelMaxList;

  this->listOfSlots.clear();
  this->channelsEachSlot.clear();
  int ret= this->Login_HV_HET();

  chanList=(unsigned short*)malloc(sizeof(unsigned short)*256);
  SerNumList=(unsigned short*)malloc(sizeof(unsigned short)*256);
  ModelList=(char**)malloc(sizeof(char)*2048);
  DescrList=(char**)malloc(sizeof(char)*2048);
  FmwRelMinList=(unsigned char**)malloc(sizeof(unsigned char)*2048);
  FmwRelMaxList=(unsigned char**)malloc(sizeof(unsigned char)*2048);
	
  if (ret==0)
	{
    int slotCount=0;
    CAENHV_GetCrateMap(this->handle,&slotNum,&chanList,ModelList,DescrList,&SerNumList,FmwRelMinList,FmwRelMaxList);
   
    for (int j=0; j < slotNum;j++)
    {
     
      if (chanList[j]>0)
      {
          this->listOfSlots.push_back(j);
         this->channelsEachSlot.push_back(chanList[j]);
      }
    }
		ret=this->Logout_HV_HET();
	}
  free(chanList);
  free(SerNumList);
  free(ModelList);
  free(DescrList);
  free(FmwRelMinList);
  free(FmwRelMaxList);
}
#endif
CAEN2527::~CAEN2527() {
}
int CAEN2527::UpdateHV(std::string& crateData) {


	//DPRINT ("Trying connection on SysName %s at IP %s",CrateName.c_str(),IPaddress.c_str());
  sleep(5);
  crateData="[";
  bool first=true;
	int ret= this->Login_HV_HET();
  CAENHVRESULT cret;
  unsigned short chList;
  float fParVarList;
  long lParVarList;
	if (ret==0)
	{
		
    for (int sl_it=0; sl_it < this->listOfSlots.size(); sl_it++)
    {
      uint16_t slot=this->listOfSlots[sl_it];
      int maxChanThisSlot=this->channelsEachSlot[sl_it];
      for (int ch=0; ch < maxChanThisSlot;ch++)
      {
        if (first) {first=false;}
			  else 	{ crateData+=","; }
        crateData+="{";
        CAENHV_GetChParam(this->handle,slot,"VMon",ch,&chList,&fParVarList);
        crateData+= "\"VMon\":"+to_string(fParVarList) +",";
        CAENHV_GetChParam(this->handle,slot,"IMon",ch,&chList,&fParVarList);
        crateData+= "\"IMon\":"+to_string(fParVarList) +",";
        CAENHV_GetChParam(this->handle,slot,"Status",ch,&chList,&lParVarList);
        crateData+= "\"status\":"+to_string(lParVarList) +",";

        crateData+= "\"alarm\":0";

        crateData+="}";
      }
    }
    crateData+="]";
   // DPRINT("ALEDEBUG %s",crateData.c_str());
		ret=this->Logout_HV_HET();
	}
	else
	{
		DPRINT("LOGIN FAILED");
	}
	
	return 0;
}
int CAEN2527::getSlotConfiguration(std::vector<int32_t>& slotNumberList,std::vector<int32_t>& channelsPerSlot) {

  slotNumberList=this->listOfSlots;
  channelsPerSlot=this->channelsEachSlot;
	return 0;
}
int CAEN2527::setChannelVoltage(int32_t slot,int32_t channel,double voltage) {
	return 0;
}
int CAEN2527::setChannelCurrent(int32_t slot,int32_t channel,double current) {
	return 0;
}
int CAEN2527::getChannelParametersDescription(std::string& outJSONString) {
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
  status=0;
	return 0;
}
int CAEN2527::getMainAlarms(int64_t& alarms,std::string& descr) {
  alarms=0;
	return 0;
}

int CAEN2527::Login_HV_HET()
{
  char arg[30], userName[20], passwd[30];
  int link;
  CAENHVRESULT ret;
  int iret;
  ;

  link = 0;
  strcpy(userName,"admin");
  strcpy(passwd,"admin");
  strcpy(arg,this->IPaddress.c_str());
  ret = CAENHV_InitSystem((CAENHV_SYSTEM_TYPE_t)0, link, arg, userName, passwd,&handle);
  if( ret == CAENHV_OK )
  {
    /* printf("CAENHVInitSystem: Connection opened (num. %d)\n\n", ret); */
    iret = 0;
  }
  else
  {
     printf("\nCAENHVInitSystem: %s (num. %d)\n\n", CAENHV_GetError(handle), ret);
     iret = -1;
  }
  return(iret);
}

int CAEN2527::Logout_HV_HET() {
  CAENHVRESULT ret;
  int iret;

  ret = CAENHV_DeinitSystem(this->handle);
  if( ret == CAENHV_OK ) {
    /* printf("CAENHVDeinitSystem: Connection closed (num. %d)\n\n", ret); */
    iret = 0;
  } else {
    /* printf("CAENHVDeinitSystem: %s (num. %d)\n\n", CAENHVGetError(name), ret); */
    iret = 1;
  }

  return(iret);
}

int CAEN2527::Slot_Present_HV_HET(unsigned short Slot)
{
  unsigned short NrOfCh1, serNumb;
  unsigned char fmwMax, fmwMin;
  char* Model[15], *Descr[80];
  CAENHVRESULT ret;
  int iret;
  ret = CAENHV_TestBdPresence(this->handle, Slot, &NrOfCh1, Model, Descr, &serNumb, &fmwMin, &fmwMax);
  if( ret != CAENHV_OK )
  {
    iret = -1;
  }
  else
  {
    iret = 1;
  }
  return(iret);
}
