# waveforest - A node-based modular (FM) synthesizer 
<img width="1906" height="649" alt="image" src="https://github.com/user-attachments/assets/4dd7f15c-9281-40be-80cb-c4296cb5e0a6" />

If you're doing music / audio stuff on Linux, you have access to a plethora of fantastic free and open source plugins and effects.
However, one department that I felt was a little bit lacking was frequency modulation (FM) synthesis.
For the more common type, subtractive synthesis, there are many options available such as the popular SurgeXT synthesizer.
But on the FM side, I felt that each of the options didn't quite do everything the way I wanted them to.
Don't get me wrong, synths like Dexed and Octasine do a very good job of emulating the 4 or 6 operator hardware FM synths, but that in itself was the issue for me.
Since we're doing everything on the computer, why should we be bound by the limited number of operators found on those synths?

So I set out to create my ideal FM synth, one where you could have any number of operators connected in any way you want.
I had many ideas for the user interface, but then it hit me.
What better way is there to represent anything connecting to anything that a node editor!

Basically I had arrived at modular synthesis through a very convoluted route :).
What we have here is basically a minimal modular synthesizer with an expandable architecture where you can write the nodes as plugins and even share them around without requiring recompilation of waveforest itself.

[TOC]

## Dependencies
- Linux
- raylib
- JACK audio kit
- LV2 sdk (if compiling the companion plugin)

## Features

The synth runs as a standalone desktop app.
It runs as a JACK client, with a MIDI input and an audio output.

### Freedom
In waveforest, you can do anything you want.
Any output from any node is able to be connected to any input in another node.
Loops are also possible but do not produce very pleasant sounds.

### Keyboard focused workflow
Most of the actions are done through keyboard shortcuts, with fuzzy find for adding nodes and loading patches.

### Expandable architecture
The nodes are loaded as dynamic libraries and there is an API through which you can build more nodes in a very straight-forward way.
The API follows a structure that is probably familiar if you have used other audio APIs such as JACK, LV2 or VST.
Adding new nodes doesn't require recompilation of waveforest itself, as it loads the nodes from a directory where these node plugin binaries reside when they are needed.

Due to this fact waveforest has the potential to not only serve as a flexible FM synthesizer, but incorporate other types of synthesis as well.
It all depends on what kinds of nodes are implemented for it.
The currently implemented nodes mostly allow you to just do FM as I haven't (at the time of writing) implemented a filter node.
That is something I want to do at some point though, as I learn more about digital signal processing.

### LV2 companion plugin
There is an LV2 companion plugin available.
It doesn't include the UI for editing patches, so it's only meant for integrating a patch into a DAW project in a more permantent way than routing MIDI and audio through JACK.
The plugin features a file selector through which you can select the patch to load, which will persist between sessions.
I have tested the plugin in Ardour as that is what I use.

## Installation

### Standalone
Run in repository root:
```
# make install
```
The binary will be in `/usr/bin/waveforest`.
Resources, such as node plugins and fonts will be at `/usr/share/waveforest`.
### LV2
Run in repository root:
```
# make lv2_install
```
The binary and LV2 manifests will be in `/usr/lib/lv2/waveforest.lv2`.
Resources, such as node plugins and fonts will be at `/usr/share/waveforest`.

## Usage

After bootup, a message in the top left will tell you the most important keyboard shortcuts needed to use waveforest.
You can use the `h` key to bring it up again at any time.

You can hit the `space` key, after which you can fuzzily find a node that you're looking for. 
More on the different nodes and what they do later.

You can click and drag to edit node slider values.
If you hold down `shift`, you can edit the values precisely.
Holding down `ctrl` enables snapping to integers, holding down both `ctrl` and `shift` enables snapping to one tenths.

By clicking and dragging from the node title area you can move a node.
To delete a node, press the `Delete` key while hovering in that area.

In a node block, the sockets on the left side are input sockets and sockets on the right side are output sockets.
To connect two nodes drag the line from an input socket to an output socket or vice versa.
To disconnect a line simply click on the input socket it's connected to.

To save a patch use `ctrl+s`, to open one use `ctrl+o`. Pressing `ctrl+n` will make a new patch.

### Some important nodes
#### output
This will be in the patch as default.
Anything (e.g. an oscillator) connected to this will be forwarded to the audio output and consequently your speakers.

#### oscillator
The thing that makes sound.
You can select between a sine wave, a square wave, a saw wave and a triangle wave.

`frequency` is the ratio at which the oscillator will oscillate relative to the note that was played.
If left to one it will play at the exact frequency of the note, at 2.0 it will be an octave higher etc.

`phase` will shift the wave forwards or backwards in time.
Using one oscillator its effects are not audible but it's usually used to connect another oscillators output into in order to achieve FM synthesis.
Despite the name frequency modulation, you usually want to plug other oscillators into this one instead of the `frequency` ratio input as most FM synths actually work through phase modulation.

`feedback` will feed the output of the oscillator into the `phase` input, multiplied by the value in the `feedback` input

`volume` is the volume. Usually connected to an `envelope` node.

#### envelope
If an oscillator is connected to the output by itself, it will just keep playing for a while after the key has been released.
An envelope is what actually controls the volume of the note over time.
You usually want to connect the output of this to an `oscillator`s `volume` input.

The inputs allow you to define an ADSR envelope for the sound.
[More on ADSR envelopes.](https://en.wikipedia.org/wiki/Envelope_(music))

#### lfo 
An oscillator but slower.
Useful for vibrato etc.
The frequency input on this one this the actual Hz frequency of the oscillator.

#### velocity
Outputs the velocity of the keypress as a $0..1$ value.

#### add
Adds all the inputs together.
There are multiple variants depending on the input count.

#### multiply
Multiplies the inputs together.
Can be used in place of a voltage-controlled amplifier (VCA).

#### unit
Basically just passes through the input to the output.
Useful in situations where you want to control multiple values (for example `envelope` release times) through a single slider.


## Writing node plugins

Writing a new node is probably most easily done by making a new C source file under `nodes` in the project root.
As things like these are best explained through source code and examples, it's best to have a look at `nodes/unit.c` for a minimal annotated example of a node plugin.
Other built in plugins in the same folder can also serve as examples.

### Building node plugins
To build all node plugins in the `nodes` folder, simply run:
```
$ make nodes
```
After which the node plugin binaries will appear in `build/nodes`.

To install them in a location where the installed waveforest instance looks for them, you can instead run:
```
# make nodes_install
```

## Limitations
As waveforest was only meant as a "do anything" FM synth, that's what it currently does best.
Currently there isn't many modules to support other kinds of synthesis, so for an actual modular synthesizer it's probably better to go with something like VCV Rack or Cardinal.
