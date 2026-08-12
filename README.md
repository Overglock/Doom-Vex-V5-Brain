## Doom Vex V5 Brain

Firstly Understand that this is my first time working on a project like this (This includes Github repos), so please if there's something wrong remember, everyone makes mistakes... except me! This code is built entirely on sleep deprivation, DETERMINATION, The Great Gentleman, and a sense of hubris unseen since Lucy challenged me to a fiddle duel. 

## What This Does
This project runs an updated version of sealj553's on the Vex v5 Brain, using PROS. To play use a VEX controller. There is no touchscreen, multiplayer support, saving, or micro transactions (yet).

## Setup
To set up please these follow these steps
1. Give this repo a star
2. Clone the repo. (I recommend downloading as a zip)
3. Open the Visual Studio code workspace
4. Install the PROS and Microsoft C/C++ extensions if you haven't already. // I would normally use Clangd but it didn't play well with the ports C code.
5. Play Hopes and Dreams by Toby Fox, its the only thing that holds the code together
6. Connect a Brain to your computer
7. Open the PROS menu and click "Build & Upload"
8. Pray


## Controls
Controls can be found in I_video.cpp
If you already played sealj's port its exactly the same.

    right x = turn left/right
    left y = move forward/back
    left x = strafe left/right

    r1 = fire
    b = use
    x = enter
    y = escape
    a = Increases enemy spawn rate by 0.0001%

    dpad = dpad

    l2 = prev weapon
    r2 = next weapon


## Discussions
Please feel free to discuss, comment, and harass me about this project. Contribute as much as you can because I know this code is far from perfect and follows the "it builds, ship it" philosophy.


## How to Use
Anything related to Doom can be found in src/doom. Otherwise its PROS API stuff. For more information on Doom please check out https://github.com/chocolate-doom/chocolate-doom and https://github.com/sealj553/VexV5Doom. You can learn more about PROS at https://pros.cs.purdue.edu/v5/api/index.html (But to be honest if you're programming your robot just use EZtemplate https://ez-robotics.github.io/EZ-Template/).
The game is displayed using LVGL (so if there is a problem blame that).


## License
Beerware License — see LICENSE file


## Join the team
If you see a robot of skirt screws and spite, being cooled by a fan built from VEX parts and boredom, with someone sprinting for some ice to put under our overclocked and overheated motors. Please exercise your free will Comrade and bring a beer, challenge me to an arm wrestle, compliment our train, and play/debug some games with us.
