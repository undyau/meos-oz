# meos
MeOS - A Much Easier Orienteering System

- MeOS project: www.melin.nu/meos
- Source code for the MeOS project: https://github.com/melinsoftware/meos
- Source code for the MeOS-OZ project: https://github.com/undyau/meos-oz

## MeOS-OZ differences from main Meos Project
- XML export by course
- Default language English
- Default Eventor is Australian Eventor
- Support to print sticky label results
- Fastest legs bold-printed on splits
- Class position on splits
- Enhanced score event support in splits and results
- Support for weird Sydney age classes in quick entry mode
- Simple set-up for Sydney Summer Series events
- Pop-up with result for score events
- Interface for upload to Sydney Summer Series results interface
- Course displayed on start list
- Support for import of Or format entries list
- Last hire sticks file is automatically loaded at start-up
- Late entries from Eventor entries imported without touching existing data
- Import another Eventor event's entries as a "season ticket list"
- Support for direct export to liveresults.page using MOP 2, including score events
- Support for Radio Online Control usage from different time zone.

## Building MeOS-OZ
### Prerequisites
- Visual Studio
- Inno Setup

### Compilation
The supplied solution file for Visual Studio is used to build the x86 version
of MeOS using DLLs from the vanilla MeOS installation or GitHub.

### Building the setup executable
Inno Setup software is used for packaging - input file is installer\meos.iss.