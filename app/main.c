#include <stdlib.h>
#include <stdint.h>

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#include "logger.h"
#include "../kmod/common.h"

#define UNUSED __attribute__((unused))

static int recv_genl(struct nlmsghdr *nlh)
{
	char *message;
	uint32_t data;
	struct nlattr *attrs[TEST_ATTR_MAX + 1];

	if (genlmsg_parse(nlh, 0, attrs, TEST_ATTR_MAX, NULL)) {
		LOG_ERROR("genlmsg_parse: couldn't parse genlmsg");
		return NL_SKIP;
	}

	if (!attrs[TEST_ATTR_MESSAGE] || !attrs[TEST_ATTR_DATA]) {
		LOG_ERROR("message or data is NULL");
		return NL_SKIP;
	}

	if (attrs[TEST_ATTR_MESSAGE]) {
		message = nla_get_string(attrs[TEST_ATTR_MESSAGE]);
	}

	if (attrs[TEST_ATTR_DATA]) {
		data = nla_get_u32(attrs[TEST_ATTR_DATA]);
	}

	LOG_INFO("receive from kernel: message=%s, data=%u.", message, data);

	return NL_OK;
}

static int recv_genl_msg(struct nl_msg *msg, UNUSED void *arg)
{
	struct nlmsghdr *nlh = nlmsg_hdr(msg);
	struct genlmsghdr *gnlh = genlmsg_hdr(nlh);

	switch (gnlh->cmd) {
		case TEST_CMD_ECHO:
			LOG_INFO("reply from kernel");
			return recv_genl(nlh);
		case TEST_CMD_NOTIFY:
			LOG_INFO("multicast from kernel");
			return recv_genl(nlh);
		default:
			return NL_SKIP;
	}
}

static int send_echo_info(struct nl_sock *sock, int family_id,
		const char *str, uint32_t data)
{
	struct nl_msg *msg;
	void *head;

	msg = nlmsg_alloc();
	if (!msg) {
		LOG_ERROR("nlmsg_alloc: couldn't alloc nlmsg");
		return -1;
	}

	head = genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, family_id, 0, NLM_F_REQUEST,
			TEST_CMD_ECHO, TEST_GENL_VERSION);
	if (!head) {
		LOG_ERROR("genlmsg_put: build nlmsg header error");
		nlmsg_free(msg);
		return -1;
	}

	NLA_PUT_STRING(msg, TEST_ATTR_MESSAGE, str);
	NLA_PUT_U32(msg, TEST_ATTR_DATA, data);

	return nl_send_auto(sock, msg);

nla_put_failure:
	LOG_ERROR("put attr error");
	nlmsg_free(msg);
	return -1;
}

int main()
{
	struct nl_sock *sock;
	int family_id, group_id;
	int ret;
	char *str = "Hello generic netlink!";
	uint32_t data = 9527;

	sock = nl_socket_alloc();
	if (!sock) {
		LOG_ERROR("nl_socket_alloc: couldn't alloc nl_sock");
		return EXIT_FAILURE;
	}

	ret = genl_connect(sock);
	if (ret < 0) {
		LOG_ERROR("genl_connect: couldn't connect the sock");
		nl_socket_free(sock);
		return EXIT_FAILURE;
	}

	family_id = genl_ctrl_resolve(sock, TEST_GENL_NAME);
	if (family_id < 0) {
		LOG_ERROR("genl_ctrl_resolve: couldn't resolve family id");
		nl_socket_free(sock);
		return EXIT_FAILURE;
	}

	group_id = genl_ctrl_resolve_grp(sock, TEST_GENL_NAME, TEST_GENL_GROUP_NAME);
	if (group_id < 0) {
		LOG_ERROR("genl_ctrl_resolve_grp: couldn't resolve group id");
		nl_socket_free(sock);
		return EXIT_FAILURE;
	}

	ret = nl_socket_add_membership(sock, group_id);
	if (ret < 0) {
		LOG_ERROR("nl_socket_add_membership: couldn't join group");
		nl_socket_free(sock);
		return EXIT_FAILURE;
	}

	nl_socket_modify_cb(sock, NL_CB_MSG_IN, NL_CB_CUSTOM, &recv_genl_msg, sock);

	LOG_INFO("start to send message");

	ret = send_echo_info(sock, family_id, str, data);
	if (ret < 0) {
		LOG_ERROR("send_echo_info: send info error");
		nl_socket_free(sock);
		return EXIT_FAILURE;
	}

	LOG_INFO("start to recv message");

	for (;;) {
		nl_recvmsgs_default(sock);
	}

	nl_socket_free(sock);

	return EXIT_SUCCESS;
}
