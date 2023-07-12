
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

volatile int pixel_buffer_start;
void swap(int *x, int *y)
{
    int temp = *x;
    *x = *y;
    *y = temp;
}

int Round(double x)
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

int ufo_center_coordinates[20][20];

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

int check_collision(int x1, int y1, int x2, int y2)
{
    int r1 = 10, r2 = 10;
    int dx = x2 - x1;
    int dy = y2 - y1;
    int distance = sqrt(dx * dx + dy * dy);
    return distance <= r1 + r2;
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
            y_pos = Round(y);

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
            x_pos = Round(x);
            write_pixel(x_pos, i, line_color);
        }
    }
}

int dinosaur_x = 120, dinosaur_y = 120;

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

move_ufo()
{
    // Artifically Detect the Dinosaur and Aim at it
    // Detect Collisions with Another UFO and based on that info it takes steps

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

void draw_airplane(int x, int y, short int colour)
{
    // Draw wings
    draw_line(x, y, x + 50, y - 10, colour);
    draw_line(x + 50, y - 10, x + 50, y - 20, colour);
    draw_line(x + 50, y - 20, x, y - 10, colour);
    draw_line(x, y - 10, x, y, colour);

    // Draw body
    draw_line(x, y, x + 30, y + 10, colour);
    draw_line(x + 30, y + 10, x + 100, y + 10, colour);
    draw_line(x + 100, y + 10, x + 70, y, colour);
    draw_line(x + 70, y, x, y, colour);

    // Draw tail
    draw_line(x + 70, y, x + 70, y - 10, colour);
    draw_line(x + 70, y - 10, x + 100, y - 10, colour);
    draw_line(x + 100, y - 10, x + 100, y - 20, colour);
    draw_line(x + 100, y - 20, x + 70, y - 20, colour);
    draw_line(x + 70, y - 20, x + 70, y - 10, colour);
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

void borders()
{

    for (int y = 236; y <= 240; y++)
    {
        draw_line(0, y, 320, y, RED); // Horizontal Line
    }

    for (int x = 0; x <= 4; x++)
    {
        draw_line(x, 24, x, 240, RED); // Vertical Line
    }

    for (int x = 316; x <= 320; x++)
    {
        draw_line(x, 24, x, 240, RED); // Vertical Line
    }

    for (int y = 20; y <= 24; y++)
    {
        draw_line(0, y, 320, y, RED); // Horizontal Line
    }
}
int score_x = 10, score_y = 3;
void write_score(char ch[9])
{

    for (int i = 0; i < 8; i++)
    {
        write_char(score_x, score_y, ch[i]);
        score_x++;
    }
    score_x--;
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

    draw_dinosaur(120, 120, GREEN);
    draw_ufo(50, 50, GREEN);
    while (1)
    {
        *RLEDs = *(switch_pt);
        if (switch_value_break == 1)
            break;
        delay();
        move_dinosaur(switch_value_right * 9, switch_value_left * 9, switch_value_up * 9, switch_value_down * 9);
        move_ufo();
        
        printf("Switch values: Up=%d, Down=%d, Right=%d, Left=%d, Break=%d\n", switch_value_up, switch_value_down, switch_value_right, switch_value_left, switch_value_break);
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