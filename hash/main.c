#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>


#include <rte_eal.h>
#include <rte_common.h>
#include <rte_lcore.h>
#include <rte_hash.h>

#if defined(RTE_ARCH_X86) || defined(RTE_MACHINE_CPUFLAG_CRC32)
#define EM_HASH_CRC 1
#endif

#ifdef EM_HASH_CRC
#include <rte_hash_crc.h>
#define DEFAULT_HASH_FUNC       rte_hash_crc
#else
#include <rte_jhash.h>
#define DEFAULT_HASH_FUNC       rte_jhash
#endif


#define MAX_ENTRY	64
#define ARRAY_SIZE	6

static inline uint32_t
ipv4_hash_crc(const void *data, __rte_unused uint32_t data_len,
		uint32_t init_val)
{
	const uint32_t *srcip;

	srcip = data;

#ifdef EM_HASH_CRC
	init_val = rte_hash_crc_4byte(*srcip, init_val);
#else
	init_val = rte_jhash_1word(*srcip, init_val);
#endif

	return init_val;
}

static inline struct rte_hash *
setup_hash(int socketid)
{
	struct rte_hash_parameters parms;

	parms.name = "myhash";
	parms.entries = MAX_ENTRY;
	parms.key_len = sizeof(int);
	parms.hash_func = ipv4_hash_crc;
	parms.hash_func_init_val = 0;
	parms.socket_id = socketid;

	return rte_hash_create(&parms);
}

int main(int argc, char *argv[]) {
	int ret;
	unsigned int lcore_id, socketid;
	struct rte_hash *h = NULL;
	int keys[ARRAY_SIZE] = {1, 2, 3, 4, 5, 6};
	int vals[ARRAY_SIZE] = {6, 5, 4, 3, 2, 1};
	int i, *v;
	const int *k;
	uint32_t n;

	ret = rte_eal_init(argc, argv);
	if(ret < 0)
		rte_exit(EXIT_FAILURE, "无法初始化eal.\n");

	// 使用主核socket建立hash table
	// 如何在多个socket上建立hash table，锁机制？？？？？？？
	lcore_id = rte_get_master_lcore();
	socketid = rte_lcore_to_socket_id(lcore_id);
	
	h = setup_hash(socketid);
	if(h == NULL)
		rte_exit(EXIT_FAILURE, "hash 初始化失败.\n");

	for(i=0; i<ARRAY_SIZE; i++) 
		rte_hash_add_key_data(h, (const void *)&keys[i], 
					(void *)&vals[i]);

	printf("主核进行key查询\n");
	for(i=0; i<ARRAY_SIZE; i++) {
		printf("key:%d\t", keys[i]);
		rte_hash_lookup_data(h, (const void *)&keys[i], 
					(void **)&v);
		printf("val:%d\n", *v);
	}

	printf("遍历hash表\n");
	n = 0;
	while((ret = rte_hash_iterate(h, (const void **)&k, 
				(void **)&v, &n)) >= 0) {
		printf("key:%d\t", *k);
		printf("val:%d\n", *v);
	}

	rte_hash_free(h);

	return 0;
}
