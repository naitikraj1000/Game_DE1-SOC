
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

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
/* set a single pixel on the screen at x,y
 * x in [0,319], y in [0,239], and colour in [0,65535]
 */
#define SW_BASE 0xff200040
#define RLEDs ((volatile long *)0xFF200000)

volatile int *const ADDR_7SEG1 = (int *)0xFF200020;
volatile int *const ADDR_7SEG2 = (int *)0xFF200030;

int score = 0;
int health_dinosaur = 200;
int game_over = -1;
volatile int pixel_buffer_start;

double sqrt_new(double x)
{
    if (x <= 0)
        return 0; // if negative number throw an exception?
    int exp = 0;
    x = frexp(x, &exp); // extract binary exponent from x
    if (exp & 1)
    { // we want exponent to be even
        exp--;
        x *= 2;
    }
    double y = (1 + x) / 2; // first approximation
    double z = 0;
    while (y != z)
    { // yes, we CAN compare doubles here!
        z = y;
        y = (y + x / y) / 2;
    }
    return ldexp(y, exp / 2); // multiply answer by 2^(exp/2)
}


void swap(int *x, int *y)
{
    int temp = *x;
    *x = *y;
    *y = temp;
}
int min(int a, int b)
{
    return a < b ? a : b;
}

int max(int a, int b)
{
    return a > b ? a : b;
}

int round_new(double x)
{
    if (x >= 0)
    {
        int y = (int)(x + 0.5);
        return y;
    }
    else
    {
        int y = (int)(x - 0.5);
        return y;
    }
}
int dinosaur_x = 70, dinosaur_y = 150;
int ufo_center_coordinates_x[3], ufo_center_coordinates_y[3];
int laser_start_coordinate_x[4], laser_start_coordinate_y[4];
double slope_laser[4];
int prev_laser_start_coordinate_x[4], prev_laser_start_coordinate_y[4];
int sign_laser_x[4], sign_laser_y[4];
void write_pixel(int x, int y, short colour)
{
    // 0xc8000000
    volatile short *vga_addr = (volatile short *)(pixel_buffer_start + (y << 10) + (x << 1));
    *vga_addr = colour;
}

/* use write_pixel to set entire screen to black (does not clear the character buffer) */
void clear_screen()
{
    int x, y;
    for (x = 0; x < 320; x++)
    {
        for (y = 0; y < 240; y++)
        {
            write_pixel(x, y, 0);
        }
    }
}

/* write a single character to the character buffer at x,y
 * x in [0,79], y in [0,59]
 */
void write_char(int x, int y, char c)
{
    // VGA character buffer
    volatile char *character_buffer = (char *)(0xc9000000 + (y << 7) + x);
    *character_buffer = c;
}

int check_collision_ufo(int x1, int x2, int left, int right)
{

    if (left)
    {
        return max(dinosaur_x, (x1 + 24));
    }
    else
    {
        return min(dinosaur_x, (x2 - 24));
    }
}

int check_collision_border(int left, int right)
{

    // (0,20) --------------- (320,20)
    //      |                   |
    //      |                   |
    //      |                   |
    //      |                   |
    // (0,240) -------------- (320,240)

    if (left)
    {
        int extra = dinosaur_x - 15;
        if (extra > 15)
            return dinosaur_x;
        else
        {
            return dinosaur_x + extra + 15;
        }
    }
    else
    {
        int extra = 280 - dinosaur_x;
        if (extra > 15)
            return dinosaur_x;
        else
        {
            return dinosaur_x + extra - 15;
        }
    }
}

void draw_line(int x_start, int y_start, int x_finish, int y_finish, short int line_color)
{
    double y;
    int y_pos;
    double x;
    int x_pos;
    double y_dis;
    double x_dis;
    double slope;
    int i;

    if (x_start == x_finish && y_start == y_finish)
    {
        write_pixel(x_start, y_start, line_color);
        return;
    }

    if (abs(x_finish - x_start) >= abs(y_finish - y_start))
    {
        if (x_start > x_finish)
        {
            swap(&x_start, &x_finish);
            swap(&y_start, &y_finish);
        }

        y_dis = y_finish - y_start;
        x_dis = x_finish - x_start;
        slope = y_dis / x_dis;
        for (i = x_start; i < x_finish; i++)
        {
            y = y_start + slope * (i - x_start);
            y_pos = round_new(y);

            write_pixel(i, y_pos, line_color);
        }
    }

    else
    {
        if (y_start > y_finish)
        {
            swap(&x_start, &x_finish);
            swap(&y_start, &y_finish);
        }
        y_dis = y_finish - y_start;
        x_dis = x_finish - x_start;
        slope = x_dis / y_dis;
        for (i = y_start; i <= y_finish; i++)
        {
            x = x_start + slope * (i - y_start);
            x_pos = round_new(x);
            write_pixel(x_pos, i, line_color);
        }
    }
}

void draw_circle(int x_center, int y_center, int radius, short int colour)
{
    // MidPoint Circle Algorithm

    int x = 0;
    int y = radius;
    int d = 1 - radius;

    while (x <= y)
    {
        write_pixel(x_center + x, y_center + y, colour);
        write_pixel(x_center - x, y_center + y, colour);
        write_pixel(x_center + x, y_center - y, colour);
        write_pixel(x_center - x, y_center - y, colour);
        write_pixel(x_center + y, y_center + x, colour);
        write_pixel(x_center - y, y_center + x, colour);
        write_pixel(x_center + y, y_center - x, colour);
        write_pixel(x_center - y, y_center - x, colour);

        if (d < 0)
        {
            d += 2 * x + 3;
        }
        else
        {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

void draw_ufo(int x, int y, short int colour)
{
    // Outer Radius 10
    draw_circle(x, y, 10, colour);
    draw_circle(x, y, 5, colour);
    draw_circle(x, y, 2, colour);
    draw_line(x - 10, y, x + 10, y, colour);
    draw_line(x - 8, y - 6, x + 8, y - 6, colour);
}

void delay()
{

    volatile int *pixel_ctrl_ptr = (int *)0xff203020;
    int status;
    *pixel_ctrl_ptr = 1;
    status = *(pixel_ctrl_ptr + 3);

    while ((status & 0x01) != 0)
    {

        status = *(pixel_ctrl_ptr + 3);
    }
}

void move_ufo()
{

    int n = 3;
    // Artifically Detect the Dinosaur and Aim at it
    // Detect Collisions with Another UFO and based on that info it takes steps
    // Detect Collisions from wall
    // for the ith active UFO

    // (0,20) --------------- (320,20)
    //      |                   |
    //      |                   |
    //      |                   |
    //      |                   |
    // (0,240) -------------- (320,240)

    int updated_x, updated_y;
    int j, k, i;
    for (i = 0; i < n; i++)
    {

        if (ufo_center_coordinates_x[i] == -1)
        {
            continue;
        }
        else
        {
            j = i - 1;
            k = i + 1;

            while (j != -1 && ufo_center_coordinates_x[j] == -1)
            {
                j--;
            }

            while (k != n && ufo_center_coordinates_x[k] == -1)
            {
                k++;
            }

            int right = 0, left = 0;
            if (ufo_center_coordinates_x[i] - dinosaur_x >= 0)
                left = 1;
            else
                right = 1;

            if (j == -1 && left)
            {
                delay();
                draw_ufo(ufo_center_coordinates_x[i], ufo_center_coordinates_y[i], 0);
                ufo_center_coordinates_x[i] = check_collision_border(left, right);
                draw_ufo(ufo_center_coordinates_x[i], ufo_center_coordinates_y[i], GREEN);
                // break;
                // Detect Collisions with Left Borders
            }
            else if (left)
            {
                // UFO
                // Detect Collissions With UFO
                delay();
                draw_ufo(ufo_center_coordinates_x[i], ufo_center_coordinates_y[i], 0);
                ufo_center_coordinates_x[i] = check_collision_ufo(ufo_center_coordinates_x[j], ufo_center_coordinates_x[i], left, right);
                draw_ufo(ufo_center_coordinates_x[i], ufo_center_coordinates_y[i], GREEN);
                // break;
            }

            if (k == n && right)
            {
                // Detect Collisions with Right Borders
                delay();
                draw_ufo(ufo_center_coordinates_x[i], ufo_center_coordinates_y[i], 0);
                ufo_center_coordinates_x[i] = check_collision_border(left, right);
                draw_ufo(ufo_center_coordinates_x[i], ufo_center_coordinates_y[i], GREEN);
                // break;
            }
            else if (right)
            {
                // UFO
                // Detect Collissions With UFO
                delay();
                draw_ufo(ufo_center_coordinates_x[i], ufo_center_coordinates_y[i], 0);
                ufo_center_coordinates_x[i] = check_collision_ufo(ufo_center_coordinates_x[i], ufo_center_coordinates_x[k], left, right);
                draw_ufo(ufo_center_coordinates_x[i], ufo_center_coordinates_y[i], GREEN);
            }
            // printf("UPDATED INDEX %d -------> %d ----------->%d  \n", i, right, ufo_center_coordinates_x[i]);
        }
    }
}

void draw_dinosaur_left(int x, int y, short int colour)
{
    draw_line(x, y, x, y + 6, colour);
    draw_line(x - 1, y + 2, x - 1, y + 7, colour);
    draw_line(x - 2, y + 3, x - 2, y + 8, colour);
    draw_line(x - 3, y + 4, x - 3, y + 9, colour);
    draw_line(x - 4, y + 4, x - 4, y + 10, colour);
    draw_line(x - 5, y + 3, x - 5, y + 11, colour);
    draw_line(x - 6, y + 2, x - 6, y + 12, colour);
    draw_line(x - 7, y + 1, x - 7, y + 10, colour);
    write_pixel(x - 7, y + 12, colour);

    draw_line(x - 8, y, x - 8, y + 10, colour);
    draw_line(x - 9, y - 5, x - 9, y + 14, colour);

    write_pixel(x - 10, y + 14, colour);

    draw_line(x - 10, y - 6, x - 10, y + 8, colour);
    draw_line(x - 11, y - 6, x - 11, y + 7, colour);
    draw_line(x - 12, y - 6, x - 12, y + 6, colour);

    write_pixel(x - 13, y + 3, colour);
    write_pixel(x - 14, y + 3, colour);
    write_pixel(x - 14, y + 4, colour);

    draw_line(x - 13, y - 6, x - 13, y, colour);

    write_pixel(x - 14, y - 2, colour);
    write_pixel(x - 15, y - 2, colour);
    write_pixel(x - 16, y - 2, colour);

    draw_line(x - 14, y - 6, x - 14, y - 2, colour);
    draw_line(x - 15, y - 6, x - 15, y - 2, colour);
    draw_line(x - 16, y - 6, x - 16, y - 2, colour);
    draw_line(x - 17, y - 5, x - 17, y - 2, colour);
}

void draw_dinosaur(int x, int y, short int colour)
{

    draw_line(x, y, x, y + 6, colour);
    draw_line(x + 1, y + 2, x + 1, y + 7, colour);
    draw_line(x + 2, y + 3, x + 2, y + 8, colour);
    draw_line(x + 3, y + 4, x + 3, y + 9, colour);
    draw_line(x + 4, y + 4, x + 4, y + 10, colour);
    draw_line(x + 5, y + 3, x + 5, y + 11, colour);
    draw_line(x + 6, y + 2, x + 6, y + 12, colour);
    draw_line(x + 7, y + 1, x + 7, y + 10, colour);
    write_pixel(x + 7, y + 12, colour);

    draw_line(x + 8, y, x + 8, y + 10, colour);
    draw_line(x + 9, y - 5, x + 9, y + 14, colour);

    write_pixel(x + 10, y + 14, colour);

    draw_line(x + 10, y - 6, x + 10, y + 8, colour);
    draw_line(x + 11, y - 6, x + 11, y + 7, colour);
    draw_line(x + 12, y - 6, x + 12, y + 6, colour);

    write_pixel(x + 13, y + 3, colour);
    write_pixel(x + 14, y + 3, colour);
    write_pixel(x + 14, y + 4, colour);

    draw_line(x + 13, y - 6, x + 13, y, colour);

    write_pixel(x + 14, y - 2, colour);
    write_pixel(x + 15, y - 2, colour);
    write_pixel(x + 16, y - 2, colour);

    draw_line(x + 14, y - 6, x + 14, y - 2, colour);
    draw_line(x + 15, y - 6, x + 15, y - 2, colour);
    draw_line(x + 16, y - 6, x + 16, y - 2, colour);
    draw_line(x + 17, y - 5, x + 17, y - 2, colour);
}

void move_dinosaur(int right, int left, int up, int down)
{

    // (0,20) --------------- (320,20)
    //      |                   |
    //      |                   |
    //      |                   |
    //      |                   |
    // (0,240) -------------- (320,240)

    if (dinosaur_y - up + down <= 30)
        return;
    if (dinosaur_y - up + down >= 225)
        return;
    if (dinosaur_x + right - left >= 300)
        return;
    if (dinosaur_x + right - left <= 20)
        return;

    draw_dinosaur_left(dinosaur_x, dinosaur_y, 0);
    draw_dinosaur(dinosaur_x, dinosaur_y, 0);
    dinosaur_x += right - left;

    dinosaur_y += -up + down;
    if (left != 0)
    {
        draw_dinosaur_left(dinosaur_x, dinosaur_y, GREEN);
    }
    else
    {
        draw_dinosaur(dinosaur_x, dinosaur_y, GREEN);
    }
}

void laser_target(int i, int x1, int y1, int x2, int y2)
{

    if (x1 <= x2)
        sign_laser_x[i] = 1;
    else
        sign_laser_x[i] = -1;

    if (y1 <= y2)
        sign_laser_y[i] = 1;
    else
        sign_laser_y[i] = -1;

    double slope;
    if ((x2 - x1) == 0)
        slope = 9999;
    else
    {
        slope = abs((y2 - y1) / (x2 - x1));
    }

    slope_laser[i] = slope;
    laser_start_coordinate_x[i] = x1;
    laser_start_coordinate_y[i] = y1;
}

void borders()
{
    int x, y;
    for (y = 236; y <= 240; y++)
    {
        draw_line(0, y, 320, y, RED); // Horizontal Line
    }

    for (x = 0; x <= 4; x++)
    {
        draw_line(x, 24, x, 240, RED); // Vertical Line
    }

    for (x = 316; x <= 320; x++)
    {
        draw_line(x, 24, x, 240, RED); // Vertical Line
    }

    for (y = 20; y <= 24; y++)
    {
        draw_line(0, y, 320, y, RED); // Horizontal Line
    }
}
int score_x = 10, score_y = 3;
void write_score(char ch[9])
{
    int i;
    for (i = 0; i < 8; i++)
    {
        write_char(score_x, score_y, ch[i]);
        score_x++;
    }
    score_x--;
}

void write_win()
{
    write_char(60, 40, 'W');
    write_char(61, 40, 'I');
    write_char(62, 40, 'N');
}

void write_lose()
{
    write_char(60, 40, 'L');
    write_char(61, 40, 'O');
    write_char(62, 40, 'S');
    write_char(63, 40, 'E');
}

void writeText(char textPtr[], int x, int y)
{
    volatile char *characterBuffer = (char *)0xC9000000;

    int offset = (y << 7) + x;

    while (*textPtr)
    {
        *(characterBuffer + offset) = *textPtr;
        ++textPtr;
        ++offset;
    }
}

int main()
{

    int i;
    for (i = 0; i < 3; i++)
    {
        ufo_center_coordinates_x[i] = -1;
        ufo_center_coordinates_y[i] = -1;
    }

    for (i = 0; i < 4; i++)
    {
        laser_start_coordinate_x[i] = -1;
        laser_start_coordinate_y[i] = -1;
        slope_laser[i] = -1;
    }
    volatile int *pixel_ctrl_ptr = (int *)0xFF203020;
    /* Read location of the pixel buffer from the pixel buffer controller */
    pixel_buffer_start = *pixel_ctrl_ptr;

    clear_screen();

    borders();
    write_score("SCORE: ");
    volatile unsigned int *switch_pt = (unsigned int *)SW_BASE;

    unsigned int switch_value_up = 0;
    unsigned int switch_value_down = 0;
    unsigned int switch_value_right = 0;
    unsigned int switch_value_left = 0;
    unsigned int switch_value_break = 0;
    if ((*switch_pt) == 1)
        switch_value_right = 1;
    else if ((*switch_pt) == 2)
    {
        switch_value_left = 1;
    }
    else if ((*switch_pt) == 4)
    {
        switch_value_up = 1;
    }
    else if ((*switch_pt) == 8)
    {
        switch_value_down = 1;
    }
    else if (switch_pt == 16)
    {
        switch_value_break = 1;
    }

    draw_dinosaur(dinosaur_x, dinosaur_y, GREEN);
    draw_ufo(50, 50, GREEN);
    draw_ufo(180, 50, GREEN);
    draw_ufo(250, 50, GREEN);
    ufo_center_coordinates_x[0] = 50;
    ufo_center_coordinates_y[0] = 50;
    ufo_center_coordinates_x[1] = 180;
    ufo_center_coordinates_y[1] = 50;
    ufo_center_coordinates_x[2] = 250;
    ufo_center_coordinates_y[2] = 50;

    write_pixel(80, 100, RED);
    write_pixel(200, 200, GREEN);

    while (1)
    {
        if (game_over == 0)
        {
            printf(" GAME OVER 1 \n");
            clear_screen();
            write_lose();
            // Print Information
            break;
        }
        else if (game_over == 3)
        {
            clear_screen();
            write_win();
            break;
        }
        // Moving Laser  code part
        int i = 0;
        int verify_laser[4];

        for (; i < 4; i++)
        {

            //    printf( "Dinosaur %d:    %d \n",dinosaur_x,dinosaur_y );
            // Detecting that laser is fired
            if (laser_start_coordinate_x[i] == -1)
            {
                // call function to set  a target
                //    printf( " Target Locking \n ");
                if (i < 3)
                {
                    // (50,50)  --> 70,150

                    // printf(" UFO CENTER COORDINATES %d:      %d\n", ufo_center_coordinates_x[i], ufo_center_coordinates_y[i]);
                    // printf(" DINOSAUR  COORDINATES %d:      %d\n", dinosaur_x, dinosaur_y);
                    laser_target(i, ufo_center_coordinates_x[i], ufo_center_coordinates_y[i], dinosaur_x, dinosaur_y);
                }
                else
                {
                    laser_target(i, dinosaur_x, dinosaur_y, ufo_center_coordinates_x[2], ufo_center_coordinates_y[2]);
                }
            }

            verify_laser[i] = 1;
            // Laser is moving towards target
            double slope = slope_laser[i];
            // printf(" Slope: %f \n", slope_laser[i]);
            double sin = slope / (sqrt_new(1 + slope * slope));
            double cos = 1 / (sqrt_new(1 + slope * slope));
            double start_line_x = laser_start_coordinate_x[i], start_line_y = laser_start_coordinate_y[i];
            int end_line_x, end_line_y;
            prev_laser_start_coordinate_x[i] = start_line_x;
            prev_laser_start_coordinate_y[i] = start_line_y;

            end_line_x = round_new(start_line_x + (10 * cos) * sign_laser_x[i]);
            end_line_y = round_new(start_line_y + (10 * sin) * sign_laser_y[i]);
            // laser hitting dinosaur
            if (i < 3)
            {
                if (abs(end_line_x - dinosaur_x) <= 5 && abs(end_line_y - dinosaur_y) <= 5)
                {
                    laser_start_coordinate_x[i] = -1;
                    // collision occured
                    health_dinosaur -= 40;
                    if (health_dinosaur <= 0)
                    {
                        game_over = 0;
                        clear_screen();
                        // write_lose();
                        break;
                    }
                }
            }
            else if (i == 3)
            {
                // dinosaur hitting any of the ufo
                int j = 0;
                int extra = 0;
                for (; j < 3; j++)
                {
                    if (ufo_center_coordinates_x[j] == -1)
                    {
                        extra++;
                        continue;
                    }
                    else
                    {
                        if (abs(end_line_x - ufo_center_coordinates_x[j]) <= 12 && abs(end_line_y - ufo_center_coordinates_y[j]) <= 12)
                        {
                            score += 10;
                            game_over = 1 + extra;
                            draw_ufo(ufo_center_coordinates_x[j], ufo_center_coordinates_y[j], 0);
                            ufo_center_coordinates_x[j] = -1;
                            ufo_center_coordinates_y[j] = -1;
                            laser_start_coordinate_x[i] = -1;
                            break;
                        }
                    }
                }
            }

            // printf("Laser_X %d:  %d \n ", end_line_x, end_line_y);
            if (end_line_x > 280 || end_line_y > 220 || end_line_y < 30 || end_line_x < 30)
            {
                // printf(" ############ \n");
                verify_laser[i] = -1;
                laser_start_coordinate_x[i] = -1;
                laser_start_coordinate_y[i] = -1;
                continue;
            }
            draw_line(start_line_x, start_line_y, end_line_x, end_line_y, ORANGE);

            start_line_x = end_line_x;
            start_line_y = end_line_y;
            laser_start_coordinate_x[i] = start_line_x;
            laser_start_coordinate_y[i] = start_line_y;
        }

        for (i = 0; i < 4; i++)
        {
            if (verify_laser[i] == 1)
            {
                double slope = slope_laser[i];
                double sin = slope / (sqrt_new(1 + slope * slope));
                double cos = 1 / (sqrt_new(1 + slope * slope));
                double start_line_x = prev_laser_start_coordinate_x[i], start_line_y = prev_laser_start_coordinate_y[i];
                int end_line_x = round_new(start_line_x + (10 * cos) * sign_laser_x[i]);
                int end_line_y = round_new(start_line_y + (10 * sin) * sign_laser_y[i]);

                delay();
                draw_line(start_line_x, start_line_y, end_line_x, end_line_y, 0);
            }
        }

        if (score == 0)
        {
            *ADDR_7SEG1 = 0x00000003f;
        }
        else if (score == 10)
        {
            *ADDR_7SEG1 = 0x00000063f;
        }
        else if (score == 20)
        {
            *ADDR_7SEG1 = 0x000005b3f;
        }
        else if (score == 30)
        {
            *ADDR_7SEG1 = 0x000004f3f;
        }

        int score_ones, score_two;
        score_ones = score % 10;
        score_two = ((score / 10)) % 10;
        char ch1 = (score_two + '0');
        char ch2 = (score_ones + '0');

        write_char(score_x, score_y, ch1);
        write_char(score_x + 1, score_y, ch2);

        *RLEDs = *(switch_pt);
        if (switch_value_break == 1)
            break;

        move_ufo();
        delay();
        move_dinosaur(switch_value_right * 9, switch_value_left * 9, switch_value_up * 9, switch_value_down * 9);
        // printf("Switch values: Up=%d, Down=%d, Right=%d, Left=%d, Break=%d\n", switch_value_up, switch_value_down, switch_value_right, switch_value_left, switch_value_break);
        switch_value_up = 0;
        switch_value_down = 0;
        switch_value_right = 0;
        switch_value_left = 0;
        switch_value_break = 0;
        if ((*switch_pt) == 1)
        {
            switch_value_right = 1;
        }
        else if ((*switch_pt) == 2)
        {
            switch_value_left = 1;
        }
        else if ((*switch_pt) == 4)
        {
            switch_value_up = 1;
        }
        else if ((*switch_pt) == 8)
        {
            switch_value_down = 1;
        }
        else if ((*switch_pt) == 16)
        {
            switch_value_break = 1;
        }
    }

    return 0;
}