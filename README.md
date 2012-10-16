# INTRO

This modification of the Source engine allows you to control the facial
expression of the virtual characters using your own facial movements. The
system uses the non-commercial faceAPI to achieve the tracking. Those wishing
to use this system will need to obtain a copy of the faceAPI.


# LEGAL STUFF

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


# REQUIREMENTS

-   **faceAPI 4** (tested with 4.0.0.2121a13) -- to obtain a demo version, please contact Seeing Machines (http://www.seeingmachines.com)

-   **Steam** -- a free program available from Valve

-   **Visual Studio 2008** or above


# SETTING IT ALL UP

Just to make your life difficult, I haven't included either the faceAPI or the
Source engine in this repo, so you'll need to do a bit of work before you have
this puppy up and running. That said, if you follow these steps closely you
should have it running pretty quickly.

1.  Grab Steam (http://store.steampowered.com/about/), install it, signup, grab
    Team Fortress 2 (it's free, yay) and then download the Source SDK (now listed
    under the Tools section of your Library)

2.  Open the Source SDK, select the Source SDK 2007 engine (note: whilst the
    2009 engine is listed, at the time of writing this the Source SDK 2009 Base -
    i.e. the files it requires to run - was not available) and choose **"Create a Mod"**

    To make life easier (for me at least), we will set the mod up like I do.
    Choose to **"Modify Half-Life 2 Single Player"**. For the name pick anything that
    suits you. For the directory, specify your Source Mods folder e.g. "C:\Program
    Files (x86)\Steam\steamapps\sourcemods\", except add to the end of this path
    the name of your mod in lowercase letters and stripped of spaces and special
    characters. So if you decide to call your mod "GitHub Avatar", you'll need to
    add to the end of your path "githubavatar". From here on in, I will refer to
    this folder as your 'project folder' (PROJECT_FOLDER)

3.  Navigate to this folder and clone this repo to it. If you're having trouble
    cloning to a non-empty directory, I would suggest grabbing the zip version
    from the website and then manually dragging the files over (you should be able
    to drag them all over in one operation). When you do this some of the game
    files will be overwritten, however for the most part it's pretty unobtrusive.

4.  In the src folder, open the properties.vsprops file and change the
    ModFolder so that it matches your project folder path

5.  Assuming you have obtained a copy of faceAPI 4 from Seeing Machines, you
    should have a faceAPI zip which contains an API directory. Ensuring you're using the x86 version of this library:

    1. Copy the 'bin' folder into your project folder.

    2. Copy the 'lib' and 'include' folders into your "PROJECT_FOLDER/src/game/shared/faceapi" folder. 

    Having done this, you should have the following directory structure:

        PROJECT_FOLDER
            bin
                faceAPI Dlls
                resources
                    ...
                cal
                    ...
            src
                game
                    shared
                        faceapi
                            include
                                ...
                            lib
                                smft40.def
                                smft40.lib
                            ...

6.  Building the mod:

    1.  Now to build the mod. Open the Game_Episodic.sln file in your src folder. *You may need to migrate this file, depending on your version of Visual Studio.*
        
        You'll also have a Game_Episodic-2005.sln file, which is the original project
        file - feel free to delete it.

    2.  Make sure the system is set to build under Release mode (Build >
        Configuration Manager > Configuration set to Release)
        
    3.  Build the solution (F7).

    4.  To run the system under the debug mode, open the 'Client Episodic'
        properties and navigate to the Debugging settings. Change the settings to the
        following:

        **COMMAND:**

            C:\Program Files (x86)\Steam\steamapps\YOUR_STEAM_USER_NAME\source sdk base 2007\hl2.exe

        **ARGUMENT:**

            -dev -window -novid -game "C:\Program Files (x86)\Steam\steamapps\SourceMods\PROJECT_FOLDER"

        **DIRECTORY:**

            C:\Program Files (x86)\Steam\steamapps\YOUR_STEAM_USER_NAME\source sdk base 2007

        If the game crashes when loading, try running 'Source SDK Base 2007' first and
        then relaunching your mod

