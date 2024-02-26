# SDDS_ESP_Extension
This is an extension from the [SDDS core library](https://github.com/mLamneck/SDDS) core library. It introduces some ESP specific functionalities like Web Spikes.

## Table of contents
- [Why to use this library](#why-to-use-this-library)
- [Installation](#installation)
  - [Arduino](#arduino)
  - [PlatformIO](#platformio)


## Installation


### Arduino
This library uses the following libraries which have to be installed first when using Arduino IDE. If you are us

 1. [SDDS core library](https://github.com/mLamneck/SDDS)
 2. [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
 3. Clone this repository in your library folder and you are ready to go.


### PlatformIO
Add the github link to this repository as a lib dependency to your platformio.ini like in the following example.

```
[env:myEspEnv]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = https://github.com/mLamneck/SDDS_ESP_Extension
```
