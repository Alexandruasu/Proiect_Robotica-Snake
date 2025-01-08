
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Arduino.h>

#define _sclk 13
#define _miso 12
#define _mosi 11
#define _cs   10
#define _dc    9
#define _rst   8

#define buzzer 4

#define JOYSTICK_X A0
#define JOYSTICK_Y A1
#define JOYSTICK_BUTTON 2

Adafruit_MPU6050 mpu;
Adafruit_ILI9341 tft = Adafruit_ILI9341(_cs, _dc, _rst);

int difficulty = 0;
int lives = 3;
byte heart[8] = 
{
    0b00000,
    0b01010,
    0b11111,
    0b11111,
    0b11111,
    0b01110,
    0b00100,
    0b00000
};
void drawHeart(int x, int y, uint16_t color, int scale) {
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 5; col++) {
      // Check if the bit is set in the heart array
      if (heart[row] & (1 << (4 - col))) {
        // Draw a scaled pixel (3x3 rectangle)
        tft.fillRect(x + col * scale, y + row * scale, scale, scale, color);
      }
    }
  }
}
enum Difficulty {
    EASY,
    MEDIUM,
    HARD
};

class LCD {
public:
    LCD(int cs, int dc, int rst) : tft(cs, dc, rst) {}
    void initLCD() {
        tft.begin();
        tft.setRotation(3);
        tft.fillScreen(ILI9341_BLACK);
    }
    void clearScreen() { tft.fillScreen(ILI9341_BLACK); }
    Adafruit_ILI9341& getTft() { return tft; }
private:
    Adafruit_ILI9341 tft;
};

class Joystick {
public:
    Joystick(int pinX, int pinY, int buttonPin) : _pinX(pinX), _pinY(pinY), _buttonPin(buttonPin) {}
    void begin() {
        pinMode(_pinX, INPUT);
        pinMode(_pinY, INPUT);
        pinMode(_buttonPin, INPUT_PULLUP);
    }
    int readX() { return analogRead(_pinX); }
    int readY() { return analogRead(_pinY); }
    bool isButtonPressed() { return digitalRead(_buttonPin) == LOW; }
private:
    int _pinX, _pinY, _buttonPin;
};

class Gyro {
public:
    Gyro() {}
    bool initGyro() {
        if (!mpu.begin()) {
            return false;
        }
        mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
        return true;
    }
    void readAccel(float &x, float &y, float &z) {
        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);
        x = a.acceleration.x;
        // Subtract 9.75 to account for gravity because the accelerometer reads 9.75 m/s^2 when stationary
        // WARNING: This value may vary depending on the orientation of the sensor
        // x -= 9.75;
        y = a.acceleration.y;
        z = a.acceleration.z;
    }
    void readGyro(float &x, float &y, float &z) {
        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);
        x = g.gyro.x;
        y = g.gyro.y;
        z = g.gyro.z;
    }
};

class Menu {
public:
    Menu() : currentOption(0), selectedOption(-1) {}
    void setDisplay(Adafruit_ILI9341 &display) { tft = &display; }
    void displayMenu() {
        // tft->fillScreen(ILI9341_BLACK);
        for (int i = 0; i < 4; i++) {
            tft->setCursor(0, i * 20);
            if (i == currentOption) {
                tft->setTextColor(ILI9341_RED, ILI9341_BLACK);
            } else {
                tft->setTextColor(ILI9341_WHITE, ILI9341_BLACK);
            }
            tft->setTextSize(2);
            if (i == 0) tft->println("1. Play");
            if (i == 1) tft->println("2. Difficulty");
            if (i == 2) tft->println("3. Debug");
            if (i == 3) tft->println("4. Exit");
        }
    }
    void navigate(int direction) {
        currentOption += direction;
        if (currentOption < 0) currentOption = 3;
        if (currentOption > 3) currentOption = 0;
        displayMenu();
    }
    void selectOption() {
        selectedOption = currentOption;
    }
    int getCurrentOption() { return currentOption; }
    int getSelectedOption() { return selectedOption; }
    void setSelectedOption(int option) { selectedOption = option; }
    void displayGameOver() {
        tft->fillScreen(ILI9341_BLACK);
        tft->setCursor(0, 0);
        tft->setTextColor(ILI9341_RED);
        tft->setTextSize(2);
        tft->println("Game Over");
    }
private:
    Adafruit_ILI9341 *tft;
    int currentOption;
    int selectedOption;
};

class Snake {
public:
    Snake() : score(0), length(5), prevLength(5), direction(0), fruitX(0), fruitY(0) {}
    void setDisplay(Adafruit_ILI9341 &display) { tft = &display; }
    int getDirection() { return direction; }
    int getLength() { return length; }
    int getScore() { return score; }
    void reset(int _score = 0) {
        length = 5;
        score = _score;
        direction = 0;
        for (int i = 0; i < length; ++i) {
            bodyX[i] = 10 - i;
            bodyY[i] = 10;
        }
        placeFruit();
    }
    void move(int newDirection) {
        if ((newDirection == 0 && direction != 2) || (newDirection == 2 && direction != 0) ||
            (newDirection == 1 && direction != 3) || (newDirection == 3 && direction != 1)) {
            direction = newDirection;
        }
        
        // Store last tail coordinates
        lastX = bodyX[length - 1];
        lastY = bodyY[length - 1];

        int prevX = bodyX[0];
        int prevY = bodyY[0];
        int prev2X, prev2Y;
        bodyX[0] += (direction == 0) - (direction == 2); // Right or Left
        bodyY[0] += (direction == 1) - (direction == 3); // Down or Up
        for (int i = 1; i < length; ++i) {
            prev2X = bodyX[i];
            prev2Y = bodyY[i];
            bodyX[i] = prevX;
            bodyY[i] = prevY;
            prevX = prev2X;
            prevY = prev2Y;
        }
    }
    bool checkCollision() {
        if (bodyX[0] < 0 || bodyX[0] >= tft->width() / 10 || bodyY[0] < 0 || bodyY[0] >= tft->height() / 10) {
            return true;
        }
        for (int i = 1; i < length; ++i) {
            if (bodyX[0] == bodyX[i] && bodyY[0] == bodyY[i]) {
                return true;
            }
        }
        return false;
    }
    void update() {
        // tft->fillRect(0, 0, tft->width(), tft->height(), ILI9341_BLACK); // Clear screen
        tft->fillRect(lastX * 10, lastY * 10, 10, 10, ILI9341_BLACK);
        for (int i = 0; i < length; ++i) {
            tft->fillRect(bodyX[i] * 10, bodyY[i] * 10, 10, 10, ILI9341_GREEN);
        }
        tft->fillRect(fruitX * 10, fruitY * 10, 10, 10, ILI9341_RED);

        // Draw border
        tft->drawRect(0, 0, tft->width(), tft->height(), ILI9341_WHITE);

        if (length != prevLength) {
            // Draw score
            tft->setCursor(0, tft->height() - 30);
            tft->setTextColor(ILI9341_WHITE, ILI9341_BLACK); // Set background color to black to overwrite previous text
            tft->setTextSize(3);
            tft->print("Score: ");
            tft->println(score);

            // Draw lives
            tft->setCursor(tft->width() - 80, tft->height() - 30);
            for (int i = 0; i < lives; i++) {
                drawHeart(tft->width() - 80 + i * 25, tft->height() - 30, ILI9341_RED, 3);
            }
            // tft->print(lives);

            // Update prevLength
            prevLength = length;
        }
    }
    void placeFruit() {
        fruitX = random(0, tft->width() / 10);
        fruitY = random(0, tft->height() / 10);
    }
    bool checkFruitCollision() {
        if (bodyX[0] == fruitX && bodyY[0] == fruitY) {
            length++;
            score++;
            placeFruit();
            return true;
        }
        return false;
    }
private:
    int bodyX[100], bodyY[100];
    int lastX, lastY;
    int score, length, prevLength, direction;
    int fruitX, fruitY;
    Adafruit_ILI9341 *tft;
};

LCD lcd(_cs, _dc, _rst);
Joystick joystick(JOYSTICK_X, JOYSTICK_Y, JOYSTICK_BUTTON);
Gyro gyro;
Menu menu;
Snake snake;

void setup() {
    tone(buzzer, 1000, 200);
    lcd.initLCD();
    gyro.initGyro();
    joystick.begin();
    menu.setDisplay(lcd.getTft());
    snake.setDisplay(lcd.getTft());
    menu.displayMenu();
}

void loop() {
    int joystickY = joystick.readX();
    bool buttonPressed = joystick.isButtonPressed();
    if (buttonPressed) {
        menu.selectOption();
    } else if (joystickY < 100) {
        menu.navigate(-1);
    } else if (joystickY > 900) {
        menu.navigate(1);
    }
    
    int selectedOption = menu.getSelectedOption();
    if (selectedOption == 0) { // Play
        lcd.clearScreen();
        snake.reset();

        switch (difficulty) {
            case EASY:
                lives = 3;
                break;
            case MEDIUM:
                lives = 2;
                break;
            case HARD:
                lives = 1;
                break;
        }

        // Draw score
        tft = lcd.getTft();
        tft.setCursor(0, tft.height() - 30);
        tft.setTextColor(ILI9341_WHITE);
        tft.setTextSize(3);
        tft.print("Score: ");
        tft.print(snake.getScore());

        // Draw lives
        tft.setCursor(tft.width() - 80, tft.height() - 30);
        for (int i = 0; i < lives; i++) {
            drawHeart(tft.width() - 80 + i * 25, tft.height() - 30, ILI9341_RED, 3);
        }

        while (true) {
            float x, y, z;
            gyro.readAccel(x, y, z);
            int direction = snake.getDirection();
            
            float threshold = 2;
            if (x > threshold) direction = 0; // Right
            else if (x < -threshold) direction = 2; // Left
            else if (y > threshold) direction = 3; // Up
            else if (y < -threshold) direction = 1; // Down
            else {
                // Read joystick input
                int joystickX = joystick.readX();
                int joystickY = joystick.readY();
                
                if (joystickX < 100) direction = 3; // Up
                else if (joystickX > 900) direction = 1; // Down
                else if (joystickY < 100) direction = 0; // Right
                else if (joystickY > 900) direction = 2; // Left
            }

            snake.move(direction);
            if (snake.checkCollision()) {
                lives--;
                if (lives > 0) {
                    lcd.clearScreen();
                    snake.reset(snake.getScore());

                    // Draw score
                    tft.setCursor(0, tft.height() - 30);
                    tft.setTextColor(ILI9341_WHITE);
                    tft.setTextSize(3);
                    tft.print("Score: ");
                    tft.println(snake.getScore());
                    
                    // Draw lives
                    tft.setCursor(tft.width() - 80, tft.height() - 30);
                    for (int i = 0; i < lives; i++) {
                        drawHeart(tft.width() - 80 + i * 25, tft.height() - 30, ILI9341_RED, 3);
                    }
                    
                    continue;
                }
                menu.displayGameOver();
                tone(buzzer, 500, 200);
                delay(200);
                tone(buzzer, 400, 200);
                delay(200);
                tone(buzzer, 300, 200);
                delay(200);
                noTone(buzzer);
                menu.setSelectedOption(-1);
                delay(1500);
                lcd.clearScreen();
                menu.displayMenu();
                break;
            }
            if (snake.checkFruitCollision()) {
                tone(buzzer, 1000, 100);
            }
            snake.update();

            switch (difficulty) {
                case EASY:
                    delay(400);
                    break;
                case MEDIUM:
                    delay(300);
                    break;
                case HARD:
                    delay(200);
                    break;
            }
        }
    } else if (selectedOption == 1) { // Difficulty
        // Change difficulty
        menu.setSelectedOption(-1);
        lcd.clearScreen();
        tft = lcd.getTft();
        const char* difficulties[] = {"Easy", "Medium", "Hard"};
        int difficultyOption = 0;

        while (true) {
            // tft.fillScreen(ILI9341_BLACK);
            for (int i = 0; i < 3; i++) {
                tft.setCursor(0, i * 20);
                if (i == difficultyOption) {
                    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
                } else {
                    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
                }
                tft.setTextSize(2);
                tft.println(difficulties[i]);
            }

            int joystickY = joystick.readX();
            if (joystickY < 100) {
                difficultyOption--;
                if (difficultyOption < 0) difficultyOption = 2;
            } else if (joystickY > 900) {
                difficultyOption++;
                if (difficultyOption > 2) difficultyOption = 0;
            }

            if (joystick.isButtonPressed()) {
                difficulty = difficultyOption;
                switch (difficulty) {
                    case EASY:
                        lives = 3;
                        break;
                    case MEDIUM:
                        lives = 2;
                        break;
                    case HARD:
                        lives = 1;
                        break;
                }
                break;
            }

            delay(200);
        }

        lcd.clearScreen();
        menu.displayMenu();
    } else if (selectedOption == 2) { // Debug
        // Display accelerometer and gyroscope data on screen until joystick button is pressed
        tft = lcd.getTft();
        while (true) {
            float accelX, accelY, accelZ;
            float gyroX, gyroY, gyroZ;
            gyro.readAccel(accelX, accelY, accelZ);
            gyro.readGyro(gyroX, gyroY, gyroZ);
            
            tft.setCursor(0, 0);
            tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); // Set background color to black to overwrite previous text
            tft.setTextSize(2);
            tft.print("Accel X: ");
            tft.println(accelX);
            tft.print("Accel Y: ");
            tft.println(accelY);
            tft.print("Accel Z: ");
            tft.println(accelZ);
            tft.print("Gyro X: ");
            tft.println(gyroX);
            tft.print("Gyro Y: ");
            tft.println(gyroY);
            tft.print("Gyro Z: ");
            tft.println(gyroZ);

            int direction = -1;
            float threshold = 2;
            if (accelX > threshold) direction = 0; // Right
            else if (accelX < -threshold) direction = 2; // Left
            else if (accelY > threshold) direction = 1; // Down
            else if (accelY < -threshold) direction = 3; // Up

            switch (direction) {
                case 0:
                    tft.println("Right  ");
                    break;
                case 1:
                    tft.println("Down   ");
                    break;
                case 2:
                    tft.println("Left   ");
                    break;
                case 3:
                    tft.println("Up     ");
                    break;
                default:
                    tft.println("Nothing");
                    break;
            }
            
            if (joystick.isButtonPressed()) {
                menu.setSelectedOption(-1);
                lcd.clearScreen();
                menu.displayMenu();
                break;
            }

            delay(200);
        }
    }
    else if (selectedOption == 3) { // Exit
        lcd.clearScreen();
    }
}
