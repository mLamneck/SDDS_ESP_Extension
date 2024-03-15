# SDDS_ESP_Extension

This is an extension of the [SDDS core library](https://github.com/mLamneck/SDDS). It introduces some ESP-specific functionalities:
* WebSpike - web-based graphical user interface
* WifiManager

The WebSpike is using the **ESPAsyncWebServer** library from [me-no-dev](https://github.com/me-no-dev) to make use of a web server and websockets. We want to thank him for his work here.

## Table of Contents
- [Installation](#installation)
  - [Arduino](#arduino)
  - [PlatformIO](#platformio)
- [Web Spike](#web-spike)
  - [Introducing the User Interface](#introducing-the-user-interface)
  - [Navigate through the Tree on Mobile Devices](#navigate-through-the-tree-on-mobile-devices)
  - [Navigate through the Tree on Desktops](#navigate-through-the-tree-on-desktops)
  - [Connect to the Access Point](#connect-to-the-access-point)
- [Wifi Manager](#wifi-manager)
  - [What is a WiFi Manager](#what-is-a-wifi-manager)
  - [How Does Our Wifi Manager Work](#how-does-our-wifi-manager-work)
  - [Using the Wifi Manager](#using-the-wifi-manager)
  - [Coding a Full-Featured Wifi Manager in 120 Lines](#coding-a-full-featured-wifi-manager-in-120-lines)
    - [Declaring the Data Structure](#declaring-the-data-structure)
    - [Implementing the Functionality](#implementing-the-functionality)

## Installation

### Arduino
This library uses the following libraries, which have to be installed first when using Arduino IDE:

 1. [SDDS Core Library](https://github.com/mLamneck/SDDS)
 2. [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
 3. Clone this repository into your library folder, and you are ready to go.

### PlatformIO
Add the GitHub link to this repository as a lib dependency to your platformio.ini file, as shown in the following example:

```
[env:myEspEnv]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = https://github.com/mLamneck/SDDS_ESP_Extension
```

## WebSpike
The Web Spike actually does two things:
* It uses the [plain protocol](https://github.com/mLamneck/SDDS?tab=readme-ov-file#plain-protocol) to publish data over websockets.
* It provides a website with a generic UI to interact with the data.

To test it, you can flash our [LED](/examples/led/led.ino) example code described in the [SDDS Core](https://github.com/mLamneck/SDDS?tab=readme-ov-file#example-for-this-documentation) example section. It includes the built-in components for parameter save and the WifiManager.

### Introducing the User Interface
As mentioned in the [SDDS Core library](https://github.com/mLamneck/SDDS?tab=readme-ov-file#why-to-use-this-library), one of the key benefits of SDDS is that you don't have to worry about user interfaces, as they are created automatically based on the variables you use in your program. In the following sections, we want to introduce the web-based user interface provided by the WebSpike and give you some basics on how to use it. To access it, you have to connect to the Access Point (explained in the next section) created by the ESP and follow the link [http://192.168.4.1](http://192.168.4.1). Below is an illustration of how the user interface of our LED example program will look:

![tree illustration](assets/structureGraph.PNG)

If you read something like "set ```led.onTime=500```" in this documentation, we mean you should follow the green line in the illustration, enter a value of 500, and accept the change.

#### Navigate through the tree on mobile devices
When you browse the website, you will first see the picture on the left. We think the usage is self-explanatory, but here are some guidelines:
* Each variable in your tree is represented by two columns with the name in the left and the value in the right.
* For readonly variables, the font color in the right column is gray instead of black, and the value cannot be edited.
* Whenever you see ```>``` in the right column, it means this is a struct. You can navigate into that structure by clicking in the right column.
* You can navigate back to the parent struct by clicking the black left arrow at the bottom left.
* For non-struct variables, you can change the value by clicking in the right column with the current value. This will open up the soft keyboard or select dialog if it's an enum.

#### Navigate through the tree on Desktops
With a keyboard available, you can navigate through the tree using the arrow keys. Use the up and down arrows to select the current row indicated by the blue line below the name. Use the right key to navigate into substructures or start entering a value. Pressing the left key in the right column stops the editing process. Pressing the left key in the right column navigates back to the parent struct. While editing a value, pressing Enter accepts the value, and pressing Escape cancels the editing process.

### Connect to the Access Point
After flashing the code, the ESP will open an Access Point with the following credentials:

* **SSID: myExcitingSSID**
* **Password: Start12345**
* **Default IP: 192.168.4.1**

Connect to this hotspot and visit the configuration [http://192.168.4.1](http://192.168.4.1).

### Configure your WiFi Credentials

* Navigate to ```wifi```.
  * Enter your SSID.
  * Enter your password.
  * Enter the hostname that the ESP should have in your network. This will determine the URL you have to enter. For example, if you go for ```myNewEspProject```, you will have to enter [http://myNewEspProject](http://myNewEspProject) when it's connected to your router.
* Navigate to set ```params.action=save```.
* Restart your board.

## WifiManager

### What is a WiFi Manager

ESP boards are amazing, and connecting them to a router is fairly easy. However, if you don't want to have hardcoded credentials, you need a way to specify them at runtime. This is what a WifiManager does. There are a lot of WifiManagers out there, but we've implemented one on our own for a few reasons:

* First of all, we don't like to be dependent on other libraries if not necessary.
* Second, the implementation of the logic is straightforward, and we wouldn't even consider searching for a library.
* Third, and most importantly, we use it as another example to prove our statement from the [SDDS Core Library](https://github.com/mLamneck/SDDS?tab=readme-ov-file#why-to-use-this-library) and show once more how you can tremendously speed up your development process and how even beginners are able to do advanced stuff like a WifiManager. Check out our A full-featured WiFi Manager chapter in 120 lines [chapter](#coding-a-full-featured-wifi-manager-in-120-lines) if this sounds interesting.

### How does our WifiManager work?

The core functionality in short words is not more than the following: If it has no credentials stored, it creates an Access Point and provides a Website to enter and store the credentials, so that with the next boot it's able to connect.

Our implementation is a bit more sophisticated:

1. Startup
2. Credentials stored?
    1. Try to connect.
    2. Connected?
       1. Check if still connected every minute, and if not, go to step 3.
    3. Not connected?
       1. Create an Access Point with a website to configure it.
       2. After 5 minutes, go to 2.1.
3. Credentials not stored?
    1. Create an Access Point with a website to configure it.

Note that we are not rebooting the ESP, and we also implemented a reconnect (The built-in auto-reconnect feature still does not work reliably on ESP. For example, if you turn off your WiFi during the night, you will find your ESP not connecting in the morning. That's why we have to check it on our own). And on the other hand, if it cannot connect, it doesn't get stuck in the Fallback AP. Instead, it opens the Fallback AP and tries to connect to your local network in 5-minute intervals.

### Using the WifiManager

Using our WiFi Manager is straightforward. Just include ```uWifiManager.h``` and add a ```TwifiManager``` struct somewhere in your tree. See the following boilerplate code for an application using SDDS.

```C++
#include "uTypedef.h"
#include "uMultask.h"
#include "uParamSave.h"

#define WIFI_MANAGER_AP_SSID "myExcitingSSID"
#define WIFI_MANAGER_AP_PW "Start12345"
#include "uWifiManager.h"

class TuserStruct : public TmenuHandle{
    sdds_struct(
        sdds_var(TparamSaveMenu,params)
        sdds_var(TwifiManager,wifi)
    )
    TuserStruct(){
        //you application code goes here... 
    }
} userStruct;

#include "uWebSpike.h"
TwebSpike webSpike(userStruct);

void setup(){

}

void loop(){
  TtaskHandler::handleEvents();
}
```

#### Testing the WiFi Manager
You can use the [LED example code](examples/led/led.ino) in order to try it.

### Coding a Full-Featured WiFi Manager in 120 Lines
#### Prerequisites
In order to follow along, it's beneficial to be familiar with the basics of the [SDDS Core library](https://github.com/mLamneck/SDDS?tab=readme-ov-file#documentation). But we think it's still possible to understand it right away and read some stuff on demand. Just try it yourself as you like.

There are a lot of WiFi Managers out there. If you have a look at the implementation, you will find a lot of code. It's going up to 10,000 lines. We don't want to say anything against these libs - they might be great - but obviously the functionality described in [how does the WifiManager work](#how-does-our-wifimanager-work) does not justify such a huge codebase and the question is where does the effort go if not into the functionality?

Let's have a look at what's necessary to create a WiFi Manager.
1. **The Functionality**
    * Piece of cake.
2. **A Website with a User Interface**
    1. Run a webserver on the ESP.
    2. Write a Website with:
        * HTML
        * CSS
        * Javascript
        * Post data to the ESP
        * Probably handle data coming from the ESP if you want some status updates.
    3. Handle posted data on the ESP.
    4. Run websockets if you want to have status updates.
3. **Save the Data to Non-Volatile Memory**
    * Use EEPROM or SPIFFS.

Obviously part 2 is the biggest and requires the most knowledge. For sure it's overwhelming for beginners, and even advanced developers won't do it in 30 minutes. And finally, you have to accomplish step 3 and test everything, which is not to underestimate as well.

Reading over the list, with the goal in mind, I get in a bad mood and I wouldn't even start. So much effort for such a simple task!? Let's see if we can do it less painfully with SDDS.

#### Declaring the Data Structure

We start by creating a new file ```uWifiManager.h``` and brainstorm about the data structure we need.
```C++
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
        TwifiManager(){
          //logic goes here
        }
```
This will look like the following in the user interface provided by the WebSpike. Note that this is not a fixed thing that we did behind the scenes; it is created based on the structure you have defined above, so adding variables in the above struct will add it in the UI without any further doing.

![WiFi menu](assets/Wifi.jpg)

Most of the variables are self-explanatory, but some might not be:
* action
    - Can be used to trigger the connection after entering the credentials.
* currMode
    - The mode the WiFi module is currently in (STA/AP).
* mode
    - Probably we want the ESP to create an Access Point with the given SSID and password instead of connecting to our router.

#### Implementing the Functionality

Our business logic goes into the constructor of our `TwifiManager` class, and all the following code snippets will be located there. First of all, we need to initialize the WiFi module.

```C++
...
class TwifiManager : public TmenuHandle{
  ...
    TwifiManager(){
        on(sdds::setup()){
            WiFi.begin();
        };
    }
```
The ```on(sdds::setup()){...}``` is basically equivalent to the ```setup(){...}``` in typical Arduino programs. But because we want to write independent components, we need a way to do the component-related initialization in that component.

Ok, let's continue with the logic for the ```enabled``` switch:
```C++
on(action){
    if (action==TwifiAction::e::connect){
        status = TwifiStatus::e::connect;
        timer.start(100);
        action = TwifiAction::e::___;
    }
};
```
Here we are just triggering the connecting phase if action is `connect` and setting it back to `___` to give some feedback.

Let's continue with the first of the 3 states in our state machine: the **connect** state.
```C++
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
    //...
```
This can be read like this:
* Do we have stored credentials?
    * Do we want to use the stationary mode (connect to a router)?
        * Call `connect()` to start the connecting process.
        * Switch state to the next phase -> wait to be connected.
    * Or do we want to create an AP with the given credentials?
        * Create the AP.
* If we don't have stored credentials:
    * Create a fallback AP.

The next state is **waitConnect**:
```C++
//...
case TwifiStatus::e::waitConnect:
    timer.start(1000);
    checkCnt = checkCnt+1;
    if (connected()) status = TwifiStatus::e::connected;
    else if (checkCnt >= 10){
        //create fallback if no connection has been established with the
        //provided credentials and try to connect again after 5 minutes
        createFallbackAP();
        timer.start(60000*5);
    }
    break;

```
Here, we are basically checking every second if we are connected, and if not, after 10 checks (10 seconds), we create the fallback AP and start a timer with 5 minutes. In the `createFallbackAP()` function, we set the state to fallback, which means after 5 minutes, we go back to phase 1, which will try to connect again.

This is what we want. If you turn your WiFi off during nighttime, it will try to connect every 5 minutes and will be connected in the morning. But if you changed your SSID or something else happened so that it cannot connect anymore, you can still catch it in AP mode to configure it without the need of a reboot.

If we are finally connected, we switch the state to connected, the final state is **connected**:
```C++
case TwifiStatus::e::connected:
    if (connected()) timer.start(60000);
    else {
        status = TwifiStatus::e::connect;
        timer.start(100);
    }
```

Here, we are checking every minute if we are still connected, and if not, we set the state back to **connect** to start all over again. The **connected** state is necessary because the auto-reconnect feature of the ESP is still not reliable, and we have to manage it on our own.

And that's it! We just have to implement a few functions that will contain the code you are most likely familiar with. Check out the full code in the [source file](src/uWifiManager.h). How amazing is that? No need to know about HTML, CSS, JavaScript, websockets, and all the stuff that actually has nothing to do with what we wanted to implement. You get it for free. That's the beauty of SDDS. We hope that after seeing the real-world example, you see the benefits as clearly as we do.
