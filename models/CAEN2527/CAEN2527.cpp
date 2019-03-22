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
inline std::string CAEN2527::trim_right_copy(const std::string& s, 
								  const std::string& delimiters)
								  {
									  return s.substr(0,s.find_last_not_of(delimiters)+1);
								  }
inline std::string CAEN2527::trim_left_copy(const std::string &s,
 								  const std::string& delimiters)
								   {
									   return s.substr(s.find_first_not_of(delimiters));
								   }
std::string CAEN2527::trim_copy(const std::string &s, const std::string& delimiters)
{
	return this->trim_left_copy(this->trim_right_copy(s,delimiters),delimiters);
}
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
  
  bool first=true;
  CAENHVRESULT *cret;
  unsigned short *chList;
  float *fgParVarList;
  long *lParVarList;
  char** cParVarList;
  CAENHVRESULT hret=CAENHV_OK;
  int ret= this->Login_HV_HET();
	if (ret==0)
	{
		crateData="[";
    for (int sl_it=0; sl_it < this->listOfSlots.size(); sl_it++)
    {
      uint16_t slot=this->listOfSlots[sl_it];
      std::vector<float> VMonRead;
      std::vector<float> IMonRead;
      std::vector<int32_t> statusRead;
      int maxChanThisSlot=this->channelsEachSlot[sl_it];
      chList=(unsigned short*)malloc(sizeof(unsigned short)*maxChanThisSlot);
      fgParVarList=(float*)malloc(sizeof(float)*maxChanThisSlot);
      lParVarList=(long*)malloc(sizeof(long)*maxChanThisSlot);
      cParVarList=(char**)malloc(sizeof(char)*maxChanThisSlot*16);
      for (int ch=0; ch < maxChanThisSlot;ch++)
      {
        chList[ch]=ch;
      }
       
        hret=CAENHV_GetChParam(this->handle,slot,"VMon",maxChanThisSlot,chList,fgParVarList);
        for (int ch=0; ch < maxChanThisSlot;ch++)
        {
          VMonRead.push_back(fgParVarList[ch]);
        }
        //
        hret=CAENHV_GetChParam(this->handle,slot,"IMon",maxChanThisSlot,chList,fgParVarList);
        for (int ch=0; ch < maxChanThisSlot;ch++)
        {
          float voltValue=fgParVarList[ch]/1000000;
          IMonRead.push_back(voltValue);
        }
        //unsigned long tipo;
        //CAENHV_GetChParamProp(this->handle,slot,0,"Pw","Type",&tipo);
        char testo[64];
        for (int ch=0; ch < maxChanThisSlot;ch++)
        {
          unsigned short lista[24];
          long readValue[24];
          int32_t tmp;
          lista[0]=ch;
          hret=CAENHV_GetChParam(this->handle,slot,"Pw",1,lista,readValue);
          sprintf(testo,"%x",readValue[0]);
          sscanf(testo,"%d",&tmp);
          if (tmp==1)
          {
            statusRead.push_back(::common::powersupply::POWER_SUPPLY_STATE_ON);
            //DPRINT("ALEDEBUG slot %d chan %d ON %s ",slot,ch,testo);
          }
          else if (tmp==0)
          {
            statusRead.push_back(::common::powersupply::POWER_SUPPLY_STATE_STANDBY);
            //DPRINT("ALEDEBUG slot %d chan %d OFF %s",slot,ch,testo);
          }
          else
          {
            statusRead.push_back(::common::powersupply::POWER_SUPPLY_STATE_UKN);
            //DPRINT("ALEDEBUG slot %d chan %d UNKNOWN %s %d",slot,ch,testo,readValue[0]);
          }
        }
        for (int ch=0; ch < maxChanThisSlot;ch++)
        {
          if (first) {first=false;}
			    else 	{ crateData+=","; }
          crateData+="{";
          crateData+= "\"VMon\":"+to_string(VMonRead[ch]) +",";
          crateData+= "\"IMon\":"+to_string(IMonRead[ch]) +",";
          crateData+= "\"status\":"+to_string(statusRead[ch]) +",";
          //crateData+= "\"status\":0,";
          crateData+= "\"alarm\":0";
   		    crateData+="}";
          //DPRINT("slot %d ch %d",slot,ch);
        }
        free(chList);
        free(fgParVarList);
        free(lParVarList);
        free(cParVarList);
    }
    crateData+="]";
    //DPRINT("ALEDEBUG %s",crateData.c_str());
		ret=this->Logout_HV_HET();
	}
	else
	{
		DPRINT("LOGIN FAILED");
    return -2;
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
  int ret= this->Login_HV_HET();
	if (ret==0)
	{
    CAENHVRESULT hret=CAENHV_OK;
    int loop=1;
    
      if (slot==-1)
        loop=this->listOfSlots.size(); 

      
      for (int i=0; i < loop;i++)
      {
        int32_t slotNum;
        slotNum= (slot==-1) ?this->listOfSlots[i] : slot;
        int chanloop=1;
        uint32_t chList[32];
        long lparVal[32];
        if (channel == -1)
        {
          if (slot==-1)
          {
            chanloop=this->channelsEachSlot[i];
          }
          else
          {
            chanloop=getNumChannelSlot(slotNum);
          }
        }
        for (int ch=0; ch< chanloop; ch++)
        {
          int32_t chNum;
          chNum= (channel == -1) ?  ch : channel;
          chList[0]=chNum;
          lparVal[0]=onState;
          hret = CAENHV_SetChParam(this->handle,slotNum,"Pw",1,(const unsigned short*)chList,lparVal);
          if (hret != CAENHV_OK )
          {
            break;
          }

        }
        if (hret != CAENHV_OK )
        {
          break;
        }
        //int slot=
      }
      //
    	ret=this->Logout_HV_HET();
      if (hret != CAENHV_OK )
      {
        return -1;
      }
  }
	return ret;
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
int CAEN2527::getNumChannelSlot(int sl)
{
  for (int i=0;i< this->listOfSlots.size();i++)
  {
    if (this->listOfSlots[i] == sl)
        return this->channelsEachSlot[i];
  }
  return 0;
}
