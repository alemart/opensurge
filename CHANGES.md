# Release Notes

## 0.6.1.1 - June 1st, 2024

* Fixed an OpenGL-related issue in the Flatpak edition
* Fixed a memory-related bug in the Entity Manager
* Updated metadata and translations
* Tweaked the water effect for extra performance
* Reintroduced the Create option at the Main Menu
* Updated the Thanks for Playing screen
* Other minor changes

## 0.6.1 - May 17th, 2024

* Ported the game engine to Android
* Partial rewrite of the engine. Made the engine mobile-ready with a massive performance update covering many areas: rendering, scripting, collision detection, entity and brick systems, loading times, and more!
* Improved the physics system. Made it faster and more robust
* Introduced the mobile mode and the mobile level editor
* Introduced the MOD loader and its compatibility mode
* Introduced the Import Utility for upgrading MODs
* Introduced support for Player 2 & AI-controlled characters
* Introduced player transformations and numerous player flags
* Introduced keyframe-based animations and custom sprite properties
* Introduced wavy water effects and internal support for shaders
* Introduced a new Pause Menu
* Introduced a new Options Screen
* Added support for various special characters in bitmap fonts
* Made numerous additions to the SurgeScript API
* Updated several SurgeScript objects. Introduced new items and new components
* Reimplemented nanoparser, the asset system, the animation system, the entity system, the brick system, the particle system, the level height sampler, and more

This massive update includes changes not detailed in the list!

## 0.6.0.3 - September 23rd, 2022

* Tweaks to the level design, to the default controls and to the build system
* Changed the way the engine handles analog sticks

## 0.6.0.2 - September 15th, 2022

* Adjusted gimmicks: Water Bubbles, Spikes
* Made tweaks to the level design, to the title screen and to the build script
* Changed the parser used on the stage select
* Added support for loading quests via developer mode
* Removed the quest selection screen

## 0.6.0.1 - September 6th, 2022

* Updated the default mapping for gamepads
* Adjusted the "Walk on Water" gimmick
* Optimized the creation and the rendering of brick particles
* Made tweaks to the level design

## 0.6.0 - September 2nd, 2022

* Renamed the game to Surge the Rabbit
* Improved the pixel art style of the game
* Created a new Waterworks Zone
* New gimmicks: Conveyor Belts, Walk on Water, Power Pluggy
* New title screen and new title card animation
* New translations: Italian, Esperanto
* Introduced language extensions (languages/extends/ folder)
* Improved macOS compatibility
* Introduced support for transitions between animations
* Introduced the concepts of action spot and sprite anchor
* Improvements to the SurgeScript API
* Improved several core scripts
* Added support for compound ${EXPRESSIONS} when evaluating text
* Changed the physics
* Added support for D-pad input on Xbox controllers
* Removed legacy Allegro 4 code
* Bugfixes and general improvements

## 0.5.2.1 - April 15th, 2021

* Fix sprites/overrides/ detection bug on Windows
* Include LMMS source files (1up, speed, drowning, gameover)
* Player: reduce jump_lock_timer after charging
* Small fixes & adjustments

## 0.5.2 - January 25th, 2021

* Improved joystick support
* New translations: Polish, Russian
* New musics
* Updated the Giant Wolf Boss
* Added new little animals - different animals will appear in different levels
* Increased the drowning time to 12 seconds
* Updated the turbo/invincibility time to 20 seconds
* Improved the springs: they now behave as expected when the player is running on the walls or on the ceiling
* Level Editor: users can now create levels in fullscreen mode
* Level Editor: users can now display collision gizmos by pressing a key
* Level Editor: the interface has been translated into multiple languages
* Fix: the engine will pause the SurgeScript VM when the game is paused
* Fix: consistent physics in slow computers
* Fix: the player can be smashed by solid moving platforms
* Improvements to the SurgeScript API
* Updated the credits screen: it now extracts data from a csv file
* Introduced CONTRIBUTING file with guidelines for contributors
* Introduced special folder sprites/overrides/ to ease sprite hacking
* Input maps are now read from the (new) inputs/ folder
* The HUD is now entirely controlled via scripting
* The 1up jingle no longer stops the level music
* Increased the maximum supported image size to 4096x4096 pixels
* Bugfixes and general improvements

## 0.5.1.2 - February 2nd, 2020

* New translations: Serbian, Bosnian, Dutch
* Updated the language selection screen
* Included AppStream metadata for Linux and for the free software ecosystem
* Physics engine: using a fixed timestep at 60 fps for improved precision
* Bugfixes and general improvements

## 0.5.1.1 - January 5th, 2020

* Added support for UTF-8 filenames on Windows
* Added the --hide-fps command line option
* Optimized the rendering of texts
* Removed unused fonts

## 0.5.1 - December 31st, 2019

* New special moves: Surge's Lightning Boom and Surge's Lightning Smash
* Improved the physics engine & the auto-angles detection method
* New translations: German, Spanish, Korean, French
* Updates to the SurgeScript API
* Included a SurgeScript template for creating your own shield abilities
* Made Life Icons and Goal Signs easier to hack
* Created an audible countdown that is played when Surge is underwater
* Introduced multilingual support for font scripts
* Aesthetic improvements to the menus
* General improvements & bugfixes

## 0.5.0.2 - November 1st, 2019

* Improved the jump and the charge-and-release move
* Updated the icon
* General improvements

## 0.5.0.1 - October 5th, 2019

* Fixed details of the initial release

## 0.5.0 - September 27th, 2019

* Initial release
