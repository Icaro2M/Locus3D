set pagination off
set print elements 8
set logging file out/build/qt-mingw-debug/visibility-validation-gdb.log
set logging overwrite on
set logging on
run
if $_isvoid($_exitcode)
    bt full
    thread apply all bt 8
    info registers
else
    printf "Visibility regression process exit code: %d\n", $_exitcode
end
