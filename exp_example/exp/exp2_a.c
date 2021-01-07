/*
 * By LJP.
 */
//#define trace_printf(...) do{}while(0)
extern void buz1_init(void);
extern void buz1_on(void);
extern void buz1_off(void);
extern void led6_init(void);
extern void led6_on(void);
extern void led6_off(void);
extern int trace_printf(const char *format, ...);
extern void HAL_Delay(volatile  unsigned int Delay);


#define DOT_DURATION  1
#define DASH_DURATION  3
#define CODE_GAP  1
#define LETTER_GAP  3
#define WORD_GAP  7
#define LOOP_GAP  10


char* letters[] = {
	".-","-...","-.-.","-..",".","..-.","--.","....","..",//A-I
	".---","-.-",".-..","--","-.","---",".--.","--.-",".-.",//J-R
	"...","-","..-","...-",".--","-..-","-.--","--..",//S-Z
};
char* numbers[] = { "-----",".----","..---","...--","....-",
".....","-....","--...","---..","----." };	//0 - 9

static int beeping;

//base action: on off delay

//led/beep on
static void on()
{
	beeping = 1;
	trace_printf("[");
	buz1_on();
	led6_on();
}

//led/beep off
static void off()
{
	beeping = 0;
	trace_printf("]");
	buz1_off();
	led6_off();
}

//delay
static void delay(int dots)
{
	HAL_Delay(80*dots);
	while(dots-- > 0){
		trace_printf(beeping ? "*": " ");
	}
}


void morse_wave(char dotOrDash)
{
	on();
	if (dotOrDash == '.')	/*// . =1t */
	{
		delay(DOT_DURATION);
	}
	else                  /*//must be a -, =3t */
	{
		delay(DASH_DURATION);
	}
	off();
	delay(CODE_GAP); /*//gap between flashes, =1t */
}

void morse_sequence(char* sequence)
{
	int i = 0;
	while (sequence[i] != '\0')
	{
		morse_wave(sequence[i]);
		i++;
	}
	delay(LETTER_GAP);             /*//gap between letters, =3t */
}

void morse_letter(char ch)
{
	if (ch >= 'a' && ch <= 'z')
	{
		morse_sequence(letters[ch - 'a']);
	}
	else if (ch >= 'A' && ch <= 'Z')
	{
		morse_sequence(letters[ch - 'A']);
	}
	else if (ch >= '0' && ch <= '9')
	{
		morse_sequence(numbers[ch - '0']);
	}
	else if (ch == ' ')
	{
		delay(WORD_GAP);         /*//gap between words, =7t or better 6t?*/
	}
}

void morse_sentence(char * text)
{
	while (*text != '\0')
	{
		morse_letter(*text);
		text++;
	}
}

void exp2_a_init(void){
	buz1_init();
	led6_init();
}

void exp2_a_do(void){
	char * morseStr = "Hello Cortex-M4";
	morse_sentence(morseStr);
	trace_printf("\r\n");
}

void exp2_a_main(void)
{
	exp2_a_init();
	exp2_a_do();
}

