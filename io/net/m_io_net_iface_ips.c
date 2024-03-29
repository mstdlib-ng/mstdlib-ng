/* The MIT License (MIT)
 * 
 * Copyright (c) 2022 Monetra Technologies, LLC.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "m_config.h"

#include <mstdlib/mstdlib.h>
#include <mstdlib/mstdlib_io.h>
#include <mstdlib/mstdlib_net.h>

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  include <iphlpapi.h>
#  include "base/platform/m_platform_win.h"
#else
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <net/if.h>
#  if defined(__ANDROID_API__) && __ANDROID_API__ < 24
#    include "android_ifaddrs/ifaddrs.h"
#  else
#    include <ifaddrs.h>
#  endif
#  include <sys/ioctl.h>
#  include <netinet/in.h>
#endif

typedef struct {
	char                    *name;
	char                    *addr;
	M_uint8                  netmask;
	M_io_net_iface_ips_flags_t  flags;
} M_ipentry_t;

struct M_io_net_iface_ips {
	M_list_t *ips;
};

static void M_ipentry_free(void *arg)
{
	M_ipentry_t *ipentry = arg;
	if (ipentry == NULL)
		return;

	M_free(ipentry->name);
	M_free(ipentry->addr);
	M_free(ipentry);
}

/* OS might list an interface first with no address followed by an address, purge the
 * no-address entry */
static void M_ipentry_remove_noaddr(M_io_net_iface_ips_t *ips, const char *name)
{
	size_t i;

	for (i=0; i<M_list_len(ips->ips); i++) {
		const M_ipentry_t *ipentry = M_list_at(ips->ips, i);

		if (!M_str_caseeq(ipentry->name, name))
			continue;

		if (ipentry->addr != NULL)
			continue;

		M_list_remove_at(ips->ips, i);
	}
}

static void M_ipentry_add(M_io_net_iface_ips_t *ips, const char *name, const unsigned char *addr, size_t addr_len, M_uint8 netmask, M_io_net_iface_ips_flags_t flags)
{
	M_ipentry_t *ipentry    = M_malloc_zero(sizeof(*ipentry));
	char         ipaddr[40] = { 0 };

	ipentry->name        = M_strdup(name);

	if (addr != NULL) {
		if (M_io_net_bin_to_ipaddr(ipaddr, sizeof(ipaddr), addr, addr_len)) {
			ipentry->addr = M_strdup(ipaddr);
			if (addr_len == 4) {
				flags |= M_NET_IFACE_IPS_FLAG_IPV4;
			} else if (addr_len == 16) {
				flags |= M_NET_IFACE_IPS_FLAG_IPV6;
			}
			M_ipentry_remove_noaddr(ips, name);
			ipentry->netmask = netmask;
		}
	}

	ipentry->flags       = flags;
	M_list_insert(ips->ips, ipentry);
}


#ifdef _WIN32
static M_bool M_io_net_iface_ips_enumerate(M_io_net_iface_ips_t *ips, M_io_net_iface_ips_flags_t flags)
{
	ULONG                 myflags   = GAA_FLAG_INCLUDE_PREFIX /*|GAA_FLAG_INCLUDE_ALL_INTERFACES */;
	ULONG                 outBufLen = 0;
	DWORD                 retval;
	M_bool                rv        = M_FALSE;
	IP_ADAPTER_ADDRESSES *addresses = NULL;
	IP_ADAPTER_ADDRESSES *address   = NULL;

	/* Get necessary buffer size */
	GetAdaptersAddresses(AF_UNSPEC, myflags, NULL, NULL, &outBufLen);
	if (outBufLen == 0)
		return M_FALSE;

	addresses = M_malloc_zero(outBufLen);
	retval    = GetAdaptersAddresses(AF_UNSPEC, myflags, NULL, addresses, &outBufLen);
	if (retval != ERROR_SUCCESS)
		goto done;

	for (address=addresses; address != NULL; address=address->Next) {
		IP_ADAPTER_UNICAST_ADDRESS *ipaddr   = NULL;
		M_io_net_iface_ips_flags_t     addrflag = 0;

		/* User is not enumerating offline interfaces */
		if (address->OperStatus != IfOperStatusUp) {
			if (!(flags & M_NET_IFACE_IPS_FLAG_OFFLINE))
				continue;
			addrflag |= M_NET_IFACE_IPS_FLAG_OFFLINE;
		}

		/* User is not enumerating loopback interfaces */
		if (address->IfType == IF_TYPE_SOFTWARE_LOOPBACK) {
			if (!(flags & M_NET_IFACE_IPS_FLAG_LOOPBACK))
				continue;
			addrflag |= M_NET_IFACE_IPS_FLAG_LOOPBACK;
		}

		/* Add interface itself, regardless if it has any addresses, but only if we're not restricting to only
		 * those that do have addresses */
		if (!(flags & (M_NET_IFACE_IPS_FLAG_IPV4|M_NET_IFACE_IPS_FLAG_IPV6))) {
			char *name = M_win32_wchar_to_char(address->FriendlyName);
			M_ipentry_add(ips, name, NULL, 0, 0, addrflag);
			M_free(name);
		}

		for (ipaddr=address->FirstUnicastAddress; ipaddr != NULL; ipaddr = ipaddr->Next) {
			const void  *addr     = NULL;
			size_t       addr_len = 0;
			char        *name     = NULL;

			/* User is restricting based on address class */
			if (flags & (M_NET_IFACE_IPS_FLAG_IPV4|M_NET_IFACE_IPS_FLAG_IPV6)) {
				/* user is not enumerating ipv4 */
				if (ipaddr->Address.lpSockaddr->sa_family == AF_INET && !(flags & M_NET_IFACE_IPS_FLAG_IPV4))
					continue;

				/* user is not enumerating ipv6 */
				if (ipaddr->Address.lpSockaddr->sa_family == AF_INET6 && !(flags & M_NET_IFACE_IPS_FLAG_IPV6))
					continue;
			}
			if (ipaddr->Address.lpSockaddr->sa_family == AF_INET) {
				struct sockaddr_in *sockaddr_in = (struct sockaddr_in *)((void *)ipaddr->Address.lpSockaddr);
				addr        = &sockaddr_in->sin_addr;
				addr_len    = 4;
			} else if (ipaddr->Address.lpSockaddr->sa_family == AF_INET6) {
				struct sockaddr_in6 *sockaddr_in6 = (struct sockaddr_in6 *)((void *)ipaddr->Address.lpSockaddr);
				addr         = &sockaddr_in6->sin6_addr;
				addr_len     = 16;
			}
			name = M_win32_wchar_to_char(address->FriendlyName);
			M_ipentry_add(ips, name, addr, addr_len, ipaddr->OnLinkPrefixLength, addrflag);
			M_free(name);
		}
	}

	rv = M_TRUE;
done:
	M_free(addresses);
	return rv;
}

#else
static M_uint8 M_net_count_bits(const unsigned char *addr, size_t addr_len)
{
	size_t  i;
	M_uint8 count = 0;

	for (i=0; i<addr_len; i++) {
		count += M_uint8_popcount(addr[i]);
	}
	return count;
}

static M_bool M_io_net_iface_ips_enumerate(M_io_net_iface_ips_t *ips, M_io_net_iface_ips_flags_t flags)
{
	struct ifaddrs *ifap = NULL;
	struct ifaddrs *ifa  = NULL;

	if (getifaddrs(&ifap) != 0)
		return M_FALSE;

	for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
		M_io_net_iface_ips_flags_t addrflag = 0;
		const void             *addr     = NULL;
		size_t                  addr_len = 0;
		M_uint8                 netmask  = 0;

		/* User is not enumerating offline interfaces */
		if (!(ifa->ifa_flags & IFF_UP) && !(flags & M_NET_IFACE_IPS_FLAG_OFFLINE))
			continue;

		/* User is not enumerating loopback interfaces */
		if (ifa->ifa_flags & IFF_LOOPBACK && !(flags & M_NET_IFACE_IPS_FLAG_LOOPBACK))
			continue;

		/* User is restricting based on address class */
		if (flags & (M_NET_IFACE_IPS_FLAG_IPV4|M_NET_IFACE_IPS_FLAG_IPV6)) {
			/* No interface family */
			if (ifa->ifa_addr == NULL)
				continue;

			/* user is not enumerating ipv4 */
			if (ifa->ifa_addr->sa_family == AF_INET && !(flags & M_NET_IFACE_IPS_FLAG_IPV4))
				continue;

			/* user is not enumerating ipv6 */
			if (ifa->ifa_addr->sa_family == AF_INET6 && !(flags & M_NET_IFACE_IPS_FLAG_IPV6))
				continue;
		}

		/* Extract pointer to ip address */
		if (ifa->ifa_addr != NULL) {
			if (ifa->ifa_addr->sa_family == AF_INET) {
				struct sockaddr_in *sockaddr_in = (struct sockaddr_in *)((void *)ifa->ifa_addr);
				addr        = &sockaddr_in->sin_addr;
				addr_len    = 4;
				sockaddr_in = (struct sockaddr_in *)((void *)ifa->ifa_netmask);
				netmask     = M_net_count_bits((const void *)&sockaddr_in->sin_addr, addr_len);
			} else if (ifa->ifa_addr->sa_family == AF_INET6) {
				struct sockaddr_in6 *sockaddr_in6 = (struct sockaddr_in6 *)((void *)ifa->ifa_addr);
				addr         = &sockaddr_in6->sin6_addr;
				addr_len     = 16;
				sockaddr_in6 = (struct sockaddr_in6 *)((void *)ifa->ifa_netmask);
				netmask      = M_net_count_bits((const void *)&sockaddr_in6->sin6_addr, addr_len);
			}
		}

		/* Set flags */
		if (ifa->ifa_flags & IFF_LOOPBACK)
			addrflag |= M_NET_IFACE_IPS_FLAG_LOOPBACK;
		if (!(ifa->ifa_flags & IFF_UP))
			addrflag |= M_NET_IFACE_IPS_FLAG_OFFLINE;

		M_ipentry_add(ips, ifa->ifa_name, addr, addr_len, netmask, addrflag);
	}
	freeifaddrs(ifap);
	return M_TRUE;
}
#endif


M_io_net_iface_ips_t *M_io_net_iface_ips(int flags)
{
	M_io_net_iface_ips_t *ips = M_malloc_zero(sizeof(*ips));
	struct M_list_callbacks listcb = {
		NULL /* equality */,
		NULL /* duplicate_insert */,
		NULL /* duplicate_copy */,
		M_ipentry_free /* value_free */
	};
	ips->ips = M_list_create(&listcb, M_LIST_NONE);

	if (!M_io_net_iface_ips_enumerate(ips, (M_io_net_iface_ips_flags_t)flags) || M_io_net_iface_ips_count(ips) == 0) {
		M_io_net_iface_ips_free(ips);
		return NULL;
	}

	return ips;
}

size_t M_io_net_iface_ips_count(M_io_net_iface_ips_t *ips)
{
	if (ips == NULL)
		return 0;
	return M_list_len(ips->ips);
}

const char *M_io_net_iface_ips_get_name(M_io_net_iface_ips_t *ips, size_t idx)
{
	const M_ipentry_t *entry = NULL;

	if (ips == NULL)
		return NULL;
	if (idx >= M_io_net_iface_ips_count(ips))
		return NULL;
	entry = M_list_at(ips->ips, idx);
	if (entry == NULL)
		return NULL;
	return entry->name;
}

const char *M_io_net_iface_ips_get_addr(M_io_net_iface_ips_t *ips, size_t idx)
{
	const M_ipentry_t *entry = NULL;

	if (ips == NULL)
		return NULL;
	if (idx >= M_io_net_iface_ips_count(ips))
		return NULL;
	entry = M_list_at(ips->ips, idx);
	if (entry == NULL)
		return NULL;
	return entry->addr;
}

M_uint8 M_io_net_iface_ips_get_netmask(M_io_net_iface_ips_t *ips, size_t idx)
{
	const M_ipentry_t *entry = NULL;

	if (ips == NULL)
		return 0;
	if (idx >= M_io_net_iface_ips_count(ips))
		return 0;
	entry = M_list_at(ips->ips, idx);
	if (entry == NULL)
		return 0;
	return entry->netmask;
}

int M_io_net_iface_ips_get_flags(M_io_net_iface_ips_t *ips, size_t idx)
{
	const M_ipentry_t *entry = NULL;

	if (ips == NULL)
		return 0;
	if (idx >= M_io_net_iface_ips_count(ips))
		return 0;
	entry = M_list_at(ips->ips, idx);
	if (entry == NULL)
		return 0;
	return (int)entry->flags;
}


M_list_str_t *M_io_net_iface_ips_get_ips(M_io_net_iface_ips_t *ips, int flags, const char *name)
{
	size_t        i;
	M_list_str_t *list = NULL;

	/* We need to have at least IPV4 or IPV6 specified */
	if (!(flags & (M_NET_IFACE_IPS_FLAG_IPV4|M_NET_IFACE_IPS_FLAG_IPV6)))
		return NULL;

	/* We're not marking this as a set as it is very unlikedly to have duplicate
	 * IPs */
	list = M_list_str_create(M_LIST_STR_NONE);

	for (i=0; i<M_io_net_iface_ips_count(ips); i++) {
		const M_ipentry_t *entry = M_list_at(ips->ips, i);
		if (entry == NULL)
			continue;

		/* Don't enumerate offline interface addresses by default */
		if (entry->flags & M_NET_IFACE_IPS_FLAG_OFFLINE && !(flags & M_NET_IFACE_IPS_FLAG_OFFLINE))
			continue;

		/* Don't enumerate loopback interface addresses by default */
		if (entry->flags & M_NET_IFACE_IPS_FLAG_LOOPBACK && !(flags & M_NET_IFACE_IPS_FLAG_LOOPBACK))
			continue;

		/* User isn't enumerating ipv4 */
		if (entry->flags & M_NET_IFACE_IPS_FLAG_IPV4 && !(flags & M_NET_IFACE_IPS_FLAG_IPV4))
			continue;

		/* User isn't enumerating ipv6 */
		if (entry->flags & M_NET_IFACE_IPS_FLAG_IPV6 && !(flags & M_NET_IFACE_IPS_FLAG_IPV6))
			continue;

		/* User is wanting to enumerate only a single interface */
		if (!M_str_isempty(name) && !M_str_caseeq(name, entry->name))
			continue;

		/* Match! */
		M_list_str_insert(list, entry->addr);
	}

	if (M_list_str_len(list) == 0) {
		M_list_str_destroy(list);
		list = NULL;
	}
	return list;
}


static char *M_net_sanitize_ipaddr(const char *ipaddr)
{
	unsigned char ipbin[16];
	char          ipstr[64];
	size_t        ipbin_len = 0;

	if (ipaddr == NULL)
		return NULL;

	/* Double convert to sanitize */
	if (!M_io_net_ipaddr_to_bin(ipbin, sizeof(ipbin), ipaddr, &ipbin_len))
		return NULL;

	if (!M_io_net_bin_to_ipaddr(ipstr, sizeof(ipstr), ipbin, ipbin_len))
		return NULL;

	return M_strdup(ipstr);
}

M_list_str_t *M_io_net_iface_ips_get_names(M_io_net_iface_ips_t *ips, int flags, const char *ipaddr)
{
	size_t        i;
	M_list_str_t *list = NULL;
	char         *ipaddr_sanitized = NULL;

	if (!M_str_isempty(ipaddr)) {
		ipaddr_sanitized = M_net_sanitize_ipaddr(ipaddr);
		if (ipaddr_sanitized == NULL)
			return NULL;
	}

	/* We mark this as a set so if the name already exists, it won't be
	 * output more than once */
	list = M_list_str_create(M_LIST_STR_SET|M_LIST_STR_CASECMP);

	for (i=0; i<M_io_net_iface_ips_count(ips); i++) {
		const M_ipentry_t *entry = M_list_at(ips->ips, i);
		if (entry == NULL)
			continue;

		/* Don't enumerate offline interface addresses by default */
		if (entry->flags & M_NET_IFACE_IPS_FLAG_OFFLINE && !(flags & M_NET_IFACE_IPS_FLAG_OFFLINE))
			continue;

		/* Don't enumerate loopback interface addresses by default */
		if (entry->flags & M_NET_IFACE_IPS_FLAG_LOOPBACK && !(flags & M_NET_IFACE_IPS_FLAG_LOOPBACK))
			continue;

		/* User is restricting based on ipv4/ipv6 */
		if (flags & (M_NET_IFACE_IPS_FLAG_IPV4|M_NET_IFACE_IPS_FLAG_IPV6)) {
			/* Interface doesn't support either ipv4 or ipv6 */
			if (!(entry->flags & (M_NET_IFACE_IPS_FLAG_IPV4|M_NET_IFACE_IPS_FLAG_IPV6)))
				continue;

			/* User isn't enumerating ipv4 */
			if (entry->flags & M_NET_IFACE_IPS_FLAG_IPV4 && !(flags & M_NET_IFACE_IPS_FLAG_IPV4))
				continue;

			/* User isn't enumerating ipv6 */
			if (entry->flags & M_NET_IFACE_IPS_FLAG_IPV6 && !(flags & M_NET_IFACE_IPS_FLAG_IPV6))
				continue;
		}

		/* IP address does not match */
		if (!M_str_isempty(ipaddr_sanitized) && !M_str_caseeq(entry->addr, ipaddr_sanitized))
			continue;

		/* Match! */
		M_list_str_insert(list, entry->name);
	}

	if (M_list_str_len(list) == 0) {
		M_list_str_destroy(list);
		list = NULL;
	}

	M_free(ipaddr_sanitized);
	return list;
}

void M_io_net_iface_ips_free(M_io_net_iface_ips_t *ips)
{
	if (ips == NULL)
		return;
	M_list_destroy(ips->ips, M_TRUE);
	M_free(ips);
}

static const M_bitlist_t ifaceflags[] = {
  { M_NET_IFACE_IPS_FLAG_OFFLINE,  "OFFLINE"  },
  { M_NET_IFACE_IPS_FLAG_LOOPBACK, "LOOPBACK" },
  { M_NET_IFACE_IPS_FLAG_IPV4,     "IPV4"     },
  { M_NET_IFACE_IPS_FLAG_IPV6,     "IPV6"     },
  { 0,                             NULL       }
};


char *M_io_net_iface_ips_flags_to_str(int flags)
{
	char *human_flags = NULL;
	if (!M_bitlist_list(&human_flags, M_BITLIST_FLAG_NONE, ifaceflags, (M_uint64)flags, '|', NULL, 0))
		return NULL;

	return human_flags;
}

