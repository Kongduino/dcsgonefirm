# dcsgonefirm

# Notes

As this repo is still being worked on, some things in this Readme are marked 
with 'Dev:'. Feel free to change this if needed.

# Building this

You need ESP-IDF, any modern version should work. (Dev: Change this if you 
require a more specific version)

## ESP IDF version

It seems to be working well with 5.5.2 and 6.0.1. Someone I met at DEF CON SG 1 said it failed with anything above 5.5.2 and below 6.0.

Command-line:

	idf.py set-target esp32c6
	idf.py build
	idf.py flash

Or do the equivalent in your IDE.

# Structure

In general, large applications should be included as components in this repo. Smaller
things that don't warrant their own component can live in the main/ directory. If
you need data in the form of a file, feel free to dump it in the spiffs/ folder.

# Developer notes

Dev: Note that the sdkconfig is *not* included in this repo and should not be checked 
in (as it gets overwritten when you run 'idf.py set-target'). If you need something
changed, run menuconfig and use 'save minimal config', then copy ``build/defconfig``
to ``sdkconfig.defaults``.

If you want to change this code, please fork it first, make your changes, then
create a merge request in the main repo. If you get the OK from at least one other
team member, you can go ahead and merge it into main.

# Kongduino's notes

This fork includes a few changes.

## LaserTag

The manual firing timeout was lowered to 0.2 second – so you can pew pew as fast as you can press that button. But the real trick was adding auto-fire. It fires every 500 ms (down from originally 1 s), and does not show any animation. It just fires :-) I tested it during the last day only, and it worked like a charm. I even managed to tag Jeff Moss.

# Screen

I got the TFT from the Cryptocurrency Village. I met there a couple of hackers, one of whom, fsnaix on Discord, managed to create the display.c/.h files in record time, and later on make [Doom run on the badge](https://github.com/bloomifycafe/DefCon-SG01-DoomBadge)...

Instead, I just display a 320 x 240 logo, encoded as RGB565 in a header file, and display that.

# Text game

There's a (kinda lame) text-based game to get you to set up wifi and do FOTA. I added a help that lists the tokens (commands) on startup.
