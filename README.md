
# Crypt

**Crypt** is a software synthesiser plugin designed for creating spacious cold hyper-unisoned
synth sounds; developed by [Vitling](https://www.vitling.xyz) for the [Bow Church](http://bowchurch.bandcamp.com/) project.

![V2 Screenshot](https://www.vitling.xyz/crypt/resources/crypt-screenshot.jpg)

It is written in C++20 and depends on the [JUCE](https://github.com/juce-framework/JUCE) framework, which is
included as a submodule.

## Installation

### Mac & Windows

Go to the [Crypt plugin download page](https://www.vitling.xyz/crypt) for conveniently packaged donwloads for **Mac** and **Windows**

### Linux

Building on Linux should be fairly straightforward, but I haven't done much testing on that front myself. 

#### Dependencies
```cmake g++ libfreetype6-dev libx11-dev libxinerama-dev libxrandr-dev libxcursor-dev mesa-common-dev libasound2-dev freeglut3-dev libxcomposite-dev pkg-config```

These are Debian/Ubuntu package names (install with `sudo apt-get install ` and paste the above), you may need to translate for your distro

#### Build

```bash
git clone --recursive --shallow-submodules https://github.com/vitling/crypt.git
cd crypt
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release  # For MacOS universal binary add "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64"
cmake --build build --parallel
```

## Contribute

This plugin is published open source primarily so that (a) Linux users are able to build from source and (b) people can learn from what I have learned and created. This is not intended to be a project that gets features added over time ad infinitum.

As such, in general, I will NOT accept Pull Requests which add new features to the plugin. Feel free to make suggestions, especially if you can back it up with a solid use case, but I make no promises.

I will, in general, accept pull requests which fix bugs or allow a broader adoption of the plugin, for example adjustments to make it build for more platforms.

If you want to take components of Crypt and do something new with them, then that is your right under the [GPL3 license](https://www.gnu.org/licenses/gpl-3.0.html), you may take code from this for your own plugin, but if you do then legally that plugin must also be published under the GPL3 license.

I kindly request (but cannot legally enforce) that you do not use the name or branding if you create a new plugin based on parts of Crypt.

## Support

If you find this useful, then please consider supporting me. This project took a lot of serious work that nobody was paying me to do.
[I accept monetary tips](https://ko-fi.com/vitling) and [Github Sponsors](https://github.com/sponsors/vitling)

You can also buy the music of [Bow Church](https://bowchurch.bandcamp.com)
or [Vitling](https://vitling.bandcamp.com); or listen and add to playlists on Spotify and/or SoundCloud.

You can also see my [website](https://www.vitling.xyz) for my latest work; and/or contact me to hire me for stuff.

## License

This plugin is free software, licensed under the [GNU General Public License v3.0](https://www.gnu.org/licenses/gpl-3.0.html). 

