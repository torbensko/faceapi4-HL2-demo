# Overview

This modification of the Source engine allows you to control the facial
expression of the virtual characters using your own facial movements. The
system uses faceAPI to achieve the tracking. Those wishing to use this system will need to obtain a copy of the faceAPI.


# Legal stuff

My code, namely the files found in the .../src/game/shared/faceapi directory,
is provided under a Creative Commons Attribution license
(http://creativecommons.org/licenses/by/3.0/). As such, you are free to use
the code for any purpose as long as you remember to mention my name (Torben
Sko) at some point. Also please note that my code is provided AS IS with NO
WARRANTY OF ANY KIND, INCLUDING THE WARRANTY OF DESIGN, 
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

This system also features the work of Ross Bencina (found within the
.../src/game/shared/ip and .../src/game/shared/osc directories). Please refer
to his files for licensing details. The remaining files have been provided by
Valve as part of the Source SDK. As taken from their website
(http://store.steampowered.com/subscriber_agreement/):

"Your Subscription(s) may contain access to the Valve software development kit
(the "SDK") for the computer game engine used in Half-Life 2 and other
compatible Valve products (the "Source Engine"). You may use, reproduce and
modify the SDK on a non-commercial basis solely to develop a modified game (a
"Mod") for Half-Life 2 or other Valve products compatible with and using the
Source Engine. You may reproduce and distribute the Mod in object code form,
solely to licensed end users of Half-Life 2 or other compatible Valve
products, provided that the Mod is made publicly available and distributed
without charge on a non-commercial basis

If you would like to use the Source SDK or a Mod for a commercial purpose or
activity, please contact Valve at sourceengine@valvesoftware.com"


# Requirements

-   **faceAPI 4** (tested with 4.0.0.2121a13) -- to obtain a demo version, please contact Seeing Machines (http://www.seeingmachines.com)

-   **Steam** -- a free program available from Valve

-   **Visual Studio 2008** or above


# Setting it all up

Just to make your life difficult, I haven't included either the faceAPI or the
Source engine in this repo, so you'll need to do a bit of work before you have
this puppy up and running. That said, if you follow these steps closely you
should have it running pretty quickly.

1.  Getting the software:

    1.  Install Steam (http://store.steampowered.com/about/) and sign-up

    2.  Grab *Team Fortress 2* or another Source title (this is required in order to gain access to the Source SDK)

    3.  Download the *Source SDK*, listed under the Tools section of your Library

2.  Setting up the game files:

    1.  Open the *Source SDK*

    2.  At the bottom of the window select the *Source SDK 2007* engine (whilst the
    2009 engine is listed, at the time of writing this the Source SDK 2009 Base -
    i.e. the files it requires to run - was not available)

    3.  Select **Create a Mod**

    4.  Within the dialog that pops up, choose to **Modify Half-Life 2 Single Player**

    5.  For the name pick anything that suits you. For the directory, specify your Source Mods folder e.g. "C:\Program Files (x86)\Steam\steamapps\sourcemods\", except add to the end of this path the name of your mod in lowercase letters and stripped of spaces and special characters. So if you decide to call your mod "GitHub Avatar", you'll need to add to the end of your path "githubavatar". From here on in, I will refer to this folder as your 'project folder' (PROJECT_FOLDER)

    6.  Navigate to this folder and clone this repo to it. If you're having trouble
    cloning to a non-empty directory, I would suggest grabbing the zip version
    from the website and then manually dragging the files over (you should be able
    to drag them all over in one operation). When you do this some of the game
    files will be overwritten, however for the most part it's pretty unobtrusive.

    5.  Assuming you have obtained a copy of faceAPI 4 from Seeing Machines, you should have a faceAPI zip which contains the following:

            faceapi-VERSION
                api [directory]
                libraries [directory]
                ...

        Do the following:

        1.  Copy the entire faceapi directory (as contained in the zip file) to your PROJECT_FOLDER. In doing so, rename the folder "faceapi", i.e. remove the version number.

        2.  Make a copy of the faceapi/api/bin folder and place it in your PROJECT_FOLDER

        Having done this, you should have the following directory structure:

            PROJECT_FOLDER
                bin
                    resources [directory]
                    cal [directory]
                    faceAPI files (.dll)
                faceapi
                    api [directory]
                    libraries [directory]
                    ... 
                src
                    game
                        shared
                            faceapi
                                source code (.cpp)

6.  Building the game:

    4.  In the *src* folder, open the properties.vsprops file and change the ModFolder so that it matches your project folder path (if you used the same naming convention suggested above, this shouldn't need changing)

    1.  Open the Game_Episodic.sln file in your src folder. You may need to migrate this file, depending on your version of Visual Studio.
        
    2.  Make sure the system is set to build under *Release* mode (*Build > Configuration Manager*)

    3.  Open the settings for the "apilibcpp.vs2008" project and under *Configuration Properties > C/C++ > Code Generation* set *Runtime Library* to **Multi-threaded (/MT)** (rather than **Multi-threaded DLL (/MD)**)
        
    4.  Build the solution (F7).

7.  To run the game, open the properties for the "Client Episodic" project and navigate to the Debugging settings. Change the settings to the following:

    **Command:**

        C:\Program Files (x86)\Steam\steamapps\YOUR_STEAM_USER_NAME\source sdk base 2007\hl2.exe

    **Argument:**

        -dev -window -novid -game "C:\Program Files (x86)\Steam\steamapps\SourceMods\PROJECT_FOLDER"

    **Directory:**

        C:\Program Files (x86)\Steam\steamapps\YOUR_STEAM_USER_NAME\source sdk base 2007

    If the game crashes when loading, try running 'Source SDK Base 2007' first and
    then relaunching your mod

