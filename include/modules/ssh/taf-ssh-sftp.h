
#ifndef MODULE_SSH2_SFTP_H
#define MODULE_SSH2_SFTP_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <libssh2.h>


int l_module_ssh_sftp_close(lua_State *L);
int l_module_ssh_sftp_close_handle(lua_State *L);
int l_module_ssh_sftp_closedir(lua_State *L);
int l_module_ssh_sftp_fsetstat(lua_State *L);
int l_module_ssh_sftp_fstat(lua_State *L);
int l_module_ssh_sftp_fstat_ex(lua_State *L);
int l_module_ssh_sftp_fstatvfs(lua_State *L);
int l_module_ssh_sftp_fsync(lua_State *L);
int l_module_ssh_sftp_get_channel(lua_State *L);
int l_module_ssh_sftp_init(lua_State *L);
int l_module_ssh_sftp_last_error(lua_State *L);
int l_module_ssh_sftp_lstat(lua_State *L);
int l_module_ssh_sftp_mkdir(lua_State *L);
int l_module_ssh_sftp_mkdir_ex(lua_State *L);
int l_module_ssh_sftp_open(lua_State *L);
int l_module_ssh_sftp_open_ex(lua_State *L);
int l_module_ssh_sftp_opendir(lua_State *L);
int l_module_ssh_sftp_read(lua_State *L);
int l_module_ssh_sftp_readdir(lua_State *L);
int l_module_ssh_sftp_readdir_ex(lua_State *L);
int l_module_ssh_sftp_readlink(lua_State *L);
int l_module_ssh_sftp_realpath(lua_State *L);
int l_module_ssh_sftp_rename(lua_State *L);
int l_module_ssh_sftp_rename_ex(lua_State *L);
int l_module_ssh_sftp_rewind(lua_State *L);
int l_module_ssh_sftp_rmdir(lua_State *L);
int l_module_ssh_sftp_rmdir_ex(lua_State *L);
int l_module_ssh_sftp_seek(lua_State *L);
int l_module_ssh_sftp_seek64(lua_State *L);
int l_module_ssh_sftp_setstat(lua_State *L);
int l_module_ssh_sftp_shutdown(lua_State *L);
int l_module_ssh_sftp_stat(lua_State *L);
int l_module_ssh_sftp_stat_ex(lua_State *L);
int l_module_ssh_sftp_statvfs(lua_State *L);
int l_module_ssh_sftp_symlink(lua_State *L);
int l_module_ssh_sftp_symlink_ex(lua_State *L);
int l_module_ssh_sftp_tell(lua_State *L);
int l_module_ssh_sftp_tell64(lua_State *L);
int l_module_ssh_sftp_unlink(lua_State *L);
int l_module_ssh_sftp_unlink_ex(lua_State *L);
int l_module_ssh_sftp_write(lua_State *L);

#endif // MODULE_SSH2_SFTP_H
