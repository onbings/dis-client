# Dis-Client: Debug Info System Client application
<!--TOC-->
  - [Dis Architecture](#dis-architecture)
    - [Advantages](#advantages)
    - [Navigation](#navigation)
    - [Configuration](#configuration)
    - [Startup Sequence](#startup-sequence)
- [Dis service c++ client](#dis-service-c-client)
<!--/TOC-->
# Dis Architecture
This is the new paradigm used by Dis

```
                                    <--------Dis Service--------->
                                                    #
:  any          : Network(internet) : any           #            : Network(internal) :  any                :
: ------------- : ----------------- : ------------- # ---------- : ----------------- : ------------------- :
: python client : ---HttpRequest--> : Dis web       # Dis grpc   : <---Json data---> : grpc server         :
:               : <---Json data---- : socket server # client     :                   : hosted on FtpServer :
                                                    #            :                   : ------------------- :
                                                                 : <---Json data---> : grpc server         :
                                                                 :                   : hosted on P6x       :
                                                                 :                   : ------------------- :
                                                                 : <---Json data---> : grpc server         :
                                                                 :                   : hosted on V6x       :

```
## Advantages
- Debug data and control info are exchanged as json text data providing a portable way to serialize data without taking care of cpu endianness
- The dis system can retrieve debug data from different board at the same time, so different view of the same board can be retrieved from different clients
- Client is multiplatform thanks to the use of python script language
- Client can connect over the internet to a centralized Dis Service thanks to the use of the WebSocket protocol
  
 A WebSocket is a protocol that provides full-duplex communication channels over a single TCP connection. It allows for interactive and real-time communication between a client (typically a web browser) and a server. WebSocket is designed to be efficient, low-latency, and suitable for applications that require continuous data exchange without the overhead of repeatedly establishing new connections.
## Navigation
To ease user navigation in the different Pages/SubPages hierarchy, the following keyboard keys are used:
- **page down**: decrease current **Page** number to request.
- **page up**: increase current **Page** number to request.
- **down arrow**: decrease current **SubPage** number to request.
- **up arrow**: increase current **SubPage** number to request.
- **f**: Pause or resume the screen periodic refresh.
- **r**: this key is used to reset information linked to this (Page,SubPage,Channel) component. For example you can implement an hw reset of a given module or reset some statistics related to this debug page info.
- **+**: increase current channel number linked to the (Page,SubPage) info to request
- **-**: decrease current channel number linked to the (Page,SubPage) info to request
- **!**: if you press this key, the following edit text control will popup and you can enter a user data which will be linked to this (Page,SubPage). The format and feature of this user data is only controlled by the developper. If was added first to be able to control a memory viewer on a target board. You can for example enter "Len,Offset" in the user input control box and this data will be send to the grpc board server. This one will decode it according to its own rule and returns information based or not on this user input.
  
  ![Alt text](pic/dis_user_input_ui.jpg)

- **?**: if you press this key, a specific help message related to the current page will be displayed on the ui. This text comes from the data/DebugPageLayout.json file.

  ![Alt text](pic/dis_help.jpg)

- Each page has a personal page number with **3 digits**, so you can just type this 3 digit identifier to go directly to the corresponding page
## Configuration
You need to have a dis service running somewhere in your infrastructure and this one:
- Must be reachable by the client renderer application network using WebSocket
- Must be able to contact the different dis grpc servers present on the different boards using its own grpc clients socket.

For this last point, you must provide on the dis service a json file which will describe the different **routes** used to contact the different dis grpc server:

```json
DebugServiceRoute.json:

{
  "version": "1.0.0",
  "protocolInfo": "GET /DebugServiceRoute?seq=1",
  "route": [
    {
      "name": "/Gbe/",
      "ip": "127.0.0.1:2510"
    },
    {
      "name": "/V6x/",
      "ip": "127.0.0.1:2511"
    },
    {
      "name": "/P6x/",
      "ip": "127.0.0.1:2512"
    }
  ]
}
```
Each dis grpc server present on each board must provide a json file describing **the debug pages and sub pages layout** it manages and which can be requested by the service.

```json
DebugPageLayout.json:

{
  "version": "1.0.0",
  "protocolInfo": "GET /Gbe/DebugPageLayout?seq=2",
  "title": "Gbe Debug Info Server",
  "page": [
    {
      "label": "Gigabit - Configuration",
      "page": 800,
      "maxChnl": 0,
      "subPage": [
        {
          "label": "Ip configuration",
          "help": "<h1>This is a heading</h1><p>This is some paragraph text.</p>"
        },
        {
          "label": "Filesystem configuration",
          "help": "untangle yourself !"
        }
      ]
    },
    {
      "label": "Gigabit - Comram",
      "page": 801,
      "maxChnl": 0,
      "subPage": [
        {
          "label": "PC to uC Comram info"
        },
        {
          "help": "<h1>Navigation</h1><p>F1 decrease offset.</p><p>Shift-F1 increase offset.</p>",
          "label": "PC to uC Comram Gbe Cmd"
        },
        {
          "help": "<h1>Navigation</h1><p>F1 decrease offset.</p><p>Shift-F1 increase offset.</p>",
          "label": "uC to PC Comram Gbe Cmd"
        }
      ]
    }
  ]
}
```
## Startup Sequence

- When the client app starts, it contacts the dis service and asks for the route definition file by sending "GET /DebugServiceRoute". This one return data/DebugServiceRoute.json.
- After this, it will contact **each route endpoints** present in data/DebugServiceRoute.json and ask for the corresponding debug page layout definition file by sending "GET **{route}**DebugPageLayout". Each route endpoint returns its own data/DebugPageLayout.json. 
- When this phase is complete, the client application knows all the route endppoints and has a global representation of all the debug page hierarchy which is spreaded over all the modules.
- Then it can call periodically the dis service with "GET {route}DebugPageInfo/**Page**/**SubPage**/?chnl=**Channel**&flag=**Flag**&key=**LastKeyboardEntry**&user_input=**UserInput**"
with:
  - **Page**/**SubPage** which specify which Page and SubPage to query
  - **Channel**: for each **SubPage** of each **Page** you can define a notion of "**channel**" which can be used to identify a specific instance in a (Page,Subpage) view: ex **Page** "*Config*" **SubPage** "*Recorder*" **Channel** *1 to 6* to display the debug page info of the config of a given recorder instance.
  - **Flag** contains a bit mask integer value which describe which information and action to process on this page (see enum DEBUG_INFO_REQUEST_FLAG just below)
    - reset the page info (0x80000000)
    - ask for background screen layout (0x00000001)
    - ask for foreground screen info (0x00000002)
    ex: **Flag** = 0x80000003->reset debug page info and request background and foreground info. 
    For performance reason you should only ask for background info once when you ask for the first time debug page info of a given Page,Subpage. After that, you can only request fore content **Flag** = 0x00000002
  - **LastKeyboardEntry** contains a binary value which describes which combination (if any) of shift/ctrl/alt and function keys has been pressed  (see enum DEBUG_INFO_KEY_FLAG and enum DEBUG_INFO_KEY just below)
  - **UserInput** is the last input entered by the user via the '?' key
  
Remark: The last two parameters can be used by the target grpc server to implement a custom navigation inside the data that it returns to the caller
For example if you want to implement a debug page to dump a very huge memory page, you can use **UserInput** to send a first command to setup the number of byte to dump and the initial offset in this memory.
Later on **LastKeyboardEntry** can be decoded by the grpc server to navigate in this memory zone (i.e. F1: decrease current offset Shift-F1: increase current offset)

# Dis service c++ client
A c++ client application based on DearImgui is been developped to test and validate all these concepts.
It integrates also the automatic discovery system used on Xts servers. This one is based on the [mDns](https://pypi.org/project/zeroconf/) python package
and gives a list of the different devices present on a local network with meta data information.
This list can be retrieved using a grpc api by our dis-client application.

## Ui
Starting with a list of dis compatible devices given by mDns we will display a first Ui:

