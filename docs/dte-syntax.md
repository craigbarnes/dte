---
title: dte-syntax
section: 5
date: November 2017
description: Format of syntax highlighting files used by `dte`
author: [Craig Barnes, Timo Hirvonen]
seealso: ["`dte`", "`dterc`"]
---

# dte-syntax

A dte syntax file consists of multiple states. A state consists of optional
conditionals and one default action. The best way understand the syntax
is to read through some of the [built-in syntax files], which can be
printed with `dte -b`, for example:

    dte -b syntax/dte

The basic syntax used is the same as in [`dterc`] files, but the
available commands are different.

Conditionals and default actions have a destination state. The special
destination state `this` can be used to jump to the current state.

# Commands

## Main commands

### **syntax** _name_

Begin a new syntax. One syntax file can contain multiple syntax
definitions, but you should only define one real syntax in one
syntax file.

See also: sub-syntaxes.

### **state** _name_ [_emit-color_]

Add new state. Conditionals (if any) and one default action must
follow. The first state is the `start` state.

### **default** _color_ _name_...

Set default _color_ for emitted _name_.

Example:

    default numeric oct dec hex

If there is no color defined for _oct_, _dec_ or _hex_ then color
_numeric_ is used instead.

### **list** [**-i**] _name_ _string_...

Define a list of strings.

Example:

    list keyword if else for while do continue switch case

Use the `inlist` command to test if a buffered string is in a list.

`-i`
:   Make list case-insensitive.

## Conditionals

### **bufis** [**-i**] _string_ _destination_ [_emit-name_]

Test if buffered bytes are same as _string_. If they are, emit
_emit-name_ and jump to _destination_ state.

### **char** [**-bn**] _characters_ _destination_ [_emit-name_]

Test if current byte is in the _characters_ list. If it is then emit
_emit-color_ and jump to _destination_ state. If _emit-name_ is not
given then the _destination_ state's emit name is used.

_characters_ is a list of strings. Ranges are supported (`a-d` is the
same as `abcd`).

`-b`
:   Add byte to buffer.

`-n`
:   Invert character bitmap.

### **heredocend** _destination_

Compare following characters to heredoc end delimiter and go to
destination state if comparison is true.

### **inlist** _list_ _destination_ [_emit-name_]

Test if buffered bytes are found in _list_. If found, emit
_emit-name_ and jump to _destination_ state.

### **str** [**-i**] _string_ _destination_ [_emit-name_]

See if following bytes are same as _string_. If they are emit
_emit-name_ and jump to _destination_ state.

`-i`
:   Case-insensitive.

NOTE: This conditional can be slow, especially if _string_ is
longer than two bytes.

## Default actions

The last command of every state must be a default action. It is an
unconditional jump.

### **eat** _destination_ [_emit-name_]

Consume byte, emit _emit-name_ color and continue to _destination_
state.

### **heredocbegin** _subsyntax_ _return-state_

Store buffered bytes as heredoc end delimiter and go to
_subsyntax_. Sub-syntax is like any other sub-syntax but it must
contain a _heredocend_ conditional.

### **noeat** [**-b**] _destination_

Continue to _destination_ state without emitting color or
consuming byte.

`-b`
:   Don't stop buffering.

## Other commands

### **recolor** _color_ [_count_]

If _count_ is given, recolor _count_ previous bytes, otherwise
recolor buffered bytes.

# Sub-syntaxes

Sub-syntaxes are useful when the same states are needed in many contexts.

Sub-syntax names must be prefixed with `.`. It's recommended to also use
the main syntax name in the prefix. For example `.c-comment` if `c` is
the main syntax.

A sub-syntax is a syntax in which some destination state's name is
`END`. `END` is a special state name that is replaced by the state
specified in another syntax.

Example:

```sh
# Sub-syntax
syntax .c-comment

state comment
    char "*" star
    eat comment

state star comment
    # END is a special state name
    char / END comment
    noeat comment

# Main syntax
syntax c

state c code
    char " \t\n" c
    char -b a-zA-Z_ ident
    char "\"" string
    char "'" char
    # Call sub-syntax
    str "/*" .c-comment:c
    eat c

# Other states removed
```

In this example the destination state `.c-comment:c` is a special syntax
for calling a sub-syntax. `.c-comment` is the name of the sub-syntax and
`c` is the return state defined in the main syntax. The whole sub-syntax
tree is copied into the main syntax and all destination states in the
sub-syntax whose name is `END` are replaced with `c`.


[`dterc`]: https://craigbarnes.gitlab.io/dte/dterc.html
[built-in syntax files]: https://github.com/craigbarnes/dte/tree/master/config/syntax
