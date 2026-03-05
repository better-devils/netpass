**AI Generated code is not allowed in any form in this repo**
# NetPass: a new way to experience StreetPass!

[![Translation status](https://weblate.sorunome.de/widget/netpass/app/svg-badge.svg)](https://weblate.sorunome.de/engage/netpass/)
[![GitLab Release](https://img.shields.io/gitlab/v/release/3ds-netpass/netpass)](https://gitlab.com/3ds-netpass/netpass/-/releases/permalink/latest/)


![QR Code](https://gitlab.com/Sorunome/3ds-streetpass/-/raw/release_builds/qr.png){width=400px}  
Scan QR code to download!

GBAtemp thread: https://gbatemp.net/threads/netpass-a-new-way-to-experience-streetpass.664005/  
Discord guild: https://netpass.cafe/discord

Introducing NetPass:

In the current state of the world, the 3DS is, sadly, on decline. That makes getting StreetPasses harder and harder, due to fewer and fewer people taking their 3DS with them on a walk. This is where NetPass comes in!  
Unlike StreetPass, NetPass works over the internet. Upon opening NetPass, you can pick various locations to go to, i.e. the train station, or a town plaza. Upon entering a location, you get passes from others who are in the same location! And, while you are at the location, others who enter it can get passes with you. But beware! You can only switch locations once every 10 hours!

## Licenses
The source code of this project is licensed under GPLv3 or later. See the included `LICENSE`.

Other assets, such as images and sounds, are licensed under CC-BY-NC. See the included `LICENSE-assets`.

## Translations
If you want to contribute to translations, we are doing that on [our Weblate](https://weblate.sorunome.de/projects/netpass)!

## Prerequisites

### For the NetPass 3DS homebrew

You need to have the following tools installed and added in your ``PATH`` environment variable:
- [devkitPro](https://devkitpro.org/wiki/Getting_Started)
  
  After installing devkitPro, you will need to download the following using [devkitPro pacman](https://devkitpro.org/wiki/devkitPro_pacman) or the devkitPro updater:
  - ``3ds-dev``
  - ``3ds-curl``
  - ``3ds-opusfile``

  In other words, you'll need to run the following command in a Terminal/command prompt (with administrator/root privileges):

  ```bash
  dkp-pacman -S 3ds-dev 3ds-curl 3ds-opusfile
  ```
  Or if you are using a customized Pacman install:
  ```bash
  pacman -S 3ds-dev 3ds-curl 3ds-opusfile
  ```
- [FFMpeg](https://ffmpeg.org/)
- [Python](https://www.python.org)
  - [Python-PyYAML](https://pypi.org/project/PyYAML/)
  - [Python-Requests](https://pypi.org/project/requests/)
- [Bannertool](https://github.com/diasurgical/bannertool/releases)

#### Additional prerequisite to build the `.cia` file
 - Makerom: You need the `makerom` executable in your `PATH` environment variable
   You can get it precompiled on https://github.com/3DSGuy/Project_CTR/releases and then copy it to `$DEVKITPRO/tools/bin`

### For the sysmodule patches

You will need to install the following tools. Make sure they are in your ``PATH`` environment variable.
- [armips](https://github.com/Kingcom/armips)
- [flips](https://github.com/Alcaro/Flips)

You will also have to manually dump the decrypted code of each of the sysmodules to patch from a 3DS, and then place the file as `code.bin` into the respective patch folder.
- Boot into [GodMode9](https://github.com/d0k3/GodMode9)
- Press the Home button, select ``Title manager`` and press A
- Select ``[1:] NAND / TWL`` and press A
- For each sysmodule:
  - Find its TitleID in the list
    - BOSS: ``0004013000003402``
    - CECD: ``0004013000002602``
    - NS: ``0004013000008002``
    - SSL: ``0004013000002F02``
  - Select it and press A
  - Select ``Open title folder`` and press A
  - Without changing your selection, press A
  - Select ``NCCH image options...`` and press A
  - Select ``Extract .code`` and press A
  - Wait for the operation to finish. When prompted, press A to continue
- Plug your SD card into your computer
- You should find files named ``<TitleID>.dec.code`` with ``<TitleID>`` being the TitleIDs of each sysmodule
- Copy them over to their respective folder (under ``./patches/<sysmodule name>/``)
- Rename each file into ``code.bin``

## Compilation

This project ships with a [Makefile](Makefile), which is meant to simplify the compilation process. If you're unfamiliar with them, you can find out more about GNU Make [here](https://www.gnu.org/software/make/).

### For the NetPass 3DS homebrew

As a 3DSX file:
```bash
make
```

As both a 3DSX and a CIA file:
```bash
make all
```

You will find the compiled binaries in the ``./out/`` directory.

### For the sysmodule patches

```bash
make patches
```

You will find the resulting IPS patches in the ``./romfs/patches`` directory.

## Credits

### Research
 - [This gist](https://gist.github.com/wwylele/29a8caa6f5e5a7d88a00bedae90472ed) by wwylele, describing some cecd functionality
 - [This repo](https://github.com/NarcolepticK/CECDocs) by NarcolepticK documenting some more of the cecd sysmodule
 - [StreetPass 2](https://gbatemp.net/threads/streetpass-2-rise-from-the-ashes.526749/) for valuable StreetPass data dumps
 - [Sheeple](https://github.com/MisterSheeple) & the [SpotPass Archival Project](https://spotpassarchive.github.io/) contributors for valuable SpotPass data dumps
 - [3DBrew](https://www.3dbrew.org) and all its contributors, especially of the [CECD service](https://www.3dbrew.org/wiki/CECD_Services) and [SpotPass](https://www.3dbrew.org/wiki/SpotPass)-related structures
 - [DaniElecta](https://github.com/DaniElectra) for his BOSS module research and documentation

### Translations
 - English: Sorunome
 - German: Sorunome
 - Russian: [Rednorka](https://gbatemp.net/members/rednorka.575239/), Geo
 - Japanese: [Akira SUNADUKA](https://gitlab.com/Akira-SN)
 - Polish: [DanteyPL](https://gitlab.com/DanteyPL)
 - Spanish: [Gato-kun](https://gitlab.com/Gato-kun), [Amnesia1000](https://gitlab.com/Amnesia1000)
 - French: [Straky](https://straky.fr/en), [Possemelius](https://gitlab.com/Essepeius), Tourneur
 - Italian: [LNLenost](https://github.com/LNLenost)
 - Chinese (Traditional): ManLok Ho
 - Ukrainian: Geo
 - Portuguese: Lia, arth
 - Dutch: Robbin12391, aiydn

 ### Additional programming

 - [RSM](https://github.com/giroletm/): SpotPass URL rewriting patch for the BOSS sysmodule
