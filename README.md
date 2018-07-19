# DDNES
This is a personal learning project for me that I don't expect to be especially useful to anyone else. As I have time, I'm implementing an NES emulator in C with SDL. I'm doing this through a combination of my own code, information from the guides at [nesdev.com](https://www.nesdev.com/), various bits and pieces of documentation all over the Web, and porting parts of (especially the PPU) [Michael Fogleman's excellent Go NES emulator](https://github.com/fogleman/nes). I'm not being a purist about this—I am looking at other people's code as I get stuck. Right now the 6502 CPU core seems to be working pretty well and the PPU (picture processing unit) can display basic backgrounds (although not quite correctly!).

[dk1!](dk1.png) [dk2!](dk2.png)

This will never be anything impressive, and it's not code I'm proud of... it's more of a hack as I have time. If you want to look at good code, please checkout Michael's project. I just plan to get Donkey Kong working and call it a day. I've learned a lot about the 6502 and NES architecture already and when I get Donkey Kong playing decently I will have achieved my learning objectives for this project.

## License

Licensed under the [Apache License](LICENSE).

Michael Fogleman's emulator that this is partially a port of (especially parts of the PPU) is licensed under the [MIT License](https://github.com/fogleman/nes/blob/master/LICENSE.md).

## To-Do
- [X] Memory Map
- [X] CPU Core
- [X] Basic PPU Background Rendering
- [ ] Correct Background Rendering
- [ ] Sprites
- [ ] Controller Support through keyboard
- [ ] Playable Donkey Kong
- [ ] APU (Audio Processing Unit)... maybe... eh would be pretty happy with the above

## Useful Links
- [Michael Fogleman's Go NES Emulator](https://github.com/fogleman/nes)
- [nesdev.com](https://www.nesdev.com/)
- [Nintaco](http://nintaco.com) - one of the only NES emulators with a good debugger that runs on macOS (Java)
- [Maciform](https://github.com/macifom/macifom) - an NES emulator with a very basic debugger written in Objective-C that you can build for the Mac natively (at least as of 10.13, although builds you'll find online don't work; you need to build it yourself)
