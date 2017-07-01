#ifndef __COMMON_H__
#define __COMMON_H__

#define TEST_GENL_NAME "testgenl"
#define TEST_GENL_VERSION 0x1
#define TEST_GENL_GROUP_NAME "testgroup"

enum {
	TEST_CMD_UNSPEC,
	TEST_CMD_ECHO,
	TEST_CMD_NOTIFY,
	__TEST_CMD_MAX,
};

#define TEST_CMD_MAX (__TEST_CMD_MAX - 1)

enum {
	TEST_ATTR_UNSPEC,
	TEST_ATTR_MESSAGE,
	TEST_ATTR_DATA,
	__TEST_ATTR_MAX,
};

#define TEST_ATTR_MAX (__TEST_ATTR_MAX - 1)

#endif /* !__COMMON_H__ */
