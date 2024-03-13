//set to 1 on ESP boards if you want to use the webSpike
//but make sure to have SDDS_ESP_EXTENSION library installed from here https://github.com/mLamneck/SDDS_ESP_Extension
#define WIFI_MANAGER_AP_SSID "myExcitingSSID"
#define WIFI_MANAGER_AP_PW "Start12345"

#include "uTypedef.h"
#include "uMultask.h"
#include "uParamSave.h"
#include "uWifiManager.h"

sdds_enum(OFF,ON) TonOffState;

class Tled : public TmenuHandle{
    Ttimer timer;
    public:
      sdds_struct(
          sdds_var(TonOffState,ledSwitch,sdds::opt::saveval)
          sdds_var(TonOffState,blinkSwitch,sdds::opt::saveval)
          sdds_var(Tuint16,onTime,sdds::opt::saveval,500)
          sdds_var(Tuint16,offTime,sdds::opt::saveval,500)
      )
      Tled(){
          pinMode(LED_BUILTIN, OUTPUT);

          on(ledSwitch){
              if (ledSwitch == TonOffState::dtype::ON) digitalWrite(LED_BUILTIN,1);
              else digitalWrite(LED_BUILTIN,0);
          };

          on(blinkSwitch){
              if (blinkSwitch == TonOffState::dtype::ON) timer.start(0);
              else timer.stop();
          };

          on(timer){
              if (ledSwitch == TonOffState::dtype::ON){
                  ledSwitch = TonOffState::dtype::OFF;
                  timer.start(offTime);
              } 
              else {
                  ledSwitch = TonOffState::dtype::ON;
                  timer.start(onTime);
              }
          };
      }
};

class TuserStruct : public TmenuHandle{
    sdds_struct(
        sdds_var(Tled,led,sdds::opt::saveval)
        sdds_var(TparamSaveMenu,params)
        sdds_var(TwifiManager,wifi)
    )
    TuserStruct(){
        //you application code goes here... 
    }
} userStruct;

#include "uSerialSpike.h"
TserialSpike serialHandler(userStruct,115200);

#include "uWebSpike.h"
TwebSpike webSpike(userStruct);

void setup(){

}

void loop(){
  TtaskHandler::handleEvents();
}