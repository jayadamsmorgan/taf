#include "util/files.h"

#include <criterion/criterion.h>
#include <criterion/new/assert.h>

#include <errno.h>
#include <unistd.h>

Test(util_files, directory_exists_when_exists) {
    bool result = directory_exists("../");
    cr_expect(result == true);
}

Test(util_files, directory_exists_when_doesnt) {
    bool result = directory_exists("doesnt_exist");
    cr_expect(result == false);
}

Test(util_files, create_directory) {
    int result = create_directory("../util/test_dir", MKDIR_MODE);
    cr_expect(result == 0);
    bool result_2 = directory_exists("../util/test_dir");
    cr_expect(result_2 == true);

    // Cleanup
    result = rmdir("../util/test_dir");
    cr_expect(result == 0, "%d", errno);
    result = rmdir("../util");
    cr_expect(result == 0, "%d", errno);
}

Test(util_files, replace_symlink_when_target_exists) {
    replace_symlink("util/util_files.c", "util/util_files_link.c");

    int result = remove("util/util_files_link.c");
    cr_expect(result == 0, "%d", errno);
}

Test(util_files, replace_symlink_when_target_doesnt) {}

Test(util_files, file_find_upwards_file_exists) {}

Test(util_files, file_find_upwards_file_doesnt_exist) {}

Test(util_files, list_lua_recursive) {}
