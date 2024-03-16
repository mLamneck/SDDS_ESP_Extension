#ifndef UWIFIMANAGER_H
#define UWIFIMANAGER_H

#include "uTypedef.h"

#ifdef ESP32
    #include <WiFi.h>
#else
    #include <ESP8266WiFi.h>        //to be checked
#endif

#ifndef WIFI_MANAGER_AP_SSID 
    #define WIFI_MANAGER_AP_SSID "espWifiAP" 
#endif
#ifndef WIFI_MANAGER_AP_PW 
    #define WIFI_MANAGER_AP_PW "Start1234" 
#endif

sdds_enum(___,connect) TwifiAction;
sdds_enum(STA,AP) TwifiMode;
sdds_enum(connect,waitConnect,connected,fallback) TwifiStatus;

class TwifiManager : public TmenuHandle{
    Ttimer timer;
    public:
        sdds_struct(
            sdds_var(TwifiAction,action)
            sdds_var(TwifiStatus,status,sdds::opt::readonly)
            sdds_var(Tuint8,checkCnt,sdds::opt::readonly);
            sdds_var(TwifiMode,currMode,sdds::opt::readonly)
            sdds_var(TwifiMode,mode)
            sdds_var(Tstring,ip,sdds::opt::readonly)
            sdds_var(Tstring,ssid,sdds::opt::saveval)
            sdds_var(Tstring,password,sdds::opt::saveval)
            sdds_var(Tstring,hostname,sdds::opt::saveval)
        )

    protected:
        void createAP(const char* _ssid, const char* _pw){
            currMode = TwifiMode::e::AP;
            disconnect();
            WiFi.mode(WIFI_AP);
            WiFi.softAP(_ssid, _pw);
            ip = WiFi.softAPIP().toString();
        };

        void createAP(){createAP(ssid,password);}

        void createFallbackAP(){ 
            status = TwifiStatus::e::fallback;
            createAP(WIFI_MANAGER_AP_SSID,WIFI_MANAGER_AP_PW); 
        }

        virtual void connect(){
            if (status == TwifiStatus::e::fallback) 
                WiFi.softAPdisconnect();
            currMode = TwifiMode::e::STA;
            WiFi.disconnect();
            WiFi.persistent(false);
            WiFi.setHostname(hostname.c_str());
            WiFi.begin(ssid.c_str(), password.c_str());
            WiFi.mode(WIFI_STA);
        }

        virtual bool connected(){ return (WiFi.status() == WL_CONNECTED); }

        virtual void disconnect(){ WiFi.disconnect(); }
        
        TwifiManager(){
            on(sdds::setup()){
                WiFi.begin();
            };
            
            on(action){
                if (action==TwifiAction::e::connect){
                    status = TwifiStatus::e::connect;
                    timer.start(100);
                    action = TwifiAction::e::___;
                }
            };

            timer.start(100);
            on(timer){
                switch(status){
                    case TwifiStatus::e::connect: case TwifiStatus::e::fallback:
                        if ((ssid.length() > 0) && (password.length()>0))
                            if (mode==TwifiMode::e::STA){
                                connect();
                                checkCnt = 0;
                                status = TwifiStatus::e::waitConnect;
                                timer.start(500);
                            } else createAP();                        //create AP with given credentials
                        else createFallbackAP();                      //create fallbackAP if credentials are not set
                        break;

                    case TwifiStatus::e::waitConnect:
                        timer.start(1000);
                        checkCnt = checkCnt+1;
                        if (connected()){
                            ip = WiFi.localIP().toString();
                            status = TwifiStatus::e::connected;
                        }
                        else if (checkCnt >= 10){
                            //create fallback if no connection has been established with the
                            //provided credentials and try to connect again after 5 minutes
                            createFallbackAP();
                            timer.start(60000*5);
                        }
                        break;

                    case TwifiStatus::e::connected:
                        if (connected()) timer.start(60000);
                        else {
                            status = TwifiStatus::e::connect;
                            timer.start(100);
                        }
                }
            };
        }
};
#endif