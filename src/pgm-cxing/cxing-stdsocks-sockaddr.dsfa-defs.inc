/* DannyNiu/NJF, 2026-06-27. Public Domain. */

#if SOCKFAM == 4

Xmacro_Unsigned(struct sockaddr_in, sin_family)
Xmacro_String(struct sockaddr_in, sin_addr)
Xmacro_U32BE(struct sockaddr_in, sin_addr.s_addr)
Xmacro_U16BE(struct sockaddr_in, sin_port)

#elif SOCKFAM == 6

Xmacro_Unsigned(struct sockaddr_in6, sin6_family)
Xmacro_String(struct sockaddr_in6, sin6_addr)
Xmacro_U16BE(struct sockaddr_in6, sin6_port)
Xmacro_U32BE(struct sockaddr_in6, sin6_flowinfo)
Xmacro_U32BE(struct sockaddr_in6, sin6_scope_id)

#elif SOCKFAM == 256

Xmacro_Unsigned(struct sockaddr, sa_family)

#endif // SOCKFAM