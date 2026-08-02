# SpaceShoot

SpaceShoot is a small arcade game made for Arduino UNO R4 and ILI9341 TFT Touch Display. The game starts simple but slowly becomes harder as your score increase The main goal is to avoid asteroids, collect hearts to recover lives and survive as long as possible while trying to beat your highest score.I made this project because I wanted to see how much game experience I could create on a small Arduino setup. At first it was just a simple idea, but slowly I kept adding new things and making it more fun. While building this project I learned a lot about touch input, game logic and EEPROM storage and graphics on embedded systems. There are still many things I want to improve, but for now I am really happy with how it turned out.

## How to Play

- Tap the screen to start the game.
- Touch left side of the screen to move left.
- Touch right side to move right.
- Avoid asteroids because every hit will cost one life.
- Hearts appear after every 350 score and can recover your health.
- Every 100 score the game changes its color theme and also becomes little faster.
- Try to survive for as long as possible and make a new high score.

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
