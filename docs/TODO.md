Todo
====

This is a list of ideas for improvements to the dte codebase that don't
particularly need a [GitLab issue], either because they're nice-to-have
features that simply need some time spent on them or because they're
preliminary ideas that may not go anywhere. The title "todo" is thus
used very loosely.

Features
--------

* Add a function for recursively expanding aliases and use in `run_commands()`
  * Add a `show alias` flag to allow viewing fully expanded aliases, instead
    of just doing a single level of expansion
  * Add an [`alias`] flag to eagerly expand aliases at creation time (instead
    of at usage time)
  * Perform recursive `alias` expansion when auto-completing commands
    (see also: [#152])

* Add a command that swaps the positions of the selection anchor point and
  cursor (so that selections can be extended on both ends, without starting
  over)

* Remove `ARGERR_OPTION_ARGUMENT_NOT_SEPARATE` and make `do_parse_args()`
  handle e.g. `exec -oeval ...` like `getopt(3)` [would][Utility Syntax Guideline 6]

* Persist the most recently recorded [`macro`], by saving `EditorState::macro`
  on exit and restoring it at startup (see also: `read_history_files()` →
  `search_set_regexp()`)

* Retain the last 10 or so recorded macros and allow them to be viewed
  (with e.g. `show macro 7`) and executed (with e.g. `macro play 7`)

* Add option to [`wrap-paragraph`] to allow finding and removing a common
  prefix string from every line and then re-inserting it afterwards
  (see also: [#118])

* Allow text selection in [`command`] and [`search`] modes, so that
  `CommandLine::buf` can be quickly manipulated by using [`copy`] and
  [`paste`] with selected substrings

* Make the [`default`] command in [`dte-syntax(5)`] files act recursively
  (for example `default X Y; default Y Z` should work as expected)

* Implement a `KITTYKBD`-specialized version of `term_read_input()` that
  omits all of the Esc-prefix=Alt and Esc timing stupidity

* Allow [`dte -s`] option to take multiple file arguments (e.g.
  `dte -s config/syntax/*`)

* Implement auto-completion for e.g. `set filetype j<Tab>`

* Suggest programs found in `$PATH` (or a cached list thereof) when
  auto-completing e.g. `exec <Tab>`

* Allow defining custom auto-completions, including the ability to run
  [`exec`]-style commands that return a list of candidates

* Make `tags -r` retain the head of the stack instead of popping it and add
  another flag (e.g. `tag -R`) to "return" in the other direction. This will
  make the "stack" more of a circular buffer, so `EditorState::bookmarks`
  should probably just be made a fixed-size array.

* Add an `exec` command to command/search mode; for use cases like e.g.
  binding `C-r` to an [`fzf(1)`] history menu (and inserting output into the
  command line)

* Bind `tab` to auto-complete search history in `search` mode (similar to
  the existing `up`/`down` bindings)

* Allow highlighting auto-completion prefix with a different color. For
  example, if `:wra<Tab>` expands to `:wrap-paragraph`, the first 3 letters
  should be demarcated as the prefix.

* Add a command to allow explicitly grouping `Change`s together, similar
  to ne's [AtomicUndo]

* Add built-in syntax highlighters for:
  * [Typst]
  * [Rust]
  * [TOML]
  * [YAML]
  * [jq]
  * [fish]
  * [Slint]

* Extend the [`dte-syntax(5)`] command set, so as to provide a proper
  solution for handling e.g. [ambiguous regexp literals in Ruby][]
  (most likely some form of backtracking)

* Handle codepoints from [Unicode categories] `Ps`, `Pe`, `Pi` and `Pf`
  in `cmd_match_bracket()` (see also: [EU's list of quotation marks] and
  Unicode's [`BidiBrackets.txt`])

* Add an option to convert U+2103 (℃ ) into U+00B0 U+0043 (°C) when
  rendering to the terminal and perhaps do likewise for some other
  "ambiguous" width characters (see also: [foot#1665] and the related
  todo item below). Alternatively, use the [explicit width] part of
  [Kitty's new text scaling protocol] to make U+2103 always occupy 2
  columns.

* Add an option to allow configuring whether pastes from the terminal
  move the cursor after the inserted text (like `paste` vs `paste -m`)

* Add a command flag to allow `macro play` to keep running after errors

* Add a command flag to make `compile` interpret parsed filenames relative
  to a specified directory (e.g. `../`), perhaps by making use of `openat(3)`.
  This seems to be necessary for output produced by e.g. `ninja -C build-dir/`.

* Add a command flag to make `compile` retain a copy of the whole
  `std{out,err}` output in memory and then provide a way to insert it
  into a buffer (e.g. `show compile`)

* Add a fallback implementation for `copy -b`, for cases where the
  terminal doesn't support OSC 52 and SSH isn't in use (try [`wl-copy(1)`],
  [`xsel(1)`], [`xclip(1)`], [`pbcopy(1)`], [`termux-clipboard-set`],
  [`/dev/clipboard`])

* Add an option for rendering a "scroll indicator" on the left and/or
  right edge of the screen, when content is horizontally scrolled out of
  view (see also: [#88])

* Allow the `cut` command to set the system clipboard, like e.g. `copy -b`
  can (see also: [#183])

* Allow the `filesize-limit` option to be specified as a percentage of
  total RAM (see also: [#203])

* Allow limiting the size of loaded `tags` files (see also: [#202])

* Add support for trimming trailing whitespace on `save` (i.e. the
  EditorConfig `trim_trailing_whitespace` property)

* Add support for interactive spell checking

* Add support for extended underline styles/colors to [`hi`] command
  (see also: [#161])

* Show raw escape sequences in `dte -K` output (see also: [#151])

* Allow custom colors/attributes in the statusline (see also: [#70])

* Allow [`include`] to run *any* command when used from command mode
  or key bindings (the restrictions noted for [configuration commands]
  should only apply to `rc` files and built-in configs)

* Extend syntax highlighting with some built-in way to handle arbitrary
  nesting of things like e.g. OCaml comments (see also: [#132])

Documentation
-------------

* Briefly describe formatting conventions used in "COMMANDS" section of
  `dterc(5)` man page (and also cross-reference `man(1)`, where similar
  conventions are mentioned):
  * **Bold**: Type exactly as shown
  * _Italic_ (or underline, in `roff(7)`): Replace with appropriate argument
  * `[-abc]`: Any or all arguments within `[ ]` are optional
  * `-a|-b`: Options delimited by `|` cannot be used together
  * `argument ...`: Argument is repeatable
  * `[expression] ...`: Entire expression within `[ ]` is repeatable

* For the [`hi`] command, mention that `gray`, `darkgray` and `white` are
  different from the names used by some other applications and perhaps
  give a brief explanation why (no real standard for aixterm "bright"
  colors, `brightblack` being nonsensical, etc.)

* For the [`hi`] command, clarify what is meant by "the name argument can
  be a token name defined by a `dte-syntax(5)` file" (see also: [#179])

* Mention [built-in filetype associations] in the [`ft`] documentation
  and also describe how precedence/ordering works (see also: [#180])

* Document the fact that `exec -o tag` tries to parse the first line of
  output as a [`tags(5)`] file entry, before falling back to a simple tag name

* Document the fact that [`exec`] sets `$LINES` and `$COLUMNS` and use e.g.
  `wsplit -t; pipe-from man ls` as an example of why

* Write a better introduction to the [`dte-syntax(5)`] man page, which
  adequately answers the question "what is this?". This man page can
  be found simply by browsing the website, so going immediately into
  technical detail could be confusing for people who are just casually
  evaluating the project.

* Add a `DESCRIPTION` section to the [`dte(1)`] man page, introducing the
  program for those unfamiliar (see also: [`nano(1)`], [`micro(1)`],
  [`ed(1)`], etc.)

* Add `ASYNCHRONOUS EVENTS`, `STDOUT`, `STDERR` and `OPERANDS` sections
  to the [`dte(1)`] man page, similar to the ones in [`ed(1)`]

* Cross-reference [`clear`] and [`delete-line`] commands (mention how
  they're similar and how they're different)

* Provide dark color scheme for website
  * Define existing light scheme with `:root {color-scheme: light; ...}`
  * Replace all existing color values with `var(--xyz)`
  * Define dark scheme via `@media (prefers-color-scheme: dark) {...}`

* Make website sidebar inset (with nested flexbox) instead of occupying
  part of the `max-width` for the entire height of the main content
  (see also: https://stackoverflow.com/a/36879080)

* Document `$DTE_LOG` and `$DTE_LOG_LEVEL`

* Mention [GNU Coding Standards §4.4] in [`errorfmt`] documentation

* Document the Unicode features that don't map well to the terminal's
  "grid of cells" display model and/or multi-process division of
  responsibilities (e.g. ambiguous display width codepoints, the
  U+2103 [example][foot#1665] mentioned above, ZWJ sequences, etc.)

Code Quality/Efficiency Improvements
------------------------------------

* Add a `ptr_array_reserve()` method and use to pre-allocate space,
  so that pointers can be mass-appended directly instead of via
  `ptr_array_append()`

* Optimize `COND_CHAR` case in `highlight_line()` to use `memchr(3)`, if
  `ConditionData::bitset` population count is 255 and the destination is
  the current state (i.e. as set by `char -n . this`)

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

* Use [`writev(3)`][] (when available) in `write_buffer()`, for
  writing `Block::data` segments to files saved as UTF-8/LF (with less
  overhead from syscalls; see also: [#86])

* Use [`vmsplice(2)`][] (when available) in `handle_piped_data()`, for
  piping `Block::data` segments to spawned processes (with less overhead
  from syscalls/copying)

* Use [`posix_fallocate(3)`][] (when available) to pre-allocate disk
  space for `save_buffer()`, `history_save()` and `file_history_save()`

* Support `posix_spawn(3)` as an alternative to `fork_exec()` (see also:
  [#164])

* Use kitty's [extended clipboard protocol], when available, to allow
  writing to the clipboard in chunks (instead of as a single `OSC 52`
  string)

* Unify `History` and `FileHistory` somehow (perhaps with function
  pointers for handling extra `FileHistoryEntry` fields)

* Make more colors "built-in" (i.e. backed by the `StyleMap::builtin`
  array), so that e.g. `&styles->builtin[BSE_COMMENT]` can be used
  (instead of `find_style(styles, "comment")`) in `hl_words()`

* Optimize `update_range()` to make handling long lines more efficient,
  both by avoiding unnecessary work and by speeding up the necessary
  parts (see also: [#220])

* Compress large undo history (`Change`) entries, e.g. those created
  when an [`exec`] command rewrites the entire buffer (see also: [#142])

* Cache EditorConfig properties in memory, so that each `.editorconfig`
  file is read only once per session (see also: [#105])

* Use 2-stage tables for looking up Unicode character properties,
  instead of the binary searched interval tables in `src/util/unidata.h`
  (see also: [#147])

Testing/Debugging
-----------------

* Set up continuous testing on OpenBSD, FreeBSD and NetBSD
  (see https://hub.docker.com/r/madworx/netbsd/ for one possible
  approach to this)

* Remove `#if` guards in `test/*.c`, so that "passed" count stays the same
  regardless of compiler/platform
  (see: https://lists.nongnu.org/archive/html/tinycc-devel/2023-09/msg00033.html)

* Add test runner option for generating JUnit XML report for GitLab CI
  * https://docs.gitlab.com/ci/testing/unit_test_reports/
  * https://gitlab.com/craigbarnes/dte/-/pipelines/1404353439/test_report

* Write a `LOG_*()` message every time a new file is opened, including the
  `Buffer::id`, and then mention the `id` in other log messages relating
  to that buffer (thus allowing logged actions to be correlated to specific
  files without spamming long filenames repeatedly)

* Enable clang-tidy's [`hicpp-signed-bitwise`] check, once we start making use
  of [C23 Enhanced Enumerations] to make bitflag enums explicitly `unsigned`

* Improve error/log messages in `set_and_check_locale()`

* Improve test coverage for:
  * `handle_exec()`
  * `spawn_compiler()`
  * `dump_buffer()`
  * `src/case.c`
  * `src/cmdline.c`
  * `src/load-save.c`
  * `src/selection.c`
  * `src/show.c`
  * `src/tag.c`
  * `src/view.c`


[`dte(1)`]: https://craigbarnes.gitlab.io/dte/dte.html
[`dte -s`]: https://craigbarnes.gitlab.io/dte/dte.html#options
["EXIT STATUS"]: https://craigbarnes.gitlab.io/dte/dte.html#exit-status

[`dte-syntax(5)`]: https://craigbarnes.gitlab.io/dte/dte-syntax.html
[`default`]: https://craigbarnes.gitlab.io/dte/dte-syntax.html#default

[configuration commands]: https://craigbarnes.gitlab.io/dte/dterc.html#configuration-commands
[`alias`]: https://craigbarnes.gitlab.io/dte/dterc.html#alias
[`clear`]: https://craigbarnes.gitlab.io/dte/dterc.html#clear
[`command`]: https://craigbarnes.gitlab.io/dte/dterc.html#command
[`copy`]: https://craigbarnes.gitlab.io/dte/dterc.html#copy
[`delete-line`]: https://craigbarnes.gitlab.io/dte/dterc.html#delete-line
[`errorfmt`]: https://craigbarnes.gitlab.io/dte/dterc.html#errorfmt
[`exec`]: https://craigbarnes.gitlab.io/dte/dterc.html#exec
[`ft`]: https://craigbarnes.gitlab.io/dte/dterc.html#ft
[`hi`]: https://craigbarnes.gitlab.io/dte/dterc.html#hi
[`include`]: https://craigbarnes.gitlab.io/dte/dterc.html#include
[`macro`]: https://craigbarnes.gitlab.io/dte/dterc.html#macro
[`paste`]: https://craigbarnes.gitlab.io/dte/dterc.html#paste
[`quit`]: https://craigbarnes.gitlab.io/dte/dterc.html#quit
[`search`]: https://craigbarnes.gitlab.io/dte/dterc.html#search
[`wrap-paragraph`]: https://craigbarnes.gitlab.io/dte/dterc.html#wrap-paragraph

[GitLab issue]: https://gitlab.com/craigbarnes/dte/-/issues
[#14]: https://gitlab.com/craigbarnes/dte/-/issues/14
[#70]: https://gitlab.com/craigbarnes/dte/-/issues/70
[#86]: https://gitlab.com/craigbarnes/dte/-/issues/86
[#88]: https://gitlab.com/craigbarnes/dte/-/issues/88
[#105]: https://gitlab.com/craigbarnes/dte/-/issues/105
[#118]: https://gitlab.com/craigbarnes/dte/-/issues/118
[#132]: https://gitlab.com/craigbarnes/dte/-/issues/132
[#142]: https://gitlab.com/craigbarnes/dte/-/issues/142
[#147]: https://gitlab.com/craigbarnes/dte/-/issues/147
[#151]: https://gitlab.com/craigbarnes/dte/-/issues/151
[#152]: https://gitlab.com/craigbarnes/dte/-/issues/152
[#161]: https://gitlab.com/craigbarnes/dte/-/issues/161
[#164]: https://gitlab.com/craigbarnes/dte/-/issues/164
[#179]: https://gitlab.com/craigbarnes/dte/-/issues/179
[#180]: https://gitlab.com/craigbarnes/dte/-/issues/180
[#183]: https://gitlab.com/craigbarnes/dte/-/issues/183
[#202]: https://gitlab.com/craigbarnes/dte/-/issues/202
[#203]: https://gitlab.com/craigbarnes/dte/-/issues/203
[#220]: https://gitlab.com/craigbarnes/dte/-/issues/220
[Utility Syntax Guideline 6]: https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap12.html#tag_12_02:~:text=Guideline%C2%A06
[AtomicUndo]: https://ne.di.unimi.it/docs/AtomicUndo.html
[Typst]: https://typst.app/docs/reference/syntax/
[Rust]: https://doc.rust-lang.org/reference/
[TOML]: https://toml.io/en/v1.0.0
[YAML]: https://yaml.org/spec/1.2.2/
[jq]: https://jqlang.org/manual/#modules
[fish]: https://fishshell.com/
[Slint]: https://docs.slint.dev/latest/docs/slint/guide/language/concepts/slint-language/
[ambiguous regexp literals in Ruby]: https://stackoverflow.com/questions/38333687/what-is-ambiguous-regexp-literal-in-rubocop
[Unicode categories]: https://www.unicode.org/reports/tr44/#GC_Values_Table
[EU's list of quotation marks]: https://op.europa.eu/en/web/eu-vocabularies/formex/physical-specifications/character-encoding/quotation-marks
[`BidiBrackets.txt`]: https://www.unicode.org/reports/tr44/#BidiBrackets.txt
[explicit width]: https://github.com/kovidgoyal/kitty/issues/8226#issuecomment-2600509809
[Kitty's new text scaling protocol]: https://github.com/kovidgoyal/kitty/issues/8226
[GNU Coding Standards §4.4]: https://www.gnu.org/prep/standards/html_node/Errors.html
[foot#1665]: https://codeberg.org/dnkl/foot/issues/1665#issuecomment-1734299
[built-in filetype associations]: https://gitlab.com/craigbarnes/dte/-/tree/master/src/filetype
[`wl-copy(1)`]: https://man.archlinux.org/man/wl-copy.1
[`xsel(1)`]: https://man.archlinux.org/man/xsel.1x
[`xclip(1)`]: https://man.archlinux.org/man/xclip.1
[`pbcopy(1)`]: https://keith.github.io/xcode-man-pages/pbcopy.1.html
[`termux-clipboard-set`]: https://wiki.termux.com/wiki/Termux-clipboard-set
[`/dev/clipboard`]: https://cygwin.com/cygwin-ug-net/using-specialnames.html#pathnames-posixdevices:~:text=/dev/clipboard
[`tags(5)`]: https://man.archlinux.org/man/tags.5.en
[`nano(1)`]: https://man.archlinux.org/man/nano.1.en
[`micro(1)`]: https://man.archlinux.org/man/micro.1.en
[`fzf(1)`]: https://man.archlinux.org/man/fzf.1.en
[`ed(1)`]: https://man7.org/linux/man-pages/man1/ed.1p.html#ASYNCHRONOUS_EVENTS
[`ltrace(1)`]: https://man7.org/linux/man-pages/man1/ltrace.1.html
[`posix_fallocate(3)`]: https://pubs.opengroup.org/onlinepubs/9799919799/functions/posix_fallocate.html
[`writev(3)`]: https://pubs.opengroup.org/onlinepubs/9799919799/functions/writev.html
[`vmsplice(2)`]: https://man7.org/linux/man-pages/man2/vmsplice.2.html
[extended clipboard protocol]: https://sw.kovidgoyal.net/kitty/clipboard/
[`hicpp-signed-bitwise`]: https://clang.llvm.org/extra/clang-tidy/checks/hicpp/signed-bitwise.html
[C23 Enhanced Enumerations]: https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3030.htm
