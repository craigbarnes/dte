/*
 Testing for: #embed cpp(1) directive
 Supported by: GCC 15+, Clang 19+
 Standardized by: ISO C23 (§6.10.3, "Binary resource inclusion")

 See also:
 • https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3017.htm
 • https://gcc.gnu.org/gcc-15/changes.html#:~:text=embed,-preprocessing
 • https://clang.llvm.org/c_status.html#:~:text=embed,-N3017
 • https://gcc.gnu.org/onlinedocs/cpp/Binary-Resource-Inclusion.html
 • https://gcc.gnu.org/onlinedocs/gcc-15.1.0/gcc/Directory-Options.html#index-embed-dir
 • https://clang.llvm.org/docs/ClangCommandLineReference.html#cmdoption-clang-embed-dir
 • mk/compiler.sh
*/

// ccache currently produces stale objects when #embed is used, so deliberately
// error out for now
#error "See: https://github.com/ccache/ccache/issues/1540#issuecomment-2701575061"

#if !__has_embed("config/syntax/dte")
    #error "embed file not found; see '--embed-dir' in mk/compiler.sh"
#endif

static const char a[] = {
    #embed "config/syntax/dte" limit(9)
};

int main(void)
{
    static_assert(sizeof(a) == 9);
    return !(a[2] == 'q' && a[8] == 'c');
}
