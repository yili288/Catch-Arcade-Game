# Catching-Arcade-Game

The objective is to catch falling boxes with a controllable basket. As the game progresses, boxes will fall faster and more frequently.

Control the basket using left and right arrow keys.
Start, pause and restart the game using the left most push button

Technology: C, GitHub, Sublime Text.

How it works:
Written in C code for the DE1-SoC Computer System with ARM Cortex-A9. This game makes use of PS/2 keyboard, interrupt request (IRQ) exception, VGA Monitor with double buffering and character buffering to work. Also works on the ARMV7 DE1-SoC simulator https://cpulator.01xz.net/?sys=arm-de1soc.

Objective: Try to catch as many boxes with the basket as possible. If you miss one, you lose.

Setup:
1. Go to https://cpulator.01xz.net/?sys=arm-de1soc .
2. Ensure that the language in the editor is set to C.
3. Open main.c .
4. Click “compile and load”.
5. Go to the VGA pixel buffer on the right side and click “show in a separate box”.
6. Change the zoom level on the VGA pixel buffer to 1.0 and resize the VGA pixel buffer
window so that the whole VGA pixel buffer is visible.
7. Move this window to the left of the screen.
8. On the right tab, scroll down to the first “PS/2 keyboard or mouse” window.

![me] (https://github.com/yili288/Catch-Arcade-Game/blob/main/Game-demo%20(1).gif =250x250)

Game play:
1. Click “continue”.
2. Go to the “Push buttons” section and press KEY0 to start the game. The VGA pixel
buffer should be cleared black, and animals should begin to fall.
3. At the “type here” entry in the first “PS/2 keyboard or mouse” window, press the left
and right arrow keys to move the basket
4. Catch the boxes with the basket to gain points.
5. Press KEY0 to pause the game.
6. The game ends when you run out of lives.
