
/**
 * @file max7221.h
 * @author Amiy Kumar
 * @version 0.1
 * @date 2020-09-11
 * 
 */

#ifndef MAX7221_H
#define MAX7221_H

/* Works with MAX7219/21 */
/* Serial data format
+-----+-----+-----+-----+-- --+-----+----+----+----+----+----+----+----+----+----+----+
| D15 | D14 | D13 | D12 | D11 | D10 | D9 | D8 | D7 | D6 | D5 | D4 | D3 | D2 | D1 | D0 |
+-----+-----+-----+-----+-- --+-----+----+----+----+----+----+----+----+----+----+----+
|  X  |  X  |  X  |  X  |       ADRESS        | MSB             DATA              LSB |
+-----+-----+-----+-----+---------------------+---------------------------------------+

No decode mode data bits and corresponding lines/dots
+----+----+----+----+----+----+----+----+
| D7 | D6 | D5 | D4 | D3 | D2 | D1 | D0 |
+----+----+----+----+----+----+----+----+
| DP |  A |  B |  C |  D |  E |  F |  G |
+----+----+----+----+----+----+----+----+

Source: MAX7221/19 Datasheet
*/

/* Timings in ns */
#define MIN_CS_HIGH 50
#define MIN_CLK_PERIOD 100
#define MIN_CLK_RISING 50
#define MIN_CLK_FALLING 50

/* Registers */
#define NOOP_REG 0x0
#define DIG_0_REG 0x1
#define DIG_1_REG 0x2
#define DIG_2_REG 0x3
#define DIG_3_REG 0x4
#define DIG_4_REG 0x5
#define DIG_5_REG 0x6
#define DIG_6_REG 0x7
#define DIG_7_REG 0x8
#define DECODE_MODE_REG 0x9
#define INTENSITY_REG 0xA
#define SCAN_LIMIT_REG 0xB
#define SHUTDOWN_REG 0xC
#define DISPLAY_TEST_REG 0xF

/* Data */

/* Decode mode data format:
Digits and their corresponding bits

+-------+-------+-------+-------+-------+-------+-------+-------+
|  D7   |  D6   |  D5   |  D4   |  D3   |  D2   |  D1   |  D0   |
+-------+-------+-------+-------+-------+-------+-------+-------+
| DIG_7 | DIG_6 | DIG_5 | DIG_4 | DIG_3 | DIG_2 | DIG_1 | DIG_0 |
+-------+-------+-------+-------+-------+-------+-------+-------+

Decodes only digits with "BCD code B" whose corresponding bits are high
 */

/* Shutdown */
#define SHUTDOWN_ENABLE 0x0
#define SHUTDOWN_DISABLE 0x1

/* Display test */
#define DISPLAY_TEST_ENABLE 0x1
#define DISPLAY_TEST_DISABLE 0x0

/* Intensity */
#define INTENSITY_MIN 0x0
#define INTENSITY_MAX 0xF

/* Scan limit
Format: DIG_x_y displays digits/lines(in dot matrix) from x to y only
*/
#define SCAN_DIGS_0_0 0x0
#define SCAN_DIGS_0_1 0x1
#define SCAN_DIGS_0_2 0x2
#define SCAN_DIGS_0_3 0x3
#define SCAN_DIGS_0_4 0x4
#define SCAN_DIGS_0_5 0x5
#define SCAN_DIGS_0_6 0x6
#define SCAN_DIGS_0_7 0x7

/* Code B fonts for 7 segement displays */
#define DECIMAL_PT 0xF	/* "OR" with other char to add DP */
#define CHAR_0 0x0
#define CHAR_1 0x1
#define CHAR_2 0x2
#define CHAR_3 0x3
#define CHAR_4 0x4
#define CHAR_5 0x5
#define CHAR_6 0x6
#define CHAR_7 0x7
#define CHAR_8 0x8
#define CHAR_9 0x9
#define CHAR__ 0xA	/* Dash */
#define CHAR_E 0xB
#define CHAR_H 0xC
#define CHAR_L 0xD
#define CHAR_P 0xE
#define CHAR_BLANK 0xF

/**DIGIT registers with respective indices so that code remains independent
 * of DIG_x_REG value
 */
const char DIG_REGS[8] = {
	DIG_0_REG,
	DIG_1_REG,
	DIG_2_REG,
	DIG_3_REG,
	DIG_4_REG,
	DIG_5_REG,
	DIG_6_REG,
	DIG_7_REG,
};

#endif	/*  max7221.h */
