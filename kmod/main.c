#include <linux/module.h>
#include <net/netlink.h>
#include <net/genetlink.h>
#include <linux/ctype.h>

#include "common.h"

static void dump_nlmsg(struct nlmsghdr *nlh)
{
	int i, j, len, datalen;
	unsigned char *data;
	int col = 16;
	struct genlmsghdr *gnlh = nlmsg_data(nlh);
	struct nlattr *nla = genlmsg_data(gnlh);
	int remaining = genlmsg_len(gnlh);

	printk(KERN_DEBUG "===============DEBUG START===============\n");
	printk(KERN_DEBUG "nlmsghdr info (%d):\n", NLMSG_HDRLEN);
	printk(KERN_DEBUG
		"  nlmsg_len\t= %d\n" "  nlmsg_type\t= %d\n"
		"  nlmsg_flags\t= %d\n" "  nlmsg_seq\t= %d\n" "  nlmsg_pid\t= %d\n",
		nlh->nlmsg_len, nlh->nlmsg_type,
		nlh->nlmsg_flags, nlh->nlmsg_seq, nlh->nlmsg_pid);

	printk(KERN_DEBUG "genlmsghdr info (%ld):\n", GENL_HDRLEN);
	printk(KERN_DEBUG "  cmd\t\t= %d\n" "  version\t= %d\n" "  reserved\t= %d\n",
		gnlh->cmd, gnlh->version, gnlh->reserved);

	while (nla_ok(nla, remaining)) {
		printk(KERN_DEBUG "nlattr info (%d):\n", nla->nla_len);
		printk(KERN_DEBUG "  nla_len\t= %d\n" "  nla_type\t= %d\n", nla_len(nla), nla_type(nla));
		printk(KERN_DEBUG "  nla_data:\n");

		datalen = nla_len(nla);
		data = nla_data(nla);

		for (i = 0; i < datalen; i += col) {
			len = (datalen - i < col) ? (datalen - i) : col;

			printk("  ");
			for (j = 0; j < col; j++) {
				if (j < len)
					printk("%02x ", data[i + j]);
				else
					printk("   ");

			}
			printk("\t");
			for (j = 0; j < len; j++) {
				if (j < len)
					if (isprint(data[i + j]))
						printk("%c", data[i + j]);
					else
						printk(".");
				else
					printk(" ");
			}
			printk("\n");
		}

		len = nla_len(nla);
		if (nla_len(nla) < NLMSG_ALIGN(len)) {
			printk(KERN_DEBUG "nlattr pad (%d)\n", NLMSG_ALIGN(len) - len);
		}

		nla = nla_next(nla, &remaining);
	}
	printk(KERN_DEBUG "===============DEBUG END===============\n");
}

static struct genl_family test_family = {
	.id = GENL_ID_GENERATE,
	.name = TEST_GENL_NAME,
	.version = TEST_GENL_VERSION,
	.maxattr = TEST_ATTR_MAX,
};

static struct genl_multicast_group test_group = {
	.name = TEST_GENL_GROUP_NAME,
};

static const struct nla_policy test_policy[TEST_ATTR_MAX + 1] = {
	[TEST_ATTR_MESSAGE] = { .type = NLA_NUL_STRING },
	[TEST_ATTR_DATA] = { .type = NLA_U32 },
};

static struct sk_buff *build_echo_msg(u32 pid, int seq, u8 cmd)
{
	struct sk_buff *skb;
	void *hdr;
	int err;
	char *str = "I am message from kernel!";
	u32 data = 7438;

	skb = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if (!skb) {
		return ERR_PTR(-ENOBUFS);
	}

	hdr = genlmsg_put(skb, pid, seq, &test_family, 0, cmd);
	if (!hdr) {
		nlmsg_free(skb);
		return ERR_PTR(-ENOBUFS);
	}

	NLA_PUT_U32(skb, TEST_ATTR_DATA, data);
	NLA_PUT_STRING(skb, TEST_ATTR_MESSAGE, str);

	err = genlmsg_end(skb, hdr);
	if (err < 0) {
		nlmsg_free(skb);
		return ERR_PTR(err);
	}

	return skb;

nla_put_failure:
	genlmsg_cancel(skb, hdr);
	nlmsg_free(skb);
	return ERR_PTR(-EMSGSIZE);
}

static int test_cmd_echo(struct sk_buff *skb, struct genl_info *info)
{
	struct sk_buff *msg;
	char *message;
	u32 data;

	if (!info->attrs[TEST_ATTR_MESSAGE] || !info->attrs[TEST_ATTR_DATA]) {
		printk(KERN_ERR "require message and data\n");
		return -EINVAL;
	}

	if (info->attrs[TEST_ATTR_MESSAGE]) {
		message = (char *) nla_data(info->attrs[TEST_ATTR_MESSAGE]);
	}

	if (info->attrs[TEST_ATTR_DATA]) {
		data = nla_get_u32(info->attrs[TEST_ATTR_DATA]);
	}

	/* debug info */
	dump_nlmsg(nlmsg_hdr(skb));

	printk(KERN_INFO "receive from user: message=%s, data=%u.\n", message, data);

	msg = build_echo_msg(info->snd_pid, info->snd_seq, TEST_CMD_NOTIFY);
	if (IS_ERR(msg)) {
		return PTR_ERR(msg);
	}

	genlmsg_multicast(msg, 0, test_group.id, GFP_ATOMIC);

	msg = build_echo_msg(info->snd_pid, info->snd_seq, TEST_CMD_ECHO);
	if (IS_ERR(msg)) {
		return PTR_ERR(msg);
	}

	/* debug info */
	dump_nlmsg(nlmsg_hdr(msg));

	return genlmsg_reply(msg, info);
}

static struct genl_ops test_ops[] = {
	{
		.cmd = TEST_CMD_ECHO,
		.doit = test_cmd_echo,
		.policy = test_policy,
	},
};

static int __init test_genl_init(void)
{
	int ret;

	ret = genl_register_family_with_ops(&test_family, test_ops, ARRAY_SIZE(test_ops));
	if (ret < 0) {
		printk(KERN_ERR "genl_register_family_with_ops: couldn't register a family and ops\n");
		return ret;
	}

	ret = genl_register_mc_group(&test_family, &test_group);
	if (ret < 0) {
		printk(KERN_ERR "genl_register_mc_group: couldn't register a multicast group\n");
		genl_unregister_family(&test_family);
		return ret;
	}

	printk(KERN_INFO "test generic netlink module init successful\n");

	return 0;
}

static void __exit test_genl_exit(void)
{
	genl_unregister_mc_group(&test_family, &test_group);
	genl_unregister_family(&test_family);

	printk(KERN_INFO "test generic netlink module exit successful\n");
}

module_init(test_genl_init);
module_exit(test_genl_exit);

MODULE_AUTHOR("houjian");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("generic netlink test module");
