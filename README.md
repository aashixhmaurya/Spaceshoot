# Spaceshoot

a space themed endless arcade game for the arduino uno r4 with an ili9341 tft touch display. play as bean a character inspired from stardance, and move left or right to dodge incoming asteroids and survive as long as possible

collect hearts to recover lives, keep an eye on your score as the game gets harder and try to beat your highest score. the highest score is saved in eeprom so it stays even after restarting the game.

## How to Play

- Tap the screen to start the game
- Touch left side of the screen to move left
- Touch right side to move right
- Avoid asteroids because every hit will cost one life
- Hearts appear after every 350 score and can recover your health
- Every 100 score the game changes its color theme and also becomes little faster
- Try to survive for as long as possible and make a new high score

## Hardware Used

- Arduino UNO R4
- ILI9341 TFT Touch Display

## Features

- Touch based controls
- Auto difficulty increase with score
- Theme colors change every 100 score
- Heart pickup system after every 350 score
- High score saved in EEPROM
- Random asteroid spawning
- Simple home screen and game over screen
