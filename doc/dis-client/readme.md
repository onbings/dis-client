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

## Json debug screen format

```json
{
    "version": "1.0.0",
    "userInfo": "GET /Gbe/DebugPageInfo/240/0/?flag=3&key=None&user_input=24,32&seq=3",
    "background": {
        "bg": "#808080",
        "line": [
            {
                "x": 2,
                "y": 4,
                "bg": "#808080",
                "fg": "#FFFFFF",
                "text": "Magic number:"
            },
            {
                "x": 2,
                "y": 5,
                "bg": "#808080",
                "fg": "#FFFFFF",
                "text": "Cdx version:"
            },
            {
                "x": 2,
                "y": 6,
                "bg": "#808080",
                "fg": "#FFFFFF",
                "text": "Cdx channel:"
            },
            {
                "x": 2,
                "y": 7,
                "bg": "#808080",
                "fg": "#FFFFFF",
                "text": "Gbe version:"
            },
            ...
            {
                "x": 2,
                "y": 22,
                "bg": "#808080",
                "fg": "#FFFFFF",
                "text": "Mac address:"
            }
        ]
    },
    "foreground": {
        "bg": "#808080",
        "line": [
            {
                "x": 21,
                "y": 4,
                "bg": "#808080",
                "fg": "#0000FF",
                "text": "0x00000000"
            },
            {
                "x": 21,
                "y": 5,
                "bg": "#808080",
                "fg": "#0000FF",
                "text": "0x08020304"
            },
            {
                "x": 21,
                "y": 6,
                "bg": "#808080",
                "fg": "#0000FF",
                "text": "0"
            },
            {
                "x": 21,
                "y": 7,
                "bg": "#808080",
                "fg": "#0000FF",
                "text": "0x20050405"
            },
            ...
            {
                "x": 21,
                "y": 22,
                "bg": "#808080",
                "fg": "#0000FF",
                "text": "00: 00.00.00.00.00.00"
            },
            {
                "x": 21,
                "y": 23,
                "bg": "red",
                "fg": "#0000FF",
                "text": "00000008"
            }
        ]
    }
}
```  

# Dis service c++ client
A c++ client application based on DearImgui is been developped to test and validate all these concepts.
It integrates also the automatic discovery system used on Xts servers. This one is based on the [mDns](https://pypi.org/project/zeroconf/) python package
and gives a list of the different devices present on a local network with meta data information.
This list can be retrieved using a grpc api by our dis-client application.

## Ui
Starting with a list of dis compatible devices given by mDns we will display a first Ui:


## Discovery

```C
struct DIS_DISCOVERY_PARAM
{
  uint32_t PollTimeInMs_U32;
};
```

The DisDiscovery::V_OnProcessing() method is a periodic thread will initiate the discovery process every DIS_DISCOVERY_PARAM.PollTimeInMs_U32 ms.
The discovered devices will will be stored in std::map<BOF::BofGuid,DIS_DEVICE> mDisDeviceCollection. 
A copy of this collection is available via the std::map<BOF::BofGuid, DIS_DEVICE> GetDisDeviceCollection() method.
A discovered device contains the following information:

```C
struct DIS_DEVICE
{
  std::string Name_S;
  std::string IpAddress_S;
};
```

## Dis-Client thread loop

```C
struct DIS_CLIENT_PARAM
{
  uint32_t PollTimeInMs_U32;              //Poll time to query discovered device
  uint32_t DisServerPollTimeInMs_U32;     //Poll time to ask debug ingo to Dis Server

  uint32_t FontSize_U32;
  uint32_t ConsoleWidth_U32;
  uint32_t ConsoleHeight_U32;
  BOF::BOF_IMGUI_PARAM ImguiParam_X;
};
```

The DisClient::V_OnProcessing() method is a periodic thread will query the result of the discovery process every DIS_CLIENT_PARAM.PollTimeInMs_U32 ms.
It compare the received result to the previous one and add/remove devices that have appeared/disappeared. the final result is a list of DIS_DEVICE.

For each new discovered device a DIS_CLIENT_DBG_SERVICE is created and added to std::map<BOF::BofGuid, DIS_CLIENT_DBG_SERVICE> mDisClientDbgServiceCollection;

```C
struct DIS_CLIENT_DBG_SERVICE
{
  bool IsVisisble_B;
  std::unique_ptr<DisService> puDisService;
  int32_t PageIndex_S32;
  int32_t SubPageIndex_S32;
};
```

Inside the DIS_CLIENT_DBG_SERVICE structure you have the std::unique_ptr<DisService> puDisService which points to a DisService object.
So for each discovered device a corresponding DisService is created, stored in mDisClientDbgServiceCollection and started.
If a device disappearred the corresponding DisService is stopped and removed from the list


```C
struct DIS_SERVICE_PARAM
{
  std::string DisServerEndpoint_S;
  uint32_t PollTimeInMs_U32;
  DIS_DEVICE DisDevice_X;
};

DisServiceParam_X.DisServerEndpoint_S = "ws://" + rItem.second.IpAddress_S + ":8080";
DisServiceParam_X.PollTimeInMs_U32 = mDisClientParam_X.DisServerPollTimeInMs_U32;
DisServiceParam_X.DisDevice_X = rItem.second;

DisClientDbgService_X.IsVisisble_B = true;
DisClientDbgService_X.PageIndex_S32=DIS_CLIENT_INVALID_INDEX;
DisClientDbgService_X.SubPageIndex_S32= DIS_CLIENT_INVALID_INDEX;
DisClientDbgService_X.puDisService = std::make_unique<DisService>(DisServiceParam_X);;
mDisClientDbgServiceCollection.emplace(std::make_pair(rItem.first, std::move(DisClientDbgService_X)));
DisClientDbgService_X.puDisService->Run();
```

The BofImgui main loop calls virtual functions on each runs. We use mainly two of them to make the the application live:
DisClient::V_PreNewFrame() and DisClient::V_RefreshGui().


DisClient::V_PreNewFrame() will call puDisService->StateMachine method to I advance the state machine which manages the dialogue with the DIS_DEVICE

```C
  for (auto &rDisClientDbgService : mDisClientDbgServiceCollection)
  {
    StateMachine_X.Channel_U32 = 0;  // rDisClientDbgService.second.;
    StateMachine_X.CtrlFlag_U32 = 0;
    StateMachine_X.DisService_S = "";
    StateMachine_X.FctKeyFlag_U32 = 0;
    StateMachine_X.Page_U32 = rDisClientDbgService.second.PageIndex_S32;
    StateMachine_X.SubPage_U32 = rDisClientDbgService.second.SubPageIndex_S32;
    StateMachine_X.UserInput_S = "";

    Sts_E = rDisClientDbgService.second.puDisService->StateMachine(StateMachine_X);
  }
```

DisClient::V_RefreshGui() will generate the ui based on the information got by the state machine. User inputs will also be used to feed the state machine.