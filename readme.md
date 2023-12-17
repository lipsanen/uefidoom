# Description
Doom as an UEFI application. I adapted this from https://github.com/ozkl/doomgeneric but transformed all the standard library calls to be prefixed by d_ and implemented replacements for them. Most of the implementations I either wrote myself or copy-pasted from musl. The printf implementation is from https://github.com/mpaland/printf although I modified it a bit to also have error handling. The doom1.wad itself is embedded into the executable and the data is from the shareware version. I grabbed the EFI binaries from GNU-EFI but slightly tinkered with them to get them to work with clang and my way of building an UEFI executable. The efidoom directory also contains some other small examples for UEFI applications.

# Building and running in QEMU
Requirements:
- QEMU
- lld
- clang
```
mkdir build_efi
cd build_efi
cmake -DUEFIDOOM=ON ..
cd ..
(cd build_efi && make -j) && ./run.bash
```
With -DUEFIDOOM=OFF the game builds two linux versions and some other crap. doom_embedded is a test version for linux that also embeds the doom wad into the executable.

# Controls
Now these are weird. I haven't implemented many of the binds, but the game works either in mouse mode or keyboard mode. The reason for this is that from the UEFI interface I can only get keystroke events so I don't get key release events. Therefore in keyboard mode, all keypresses are treated as toggles. This is a little bit annoying, so if mouse movement is detected the game moves into mouse mode, where keystrokes are treated as individual presses. However you can get mouse1/mouse2 release events from UEFI, so these work held down. The binds are:
- Mouse1/Up arrow - forward
- Mouse2/Down arrow - back
- Mouse - look left / right
- Left/right arrow - looking left/right
- Space - use
- Tab - fire
