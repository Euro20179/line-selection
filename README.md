# line-selection

Lets you select line from a file.

Sadly it cannot read from stdin because I'm not sure how to do that without breaking `getchar()`

# Controls

* j/n/down: go down 1 line
* k/p/up: go up 1 line
* +[numbers]<enter>: go to line: [numbers]
* 0-9: select the 0, 1, 2... line
* <enter>: select the current line
* $: select the last line
* q: quit with exit code 5
* Q: quit with exit code 6 (might be helpful with scripting idk)

# Build

`make realease`


## Plans if i continue working on this

* Fix scrolling when it doesn't need to
* customizable colors
* Display the current key press when doing the `+[numbers]` thing
* maybe one day be able to read from stdin
* One reason i want to do this is to allow terminals to render sixel images instead of it having to go through curses, but we'll see if that happens

## Why

I wanted to learn a bit more about terminals, and how ansi escape sequences work, so i built a tui without curses.
