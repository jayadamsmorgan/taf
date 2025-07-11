local http = require("taf-http")

local M = {}

M.low = http

--- @alias setoptfunc fun(self: http_handle, curlopt: integer, value: boolean|integer|string|[string]|function): http_handle
--- @alias performfunc fun(self:http_handle)
--- @alias cleanupfunc fun(self:http_handle)

--- @class http_handle
--- @field setopt setoptfunc pretty much cURL easy setopt (chainable)
--- @field perform performfunc pretty much cURL easy perform
--- @field cleanup cleanupfunc cleanup after done using (also invoked by GC)

--- @return http_handle
M.new = function()
	return http:new()
end

local OPTTYPE_LONG = 0
local OPTTYPE_OBJECTPOINT = 10000
local OPTTYPE_FUNCTIONPOINT = 20000
local OPTTYPE_OFF_T = 30000
local OPTTYPE_BLOB = 40000

local OPTTYPE_STRINGPOINT = OPTTYPE_OBJECTPOINT
local OPTTYPE_SLISTPOINT = OPTTYPE_OBJECTPOINT
local OPTTYPE_CBPOINT = OPTTYPE_OBJECTPOINT
local OPTTYPE_VALUES = OPTTYPE_LONG

M.OPT_WRITEDATA = OPTTYPE_OBJECTPOINT + 1
M.OPT_URL = OPTTYPE_OBJECTPOINT + 2
M.OPT_PORT = OPTTYPE_LONG + 3
M.OPT_PROXY = OPTTYPE_OBJECTPOINT + 4
M.OPT_USERPWD = OPTTYPE_OBJECTPOINT + 5
M.OPT_PROXYUSERPWD = OPTTYPE_STRINGPOINT + 6
M.OPT_RANGE = OPTTYPE_STRINGPOINT + 7
M.OPT_READDATA = OPTTYPE_CBPOINT + 9
M.OPT_ERRORBUFFER = OPTTYPE_OBJECTPOINT + 10
M.OPT_WRITEFUNCTION = OPTTYPE_FUNCTIONPOINT + 11
M.OPT_READFUNCTION = OPTTYPE_FUNCTIONPOINT + 12
M.OPT_TIMEOUT = OPTTYPE_LONG + 13
M.OPT_INFILESIZE = OPTTYPE_LONG + 14
M.OPT_POSTFIELDS = OPTTYPE_OBJECTPOINT + 15
M.OPT_REFERER = OPTTYPE_STRINGPOINT + 16
M.OPT_FTPPORT = OPTTYPE_STRINGPOINT + 17
M.OPT_USERAGENT = OPTTYPE_STRINGPOINT + 18
M.OPT_LOW_SPEED_LIMIT = OPTTYPE_LONG + 19
M.OPT_LOW_SPEED_TIME = OPTTYPE_LONG + 20
M.OPT_RESUME_FROM = OPTTYPE_LONG + 21
M.OPT_COOKIE = OPTTYPE_STRINGPOINT + 22
M.OPT_HTTPHEADER = OPTTYPE_SLISTPOINT + 23
M.OPT_SSLCERT = OPTTYPE_STRINGPOINT + 25
M.OPT_KEYPASSWD = OPTTYPE_STRINGPOINT + 26
M.OPT_CRLF = OPTTYPE_LONG + 27
M.OPT_QUOTE = OPTTYPE_SLISTPOINT + 28
M.OPT_HEADERDATA = OPTTYPE_CBPOINT + 29
M.OPT_COOKIEFILE = OPTTYPE_STRINGPOINT + 31
M.OPT_SSLVERSION = OPTTYPE_VALUES + 32
M.OPT_TIMECONDITION = OPTTYPE_VALUES + 33
M.OPT_TIMEVALUE = OPTTYPE_LONG + 34
M.OPT_CUSTOMREQUEST = OPTTYPE_STRINGPOINT + 36
M.OPT_STDERR = OPTTYPE_OBJECTPOINT + 37
M.OPT_POSTQUOTE = OPTTYPE_SLISTPOINT + 39
M.OPT_VERBOSE = OPTTYPE_LONG + 41
M.OPT_HEADER = OPTTYPE_LONG + 42
M.OPT_NOPROGRESS = OPTTYPE_LONG + 43
M.OPT_NOBODY = OPTTYPE_LONG + 44
M.OPT_FAILONERROR = OPTTYPE_LONG + 45
M.OPT_UPLOAD = OPTTYPE_LONG + 46
M.OPT_POST = OPTTYPE_LONG + 47
M.OPT_DIRLISTONLY = OPTTYPE_LONG + 48
M.OPT_APPEND = OPTTYPE_LONG + 50
M.OPT_NETRC = OPTTYPE_VALUES + 51
M.OPT_FOLLOWLOCATION = OPTTYPE_LONG + 52
M.OPT_TRANSFERTEXT = OPTTYPE_LONG + 53
M.OPT_XFERINFODATA = OPTTYPE_CBPOINT + 57
M.OPT_PROGRESSDATA = OPTTYPE_CBPOINT + 57
M.OPT_AUTOREFERER = OPTTYPE_LONG + 58
M.OPT_PROXYPORT = OPTTYPE_LONG + 59
M.OPT_POSTFIELDSIZE = OPTTYPE_LONG + 60
M.OPT_HTTPPROXYTUNNEL = OPTTYPE_LONG + 61
M.OPT_INTERFACE = OPTTYPE_STRINGPOINT + 62
M.OPT_KRBLEVEL = OPTTYPE_STRINGPOINT + 63
M.OPT_SSL_VERIFYPEER = OPTTYPE_LONG + 64
M.OPT_CAINFO = OPTTYPE_STRINGPOINT + 65
M.OPT_MAXREDIRS = OPTTYPE_LONG + 68
M.OPT_FILETIME = OPTTYPE_LONG + 69
M.OPT_TELNETOPTIONS = OPTTYPE_SLISTPOINT + 70
M.OPT_MAXCONNECTS = OPTTYPE_LONG + 71
M.OPT_FRESH_CONNECT = OPTTYPE_LONG + 74
M.OPT_FORBID_REUSE = OPTTYPE_LONG + 75
M.OPT_CONNECTTIMEOUT = OPTTYPE_LONG + 78
M.OPT_HEADERFUNCTION = OPTTYPE_FUNCTIONPOINT + 79
M.OPT_HTTPGET = OPTTYPE_LONG + 80
M.OPT_SSL_VERIFYHOST = OPTTYPE_LONG + 81
M.OPT_COOKIEJAR = OPTTYPE_STRINGPOINT + 82
M.OPT_SSL_CIPHER_LIST = OPTTYPE_STRINGPOINT + 83
M.OPT_HTTP_VERSION = OPTTYPE_VALUES + 84
M.OPT_FTP_USE_EPSV = OPTTYPE_LONG + 85
M.OPT_SSLCERTTYPE = OPTTYPE_STRINGPOINT + 86
M.OPT_SSLKEY = OPTTYPE_STRINGPOINT + 87
M.OPT_SSLKEYTYPE = OPTTYPE_STRINGPOINT + 88
M.OPT_SSLENGINE = OPTTYPE_STRINGPOINT + 89
M.OPT_SSLENGINE_DEFAULT = OPTTYPE_LONG + 90
M.OPT_DNS_CACHE_TIMEOUT = OPTTYPE_LONG + 92
M.OPT_PREQUOTE = OPTTYPE_SLISTPOINT + 93
M.OPT_DEBUGFUNCTION = OPTTYPE_FUNCTIONPOINT + 94
M.OPT_DEBUGDATA = OPTTYPE_CBPOINT + 95
M.OPT_COOKIESESSION = OPTTYPE_LONG + 96
M.OPT_CAPATH = OPTTYPE_STRINGPOINT + 97
M.OPT_BUFFERSIZE = OPTTYPE_LONG + 98
M.OPT_NOSIGNAL = OPTTYPE_LONG + 99
M.OPT_SHARE = OPTTYPE_OBJECTPOINT + 100
M.OPT_PROXYTYPE = OPTTYPE_VALUES + 101
M.OPT_ACCEPT_ENCODING = OPTTYPE_STRINGPOINT + 102
M.OPT_PRIVATE = OPTTYPE_OBJECTPOINT + 103
M.OPT_HTTP200ALIASES = OPTTYPE_SLISTPOINT + 104
M.OPT_UNRESTRICTED_AUTH = OPTTYPE_LONG + 105
M.OPT_FTP_USE_EPRT = OPTTYPE_LONG + 106
M.OPT_HTTPAUTH = OPTTYPE_VALUES + 107
M.OPT_SSL_CTX_FUNCTION = OPTTYPE_FUNCTIONPOINT + 108
M.OPT_SSL_CTX_DATA = OPTTYPE_CBPOINT + 109
M.OPT_FTP_CREATE_MISSING_DIRS = OPTTYPE_LONG + 110
M.OPT_PROXYAUTH = OPTTYPE_VALUES + 111
M.OPT_SERVER_RESPONSE_TIMEOUT = OPTTYPE_LONG + 112
M.OPT_IPRESOLVE = OPTTYPE_VALUES + 113
M.OPT_MAXFILESIZE = OPTTYPE_LONG + 114
M.OPT_INFILESIZE_LARGE = OPTTYPE_OFF_T + 115
M.OPT_RESUME_FROM_LARGE = OPTTYPE_OFF_T + 116
M.OPT_MAXFILESIZE_LARGE = OPTTYPE_OFF_T + 117
M.OPT_NETRC_FILE = OPTTYPE_STRINGPOINT + 118
M.OPT_USE_SSL = OPTTYPE_VALUES + 119
M.OPT_POSTFIELDSIZE_LARGE = OPTTYPE_OFF_T + 120
M.OPT_TCP_NODELAY = OPTTYPE_LONG + 121
M.OPT_FTPSSLAUTH = OPTTYPE_VALUES + 129
M.OPT_FTP_ACCOUNT = OPTTYPE_STRINGPOINT + 134
M.OPT_COOKIELIST = OPTTYPE_STRINGPOINT + 135
M.OPT_IGNORE_CONTENT_LENGTH = OPTTYPE_LONG + 136
M.OPT_FTP_SKIP_PASV_IP = OPTTYPE_LONG + 137
M.OPT_FTP_FILEMETHOD = OPTTYPE_VALUES + 138
M.OPT_LOCALPORT = OPTTYPE_LONG + 139
M.OPT_LOCALPORTRANGE = OPTTYPE_LONG + 140
M.OPT_CONNECT_ONLY = OPTTYPE_LONG + 141
M.OPT_MAX_SEND_SPEED_LARGE = OPTTYPE_OFF_T + 145
M.OPT_MAX_RECV_SPEED_LARGE = OPTTYPE_OFF_T + 146
M.OPT_FTP_ALTERNATIVE_TO_USER = OPTTYPE_STRINGPOINT + 147
M.OPT_SOCKOPTFUNCTION = OPTTYPE_FUNCTIONPOINT + 148
M.OPT_SOCKOPTDATA = OPTTYPE_CBPOINT + 149
M.OPT_SSL_SESSIONID_CACHE = OPTTYPE_LONG + 150
M.OPT_SSH_AUTH_TYPES = OPTTYPE_VALUES + 151
M.OPT_SSH_PUBLIC_KEYFILE = OPTTYPE_STRINGPOINT + 152
M.OPT_SSH_PRIVATE_KEYFILE = OPTTYPE_STRINGPOINT + 153
M.OPT_FTP_SSL_CCC = OPTTYPE_LONG + 154
M.OPT_TIMEOUT_MS = OPTTYPE_LONG + 155
M.OPT_CONNECTTIMEOUT_MS = OPTTYPE_LONG + 156
M.OPT_HTTP_TRANSFER_DECODING = OPTTYPE_LONG + 157
M.OPT_HTTP_CONTENT_DECODING = OPTTYPE_LONG + 158
M.OPT_NEW_FILE_PERMS = OPTTYPE_LONG + 159
M.OPT_NEW_DIRECTORY_PERMS = OPTTYPE_LONG + 160
M.OPT_POSTREDIR = OPTTYPE_VALUES + 161
M.OPT_SSH_HOST_PUBLIC_KEY_MD5 = OPTTYPE_STRINGPOINT + 162
M.OPT_OPENSOCKETFUNCTION = OPTTYPE_FUNCTIONPOINT + 163
M.OPT_OPENSOCKETDATA = OPTTYPE_CBPOINT + 164
M.OPT_COPYPOSTFIELDS = OPTTYPE_OBJECTPOINT + 165
M.OPT_PROXY_TRANSFER_MODE = OPTTYPE_LONG + 166
M.OPT_SEEKFUNCTION = OPTTYPE_FUNCTIONPOINT + 167
M.OPT_CRLFILE = OPTTYPE_STRINGPOINT + 169
M.OPT_ISSUERCERT = OPTTYPE_STRINGPOINT + 170
M.OPT_ADDRESS_SCOPE = OPTTYPE_LONG + 171
M.OPT_CERTINFO = OPTTYPE_LONG + 172
M.OPT_USERNAME = OPTTYPE_STRINGPOINT + 173
M.OPT_PASSWORD = OPTTYPE_STRINGPOINT + 174
M.OPT_PROXYUSERNAME = OPTTYPE_STRINGPOINT + 175
M.OPT_PROXYPASSWORD = OPTTYPE_STRINGPOINT + 176
M.OPT_NOPROXY = OPTTYPE_STRINGPOINT + 177
M.OPT_TFTP_BLKSIZE = OPTTYPE_LONG + 178
M.OPT_SOCKS5_GSSAPI_NEC = OPTTYPE_LONG + 180
M.OPT_SSH_KNOWNHOSTS = OPTTYPE_STRINGPOINT + 183
M.OPT_SSH_KEYFUNCTION = OPTTYPE_FUNCTIONPOINT + 184
M.OPT_SSH_KEYDATA = OPTTYPE_CBPOINT + 185
M.OPT_MAIL_FROM = OPTTYPE_STRINGPOINT + 186
M.OPT_MAIL_RCPT = OPTTYPE_SLISTPOINT + 187
M.OPT_FTP_USE_PRET = OPTTYPE_LONG + 188
M.OPT_RTSP_REQUEST = OPTTYPE_VALUES + 189
M.OPT_RTSP_SESSION_ID = OPTTYPE_STRINGPOINT + 190
M.OPT_RTSP_STREAM_URI = OPTTYPE_STRINGPOINT + 191
M.OPT_RTSP_TRANSPORT = OPTTYPE_STRINGPOINT + 192
M.OPT_RTSP_CLIENT_CSEQ = OPTTYPE_LONG + 193
M.OPT_RTSP_SERVER_CSEQ = OPTTYPE_LONG + 194
M.OPT_INTERLEAVEDATA = OPTTYPE_CBPOINT + 195
M.OPT_INTERLEAVEFUNCTION = OPTTYPE_FUNCTIONPOINT + 196
M.OPT_WILDCARDMATCH = OPTTYPE_LONG + 197
M.OPT_CHUNK_BGN_FUNCTION = OPTTYPE_FUNCTIONPOINT + 198
M.OPT_CHUNK_END_FUNCTION = OPTTYPE_FUNCTIONPOINT + 199
M.OPT_FNMATCH_FUNCTION = OPTTYPE_FUNCTIONPOINT + 200
M.OPT_CHUNK_DATA = OPTTYPE_CBPOINT + 201
M.OPT_FNMATCH_DATA = OPTTYPE_CBPOINT + 202
M.OPT_RESOLVE = OPTTYPE_SLISTPOINT + 203
M.OPT_TLSAUTH_USERNAME = OPTTYPE_STRINGPOINT + 204
M.OPT_TLSAUTH_PASSWORD = OPTTYPE_STRINGPOINT + 205
M.OPT_TLSAUTH_TYPE = OPTTYPE_STRINGPOINT + 206
M.OPT_TRANSFER_ENCODING = OPTTYPE_LONG + 207
M.OPT_CLOSESOCKETFUNCTION = OPTTYPE_FUNCTIONPOINT + 208
M.OPT_CLOSESOCKETDATA = OPTTYPE_CBPOINT + 209
M.OPT_GSSAPI_DELEGATION = OPTTYPE_VALUES + 210
M.OPT_DNS_SERVERS = OPTTYPE_STRINGPOINT + 211
M.OPT_ACCEPTTIMEOUT_MS = OPTTYPE_LONG + 212
M.OPT_TCP_KEEPALIVE = OPTTYPE_LONG + 213
M.OPT_TCP_KEEPIDLE = OPTTYPE_LONG + 214
M.OPT_TCP_KEEPINTVL = OPTTYPE_LONG + 215
M.OPT_SSL_OPTIONS = OPTTYPE_VALUES + 216
M.OPT_MAIL_AUTH = OPTTYPE_STRINGPOINT + 217
M.OPT_SASL_IR = OPTTYPE_LONG + 218
M.OPT_XFERINFOFUNCTION = OPTTYPE_FUNCTIONPOINT + 219
M.OPT_XOAUTH2_BEARER = OPTTYPE_STRINGPOINT + 220
M.OPT_DNS_INTERFACE = OPTTYPE_STRINGPOINT + 221
M.OPT_DNS_LOCAL_IP4 = OPTTYPE_STRINGPOINT + 222
M.OPT_DNS_LOCAL_IP6 = OPTTYPE_STRINGPOINT + 223
M.OPT_LOGIN_OPTIONS = OPTTYPE_STRINGPOINT + 224
M.OPT_SSL_ENABLE_ALPN = OPTTYPE_LONG + 226
M.OPT_EXPECT_100_TIMEOUT_MS = OPTTYPE_LONG + 227
M.OPT_PROXYHEADER = OPTTYPE_SLISTPOINT + 228
M.OPT_HEADEROPT = OPTTYPE_VALUES + 229
M.OPT_PINNEDPUBLICKEY = OPTTYPE_STRINGPOINT + 230
M.OPT_UNIX_SOCKET_PATH = OPTTYPE_STRINGPOINT + 231
M.OPT_SSL_VERIFYSTATUS = OPTTYPE_LONG + 232
M.OPT_SSL_FALSESTART = OPTTYPE_LONG + 233
M.OPT_PATH_AS_IS = OPTTYPE_LONG + 234
M.OPT_PROXY_SERVICE_NAME = OPTTYPE_STRINGPOINT + 235
M.OPT_SERVICE_NAME = OPTTYPE_STRINGPOINT + 236
M.OPT_PIPEWAIT = OPTTYPE_LONG + 237
M.OPT_DEFAULT_PROTOCOL = OPTTYPE_STRINGPOINT + 238
M.OPT_STREAM_WEIGHT = OPTTYPE_LONG + 239
M.OPT_STREAM_DEPENDS = OPTTYPE_OBJECTPOINT + 240
M.OPT_STREAM_DEPENDS_E = OPTTYPE_OBJECTPOINT + 241
M.OPT_TFTP_NO_OPTIONS = OPTTYPE_LONG + 242
M.OPT_CONNECT_TO = OPTTYPE_SLISTPOINT + 243
M.OPT_TCP_FASTOPEN = OPTTYPE_LONG + 244
M.OPT_KEEP_SENDING_ON_ERROR = OPTTYPE_LONG + 245
M.OPT_PROXY_CAINFO = OPTTYPE_STRINGPOINT + 246
M.OPT_PROXY_CAPATH = OPTTYPE_STRINGPOINT + 247
M.OPT_PROXY_SSL_VERIFYPEER = OPTTYPE_LONG + 248
M.OPT_PROXY_SSL_VERIFYHOST = OPTTYPE_LONG + 249
M.OPT_PROXY_SSLVERSION = OPTTYPE_VALUES + 250
M.OPT_PROXY_TLSAUTH_USERNAME = OPTTYPE_STRINGPOINT + 251
M.OPT_PROXY_TLSAUTH_PASSWORD = OPTTYPE_STRINGPOINT + 252
M.OPT_PROXY_TLSAUTH_TYPE = OPTTYPE_STRINGPOINT + 253
M.OPT_PROXY_SSLCERT = OPTTYPE_STRINGPOINT + 254
M.OPT_PROXY_SSLCERTTYPE = OPTTYPE_STRINGPOINT + 255
M.OPT_PROXY_SSLKEY = OPTTYPE_STRINGPOINT + 256
M.OPT_PROXY_SSLKEYTYPE = OPTTYPE_STRINGPOINT + 257
M.OPT_PROXY_KEYPASSWD = OPTTYPE_STRINGPOINT + 258
M.OPT_PROXY_SSL_CIPHER_LIST = OPTTYPE_STRINGPOINT + 259
M.OPT_PROXY_CRLFILE = OPTTYPE_STRINGPOINT + 260
M.OPT_PROXY_SSL_OPTIONS = OPTTYPE_LONG + 261
M.OPT_PRE_PROXY = OPTTYPE_STRINGPOINT + 262
M.OPT_PROXY_PINNEDPUBLICKEY = OPTTYPE_STRINGPOINT + 263
M.OPT_ABSTRACT_UNIX_SOCKET = OPTTYPE_STRINGPOINT + 264
M.OPT_SUPPRESS_CONNECT_HEADERS = OPTTYPE_LONG + 265
M.OPT_REQUEST_TARGET = OPTTYPE_STRINGPOINT + 266
M.OPT_SOCKS5_AUTH = OPTTYPE_LONG + 267
M.OPT_SSH_COMPRESSION = OPTTYPE_LONG + 268
M.OPT_MIMEPOST = OPTTYPE_OBJECTPOINT + 269
M.OPT_TIMEVALUE_LARGE = OPTTYPE_OFF_T + 270
M.OPT_HAPPY_EYEBALLS_TIMEOUT_MS = OPTTYPE_LONG + 271
M.OPT_RESOLVER_START_FUNCTION = OPTTYPE_FUNCTIONPOINT + 272
M.OPT_RESOLVER_START_DATA = OPTTYPE_CBPOINT + 273
M.OPT_HAPROXYPROTOCOL = OPTTYPE_LONG + 274
M.OPT_DNS_SHUFFLE_ADDRESSES = OPTTYPE_LONG + 275
M.OPT_TLS13_CIPHERS = OPTTYPE_STRINGPOINT + 276
M.OPT_DISALLOW_USERNAME_IN_URL = OPTTYPE_LONG + 278
M.OPT_DOH_URL = OPTTYPE_STRINGPOINT + 279
M.OPT_UPLOAD_BUFFERSIZE = OPTTYPE_LONG + 280
M.OPT_UPKEEP_INTERVAL_MS = OPTTYPE_LONG + 281
M.OPT_CURLU = OPTTYPE_OBJECTPOINT + 282
M.OPT_TRAILERFUNCTION = OPTTYPE_FUNCTIONPOINT + 283
M.OPT_TRAILERDATA = OPTTYPE_CBPOINT + 284
M.OPT_HTTP09_ALLOWED = OPTTYPE_LONG + 285
M.OPT_ALTSVC_CTRL = OPTTYPE_LONG + 286
M.OPT_ALTSVC = OPTTYPE_STRINGPOINT + 287
M.OPT_MAXAGE_CONN = OPTTYPE_LONG + 288
M.OPT_SASL_AUTHZID = OPTTYPE_STRINGPOINT + 289
M.OPT_MAIL_RCPT_ALLOWFAILS = OPTTYPE_LONG + 290
M.OPT_SSLCERT_BLOB = OPTTYPE_BLOB + 291
M.OPT_SSLKEY_BLOB = OPTTYPE_BLOB + 292
M.OPT_PROXY_SSLCERT_BLOB = OPTTYPE_BLOB + 293
M.OPT_PROXY_SSLKEY_BLOB = OPTTYPE_BLOB + 294
M.OPT_ISSUERCERT_BLOB = OPTTYPE_BLOB + 295
M.OPT_PROXY_ISSUERCERT = OPTTYPE_STRINGPOINT + 296
M.OPT_PROXY_ISSUERCERT_BLOB = OPTTYPE_BLOB + 297
M.OPT_SSL_EC_CURVES = OPTTYPE_STRINGPOINT + 298
M.OPT_HSTS_CTRL = OPTTYPE_LONG + 299
M.OPT_HSTS = OPTTYPE_STRINGPOINT + 300
M.OPT_HSTSREADFUNCTION = OPTTYPE_FUNCTIONPOINT + 301
M.OPT_HSTSREADDATA = OPTTYPE_CBPOINT + 302
M.OPT_HSTSWRITEFUNCTION = OPTTYPE_FUNCTIONPOINT + 303
M.OPT_HSTSWRITEDATA = OPTTYPE_CBPOINT + 304
M.OPT_AWS_SIGV4 = OPTTYPE_STRINGPOINT + 305
M.OPT_DOH_SSL_VERIFYPEER = OPTTYPE_LONG + 306
M.OPT_DOH_SSL_VERIFYHOST = OPTTYPE_LONG + 307
M.OPT_DOH_SSL_VERIFYSTATUS = OPTTYPE_LONG + 308
M.OPT_CAINFO_BLOB = OPTTYPE_BLOB + 309
M.OPT_PROXY_CAINFO_BLOB = OPTTYPE_BLOB + 310
M.OPT_SSH_HOST_PUBLIC_KEY_SHA256 = OPTTYPE_STRINGPOINT + 311
M.OPT_PREREQFUNCTION = OPTTYPE_FUNCTIONPOINT + 312
M.OPT_PREREQDATA = OPTTYPE_CBPOINT + 313
M.OPT_MAXLIFETIME_CONN = OPTTYPE_LONG + 314
M.OPT_MIME_OPTIONS = OPTTYPE_LONG + 315
M.OPT_SSH_HOSTKEYFUNCTION = OPTTYPE_FUNCTIONPOINT + 316
M.OPT_SSH_HOSTKEYDATA = OPTTYPE_CBPOINT + 317
M.OPT_PROTOCOLS_STR = OPTTYPE_STRINGPOINT + 318
M.OPT_REDIR_PROTOCOLS_STR = OPTTYPE_STRINGPOINT + 319
M.OPT_WS_OPTIONS = OPTTYPE_LONG + 320
M.OPT_CA_CACHE_TIMEOUT = OPTTYPE_LONG + 321
M.OPT_QUICK_EXIT = OPTTYPE_LONG + 322
M.OPT_HAPROXY_CLIENT_IP = OPTTYPE_STRINGPOINT + 323
M.OPT_SERVER_RESPONSE_TIMEOUT_MS = OPTTYPE_LONG + 324

return M
