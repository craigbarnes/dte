Todo
====

This is a list of ideas for improvements to the dte codebase that don't
particularly need a GitLab issue, either because they're too minor or
obscure to warrant public discussion or because they're just preliminary
ideas that may not go anywhere. The title "todo" is thus used very loosely.

Features
--------

* Add a function for recursively expanding aliases and use in `run_commands()`
  * Add a `show alias` flag to allow viewing fully expanded aliases, instead
    of just doing a single level of expansion
  * Add an `alias` flag to eagerly expand aliases at creation time (instead
    of at usage time)

* Add a command that swaps the positions of the selection anchor point and
  cursor (so that selections can be extended on both ends, without starting
  over)

* Remove `ARGERR_OPTION_ARGUMENT_NOT_SEPARATE` and make `do_parse_args()`
  handle e.g. `exec -oeval ...` like `getopt(3)` would

* Persist the most recently recorded `macro`, by saving `EditorState::macro`
  on exit and restoring it at startup (see also: `read_history_files()` →
  `search_set_regexp()`)

* Retain the last 10 or so recorded macros and allow them to be viewed
  (with e.g. `show macro 7`) and executed (with e.g. `macro play 7`)

* Allow multiple tags to be specified when using e.g. `dte -t A -t B`,
  `tag A B` or `exec -s -o tag echo "A\nB\n"` (if specifying one tag
  can create multiple messages, we may as well use the same mechanism
  to support multiple tags)

* Add option to `wrap-paragraph` that allows finding a common prefix on every
  affected/selected line, removing it before wrapping and then re-inserting it
  afterwards (also factoring it into the line width calculation)

* Allow text selection in `command` and `search` modes, so that
  `CommandLine::buf` can be quickly manipulated by using `copy` and
  `paste` with selected substrings

* Make the `default` command in `dte-syntax(5)` files act recursively
  (for example `default X Y; default Y Z` should work as expected)

* Implement a `KITTYKBD`-specialized version of `term_read_input()` that
  omits all of the Esc-prefix=Alt and Esc timing stupidity

* Allow `dte -s` option to take multiple file arguments (e.g.
  `dte -s config/syntax/*`)

* Implement auto-completion for e.g. `hi c.<Tab>`, by collecting all
  `Action::emit_name` strings from the appropriate `Syntax` (if loaded)

* Implement auto-completion for e.g. `set filetype j<Tab>`

* Make `tags -r` retain the head of the stack instead of popping it and add
  another flag (e.g. `tag -R`) to "return" in the other direction. This will
  make the "stack" more of a circular buffer, so `EditorState::bookmarks`
  should probably just be made a fixed-size array.

* Add an `exec` command to command/search mode; for use cases like e.g.
  binding `C-r` to an `fzf` history menu (and inserting output into the
  command line)

* Bind `tab` to auto-complete search history in `search` mode (similar to
  the existing `up`/`down` bindings)

* Suggest programs found in `$PATH` (or a cached list thereof) when
  auto-completing e.g. `exec <Tab>`

* Allow highlighting auto-completion prefix with a different color. For
  example, if `:wra<Tab>` expands to `:wrap-paragraph`, the first 3 letters
  should be demarcated as the prefix.

Documentation
-------------

* For the `hi` command, mention that `gray`, `darkgray` and `white` are
  different from the names used by some other applications and perhaps
  give a brief explanation why (no real standard for aixterm "bright"
  colors, `brightblack` being nonsensical, etc.)

* Document the fact that `exec -o tag` tries to parse the first line of
  output as a `tags` file entry, before falling back to a simple tag name

* Document the fact that `exec` sets `$LINES` and `$COLUMNS` and use e.g.
  `wsplit -t; pipe-from man ls` as an example of why

* Write a better introduction to the `dte-syntax(5)` man page, which
  adequately answers the question "what is this?". This man page can
  be found simply by browsing the website, so going immediately into
  technical detail could be confusing for people who are just casually
  evaluating the project.

* Cross-reference `clear` and `delete-line` commands (mention how they're
  similar and how they're different)

* Mention POSIX exit code rationale in docs for `quit` and/or "EXIT STATUS":
  https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_21_18

* Provide dark color scheme for website
  * Define existing light scheme with `:root {color-scheme: light; ...}`
  * Replace all existing color values with `var(--xyz)`
  * Define dark scheme via `@media (prefers-color-scheme: dark) {...}`

* Make website sidebar inset (with nested flexbox) instead of occupying
  part of the `max-width` for the entire height of the main content
  (see also: https://stackoverflow.com/a/36879080)

* Document `$DTE_LOG` and `$DTE_LOG_LEVEL`

* Mention GNU Coding Standards §4.4 in `errorfmt` documentation
  (see: https://www.gnu.org/prep/standards/html_node/Errors.html)

Efficiency Improvements
-----------------------

* Add a `ptr_array_reserve()` method and use to pre-allocate space,
  so that pointers can be mass-appended directly instead of via
  `ptr_array_append()`

* Replace most uses of `BSEARCH()` with [`gperf`]-generated "perfect" hashes

* Optimize `COND_CHAR` case in `highlight_line()` to use `memchr(3)`, if
  `ConditionData::bitset` population count is 255 and the destination is
  the current state (i.e. as set by `char -n . this`)

* Use `sysconf(_SC_PAGESIZE)` instead of hard-coding `4096` (or multiples
  thereof) in various places?

* Use [`ltrace(1)`] to find and reduce unnecessary library calls

* Check `TermOutputBuffer::cursor_style` fields inidividually in
  `term_set_cursor_style()` and only set each one if the parameter
  changes the current values (currently done in `update_cursor_style()`,
  but in a less granular fashion)

* Reduce the size of `ConditionData` by making `::bitset` and `::str`
  pointers to interned objects, instead of embedded value types

* For `HashSet` objects that live for the whole duration of the process
  and don't need to support deletions, re-implement as a collection of
  interned strings (sharing the interned string allocation)

* Add a flag to `CommandRunner` that makes `run_commands()` return early
  (without executing more commands) if `run_command()` returns `false`.
  This may be useful for `macro play`, `load_syntax_file()`, etc.

* Add a helper function (e.g. `pick_flag()`) that abstracts away the
  code for handling mutually exclusive subsets of `Command` flags
  (e.g. `save [-b|-B]`, `paste [-a|-c]`, etc.)


[`gperf`]: https://www.gnu.org/software/gperf/
[`ltrace(1)`]: https://man7.org/linux/man-pages/man1/ltrace.1.html
