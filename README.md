# ElenixOS Simulator
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FElenixOS%2FElenixOS-Simulator.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2FElenixOS%2FElenixOS-Simulator?ref=badge_shield)


This repository is the dedicated simulator project for ElenixOS on desktop platforms.
It is pre-configured for VSCode and supports Windows, Linux, and macOS.
FreeRTOS is also included and can be optionally enabled to better simulate embedded system behavior.


## Get started
### Get the PC project

Clone the simulator project and related submodules:

```bash
git clone --recursive https://github.com/ElenixOS/ElenixOS-Simulator
```

### Install SDL and build tools

You can download SDL from https://www.libsdl.org/

#### Linux

Copy below in the Terminal:
For Ubuntu

```bash
sudo apt-get update && sudo apt-get install -y build-essential libsdl2-dev cmake
```

For ArchLinux

```bash
sudo pacman -Syu && sudo pacman -S sdl2 libsdl2-devel sdl2_mixer sdl2-devel base-devel gcc make
```

## Documentation

The usage guide has been moved to the ElenixOS documentation site.

Please read:
https://docs.elenixos.com/docs/simulator/overview

## WASM publish to GitHub Pages

This repository includes the workflow `.github/workflows/deploy-wasm-pages.yml`.
It builds the WASM target and publishes the output to GitHub Pages automatically on pushes to `main`.

After enabling Pages in repository settings (`Build and deployment -> Source: GitHub Actions`), artifacts are published to:

Important: make sure the repository Pages source is set to `GitHub Actions`, not `Deploy from a branch`. The branch-based Pages path runs the Jekyll build pipeline and can hide the WASM output.

- `https://<owner>.github.io/<repo>/wasm/latest/main.html`
- `https://<owner>.github.io/<repo>/wasm/latest/main.js`
- `https://<owner>.github.io/<repo>/wasm/latest/main.wasm`
- `https://<owner>.github.io/<repo>/wasm/latest/main.data`

The workflow also keeps a commit-pinned copy under:

- `https://<owner>.github.io/<repo>/wasm/<sha7>/...`

Use `latest` for always-updated preview links, and use `<sha7>` for stable references in external pages.

### How to use in another page

If your page is hosted under the same GitHub Pages site, you can link directly:

```html
<a href="/ElenixOS-Simulator/wasm/latest/main.html" target="_blank" rel="noopener">Open ElenixOS Simulator</a>
```

Or embed it in an iframe:

```html
<iframe
	src="/ElenixOS-Simulator/wasm/latest/main.html"
	width="100%"
	height="720"
	style="border:0"
	loading="lazy"
	referrerpolicy="no-referrer"
></iframe>
```

If your page is hosted elsewhere, replace with the full absolute URL:

`https://<owner>.github.io/<repo>/wasm/latest/main.html`

## Optional library

There are also FreeType and FFmpeg support. You can install these according to the followings:

### Linux

```bash
# FreeType support
wget https://kumisystems.dl.sourceforge.net/project/freetype/freetype2/2.13.2/freetype-2.13.2.tar.xz
tar -xf freetype-2.13.2.tar.xz
cd freetype-2.13.2
make
make install
```

```bash
# FFmpeg support
git clone https://git.ffmpeg.org/ffmpeg.git ffmpeg
cd ffmpeg
git checkout release/6.0
./configure --disable-all --disable-autodetect --disable-podpages --disable-asm --enable-avcodec --enable-avformat --enable-decoders --enable-encoders --enable-demuxers --enable-parsers --enable-protocol='file' --enable-swscale --enable-zlib
make
sudo make install
```
### (RT)OS support
Works with any OS like pthred, Windows, FreeRTOS, etc. It has build in support for FreeRTOS.

## Test
This project is configured for [VSCode](https://code.visualstudio.com) and is tested on:
- Ubuntu Linux
- Windows WSL (Ubuntu Linux)

It requires a working version of GCC, GDB and make in your path.

To allow debugging inside VSCode you will also require a GDB [extension](https://marketplace.visualstudio.com/items?itemName=webfreak.debug) or other suitable debugger. All the requirements, build and debug settings have been pre-configured in the [.workspace](simulator.code-workspace) file.

The project can use **SDL** but it can be easily relaced by any other built-in LVGL dirvers.


## License
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FElenixOS%2FElenixOS-Simulator.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2FElenixOS%2FElenixOS-Simulator?ref=badge_large)