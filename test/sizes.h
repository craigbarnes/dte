#ifndef TEST_SIZES_H
#define TEST_SIZES_H

#include "command/cache.h"
#include "command/run.h"
#include "compiler.h"
#include "editor.h"
#include "editorconfig/editorconfig.h"
#include "editorconfig/ini.h"
#include "indent.h"
#include "load-save.h"
#include "regexp.h"
#include "selection.h"
#include "spawn.h"
#include "syntax/syntax.h"
#include "terminal/parse.h"
#include "terminal/terminal.h"
#include "ui.h"
#include "util/exitcode.h"

#define entry(type, level) { \
    .name = #type, \
    .size = sizeof(type), \
    .indent_level = level \
}

#define S(type) entry(type, 0)
#define S1(type) entry(type, 1)
#define S2(type) entry(type, 2)
#define DIVIDER {NULL, 0, 0}

static const struct {
    const char *name;
    unsigned int size;
    uint8_t indent_level;
} ssizes[] = {
    S(EditorState),
        S1(Terminal),
            S2(TermOutputBuffer),
            S2(TermInputBuffer),
        S1(ErrorBuffer),
        S1(StyleMap),
            S2(TermStyle),
        S1(CommandLine),
            S2(CompletionState),
        S1(Clipboard),
        S1(FileHistory),
        S1(FileLocksContext),
        S1(GlobalOptions),
        S1(History),
        S1(MacroRecorder),
        S1(MessageList),
        S1(ModeHandler),
        S1(SearchState),
        S1(SyntaxLoader),
        S1(TagFile),

    DIVIDER,
    S(Buffer),
        S1(FileInfo),
        S1(Change),
        S1(LocalOptions),

    DIVIDER,
    S(View),
        S1(BlockIter),

    DIVIDER,
    S(Window),
    S(Frame),
    S(Block),

    DIVIDER,
    S(Command),
    S(CachedCommand),
        S1(CommandArgs),
    S(CommandRunner),
    S(CommandSet),

    DIVIDER,
    S(Syntax),
    S(State),
    S(Condition),
        S1(ConditionData),
        S1(Action),
    S(HeredocState),
    S(StringList),

    DIVIDER,
    S(Compiler),
    S(EditorConfigOptions),
    S(ErrorFormat),
    S(FileHistoryEntry),
    S(FileLocation),
    S(FileSaveContext),
    S(HistoryEntry),
    S(IndentInfo),
    S(IniParser),
    S(InternedRegexp),
        S1(regex_t),
    S(Message),
    S(ScreenState),
    S(SelectionInfo),
    S(SpawnContext),
    S(Tag),
    S(TermControlParams),
    S(TermCursorStyle),
};

/*
For a list of typedef'd struct names, see:

    make tags && readtags -lF '(if
        (and (eq? $kind "t") (prefix? $typeref "struct"))
        (list $name #t)
        #f
    )'

See also:
 • make print-struct-sizes
 • build/test/test -h
 • build/test/test -s
*/

static inline ExitCode print_struct_sizes(void)
{
    puts (
        "\n"
        "---------------------------------\n"
        " Struct                     Size\n"
        "---------------------------------"
    );

    for (size_t i = 0; i < ARRAYLEN(ssizes); i++) {
        const char *name = ssizes[i].name;
        int indent = 3 * ssizes[i].indent_level;
        if (name) {
            printf("%*s %-*s  %5u\n", indent, "", 24 - indent, name, ssizes[i].size);
        } else {
            putchar('\n');
        }
    }

    putchar('\n');
    return EC_OK;
}

#endif
