/* DannyNiu/NJF, 2026-06-28. Public Domain. */

X_SockOpt_Bool(false, SOL_SOCKET, SO_ACCEPTCONN)
X_SockOpt_Bool(true, SOL_SOCKET, SO_BROADCAST)
X_SockOpt_Bool(true, SOL_SOCKET, SO_DEBUG)
// X_SockOpt_Int(false, SOL_SOCKET, SO_DOMAIN) // too new in Issue 8.
X_SockOpt_Bool(true, SOL_SOCKET, SO_DONTROUTE)
X_SockOpt_Int(false, SOL_SOCKET, SO_ERROR)
X_SockOpt_Bool(true, SOL_SOCKET, SO_KEEPALIVE)
X_SockOpt_DSFA(SockLinger, true, SOL_SOCKET, SO_LINGER)
X_SockOpt_Int(false, SOL_SOCKET, SO_OOBINLINE) // See spec for why it's false.
// X_SockOpt_Int(false, SOL_SOCKET, SO_PROTOCOL) // too new in Issue 8.
X_SockOpt_Int(true, SOL_SOCKET, SO_RCVBUF)
X_SockOpt_Int(true, SOL_SOCKET, SO_RCVLOWAT)
X_SockOpt_Time(true, SOL_SOCKET, SO_RCVTIMEO)
X_SockOpt_Bool(true, SOL_SOCKET, SO_REUSEADDR)
X_SockOpt_Int(true, SOL_SOCKET, SO_SNDBUF)
X_SockOpt_Int(true, SOL_SOCKET, SO_SNDLOWAT)
X_SockOpt_Time(true, SOL_SOCKET, SO_SNDTIMEO)
X_SockOpt_Int(false, SOL_SOCKET, SO_TYPE)

X_SockOpt_DSFA(IPv6MReq, true, IPPROTO_IPV6, IPV6_JOIN_GROUP)
X_SockOpt_DSFA(IPv6MReq, true, IPPROTO_IPV6, IPV6_LEAVE_GROUP)
X_SockOpt_Int(true, IPPROTO_IPV6, IPV6_MULTICAST_HOPS)
X_SockOpt_Int(true, IPPROTO_IPV6, IPV6_MULTICAST_IF)
X_SockOpt_Int(true, IPPROTO_IPV6, IPV6_MULTICAST_LOOP)
X_SockOpt_Int(true, IPPROTO_IPV6, IPV6_UNICAST_HOPS)
X_SockOpt_Int(true, IPPROTO_IPV6, IPV6_V6ONLY)