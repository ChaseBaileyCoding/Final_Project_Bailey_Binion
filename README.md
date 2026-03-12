Team Members: Aaron Binion, Chase Bailey 


# Summary

   Our project, The Safe Safe, aims to safely and portably lock items in a drawer-like safe with two factor authentication and security lockouts. 
Hardware includes a servo motor, a green and red LED, a ESP32-S3 microcontroller, breadboard, LCD screen with contrast potentiometer, a keypad, three combination potentiometers, 
resistors, two battery packs, and a buzzer. The LCD shows “Passcode: ”, with characters, up to 6, being able to be inputted from the keypad after the colon. 
When the safe is locked, a code can be inputted, entered with #, and cleared with *. After the correct code, correct potentiometer positions, and # is pressed, the green LED turns on, 
red LED turns off, “OPEN” is displayed, and the safe can be pulled from its drawer. 
The safe can then be locked by inserting the safe into the drawer and pressing “ #” : the red LED being on and the screen showing “Passcode: ”, respectively. 
Each failed unlock attempt inhibits a buzzer to sound and a lockout timer to be displayed, starting with 1 second until the third fail, which is then 60 seconds. 
The timer should double its length after subsequent failed attempts, and the user cannot alter any outputs of the safe while the LCD shows a timer counting down. 



# Specification

Successfully unlocking the safe: 
- Keypad (KP) presses updates the LCD screen
- Input the correct passcode (3C3218, code max is 6 characters)
- Passcode is visible on the LCD screen 
- Press # to enter code and * to clear the passcode 
- LCD screen goes from displaying “Passcode: 3C3218” to “OPENING”, to “OPEN” after correct code is entered
- Have the potentiometers (P1, P2, P3) in the correct position (bottom left, middle, bottom right)
- Servo motor turns and unlocks the safe 
- Green LED turns on & red LED turns off
- Safe can be pulled out of drawer
  
Test Process

Inputs: KP, P1, P2, P3
1. Type code onto LDC with KP
2. Enter correct code, then press # with P1, P2, and P3 in the correct positions
3. Press * when there is a passcode on the LCD display

Results

1. LCD screen updates with inputted code from the keypad, red LED is on, green LED is off
2. The LCD screen goes from “Passcode: 3C3218” to “OPENING”, to “OPEN”, red LED turns off and green LED turns on. Servo motor turns 90 degrees clockwise,
   safe is able to be pulled from its drawer
3. The LCD display cleared the code and displayed “Passcode:”

Specification

Locking the safe: 
- Press # to lock the safe when currently unlocked
- Servo motor turns and locks the safe 
- Red LED turns on, green LED turns on 
- LCD displays “CLOSING”, then “Passcode:” and a passcode is able to be inputted 
- Safe should not be able to be pulled from its drawer

Test Process

Input: KP
1. Press # when safe is reinserted into the drawer and currently unlocked

Results

1. The servo motor turns 90 degrees counter-clockwise to lock the safe. LCD displays “CLOSING” then “Passcode:”. The green LED turns off and the red LED turns on,
and a new passcode is able to be typed in. The safe is locked and cannot be pulled from its drawer

Specification

Security functions: 
Limited attempts and lockout feature (
- After an incorrect code or potentiometer positions
- The LCD shows“Timer: _” with _ being the delay before subsequent entry attempts in seconds
- The delay should be “1” until the third failed attempt, where a delay of “60” is implemented
- Each failed attempt after three doubles the “60” delay
- With each failed attempt a buzzer will sound
- The inputted code stays on the screen after failed attempts and lockout, and the failed attempts counter resets after the safe is successfully unlocked
- While the timer is displayed, no user inputs alter any outputs of the safe or unlock it)

Test Process 

Input: KP
1. "#" is pressed with an incorrect code and / or incorrect potentiometer positions once, twice, three, and four times
2. An incorrect code is imputed twice, the safe is opened, then closed, then an incorrect code is attempted again
3. The display shows “Timer: _”, with _ being an integer timer in seonds

Results

1. A buzzer sounds for each failed attempt. For the first two attempts, “Timer: 1” is displayed, on the third “Timer: 60” is displayed, and on the fourth “Timer: 120” is displayed.
The delay integer decrements every second on the LCD, and reverts back to displaying “Passcode: ” with the previously entered code
2. The LCD screen displays “Timer: 1”, proving that the number of failed attempts resets after a successful attempt and relocking of the safe
3. No user inputs can alter the LCD display, buzzer, motor, or LEDs


