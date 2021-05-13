/* This files provides address values that exist in the system */

#define SDRAM_BASE            0xC0000000
#define FPGA_ONCHIP_BASE      0xC8000000
#define FPGA_CHAR_BASE        0xC9000000
#define A9_ONCHIP_END         0xFFFFFFFF

/* IRQ Signal*/
#define ENABLE                0x1
#define IRQ_MODE              0b10010
#define SVC_MODE              0b10011
#define INT_ENABLE            0b01000000
#define INT_DISABLE           0b11000000
#define KEYS_IRQ  			  73

/* Interrupt controller (GIC) CPU interface(s) */
#define MPCORE_GIC_CPUIF      0xFFFEC100    // PERIPH_BASE + 0x100
#define ICCICR                0x00          // offset to CPU interface control reg
#define ICCPMR                0x04          // offset to interrupt priority mask reg
#define ICCIAR                0x0C          // offset to interrupt acknowledge reg
#define ICCEOIR               0x10          // offset to end of interrupt reg
/* Interrupt controller (GIC) distributor interface(s) */
#define MPCORE_GIC_DIST       0xFFFED000    // PERIPH_BASE + 0x1000
#define ICDDCR                0x00          // offset to distributor control reg
#define ICDISER               0x100         // offset to interrupt set-enable regs
#define ICDICER               0x180         // offset to interrupt clear-enable regs
#define ICDIPTR               0x800         // offset to interrupt processor targets regs
#define ICDICFR               0xC00         // offset to interrupt configuration regs

/* Cyclone V FPGA devices */
#define LEDR_BASE             0xFF200000
#define HEX3_HEX0_BASE        0xFF200020
#define HEX5_HEX4_BASE        0xFF200030
#define SW_BASE               0xFF200040
#define KEY_BASE              0xFF200050
#define TIMER_BASE            0xFF202000
#define PIXEL_BUF_CTRL_BASE   0xFF203020
#define CHAR_BUF_CTRL_BASE    0xFF203030

/* VGA colors */
#define WHITE 0xFFFF
#define YELLOW 0xFFE0
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define GREY 0xC618
#define PINK 0xFC18
#define ORANGE 0xFC00
#define BROWN 0xAB40
#define BLACK 0x0000

/*C libraries*/
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

/*Miscellaneous*/
#define ABS(x) (((x) > 0) ? (x) : -(x))

/* Screen size. */
#define RESOLUTION_X 320
#define RESOLUTION_Y 240

/* Constants for animation */
#define BOX_LEN 3
#define NUM_BOXES 10
#define BASKET_H 230 
#define MOVE_INCR 6 //MOVES BASKET
//160 midpoint
#define X_BEGIN 145
#define X_END 176

#define FALSE 0
#define TRUE 1

//function declarations
void wait_for_vsync();
void draw_basket(int x0, int y0, int x1, int y1, unsigned short int line_color);
void blacken();
void swap(int *a, int *b);
void wait_for_vsync();
void drawBox();
void drawBoxWords (int xT, int yT, unsigned short int line_color);    
void clearBox(int x, int y);   //x & y refer to top left pixel of the box
void draw_falling_item();
bool detectItem(int *left, int *right, int height);
void fastErase();
void eraseBasket(int basketL, int basketR, unsigned char keyPressed);
void move(char key, int *left, int *right);
void plot_text(int x, int y, char *text_ptr);
void dispScore();
void dispLevel();
void dispLives();
void checkLose(int y, bool *gameOver, int *lives);
bool inScreen (int x, int lo, int hi);
void endGameScreen();
void startGameScreen();
void eraseStartScreen();
void blankEndScreen();
void move(char key, int *left, int *right);
void plot_pixel(int x, int y, unsigned short int line_color);
bool check(int y, int *lives);

//for IRQ Signals
void set_A9_IRQ_stack(void);
void enable_A9_interrupts(void);
void config_GIC(void);
void config_KEYs();
void pushbutton_ISR(void);
void __attribute__((interrupt)) __cs3_isr_irq(void);

volatile int pixel_buffer_start; // global variable
volatile int key_dir = 0;  //written by interrupt service routine
int col[10] = {WHITE, YELLOW, RED, GREEN, MAGENTA, CYAN, PINK, ORANGE, BLUE, GREY};

unsigned short int cat1[9][13] = {
	{BLACK, BLACK, BLACK, BLACK, BLACK, BROWN, BROWN, BLACK, BLACK, BLACK, BROWN, BLACK, BLACK},
	{BLACK, BLACK, BLACK, BLACK, BLACK, BROWN, GREY, BROWN, BROWN, BROWN, GREY, BLACK, BLACK},
	{BLACK, BROWN, BLACK, BLACK, BROWN, BROWN, BROWN, BROWN, BROWN, BROWN, BROWN, BLACK, BROWN},
	{BROWN, BLACK, BLACK, BLACK, BLACK, BROWN, BROWN, YELLOW, BROWN, BROWN, YELLOW, BROWN, BLACK},
	{BROWN, BLACK, BLACK, BLACK, BROWN, BROWN, BROWN, BROWN, BROWN, BROWN, BROWN, BLACK, BROWN},
	{BROWN, BLACK, BLACK, BLACK, BLACK, BLACK, WHITE, WHITE, WHITE, WHITE, BLACK, BLACK, BLACK},
	{BLACK, BROWN, BLACK, BLACK, BLACK, BROWN, BROWN, WHITE, WHITE, WHITE, BLACK, BLACK, BLACK},
	{BLACK, BLACK, BROWN, BROWN, BROWN, BROWN, BROWN, BROWN, WHITE, BROWN, BLACK, BLACK, BLACK},
	{BLACK, BLACK, BROWN, BROWN, BROWN, BROWN, WHITE, BROWN, BROWN, BROWN, WHITE, BLACK, BLACK}
};

struct box {
    int x;
    int y;
    int lastX;
    int secondLastX;
    int lastY;
    int secondLastY;
    int colour;
    int speed;  //2-4, as number increases, speed increases
    bool caught;
    unsigned short int species[][13];  //cat1, dog1 ...
};

struct box animals[NUM_BOXES];
bool lose = false; 
int items = 0;
int rank = 4;   //number of boxes shown
int lowestSpeed = 2;
int topSpeed = 2;
int level = 1;
double lives = 6;

int main(void){
	volatile int * pixel_ctrl_ptr = (int *)0xFF203020; //PIXEL_BUF_CTRL_BASE
	volatile int * PS2_ptr = (int *) 0xFF200100;       //PS/2 address

	*(PS2_ptr) = 0xFF;  //reset
	int PS2_data;
	unsigned char key = 0;
	int left = X_BEGIN, right = X_END;
	
	/*Declare other variables and initialize stuff here*/
	srand(time(NULL));

    // initialize location and direction of count
    for(int i =0; i < NUM_BOXES; i++){
        animals[i].x = rand() % (RESOLUTION_X);      //to limit rand range
        animals[i].y = -(rand() % 400);
        animals[i].colour = col[rand() % 9];
        animals[i].lastX = animals[i].x;
        animals[i].secondLastX = animals[i].x;
        animals[i].lastY = 0;
        animals[i].secondLastY = 0;
        animals[i].speed = (rand() % (topSpeed + 1 - lowestSpeed)) + lowestSpeed;
        animals[i].caught = false;
        //animals[i].species[][] 
    }

	//Blank the end screen 
	blankEndScreen();
	//blank score in the top right corner
	char blank[30] = {' ',' ',' ',' ',' ',
					  ' ',' ',' ',' ',' ',
					  ' ',' ',' ',' ',' ',
					  ' ',' ',' ',' ',' ',
					  ' ',' ',' ',' ',' ',
					  ' ',' ',' ',' ',' '};
	
	//blank lives, level and score at top right
	plot_text(50,0,blank);
	plot_text(50,1,blank);

	/* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the 
                                        // back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    
	blacken(); // pixel_buffer_start points to the pixel buffer
    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer
    
	
	set_A9_IRQ_stack();
	config_GIC();
	config_KEYs();
	enable_A9_interrupts();
	
	while(1){
		//waiting for user to press pushbutton
		
		if(key_dir){   //game on

			blacken();
			eraseStartScreen(); //remove from current buffer
			wait_for_vsync();    
			pixel_buffer_start = *(pixel_ctrl_ptr + 1);  

			eraseStartScreen(); //remove from other buffer
			wait_for_vsync(); 
			pixel_buffer_start = *(pixel_ctrl_ptr + 1); 

			while (!lose && key_dir != 0) //ongoing game
		    {
				/* Erase any boxes that were drawn in the last iteration */
				fastErase();
				eraseBasket(left, right, key);

				//Update new basket position when arrow keys are used
				PS2_data = *(PS2_ptr); //read Data reg in PS2 port
				int RVALID = (PS2_data & 0x8000);	// extract the RVALID field
				if (RVALID != 0){
					key = PS2_data & 0xFF;
					move(key, &left, &right);
				}

				//code for drawing the boxes & updating their locations
		        draw_falling_item();
		        //draw basket
			    draw_basket(left, BASKET_H, right, BASKET_H, WHITE);

				//check if box hits basket
				if(detectItem(&left, &right, BASKET_H-1)){
					++items;
					
					//find the animal caught
					for(int boxID = 0; boxID < rank; boxID++){
						if(animals[boxID].x >= left - BOX_LEN && animals[boxID].x + BOX_LEN <= right + BOX_LEN 
							&& animals[boxID].y + BOX_LEN >= BASKET_H - 1){
							animals[boxID].caught = true;
						}
					}
				}

				//change levels, with speed increasing with each level
				if(items >= 10){
					level = items/10 + 1; //floor division
					// increase the number of objects on screen with increasing level 			
					if(items % 10 == 0 && rank != 10 && topSpeed != 4){
						rank++;
						topSpeed = topSpeed + 2;
						lowestSpeed++;
					}
				}
				
				/*checks for lose condition*/ 
				//checkLose(BASKET_H+1, &lose, &lives); //bugged
				bool miss = false;
				miss = check(BASKET_H+1,&lives);
				if(miss){
					if(lives == 0){
						lose = true;
						break;
					}
					lives = lives - 0.5; //kind of works
				}
				
				//update level and score
				dispLevel();
				dispScore();
				dispLives(); //this clears properly now

				
		        
				wait_for_vsync(); // swap front and back buffers on VGA vertical sync
		        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
		    }

		    if(lose){ //game over
				wait_for_vsync(); // swap front and back buffers on VGA vertical sync
			    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
				endGameScreen(); //display game over

				//wait for pushbutton to be pressed to restart game
				while(key_dir != 0){
					//wait
				}
				lose = false;
				lives = 6;
				left = X_BEGIN;
				right = X_END;
				blankEndScreen();
				 // initialize location and direction of count
			    for(int i =0; i < NUM_BOXES; i++){
			        animals[i].x = rand() % (RESOLUTION_X);      //to limit rand range
			        animals[i].y = -(rand() % 400);
			        animals[i].colour = col[rand() % 9];
			        animals[i].lastX = animals[i].x;
			        animals[i].secondLastX = animals[i].x;
			        animals[i].lastY = 0;
			        animals[i].secondLastY = 0;
			        animals[i].speed = (rand() % (topSpeed + 1 - lowestSpeed)) + lowestSpeed;
			        animals[i].caught = false;
			        //animals[i].species[][] 
			    }

			}else if (key_dir == 0){ //game paused
				while(key_dir == 0){
					//wait
				}
			}
		}
		else{   //game off
			blacken();
			startGameScreen();
			wait_for_vsync(); // swap front and back buffers on VGA vertical sync
			pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
		}	

	}

  return 0;
}

void draw_falling_item(){
	for(int i = 0; i < rank; i++){

		//store previous locations
        animals[i].secondLastX = animals[i].lastX;
        animals[i].lastX = animals[i].x;
		animals[i].secondLastY = animals[i].lastY;
		animals[i].lastY = animals[i].y;

		if(animals[i].caught){
			animals[i].y = -(rand() % 350);  //reset x and y values
			animals[i].x = rand() % (RESOLUTION_X);
			animals[i].speed = (rand() % (topSpeed + 1 - lowestSpeed)) + lowestSpeed;
			animals[i].caught = false;
		}

		if(animals[i].y >= 0){
			drawBox(animals[i]);
		}
		
		if(animals[i].y < RESOLUTION_Y){
			animals[i].y = animals[i].y + animals[i].speed;       //update box locations
		}else{
			//when the box reaches the bottom of page, reset coordinates to fall from the sky again
			animals[i].y = -(rand() % 350);
			animals[i].x = rand() % (RESOLUTION_X);
			animals[i].speed = (rand() % (topSpeed + 1 - lowestSpeed)) + lowestSpeed;
		}	
	}
}

//moves the line 
void move(char key, int *left, int *right){
	if(key == 'k'){//left arrow		
		//update new basket coordinates
		if(inScreen(*left-MOVE_INCR, 0, RESOLUTION_X)){
			*left -= MOVE_INCR;
			*right -= MOVE_INCR;
		}
	}

	else if(key == 't'){//right arrow
		//update new basket coordinates
		if(inScreen(*right+MOVE_INCR, 0, RESOLUTION_X)){
			*left += MOVE_INCR;
			*right += MOVE_INCR;
		}
	}
}

//detects if item lands in basket
bool detectItem(int *left, int *right, int height){
	bool found = false;
	for(int i = *left+1; i <= *right-1; i += BOX_LEN){
		if(*(short int *)(pixel_buffer_start + (height << 10) + (i << 1)) != BLACK){
			found = true;
		}
	}
	int l = *left, r = *right;
	if(*(short int *)(pixel_buffer_start + ((height-4) << 10) + (l << 1)) != BLACK || (*(short int *)(pixel_buffer_start + ((height-4) << 10) + (r << 1)) != BLACK)){
		found = true;
	}
	return found;	
}

//check for lose condition (i.e. box passes the basket and 0 lives left)
//buggy. decrements the number of lives by 2-3 each time
/*void checkLose(int y, bool *gameOver, int *lives){ 
	for(int i = 0; i < RESOLUTION_X-4; i++){
		if(*(short int *)(pixel_buffer_start + (y << 10) + (i << 1)) != BLACK){

			//this condition only works for 5x5 boxes
			//iterate through each set of 5 pixels below the basket base
			if(
			(*(unsigned short int *)(pixel_buffer_start + (y << 10) + (i << 1))) == 
			(*(unsigned short int *)(pixel_buffer_start + (y << 10) + ((i+1) << 1))) == 
			(*(unsigned short int *)(pixel_buffer_start + (y << 10) + ((i+2) << 1))) == 
			(*(unsigned short int *)(pixel_buffer_start + (y << 10) + ((i+3) << 1))) == 
			(*(unsigned short int *)(pixel_buffer_start + (y << 10) + ((i+4) << 1))) 
			){
				(*lives)--;
				break;
			}
			if((*lives) < 1){
				*gameOver = true;
				break;
			}
		}
	}
}*/

//alternate checkLose: decrements by 2, doesn't work but detects lose condition
void checkLose(int y, bool *gameOver, int *lives){ 
	for(int i = 0; i < RESOLUTION_X; i++){
		if(*(short int *)(pixel_buffer_start + (y << 10) + (i << 1)) != BLACK){
			if(*lives == 0){
				*gameOver = true;
			}
			(*lives)--;
			break;
			
			if((*lives) < 1){
				*gameOver = true;
				break;
			}
		}
	}
}

//another alternative that omits decrementing lives, works
/*void checkLose(int y, bool *gameOver, int *lives){ 
	for(int i = 0; i < RESOLUTION_X; i++){
		if(*(short int *)(pixel_buffer_start + (y << 10) + (i << 1)) != BLACK){
			*gameOver = true;
			break;
		}
	}
}*/

//alternate check lose, doesn't work
/*bool check(int y, int *lives){
	for(int i = 0; i < RESOLUTION_X; i++){
		if(*(short int *)(pixel_buffer_start + (y << 10) + (i << 1)) != BLACK){
			(*lives)--;
			break;
			
			if((*lives) < 1){
				return true;
				break;
			}
		}
	}
	return false;
}*/

//checks if object missed, kind of works with the hack in main
bool check(int y, int *lives){
	for(int i = 0; i < RESOLUTION_X; i++){
		if(*(short int *)(pixel_buffer_start + (y << 10) + (i << 1)) != BLACK){
			return true;
		}
	}
	return false;
}

//call this function after player loses
void endGameScreen(){
	blacken();
	//blank score in the top right corner
	char blank[30] = {' ',' ',' ',' ',' ',
					  ' ',' ',' ',' ',' ',
					  ' ',' ',' ',' ',' ',
					  ' ',' ',' ',' ',' ',
					  ' ',' ',' ',' ',' ',
					  ' ',' ',' ',' ',' '};
	
	//blank lives, level and score at top right
	plot_text(50,0,blank);
	plot_text(50,1,blank);
	
	/*display game over*/
	char msg[10] = "GAME OVER\0";
	plot_text(39, 29, msg);
	
	/*display score below game over message*/
	char score_top [10] = "SCORE\0";
	char score_bottom[10];
	itoa(items, score_bottom, 10);
		
	plot_text(39,30,score_top);
	plot_text(39,31,score_bottom);	
	
	//display level in the centre
	char label[8] = "LEVEL\0";
	char l[8] ;
	itoa(level, l, 10);
	
	plot_text(39,32,label);
	plot_text(39,33,l);
}

void blankEndScreen(){
	char blank[10] = {' ',' ',' ',' ',' ',
					  ' ',' ',' ',' ',' '};
	for(int i = 29; i <= 33; i++){
		plot_text(39,i,blank);
	}
}

void startGameScreen(){
	unsigned short int start[5][19] = {
	{ORANGE, ORANGE, ORANGE, BLACK, ORANGE, ORANGE, ORANGE, BLACK, BLACK, ORANGE, BLACK, BLACK, ORANGE, ORANGE, BLACK, BLACK, ORANGE, ORANGE, ORANGE},
	{ORANGE, BLACK, BLACK, BLACK, BLACK, ORANGE, BLACK, BLACK, ORANGE, BLACK, ORANGE, BLACK, ORANGE, BLACK, ORANGE, BLACK, BLACK, ORANGE, BLACK},
	{ORANGE, ORANGE, ORANGE, BLACK, BLACK, ORANGE, BLACK, BLACK, ORANGE, ORANGE, ORANGE, BLACK, ORANGE, ORANGE, BLACK, BLACK, BLACK, ORANGE, BLACK},
	{BLACK, BLACK, ORANGE, BLACK,  BLACK, ORANGE, BLACK,  BLACK, ORANGE, BLACK, ORANGE, BLACK, ORANGE, BLACK, ORANGE, BLACK, BLACK, ORANGE, BLACK},
	{ORANGE, ORANGE, ORANGE, BLACK,  BLACK, ORANGE, BLACK,  BLACK, ORANGE, BLACK, ORANGE, BLACK, ORANGE, BLACK, ORANGE, BLACK, BLACK, ORANGE, BLACK},
	};

	for(int j = 0; j < 5; j++){
		for(int i = 0; i < 19; i++){
			 //text starts at (99, 119), 6 is the num of pixels to skip
			drawBoxWords(99 + (i*6), 119 + (j*6), start[j][i] ); 

		}
	}
	
}

//draws 5x5 boxs used for words
void drawBoxWords (int xT, int yT, unsigned short int colour) {
    for (int x = xT; x < xT + 5; x++) {
        for (int y = yT; y < yT + 5; y++) {
            if(inScreen(x, 0, RESOLUTION_X-1) && inScreen(y, 0, RESOLUTION_Y-1)) {
                plot_pixel(x, y, colour);
            }
        }
    }
}

void eraseStartScreen(){
	for(int j = 0; j <= 5; j++){
		for(int i = 0; i <=19; i++){
			 //text starts at (99, 119), 6 is the num of pixels to skip
			drawBoxWords(99 + (i*6), 119 + (j*6), BLACK);    
		}
	}
}

//displays lives
void dispLives(){
	char label[9] = "LIVES\0";
	char l[9];
	itoa(lives, l, 10);
		
	plot_text(50,0,label);
	plot_text(50,1,l);
}

//displays score in top right corner
void dispScore(){
	char score_top [9] = "SCORE\0";
	char score_bottom[9];
	itoa(items, score_bottom, 10);
		
	plot_text(70,0,score_top);
	plot_text(70,1,score_bottom);	
}

//displays level
void dispLevel(){
	char label[8] = "LEVEL\0";
	char l[8] ;
	itoa(level, l, 10);
	
	plot_text(60,0,label);
	plot_text(60,1,l);	
}

/*Writes one line of text on character buffer at a time*/
void plot_text(int x, int y, char *text_ptr){
	int offset;
	volatile char *char_buffer = (char*)FPGA_CHAR_BASE; 
	
	//write to character buffer
	offset = (y << 7) + x;
	while(*(text_ptr)){
		*(char_buffer + offset) = *text_ptr;
		++text_ptr;
		++offset;
	}
}

//plots on pixel buffer
void plot_pixel(int x, int y, unsigned short int line_color)
{
    *(unsigned short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

//draw_line() definition
void draw_basket(int x0, int y0, int x1, int y1, unsigned short int line_color){
	
	if(x0>x1){
		swap(&x0,&x1);
		swap(&y0,&y1);
	}

	for (int i = 1; i < 5; i++){
		plot_pixel(x0, y0-i, line_color);
		plot_pixel(x1, y0-i, line_color);
	}

	for(int x = x0; x <= x1; x++){
		plot_pixel(x, y0, line_color);
	}
}

//clear screen definition (fast)
void fastErase(){

	//erase boxes
	for(int i = 0; i < NUM_BOXES; i++){
		clearBox(animals[i].secondLastX, animals[i].secondLastY);
	}
}

void eraseBasket(int basketL, int basketR, unsigned char keyPressed){

	//erase basket
	if(keyPressed == 'k'){//left arrow		
		basketL = basketL + (1*MOVE_INCR);
		basketR = basketR + (1*MOVE_INCR);
	}
	else if(keyPressed == 't'){//right arrow
		basketL = basketL - (1*MOVE_INCR);
		basketR = basketR - (1*MOVE_INCR);
	}

	for (int i = 1; i < 5; i++){
		plot_pixel(basketL, BASKET_H-i, BLACK);
		plot_pixel(basketR, BASKET_H-i, BLACK);
	}

	for(int i = basketL; i <= basketR; i++){
		plot_pixel(i,BASKET_H,BLACK);
	}

}

//clear_screen definition (slow)
void blacken(){
	for(int x = 0; x < RESOLUTION_X; x++){
		for(int y = 0; y < RESOLUTION_Y; y++){
			plot_pixel(x, y, BLACK);
		}
	}
}

//helper swap function
void swap(int *a, int *b){
	int temp = *a;
	*a = *b;
	*b = temp;
}

//waits 1/60th of a second for buffers to swap
void wait_for_vsync(){
	volatile int *pixel_ctrl_ptr = (int*) 0xFF203020;
	register int status;
	
	//launch swap process
	*pixel_ctrl_ptr = 1; //sets S bit to 1
	//poll S bit
	status = *(pixel_ctrl_ptr + 3); //deref address 0xFF20302C, 3*4=12
	
	while((status & 0x01) != 0){
		//read S bit
		status = *(pixel_ctrl_ptr + 3);
	}
	//exits when status bit s = 0
}

//draws a box
void drawBox (struct box b) {
    for (int x = b.x-BOX_LEN; x <= b.x+BOX_LEN; x++) {
        for (int y = b.y-BOX_LEN; y <= b.y+BOX_LEN; y++) {
            if(inScreen(x, 0, RESOLUTION_X-1) && inScreen(y, 0, RESOLUTION_Y-1)) {
                plot_pixel(x, y, b.colour);
            }
        }
    }
}

//erases a box of specified location
void clearBox(int x, int y)
{
    for(int i = x-BOX_LEN; i <= x+BOX_LEN; i++){
        for(int j = y-BOX_LEN; j <= y+BOX_LEN; j++){
        	if(inScreen(i, 0, RESOLUTION_X-1) && inScreen(j, 0, RESOLUTION_Y-1)) {
            	plot_pixel(i, j, BLACK);
            }
        }
    }
}

//bounds check 
bool inScreen (int x, int lo, int hi) {
    return (x >= lo && x <= hi);
}


// Define the IRQ exception handler
void __attribute__((interrupt)) __cs3_isr_irq(void) {
	 
	// Read the ICCIAR from the processor interface
	int address = MPCORE_GIC_CPUIF + ICCIAR; 
	int int_ID = *((int *)address);
	
	if (int_ID == KEYS_IRQ) // check if interrupt is from the KEYs
	    pushbutton_ISR();
	else
		while (1)
			; // if unexpected, then stay here
   
    // Write to the End of Interrupt Register (ICCEOIR)
	address = MPCORE_GIC_CPUIF + ICCEOIR; 
	*((int *)address) = int_ID;
	return; 
}

/*
 * Initialize the banked stack pointer register for IRQ mode
*/
void set_A9_IRQ_stack(void) {
	int stack, mode;
	stack = A9_ONCHIP_END - 7; // top of A9 onchip memory, aligned to 8 bytes /* change processor to IRQ mode with interrupts disabled */
	mode = INT_DISABLE | IRQ_MODE;
	asm("msr cpsr, %[ps]" : : [ps] "r"(mode));
	/* set banked stack pointer */
	asm("mov sp, %[ps]" : : [ps] "r"(stack));
    /* go back to SVC mode before executing subroutine return! */
    mode = INT_DISABLE | SVC_MODE;
    asm("msr cpsr, %[ps]" : : [ps] "r"(mode));
}

/*
 * Turn on interrupts in the ARM processor
*/
void enable_A9_interrupts(void) {
	int status = SVC_MODE | INT_ENABLE;
    asm("msr cpsr, %[ps]" : : [ps] "r"(status));
}


/*
 * Configure the Generic Interrupt Controller (GIC)
*/
void config_GIC(void) {
	int address; // used to calculate register addresses
	
	/* configure the FPGA interval timer and KEYs interrupts */
	*((int *)0xFFFED848) = 0x00000101; 
	*((int *)0xFFFED108) = 0x00000300;

	// Set Interrupt Priority Mask Register (ICCPMR). Enable interrupts of all // priorities
	address = MPCORE_GIC_CPUIF + ICCPMR;
	*((int *)address) = 0xFFFF;

	// Set CPU Interface Control Register (ICCICR). Enable signaling of // interrupts
	address = MPCORE_GIC_CPUIF + ICCICR;
	*((int *)address) = ENABLE;

	// Configure the Distributor Control Register (ICDDCR) to send pending // interrupts to CPUs
	address = MPCORE_GIC_DIST + ICDDCR;
	*((int *)address) = ENABLE;

}

void config_KEYs() {
	volatile int * KEY_ptr = (int *)KEY_BASE; // pushbutton KEY address
	*(KEY_ptr + 2) = 0x1; // enable interrupts form KEY[0] 
}

void pushbutton_ISR(void) {

	volatile int * KEY_ptr = (int *)KEY_BASE; 
	int press;
	press = *(KEY_ptr + 3); // read the pushbutton interrupt register 
	*(KEY_ptr + 3) = press; // Clear the interrupt
	key_dir ^= 1; // Toggle key_dir value r
	
	return;
}