# Volume Rendering (Raymarching) Plugin Showcase Project for Unreal Engine
Allows volumetric rendering of DICOM and .MHD data with Unreal Engine.

Uses https://github.com/tommybazar/TBRaymarcherPlugin as a submodule providing all the functionality.

This repo should mostly just show you the Default(Engine/Game/Input).ini that will work well with the plugin. 

## Submodules
To clone this repo with the plugin included, use this command:

`$ git clone --recurse-submodules https://github.com/tommybazar/TBRaymarchProject.git`

Or after a regular clone, perform a 

`$ git submodule init`

`$ git submodule update`

## UE Version
I will try to make the project compatible with the latest version (Currently 4.27.2).
For older versions of the engine, go to the branch with the appropriate name. Older version are not be updated with new functionality.

There is also a branch called "ue5" that works with Unreal Engine 5.0 (after some cleanup of the code, this will be merged into master and work will continue only on UE5).

## Readme
For a full readme, go the submodule's repo :

https://github.com/tommybazar/TBRaymarcherPlugin

![
](https://github.com/tommybazar/TBRaymarchProject/blob/master/Documents/Render1.png)


![
](https://github.com/tommybazar/TBRaymarchProject/blob/master/Documents/Render2.png)

Both of these renders are using the same CT scan, only difference is windowing and used transfer function.

# Video showcase / deep-dive tutorials
Part1 - Showcase & Intro : https://youtu.be/-HDVXehPolM

# Discord
If you want to ask me anything or (potentially) talk to other people using the plugin, here's a discord server for it : https://discord.gg/zKutZpmFXh

# Example
 * The project works out-of-the-box with everything being included in the TBRaymarcherPlugin. There is an example map for Mouse and Keyboard and
 an example map for VR, input bindings are only setup for Oculus, go into Project settings -> Input if you're using a different headset. Both are included within the plugin.

# License 
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

Licensed under MIT license.

Both me and Technical University of Munich are copyright holders, as major parts of this software were written as part of my thesis and/or working-student employment for TUM.

See LICENSE file for full license text.

DICOM loading is utilizing a modified version of VTK DicomParser, made by Matt Turek. See  License.txt in /Source/VolumeTextureToolkit/Public/VolumeAsset/DICOMParser/License.txt for full license text.