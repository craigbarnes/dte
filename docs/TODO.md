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

* Add a command-line flag that allows e.g. `ls | dte -X` to open each
  word/line of `stdin` as if it were a file argument

* Allow `tag` command to be given multiple arguments (e.g. `tag main xmul`)
  and append tags matching any argument to `EditorState::messages` (sorted
  according to the given order)

* Add option to `wrap-paragraph` that allows finding a common prefix on every
  affected/selected line, removing it before wrapping and then re-inserting it
  afterwards (also factoring it into the line width calculation)

Documentation
-------------

* For the `hi` command, mention that `gray`, `darkgray` and `white` are
  different from the names used by some other applications and perhaps
  give a brief explanation why (no real standard for aixterm "bright"
  colors, `brightblack` being nonsensical, etc.)

* Provide dark color scheme for website
  * Define existing light scheme with `:root {color-scheme: light; ...}`
  * Replace all existing color values with `var(--xyz)`
  * Define dark scheme via `@media (prefers-color-scheme: dark) {...}`

* Document the fact that `exec -o tag` tries to parse the first line of
  output as a `tags` file entry, before falling back to a simple tag name

Efficiency Improvements
-----------------------

* Add a `ptr_array_reserve()` method and use to pre-allocate space,
  so that pointers can be mass-appended directly instead of via
  `ptr_array_append()`

* Replace most uses of `BSEARCH()` with [`gperf`]-generated "perfect" hashes

* Use `sysconf(_SC_PAGESIZE)` instead of hard-coding `4096` (or multiples
  thereof) in various places?

* Use [`ltrace(1)`] to find and reduce unnecessary library calls


[`gperf`]: https://www.gnu.org/software/gperf/
[`ltrace(1)`]: https://man7.org/linux/man-pages/man1/ltrace.1.html
