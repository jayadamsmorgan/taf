criterion_dep = dependency('criterion', required: true)

fs = import('fs')

test_sources = files('modules/modules_taf-main.c', 'util/util_files.c')

foreach src : test_sources
    exe_name = fs.stem(src.full_path())

    exe = executable(
        exe_name,
        src,
        dependencies: [criterion_dep, taf_dep],
        install: false,
    )
    test(exe_name, exe)
endforeach
