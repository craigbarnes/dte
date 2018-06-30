TODO
====

* Add a flag to the `bind` command to allow creating key bindings only
  for specific terminals (e.g. `bind -t ^tmux ^H 'erase-word -s'`)
* Add a flag to the `bind` command to allow binding raw escape sequences
  (e.g. `bind -r '\x1b[1;5C' word-fwd`), instead of interpretted key codes
* Allow adding text to the initial buffer opened at startup by piping to
  `stdin`
* Implement a "smart" home command that moves the cursor to the first
  non-whitespace character on the current line
* Implement macro recording and playback
* Add support for 24-bit terminal colors
* Improve automatic charset encoding detection
* Add a contextual help bar for command mode
* Add better documentation for custom color schemes
* Add a plain-text only option for the `search` and `replace` commands
* Add support for EditorConfig files
* Allow custom key bindings in command mode
* Allow adding custom auto-completions for command aliases
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
