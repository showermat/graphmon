# graphmon

This program provides a basic curses-based UI for monitoring the output of some data stream using a bar graph.  It reads newline-separated positive integers from standard input and appends them to the right of the bar graph, adjusting the scale so that all bars can fit vertically.  It continues until it is killed, usually with `^C`.  If the end of the file is reached, the program displays the final data until it is interrupted.

## Usage

The ncurses libraries must be installed to use graphmon.

Build the executable:

    ./build.sh

To run, pipe some data stream into graphmon.  The program will read data as quickly as it can, so it's good if there's some sort of rate-limiting on the stream.  Real-time data streams, for example, are ideal.

For example, to plot infinite random data:

    while true; do rand 0 100; sleep 1; done | graphmon

Negative numbers are counted properly but are graphed as though they were 0.  Lines containing anything but a valid integer are not counted and are displayed as an asterisk with no bar.  A line containing only a single hyphen will create a visual separator in the output.  Numbers that are too wide to fit on top of a bar are replaced by a single asterisk.  Changing the constant BAR_W in the source allows the width of bars to be easily changed.

