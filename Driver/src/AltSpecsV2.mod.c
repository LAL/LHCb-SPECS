#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x14522340, "module_layout" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0x42e80c19, "cdev_del" },
	{ 0xc917223d, "pci_bus_read_config_byte" },
	{ 0xc45a9f63, "cdev_init" },
	{ 0x6980fe91, "param_get_int" },
	{ 0xd2037915, "dev_set_drvdata" },
	{ 0xd8e484f0, "register_chrdev_region" },
	{ 0xd691cba2, "malloc_sizes" },
	{ 0xa30682, "pci_disable_device" },
	{ 0xeae3dfd6, "__const_udelay" },
	{ 0x102b9c3, "pci_release_regions" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0xff964b25, "param_set_int" },
	{ 0xffc7c184, "__init_waitqueue_head" },
	{ 0xaf559063, "pci_set_master" },
	{ 0x9f1019bd, "pci_set_dma_mask" },
	{ 0x747f9a8e, "pci_iounmap" },
	{ 0xea147363, "printk" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0xb4390f9a, "mcount" },
	{ 0x6dcaeb88, "per_cpu__kernel_stack" },
	{ 0x859c6dc7, "request_threaded_irq" },
	{ 0xa6d1bdca, "cdev_add" },
	{ 0xb2fd5ceb, "__put_user_4" },
	{ 0x42c8de35, "ioremap_nocache" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x68f7c535, "pci_unregister_driver" },
	{ 0x2044fa9e, "kmem_cache_alloc_trace" },
	{ 0x4d7d27b8, "pci_bus_write_config_byte" },
	{ 0x37a0cba, "kfree" },
	{ 0xc911f7f0, "remap_pfn_range" },
	{ 0x6d090f30, "pci_request_regions" },
	{ 0x5f07b9f3, "__pci_register_driver" },
	{ 0x436c2179, "iowrite32" },
	{ 0xa12add91, "pci_enable_device" },
	{ 0xb02504d8, "pci_set_consistent_dma_mask" },
	{ 0x3302b500, "copy_from_user" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0xe484e35f, "ioread32" },
	{ 0xf20dabd8, "free_irq" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("pci:v00001172d00000005sv*sd*bc*sc*i*");

MODULE_INFO(srcversion, "94CD4F6A3540D097DF5B459");

static const struct rheldata _rheldata __used
__attribute__((section(".rheldata"))) = {
	.rhel_major = 6,
	.rhel_minor = 6,
};
