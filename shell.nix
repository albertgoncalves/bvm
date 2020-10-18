with import <nixpkgs> {};
mkShell {
    buildInputs = [
        clang_10
        cppcheck
        glibcLocales
        shellcheck
        valgrind
    ];
    shellHook = ''
        . .shellhook
    '';
}
