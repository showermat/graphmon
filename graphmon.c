#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <errno.h>
#include <getopt.h>
#include <ncurses.h>
#define BAR_W 4
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

int *list;
int listlen;

int die(char *msg)
{
	printf("%s", msg);
	exit(1);
}

void list_alloc()
{
	static int alloced = 0;
	int oldlistlen = listlen;
	listlen = COLS / (BAR_W + 1);
	if (alloced)
	{
		int *oldlist = list;
		list = (int *) malloc(listlen * sizeof(int));
		int i = oldlistlen, j = listlen;
		while (i > 0 && j > 0) list[j--] = oldlist[i--];
	}
	else
	{
		list = (int *) malloc(listlen * sizeof(int));
		for (int i = 0; i < listlen; i++) list[i] = 0;
		alloced = 1;
	}
}

void block()
{
	move(LINES - 1, COLS - 1);
}

void rect(int y, int x, int w, int h)
{
	char fill[w + 1];
	for (int i = 0; i < w; i++) fill[i] = ' ';
	fill[w] = 0;
	for (int i = y; i < y + h; i++)
	{
		move(i, x);
		printw("%s", fill);
	}
}

void stipple_rect(int y, int x, int w, int h)
{
	char fill_even[w + 1], fill_odd[w + 1];
	for (int i = 0; i < w; i++)
	{
		fill_even[i] = i % 2 ? ' ' : '.';
		fill_odd[i] = i % 2 ? '.' : ' ';
	}
	fill_even[w] = fill_odd[w] = 0;
	for (int i = y; i < y + h; i++)
	{
		move(i, x);
		printw("%s", i % 2 ? fill_even : fill_odd);
	}
}

void clearbar(int x)
{
	rect(3, x * (BAR_W + 1), BAR_W, LINES - 5);
}

void drawsep(int x)
{
	stipple_rect(3, x * (BAR_W + 1), BAR_W, LINES - 5);
}

void drawbar(int x, int n, int max)
{
	int h = 0;
	if (n < 0) h = 0;
	else h = n * (LINES - 6) / max;
	attron(COLOR_PAIR(1));
	rect(LINES - h - 2, x * (BAR_W + 1), BAR_W, h);
	attroff(COLOR_PAIR(1));
	move(LINES - h - 3, x * (BAR_W + 1));
	attron(COLOR_PAIR(5));
	if (n >= pow(10, BAR_W) || n <= -1 * pow(10, BAR_W - 1) || n == INT_MIN) printw("  *");
	else printw("%4d", n);
	attroff(COLOR_PAIR(5));
}

void draw(int max)
{
	for (int i = 0; i < listlen; i++)
	{
		clearbar(i);
		if (list[i] == INT_MAX) drawsep(i);
		else drawbar(i, list[i], max ? max : 1);
	}
}

void drawstat(int n, int min, int max, int avg)
{
	move(0, 0);
	clrtoeol();
	attron(COLOR_PAIR(7));
	printw("Records:  %u\t  Minimum:  %d\t  Maximum:  %d\t  Average:  %d", n, min == INT_MAX ? 0 : min, max == INT_MIN ? 0 : max, avg);
	attroff(COLOR_PAIR(7));
}

void winch_update()
{
	clear();
	list_alloc();
}

void terminate()
{
	endwin();
	exit(0);
}

void shift_in(int item)
{
	for (int i = 1; i < listlen; i++) list[i - 1] = list [i];
	list[listlen - 1] = item;
}

int main(int argc, char **argv)
{
	int opt;
	while ((opt = getopt(argc, argv, "")) != -1) { }
	initscr();
	timeout(-1);
	cbreak();
	keypad(stdscr, TRUE);
	noecho();
	start_color();
	init_pair(1, COLOR_BLACK, COLOR_WHITE); // For bars.  To be changed
	init_pair(5, COLOR_YELLOW, COLOR_BLACK); // For text labels
	init_pair(6, COLOR_WHITE, COLOR_RED); // For alerts
	init_pair(7, COLOR_CYAN, COLOR_BLACK); // For status bar
	char inbuf[80];
	int cur = INT_MAX;
	int n = 0, min = INT_MAX, max = INT_MIN, tot = 0, avg = 0;
	signal(SIGWINCH, winch_update);
	signal(SIGINT, terminate);
	list_alloc();
	while (fgets(inbuf, 80, stdin) != NULL)
	{
		if (inbuf[strlen(inbuf) - 1] == '\n') inbuf[strlen(inbuf) - 1] = 0;
		char *end;
		cur = strtol(inbuf, &end, 10);
		if (strlen(inbuf) == 0 || strlen(end) != 0)
		{
			if (! strcmp(inbuf, "-")) cur = INT_MAX;
			else cur = INT_MIN;
		}
		else
		{
			n++;
			min = MIN(min, cur);
			max = MAX(max, cur);
			tot += cur;
			avg = tot / n;
		}
		shift_in(cur);
		draw(max);
		drawstat(n, min, max, avg);
		block();
		refresh();
	}
	move(LINES - 1, 0);
	attron(COLOR_PAIR(6));
	printw("EOF");
	attroff(COLOR_PAIR(6));
	block();
	while (1) sleep(1);
	endwin();
	return 0;
}
