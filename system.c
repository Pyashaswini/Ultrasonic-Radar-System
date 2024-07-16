#include <stdio.h> // Include standard input/output library
#include <LPC17xx.h> // Include LPC17xx MCU header file
#include <math.h> // Include math library for mathematical functions
#include <string.h> // Include string library for string manipulation

#define TRIG (1 << 15) // Define TRIG pin as P0.15
#define ECHO (1 << 16) // Define ECHO pin as P0.16
#define THRESHOLD 20 // Define distance threshold for obstacle detection

int i, echoTime = 5000; // Declare variables for iteration and echo time, initialize echoTime
char clear[] = "Clear"; // Declare string for "Clear" message
char obstacle[] = "Obstacle"; // Declare string for "Obstacle" message
char frontStr[] = "Front "; // Declare string for "Front " message
char leftStr[] = "Left "; // Declare string for "Left " message
char rightStr[] = "Right "; // Declare string for "Right " message
int obstacleArray[25] = {0}; // Declare array to store obstacle information for each degree

// Function to introduce delay in microseconds using Timer0
void delayUS(unsigned int microseconds) {
    // Configure Timer0
    LPC_SC->PCLKSEL0 &= ~(0x3 << 2); // Select peripheral clock for Timer0
    LPC_TIM0->TCR = 0x02; // Reset Timer0
    LPC_TIM0->PR = 0; // Set Timer0 prescaler to 0
    LPC_TIM0->MR0 = microseconds - 1; // Set match register for specified microseconds
    LPC_TIM0->MCR = 0x01; // Interrupt on match
    LPC_TIM0->TCR = 0x01; // Enable Timer0
    // Wait for interrupt flag
    while ((LPC_TIM0->IR & 0x01) == 0); 
    LPC_TIM0->TCR = 0x00; // Disable Timer0
    LPC_TIM0->IR = 0x01; // Clear interrupt flag
}

// Function to introduce delay in milliseconds using Timer0
void delayMS(unsigned int milliseconds) {
    delayUS(milliseconds * 1000); // Call delayUS function with milliseconds converted to microseconds
}

// Function to initialize Timer0
void initTimer0(void) {
    // Configure Timer0 for distance measurement
    LPC_TIM0->CTCR = 0x0; // Select Timer mode
    LPC_TIM0->PR = 11999999; // Set prescaler value for Timer0
    LPC_TIM0->TCR = 0x02; // Reset Timer0
}

// Function to start Timer0
void startTimer0() {
    LPC_TIM0->TCR = 0x02; // Reset Timer0
    LPC_TIM0->TCR = 0x01; // Enable Timer0
}

// Function to stop Timer0 and return elapsed time
float stopTimer0() {
    LPC_TIM0->TCR = 0x0; // Disable Timer0
    return LPC_TIM0->TC; // Return Timer0 counter value
}
// Function to initialize Timer1
void inittimer1() {
    // Configure Timer1
    LPC_SC->PCLKSEL1 &= ~(0x3 << 2); // Select peripheral clock for Timer1
    LPC_TIM1->CTCR = 0x0; // Select Timer mode
    LPC_TIM1->TCR = 0x2; // Reset Timer1
    LPC_TIM1->PR = 0; // Set Timer1 prescaler to 0
}

// Function to introduce delay using Timer1
void delay(int milliseconds) {
    LPC_TIM1->TCR = 0x2; // Reset Timer1
    LPC_TIM1->TCR = 0x1; // Enable Timer1
    while (LPC_TIM1->TC < milliseconds); // Wait until Timer1 reaches specified milliseconds
    LPC_TIM1->TCR = 0x0; // Disable Timer1
}

// Function to measure distance using ultrasonic sensor
float getDistance(){
    LPC_GPIO0->FIOSET = 0x00000800; // Set TRIG pin high
    delayUS(10); // Wait for 10 microseconds
    LPC_GPIO0->FIOCLR |= TRIG; // Set TRIG pin low
    while (!(LPC_GPIO0->FIOPIN & ECHO)) { // Wait for a HIGH on ECHO pin
    }
    startTimer0(); // Start Timer0
    while (LPC_GPIO0->FIOPIN & ECHO) { // Wait for a LOW on ECHO pin
    }
    echoTime = stopTimer0(); // Stop Timer0 and get elapsed time
    return (0.0343 * echoTime) / 40; // Calculate and return distance based on time
}

// Function to rotate the robot clockwise
void clockwise() {
    int v1;
    v1 = 0x8;
    for(i = 0; i < 4; i++) {
        v1 = v1 << 1;
        LPC_GPIO0->FIOPIN = v1; // Set GPIO pins to rotate clockwise
        delay(1000000); // Delay for rotation
    }
}

// Function to rotate the robot anti-clockwise
void anti_clockwise() {
    int v1;
    v1 = 0x100;
    for(i = 0; i < 4; i++) {
        v1 = v1 >> 1;
        LPC_GPIO0->FIOPIN = v1; // Set GPIO pins to rotate anti-clockwise
        delay(1000000); // Delay for rotation
    }
}

// Function to clear GPIO ports
void clear_ports(void){
    LPC_GPIO0->FIOCLR = 0x3F<<23; // Clear GPIO ports
}

// Function to write data to LCD
void lcdWrite(unsigned int data, int flag) {
    clear_ports(); // Clear GPIO ports
    LPC_GPIO0->FIOPIN = data<<23; // Write data to GPIO pins
    if(flag==0)
        LPC_GPIO0->FIOCLR = 1<<27; // Clear RS pin
    else
        LPC_GPIO0->FIOSET = 1<<27; // Set RS pin
    LPC_GPIO0->FIOSET = 1<<28; // Set EN pin
    delayUS(100); // Delay
    LPC_GPIO0->FIOCLR = 1<<28; // Clear EN pin
    return;
}

// Function to send command to LCD
void lcdCom(unsigned int data, int flag){
    int temp;
    temp = (data&0xF0)>>4; // Extract upper nibble
    lcdWrite(temp,flag); // Write upper nibble to LCD
    delayUS(1000); // Delay
    temp = (data&0xF); // Extract lower nibble
    lcdWrite(temp,flag); // Write lower nibble to LCD
    delayUS(1000); // Delay
}

// Function to initialize LCD
void lcdInit(){
    int i;
    int array[] = {0x33,0x32,0x28,0x0c,0x06,0x01}; // Array of commands to initialize LCD
    LPC_GPIO0->FIODIR = 0xFF<<23; // Set data lines as output
    for(i = 0;i<6;i++){
        lcdCom(array[i],0); // Send initialization commands to LCD
        delayUS(1000000); // Delay
    }
}

// Function to update LCD display based on obstacle detection
void update(){
    int i;
    int left = 0,right = 0, front = 0; // Initialize variables to track obstacles on left, right, and front
    for(i=0;i<25;i++){
        if(obstacleArray[i] == 1){
            if(i<9)
                left = 1; // Obstacle detected on left
            else if(i<16)
                front = 1; // Obstacle detected in front
            else
                right = 1; // Obstacle detected on right
        }
    }
    lcdCom(0x01,0); // Clear display
    delayUS(100000); // Delay
    lcdCom(0x80,0); // Move cursor to line 1, position 1
    delayUS(100000); // Delay
    if(left|front|right){ // If obstacle detected
        for(i=0;obstacle[i]!='\0';i++){
            lcdCom(obstacle[i],1); // Display "Obstacle" message
            delayUS(100000); // Delay
        }
        lcdCom(0xc0,0); // Move cursor to line 2, position 1
        delayUS(100000); // Delay
        if(front)
            for(i=0;frontStr[i]!='\0';i++){
                lcdCom(frontStr[i],1); // Display "Front" message
                delayUS(100000); // Delay
            }
        if(left)
            for(i=0;leftStr[i]!='\0';i++){
                lcdCom(leftStr[i],1); // Display "Left" message
                delayUS(100000); // Delay
            }
        if(right)
            for(i=0;rightStr[i]!='\0';i++){
                lcdCom(rightStr[i],1); // Display "Right" message
                delayUS(100000); // Delay
            }
    }else{
        for(i=0;clear[i]!='\0';i++){
            lcdCom(clear[i],1); // Clear LCD
            delayUS(100000); // Delay
        }
    }
}

// Main function
int main() {
    float distance = 0; // Initialize distance variable
    int degree = 0,flag = 0; // Initialize degree and flag variables
    SystemInit(); // Initialize system
    SystemCoreClockUpdate(); // Update system core clock
    initTimer0(); // Initialize Timer0
    inittimer1(); // Initialize Timer1
    lcdInit(); // Initialize LCD
    // Interface TRIG P0.15
    // Interface ECHO P0.16
    LPC_GPIO0->FIODIR |= TRIG | 1 << 17; // Set direction for TRIGGER pin
    LPC_GPIO1->FIODIR |= 0 << 16; // Set direction for ECHO PIN
    LPC_PINCON->PINSEL1 |= 0; // Select GPIO function for ECHO pin
    LPC_GPIO0->FIODIR |= 0XF<<4; // Set GPIO direction for rotating pins
    i = 0; // Initialize iteration variable
    LPC_GPIO0->FIOCLR |= TRIG; // Set TRIG pin low
    lcdCom(0x01,0); // Clear LCD
    while(1){
        delay(1000); // Delay
        if(degree==25) // If degree reaches 25
            flag = 1; // Set flag to 1
        if(degree==-1 ) // If degree reaches -1
            flag = 0; // Set flag to 0
        if(flag){
            clockwise(); // Rotate clockwise
            degree--; // Decrement degree
        } else {
            anti_clockwise(); // Rotate anti-clockwise
            degree++; // Increment degree
        }
        distance = getDistance(); // Measure distance
        obstacleArray[degree] = distance<THRESHOLD; // Update obstacle array
        update(); // Update LCD display
        delay(1000000); // Delay
    }
}
