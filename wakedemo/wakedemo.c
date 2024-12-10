#include <msp430.h>
#include <libTimer.h>
#include "lcdutils.h"
#include "lcddraw.h"
#include "buzzer.h"

// WARNING: LCD DISPLAY USES P1.0.  Do not touch!!! 

#define LED BIT6		/* note that bit zero req'd for display */

#define SCREEN_WIDTH 120
#define SCREEN_HEIGHT 160
#define TITLE_HEIGHT 20

#define SW_UP 1
#define SW_DOWN 2
#define SW_LEFT 4
#define SW_RIGHT 8
#define SW_CLEAR BIT4

#define SWITCHES 15                                                

char blue = 31, green = 0, red = 31;
short drawPos[2] = {screenWidth / 2,(SCREEN_HEIGHT + TITLE_HEIGHT)/2};
int switches = 0;
volatile char redrawScreen = 0;
const unsigned short cursor_colors[] = {COLOR_WHITE, COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_YELLOW};
const int num_colors = sizeof(cursor_colors) / sizeof(cursor_colors[0]);
int cursor_color_index = 0; // Current cursor color index

static char switch_update_interrupt_sense()
{
  char p2val = P2IN;
  P2IES |= (p2val & SWITCHES);
  P2IES &= (p2val | ~SWITCHES);
  return p2val;
}
void switch_init(){
  P2REN |= SWITCHES;
  P2IE |= SWITCHES;
  P2OUT |= SWITCHES;
  P2DIR &= ~ SWITCHES;
  switch_update_interrupt_sense();
}

void draw_pixel (int col, int row, unsigned short color){
  fillRectangle(col,row,1,1,color);
}
void clear_screen(){
  clearScreen(COLOR_BLUE);  // Clear the entire screen
  drawString5x7(10, 10, "Etch Sketch", COLOR_RED, COLOR_BLUE);  // Redraw the title
}
/**
void draw_cursor(int col, int row){
 
  draw_pixel(col, row, COLOR_WHITE);//center
  draw_pixel(col- 1 , row,COLOR_WHITE);//left
  draw_pixel(col+1, row, COLOR_WHITE);//right
  draw_pixel(col, row-1, COLOR_WHITE);//top
  draw_pixel(col, row+1, COLOR_WHITE);//bottom
}
*/
void draw_cursor(int col, int row) {
  unsigned short color = cursor_colors[cursor_color_index];
  draw_pixel(col, row, color);      // Center
  draw_pixel(col - 1, row, color); // Left
  draw_pixel(col + 1, row, color); // Right
  draw_pixel(col, row - 1, color); // Top
  draw_pixel(col, row + 1, color); // Bottom
}

void
switch_interrupt_handler()
{
  char p2val = switch_update_interrupt_sense();
  switches = ~p2val & SWITCHES;
  if (switches) {
    buzzer_set_period(2000); // Set buzzer frequency when pressed
  } else {
    buzzer_set_period(0); // Stop buzzing when released
  }
}

void update_position(){
  // erase_cursor(drawPos[0], drawPos[1]);
  
  if(switches & SW_UP){
    if (drawPos[1] > TITLE_HEIGHT){
      drawPos[1]--;
      redrawScreen = 1;
      buzzer_set_period(2000);
    }
  }
  if (switches & SW_DOWN){
    if (drawPos[1] < SCREEN_HEIGHT + TITLE_HEIGHT -1){
      drawPos[1]++;
      redrawScreen = 1;
    }
  }
  if (switches & SW_LEFT) {
    if (drawPos[0] > 0){
	  drawPos[0]--;
	  redrawScreen = 1;
    }
  }
  if (switches & SW_RIGHT) {
    if (drawPos[0] < SCREEN_WIDTH- 1){
      drawPos[0]++;
      redrawScreen = 1;
    }
  }

  //diagonal movement
  if((switches & SW_UP) && (switches & SW_LEFT)){
    if (drawPos[1] > TITLE_HEIGHT && drawPos[0] > 0){
      drawPos[1]--; // move up
      drawPos[0] --; // move left
      redrawScreen = 1;
    }
  }
  if((switches & SW_UP) && (switches & SW_RIGHT)){
    if(drawPos[1] > TITLE_HEIGHT && drawPos[0] < SCREEN_WIDTH -1){
      drawPos[1]--;//Move up
      drawPos[0]++; // Move right
      redrawScreen = 1;
    }
  }
  if((switches & SW_DOWN) && (switches & SW_LEFT)){
    if(drawPos[1] < SCREEN_HEIGHT + TITLE_HEIGHT -1 && drawPos[0] > 0 ){
      drawPos[1]++; // move down
      drawPos[0]--;
      redrawScreen = 1;
    }
  }
if((switches & SW_DOWN) && (switches & SW_RIGHT)){
  if(drawPos[1] < SCREEN_HEIGHT + TITLE_HEIGHT -1 && drawPos[0] < SCREEN_HEIGHT -1 ){
    drawPos[1]++;
    drawPos[0]++;
    redrawScreen =1;
  }
 }
    
  if(switches & SW_CLEAR) {
	clear_screen(); // Clear the screen
	drawPos[0] = SCREEN_WIDTH / 2; // Reset position
	drawPos[1] = SCREEN_HEIGHT + TITLE_HEIGHT / 2; // Reset position
        redrawScreen =1;
  }
  if(redrawScreen){
    draw_cursor(drawPos[0],drawPos[1]);
  }
  
}
void wdt_c_handler(){
  static int secCount = 0;
  secCount ++;
  if (secCount >= 10) {	 
    if(redrawScreen) {   			      
      update_position();
      draw_cursor(drawPos[0], drawPos[1]);
      draw_pixel(drawPos[0], drawPos[1], COLOR_WHITE);
      redrawScreen = 0;
    }
     secCount = 0;
     update_position();
  }
  // update_position();
 }
void __interrupt_vec(TIMER0_A0_VECTOR) Timer_A() {
  // Change the cursor color every time the interrupt is triggered
  cursor_color_index = (cursor_color_index + 1) % num_colors; // Increment the color index and wrap around
  redrawScreen = 1; // Set redraw flag to true so cursor gets redrawn
}
void timer_init() {
  TA0CCR0 = 32768 - 1;               // Set the timer count for 1 second (ACLK = 32768Hz)
  TA0CCTL0 = CCIE;                   // Enable capture/compare interrupt
  TA0CTL = TASSEL_1 | MC_1 | TACLR;  // ACLK, Up mode, Clear timer
}
void main()
{
  
  P1DIR |= LED;/**< Green led on when CPU on */
  P1OUT |= LED; // turn on led initially

  configureClocks();
  lcd_init();
  buzzer_init(); 
  switch_init();
  timer_init();
  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);              /**< GIE (enable interrupts) */
  clear_screen();
  draw_cursor(drawPos[0], drawPos[1]);
 
  while (1) {/* forever */
    if (redrawScreen) {
      draw_cursor(drawPos[0], drawPos[1]);
      redrawScreen = 0;
      
    }

    P1OUT &= ~LED;/* led off */
    or_sr(0x10);/**< CPU OFF */
    P1OUT |= LED;/* led on */
    
  }
}


void
__interrupt_vec(PORT2_VECTOR) Port_2(){
  if (P2IFG & SWITCHES) {	      /* did a button cause this interrupt? */
    P2IFG &= ~SWITCHES;		      /* clear pending sw interrupts */
    switch_interrupt_handler();	/* single handler for all switches */
  }
}

