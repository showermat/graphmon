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
#include <locale.h>
#define BAR_W 4
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

int *list;
int listlen;
const wchar_t *partfill = L" ▁▂▃▄▅▆▇█";

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
		int i = oldlistlen - 1, j = listlen - 1;
		while (i > 0 && j > 0) list[j--] = oldlist[i--];
		free(oldlist);
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

void oldrect(int y, int x, int w, int h)
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

void rect(int y, int x, int w, int h, int top)
{
	wchar_t fill[w + 1], topfill[w + 1];
	for (int i = 0; i < w; i++)
	{
		fill[i] = partfill[8];
		topfill[i] = partfill[top];
	}
	fill[w] = topfill[w] = 0;
	if (h <= 0) return;
	move(y, x);
	/*printw("%d", top);*/
	printw("%ls", topfill);
	for (int i = y + 1; i < y + h; i++)
	{
		move(i, x);
		printw("%ls", fill);
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
	oldrect(2, x * (BAR_W + 1), BAR_W, LINES - 4);
}

void drawsep(int x)
{
	stipple_rect(2, x * (BAR_W + 1), BAR_W, LINES - 3);
}

void drawbar(int x, int n, int max, bool unicode)
{
	int h = 0, top = 0;
	if (n < 0) h = 0;
	else if (n > max)
	{
		h = LINES - 4;
		top = 8;
	}
	else if (! unicode)
	{
		h = n * (LINES - 4) / max;
		top = 8;
	}
	else
	{
		int tot = n * (LINES - 4) * 8 / max;
		top = tot % 8;
		h = tot / 8;
		if (top == 0 && h > 0)
		{
			h -= 1;
			top = 8;
		}
	}
	rect(LINES - h - 1, x * (BAR_W + 1), BAR_W, h, top);
	move(LINES - h - 2, x * (BAR_W + 1));
	attron(COLOR_PAIR(5));
	if (n >= pow(10, BAR_W) || n <= -1 * pow(10, BAR_W - 1) || n == INT_MIN) printw("  *");
	else printw("%4d", n);
	attroff(COLOR_PAIR(5));
}

void draw(int max, bool unicode)
{
	for (int i = 0; i < listlen; i++)
	{
		clearbar(i);
		if (list[i] == INT_MAX) drawsep(i);
		else drawbar(i, list[i], MAX(max, 1), unicode);
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

void winch_update() // FIXME Broken
{
	clear();
	//list_alloc();
}

void terminate()
{
	endwin();
	free(list);
	exit(0);
}

void shift_in(int item)
{
	for (int i = 1; i < listlen; i++) list[i - 1] = list[i];
	list[listlen - 1] = item;
}

int main(int argc, char **argv)
{
	int opt;
	int usermax = 0;
	bool unicode = false;
	while ((opt = getopt(argc, argv, "m:u")) != -1)
	{
		if (opt == 'm') usermax = atoi(optarg);
		else if (opt == 'u') unicode = true;
	}
	setlocale(LC_ALL, "");
	initscr();
	timeout(-1);
	cbreak();
	keypad(stdscr, TRUE);
	noecho();
	start_color();
	init_pair(1, COLOR_BLACK, COLOR_WHITE); // For bars
	init_pair(5, COLOR_YELLOW, COLOR_BLACK); // For text labels
	init_pair(6, COLOR_WHITE, COLOR_RED); // For alerts
	init_pair(7, COLOR_CYAN, COLOR_BLACK); // For status bar
	char inbuf[80];
	int cur = INT_MAX;
	int n = 0, min = INT_MAX, max = INT_MIN, tot = 0, avg = 0;
	signal(SIGWINCH, winch_update);
	signal(SIGINT, terminate);
	list_alloc();
	draw(usermax > 0 ? usermax : max, unicode);
	drawstat(n, min, max, avg);
	block();
	refresh();
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
		draw(usermax > 0 ? usermax : max, unicode);
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
