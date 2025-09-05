#ifndef TEST_SIZES_H
#define TEST_SIZES_H

#include "compiler.h"
#include "editor.h"
#include "editorconfig/editorconfig.h"
#include "editorconfig/ini.h"
#include "indent.h"
#include "load-save.h"
#include "selection.h"
#include "spawn.h"
#include "syntax/syntax.h"
#include "terminal/parse.h"
#include "terminal/terminal.h"
#include "ui.h"
#include "util/exitcode.h"
#include "util/string.h"

#define S(type) {#type, sizeof(type)}
#define DIVIDER {NULL, 0}

static const struct {
    const char *name;
    unsigned int size;
} ssizes[] = {
    S(EditorState),
    S(Buffer),
    S(CommandLine),
    S(ErrorBuffer),
    S(FileHistory),
    S(FileInfo),
    S(FileLocksContext),
    S(Frame),
    S(GlobalOptions),
    S(History),
    S(LocalOptions),
    S(MacroRecorder),
    S(MessageList),
    S(ModeHandler),
    S(SearchState),
    S(StyleMap),
    S(SyntaxLoader),
    S(TagFile),
    S(TermInputBuffer),
    S(TermOutputBuffer),
    S(Terminal),
    S(View),
    S(Window),
    DIVIDER,

    S(Block),
    S(BlockIter),
    S(Change),
    S(Command),
    S(CommandArgs),
    S(CommandRunner),
    S(CommandSet),
    S(Compiler),
    S(CompletionState),
    S(EditorConfigOptions),
    S(ErrorFormat),
    S(FileHistoryEntry),
    S(FileLocation),
    S(FileSaveContext),
    S(HistoryEntry),
    S(IndentInfo),
    S(IniParser),
    S(InternedRegexp),
    S(Message),
    S(ScreenState),
    S(SelectionInfo),
    S(SpawnContext),
    S(Tag),
    S(TermControlParams),
    S(TermCursorStyle),
    S(TermStyle),
    DIVIDER,

    S(Syntax),
    S(State),
    S(Condition),
    S(ConditionData),
    S(HeredocState),
    S(StringList),
    DIVIDER,
};

/*
 For a list of typedef'd struct names, see:

    make tags && readtags -lF '(if
        (and (eq? $kind "t") (prefix? $typeref "struct"))
        (list $name #t)
        #f
    )'
*/

static inline ExitCode print_struct_sizes(void)
{
    puts (
        "\n"
        "-----------------------------\n"
        " Struct                 Size\n"
        "-----------------------------"
    );

    for (size_t i = 0; i < ARRAYLEN(ssizes); i++) {
        const char *name = ssizes[i].name;
        if (name) {
            printf(" %-20s  %5u\n", name, ssizes[i].size);
        } else {
            putchar('\n');
        }
    }

    return EC_OK;
}

#endif
