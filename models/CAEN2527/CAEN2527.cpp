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
std::string CAEN2527::trim_copy(const std::string &s, const std::string& delimiters) {
	return this->trim_left_copy(this->trim_right_copy(s,delimiters),delimiters);
}
CAEN2527::CAEN2527(const std::string Parameters) {
}
#ifdef CHAOS
CAEN2527::CAEN2527(const chaos::common::data::CDataWrapper &config) { 
  this->driverConnected=false;
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
    this->driverConnected=true;
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
		
	}
  else
  {
    this->driverConnected=false;
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
  int ret=this->Logout_HV_HET();
}
int CAEN2527::UpdateHV(std::string& crateData) {
  sleep(1);

  bool first=true;
  CAENHVRESULT *cret;
  unsigned short chList[128];
  float fgParVarList[128];
  uint32_t lParVarList[128];

  CAENHVRESULT hret=CAENHV_OK;

	if (this->driverConnected==true)
	{
    std::string complementaryString[this->toShow.size()];
		crateData="[";
    for (int sl_it=0; sl_it < this->listOfSlots.size(); sl_it++)
    {
      uint16_t slot=this->listOfSlots[sl_it];
      std::vector<float> VMonRead;
      std::vector<float> IMonRead;
      std::vector<int32_t> statusRead;
      std::vector<int32_t> alarmRead;
      int maxChanThisSlot=this->channelsEachSlot[sl_it];
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
      //hret=CAENHV_GetChParam(this->handle,slot,"Pw",maxChanThisSlot,chList,lParVarList);
      for (int ch=0; ch < maxChanThisSlot;ch++)
      {
        unsigned short lista[24];
        uint32_t readValue[24];
        lista[0]=ch;
        hret=CAENHV_GetChParam(this->handle,slot,"Status",1,lista,readValue);
        if (CHECKBIT(readValue[0],0))
        {
          statusRead.push_back(::common::powersupply::POWER_SUPPLY_STATE_ON);
          //DPRINT("ALEDEBUG slot %d chan %d ON %s ",slot,ch,testo);
        }
        else 
        {
          statusRead.push_back(::common::powersupply::POWER_SUPPLY_STATE_STANDBY);
          //DPRINT("ALEDEBUG slot %d chan %d OFF %s",slot,ch,testo);
        }
        alarmRead.push_back(this->EncodeAlarms(readValue[0]));

      }
      if (this->toShow.size() > 0)
      {
          float FvalList[128];
          for (int i=0; i < this->toShow.size(); i++)
          {

              uint32_t tipo;
              complementaryString[i]="";
              hret = CAENHV_GetChParamProp(handle, slot, chList[0], this->toShow[i].c_str(), "Type", &tipo);
              if (hret == CAENHV_OK)
              {
                  //DPRINT("ALEDEBUG tipo of %s is %lu",this->toShow[i].c_str(),tipo);
                  if( tipo == PARAM_TYPE_NUMERIC )
                  {
                    hret = CAENHV_GetChParam(handle, slot, this->toShow[i].c_str(), maxChanThisSlot, chList, FvalList);
                    if (hret == CAENHV_OK)
                    {
                      char tmp[16];
                      std::string tmpStr;
                      for (int ch=0; ch < maxChanThisSlot;ch++)
                      {
                          sprintf(tmp,"%.3f",FvalList[ch]);
                          tmpStr=tmp;
                          //DPRINT("ALEDEBUG read %s as float value %f adding %s",this->toShow[i].c_str(),FvalList[ch],tmpStr.c_str());
                          complementaryString[i]+=tmpStr+" ";
                      }
                    }
                  }
                  else
                  {
                    DPRINT("ALEDEBUG is not numeric (%d)",tipo);
                    for (int ch=0; ch < maxChanThisSlot;ch++)
                    {
                      unsigned short lista[24];
                      uint32_t readValue[24];
                      lista[0]=ch;
                      hret=CAENHV_GetChParam(this->handle,slot,this->toShow[i].c_str(),1,lista,readValue);
                      //DPRINT("ALEDEBUG read %s as long value %d",this->toShow[i].c_str(),tmp);
                      complementaryString[i]+=to_string(readValue[0])+" ";
                    }
                  }

              }
              //DPRINT("ALEDEBUG complementary string is %s",complementaryString[i].c_str());
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
        for (int i=0; i < this->toShow.size(); i++)
        {
          crateData+=",";
          std::vector<std::string> tokens= this->split(complementaryString[i],' ');
          crateData+="\""+this->toShow[i]+"\":"+tokens[ch];
          
        }
        crateData+="}";
      }
    }
    crateData+="]";
    //DPRINT("ALEDEBUG %s",crateData.c_str());
	
	}
	else
	{
		DPRINT("DRIVER CONNECTION PROBLEM");
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
   if (this->driverConnected)
  {
    uint32_t chList[24];
    float   fparVal[24];
    uint32_t readValue[24];
    CAENHVRESULT hret=CAENHV_OK;
    chList[0]=channel;
    fparVal[0]=(float)voltage;
    hret = CAENHV_SetChParam(this->handle,slot,"V0Set",1,(const unsigned short*)chList,fparVal);

    sleep(1);
    CAENHV_GetChParam(this->handle,slot,"Status",1,(const unsigned short*)chList,readValue);
    DPRINT("ALEDEBUG channel status chan %d slot %d is %d",channel,slot,readValue[0]);



    if (hret==CAENHV_OK)
      return 0;
    else
    {
      return (int)hret;
    }
    
  }
	return -2;
	
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
  
	if (this->driverConnected)
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
        uint32_t lparVal[32];
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
    
      if (hret != CAENHV_OK )
      {
        return -1;
      }
      else
      {
        return 0;
      }
      
  }
  else
	  return -2;
}
int CAEN2527::MainUnitPowerOn(int32_t on_state) {
  CAENHVRESULT ret;
  
  unsigned short lista[24];
  uint32_t readValue[24];
  lista[0]=0;
  ret=CAENHV_GetChParam(this->handle,0,"Status",1,lista,readValue);
  DPRINT("ALEDEBUG channel status chan 0 slot 0 is %d",readValue[0]);
	return 0;
}
int CAEN2527::getMainStatus(int32_t& status,std::string& descr) {
  if (this->driverConnected)
  {
    status=::common::powersupply::POWER_SUPPLY_STATE_ON;
    descr="Main Unit ON";
  }
  else
  {
    status=::common::powersupply::POWER_SUPPLY_STATE_UKN;
    descr="Main Unit state Unknown";
  }
  
	return 0;
}
int CAEN2527::getMainAlarms(int64_t& alarms,std::string& descr) {
  if (this->driverConnected)
  {
    alarms=0;
  }
  else
  {
    alarms=::common::powersupply::POWER_SUPPLY_COMMUNICATION_FAILURE;
    descr="Main unit unreachable";
  }
  
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
  ret = CAENHV_InitSystem((CAENHV_SYSTEM_TYPE_t)1, link, arg, userName, passwd,&handle);
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

std::vector<std::string> CAEN2527::split(std::string toSplit,char sep)
{
  std::vector<std::string> retVec;
  //std::string toSplit=this->trim_copy(toSplitNT);
  size_t  start = 0, end = 0;

    while ( end != std::string::npos)
    {
        end = toSplit.find( sep, start);

        // If at end, use length=maxLength.  Else use length=end-start.
        retVec.push_back( toSplit.substr( start,
                       (end == std::string::npos) ? std::string::npos : end - start));

        // If at end, use start=maxSize.  Else use start=end+delimiter.
        start = (   ( end > (std::string::npos - 1) )
                  ?  std::string::npos  :  end + 1);
    }
    return retVec;
}
int32_t CAEN2527::EncodeAlarms(uint32_t hwmask)
{
  int32_t retmask=0;
  if (CHECKBIT(hwmask,3))
  {
    RAISEBIT(retmask,::common::powersupply::POWER_SUPPLY_OVER_CURRENT);
  }
  if ( (CHECKBIT(hwmask,4)) || (CHECKBIT(hwmask,13)) )
  {
    RAISEBIT(retmask,::common::powersupply::POWER_SUPPLY_OVER_VOLTAGE);
  }
  if (CHECKBIT(hwmask,15))
  {
    RAISEBIT(retmask,::common::powersupply::POWER_SUPPLY_EVENT_OVER_TEMP);
  }
  if (
      (CHECKBIT(hwmask,5)) ||
      (CHECKBIT(hwmask,6)) ||
      (CHECKBIT(hwmask,9)) ||
      (CHECKBIT(hwmask,10)) ||
      (CHECKBIT(hwmask,14))

  )
  {
    RAISEBIT(retmask,::common::powersupply::POWER_SUPPLY_ALARM_UNDEF);
  }



  return retmask;
}
