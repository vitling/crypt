
# Crypt
![Build](https://github.com/DavW/crypt/workflows/Build/badge.svg)

**Crypt** is a software synthesiser plugin designed for creating spacious cold hyper-unisoned
synth sounds; developed by [Vitling](https://www.vitling.xyz) for the [Bow Church](http://bowchurch.bandcamp.com/) project.

![Screenshot](https://github.com/DavW/crypt/blob/main/screenshot.jpg?raw=true)

It is written in C++20 and depends on the [JUCE](https://github.com/juce-framework/JUCE) framework, which is
included as a submodule.

## Installation

### Mac & Windows

Go to the [Vitling.xyz plugin download page](https://www.vitling.xyz/plugins) for conveniently packaged donwloads for **Mac** and **Windows**

#### Extra notes for Mac users

I have now finally joined the Apple Developer Program, so I am pleased to be able to offer proper installers and notarized binaries for Mac

However, the Apple Developer Program still costs money, even for an open source developer. I suggest you write to Apple and lawmakers in your jurisdiction and complain about their anti-competitive practices.

If you use the Mac version, please consider [donating some money](https://paypal.me/vitling) to me to offset the cost I have incurred to make this possible.

### Windows

Windows builds are also built by the GitHub Actions CD system, but I do not have a Windows computer available to verify that they work
as expected. If you're a Windows user, please let me know how your experience is and whether it works or not.

You can download whatever binaries that JUCE produces during the make process from the [Release Page](https://github.com/DavW/crypt/releases) and copy them into your VST3 plugin folder


### Linux

Building on Linux should be fairly straightforward, but I had some slightly confusing linker errors when I tried. Let me know if you manage to get it to work.

#### Dependencies
```cmake g++ libfreetype6-dev libx11-dev libxinerama-dev libxrandr-dev libxcursor-dev mesa-common-dev libasound2-dev freeglut3-dev libxcomposite-dev pkg-config```

These are Debian/Ubuntu package names (install with `sudo apt-get install ` and paste the above), you may need to translate for your distro

#### Build

```bash
git clone --recursive --shallow-submodules https://github.com/DavW/crypt.git
cd crypt
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

## Support

If you find this useful, then please consider supporting my work. You can do that by buying the music of [Bow Church](https://bowchurch.bandcamp.com)
or [Vitling](https://vitling.bandcamp.com); or listen and add to playlists on Spotify and/or SoundCloud.

You can also see my [website](https://www.vitling.xyz), [Instagram](https://instagram.com/vvitling) or [Twitter](https://twitter.com/vvitling) to follow
my latest work; and/or contact me to hire me for stuff.

## License

This plugin is free software, licensed under the [GNU General Public License v3.0](https://www.gnu.org/licenses/gpl-3.0.html). 

However, the JUCE framework that it depends on as a submodule has [its own license](https://github.com/juce-framework/JUCE/blob/master/LICENSE.md)
