
#ifndef MODULE_SSH2_CHANNEL_H
#define MODULE_SSH2_CHANNEL_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <libssh2.h>

int l_module_ssh_channel_close(lua_State *L);
int l_module_ssh_channel_direct_tcpip(lua_State *L);
int l_module_ssh_channel_direct_tcpip_ex(lua_State *L);
int l_module_ssh_channel_eof(lua_State *L);
int l_module_ssh_channel_exec(lua_State *L);
int l_module_ssh_channel_flush(lua_State *L);
int l_module_ssh_channel_flush_ex(lua_State *L);
int l_module_ssh_channel_flush_stderr(lua_State *L);
int l_module_ssh_channel_forward_accept(lua_State *L);
int l_module_ssh_channel_forward_cancel(lua_State *L);
int l_module_ssh_channel_forward_listen(lua_State *L);
int l_module_ssh_channel_forward_listen_ex(lua_State *L);
int l_module_ssh_channel_free(lua_State *L);
int l_module_ssh_channel_get_exit_signal(lua_State *L);
int l_module_ssh_channel_get_exit_status(lua_State *L);
int l_module_ssh_channel_handle_extended_data(lua_State *L);  // _handle_extended_data2 used
int l_module_ssh_channel_ignore_extended_data(lua_State *L);
int l_module_ssh_channel_open_ex(lua_State *L);
int l_module_ssh_channel_open_session(lua_State *L);
int l_module_ssh_channel_process_startup(lua_State *L);
int l_module_ssh_channel_read(lua_State *L);
int l_module_ssh_channel_read_ex(lua_State *L);
int l_module_ssh_channel_read_stderr(lua_State *L);
int l_module_ssh_channel_receive_window_adjust(lua_State *L);
int l_module_ssh_channel_request_auth_agent(lua_State *L);
int l_module_ssh_channel_request_pty(lua_State *L);
int l_module_ssh_channel_request_pty_ex(lua_State *L);
int l_module_ssh_channel_request_pty_size(lua_State *L);
int l_module_ssh_channel_request_pty_size_ex(lua_State *L);
int l_module_ssh_channel_send_eof(lua_State *L);
int l_module_ssh_channel_set_blocking(lua_State *L);
int l_module_ssh_channel_setenv(lua_State *L);
int l_module_ssh_channel_setenv_ex(lua_State *L);
int l_module_ssh_channel_shell(lua_State *L);
int l_module_ssh_channel_subsystem(lua_State *L);
int l_module_ssh_channel_wait_closed(lua_State *L);
int l_module_ssh_channel_wait_eof(lua_State *L);
int l_module_ssh_channel_window_read(lua_State *L);
int l_module_ssh_channel_window_read_ex(lua_State *L);
int l_module_ssh_channel_window_write(lua_State *L);
int l_module_ssh_channel_window_write_ex(lua_State *L);
int l_module_ssh_channel_write(lua_State *L);
int l_module_ssh_channel_write_ex(lua_State *L);
int l_module_ssh_channel_write_stderr(lua_State *L);
int l_module_ssh_channel_x11_req(lua_State *L);
int l_module_ssh_channel_x11_req_ex(lua_State *L);

#endif // MODULE_SSH2_CHANNEL_H
