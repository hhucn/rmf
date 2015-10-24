#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
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
	{ 0x5c538fba, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x149c7752, __VMLINUX_SYMBOL_STR(param_ops_uint) },
	{ 0x62a79a6c, __VMLINUX_SYMBOL_STR(param_ops_charp) },
	{ 0x354c7b21, __VMLINUX_SYMBOL_STR(single_release) },
	{ 0xebb3008e, __VMLINUX_SYMBOL_STR(seq_read) },
	{ 0x4901825, __VMLINUX_SYMBOL_STR(seq_lseek) },
	{ 0xf4f8845c, __VMLINUX_SYMBOL_STR(remove_proc_entry) },
	{ 0x87f09b3b, __VMLINUX_SYMBOL_STR(proc_create_data) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x5a7cddb9, __VMLINUX_SYMBOL_STR(neigh_lookup) },
	{ 0xa7a89b28, __VMLINUX_SYMBOL_STR(arp_tbl) },
	{ 0x91831d70, __VMLINUX_SYMBOL_STR(seq_printf) },
	{ 0xff77b889, __VMLINUX_SYMBOL_STR(dev_get_by_name) },
	{ 0x1aecdd14, __VMLINUX_SYMBOL_STR(init_net) },
	{ 0x51f36dac, __VMLINUX_SYMBOL_STR(single_open) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "BC43AC0D8B7ADA8C77B754D");
