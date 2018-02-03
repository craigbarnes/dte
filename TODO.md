TODO
====

* Add a flag to the `bind` command to allow creating key bindings only
  for specific terminals (e.g. `bind -t ^xterm ^H 'erase-word -s'`)
* Allow adding text to the initial buffer opened at startup by piping to
  `stdin`
* Implement a "smart" home command that moves the cursor to the first
  non-whitespace character on the current line
* Implement macro recording and playback
* Add support for 24-bit terminal colors
* Add a script for reproducing and updating the generated Unicode tables
  in `src/unicode.c`
* Improve automatic charset encoding detection
* Add a contextual help bar for command mode
* Add built-in support for `vt220` and `linux` terminals
* Add better documentation for custom color schemes
* Add a plain-text only (no regex) option for the `search` and `replace`
  commands
* Add support for EditorConfig files
* Allow custom key bindings in command mode
* Allow adding custom auto-completions for command aliases
* Fix the `left`, `right` and `delete` commands to behave correctly with
  combining characters.
* Add a user guide covering basic editor usage and also common issues faced
  by new users (e.g. configuring the Meta key on OS X Terminal/FreeBSD VT,
  quitting with unsaved changes, etc.)
* Gradually replace `xmalloc`/`xstrdup`/etc. with situational error recovery.
* Add more syntax highlighting definitions
   * pkg-config
   * JSON
   * YAML
   * Rust
   * C#
   * TypeScript
   * GLSL
   * Graphviz (DOT)
   * gettext
   * gnuplot
   * ninja
