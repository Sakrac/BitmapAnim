# Bitmap Anim Tool #
A simple tool to create bitmap animation data for the Commodore multi-color bitmap screen mode. Originally used for the Vovve demo.

## Building ##

The tools should build without issue using Visual Studio 2022.

## Command Line ##

bitmapanim.exe \<image.png> -bg=\<background color> -out=\<output file>

## Basic Usage ##

The tool expects animations in the form of full screen c64 bitmap images with the .png extension and numbered, for instance

 * frame1.png
 * frame2.png
 * frame3.png

frame1.png in this case is the screen before the animation starts and it will generate data for transforming the image from frame1.png to frame2.png, and from frame2.png to frame3.png.

This allows for creating series of animations that can be played back and it gets a little easier to insert or remove animations into the full sequence.

## Output ##
The output is generated to be assembled with x65, my own 65xx assembler. The tool can easily be modified to generate data for other syntaxes if desired.

Also note that the data is hardcoded for a screen address of $4000 which can also be fixed in the tool source code.

Each file begins with the number of frames in the animation:
```
; stripes generated from frame
	dc.b 10 ; number of frames in this anim
```
Then for each frame a number of stripes is output.
```
	 ; frame 1
	dc.b 5 ; this frame has 5 stripes
```
A "stripe" is just a bunch of characters in sequence on the bitmap screen and looks like this
```
	dc.b 1 ; stripe 0 is 1 characters
	dc.w $41d1 ; screen addr = $41d1 (25, 11)
	dc.b $f3 ; screen data
	dc.b $04 ; color data
	dc.w $6e90 ; bitmap addr = $6e90 (26, 11)
	dc.b $00, $00, $00, $00, $00, $00, $00, $40 ; char 0
```

## Example Playback ##

Look in the 6502 folder for an example implementation for drawing animation frames. Note that the initial image is not provided by the system.

## Notes ##

As should be obvious the more characters that change on-screen the bigger the data, but also requires more cycles to draw.

Since multiple vblanks probably occur between updates it might be beneficial to implement a double buffer method to animate more of the screen.