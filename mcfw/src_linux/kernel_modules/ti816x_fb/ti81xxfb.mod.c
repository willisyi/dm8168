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
	{ 0x9e03639, "module_layout" },
	{ 0xca09bb38, "vps_grpx_get_num_grpx" },
	{ 0x867f7d04, "kmalloc_caches" },
	{ 0xa7fb4150, "vps_grpx_get_ctrl" },
	{ 0xfbc74f64, "__copy_from_user" },
	{ 0xa90c928a, "param_ops_int" },
	{ 0x67c2fa54, "__copy_to_user" },
	{ 0x5eb91978, "framebuffer_release" },
	{ 0x65f99380, "dev_set_drvdata" },
	{ 0xad0e2bc1, "ti81xx_vram_free" },
	{ 0xdd122555, "dma_alloc_writecombine" },
	{ 0x2f83284c, "cfb_fillrect" },
	{ 0x62b72b0d, "mutex_unlock" },
	{ 0xe707d823, "__aeabi_uidiv" },
	{ 0xfa2a45e, "__memzero" },
	{ 0xfe9767af, "cfb_imageblit" },
	{ 0xc949e81c, "dev_err" },
	{ 0xdc798d37, "__mutex_init" },
	{ 0xea147363, "printk" },
	{ 0x7a890c8, "fb_alloc_cmap" },
	{ 0xf27c4cd2, "register_framebuffer" },
	{ 0x328a05f1, "strncpy" },
	{ 0xcedd5d39, "dma_free_coherent" },
	{ 0xe16b893b, "mutex_lock" },
	{ 0xdd43d5f, "vps_fvid2_queue" },
	{ 0xc438efc1, "platform_driver_register" },
	{ 0x98b71c6, "fb_dealloc_cmap" },
	{ 0x20179bfe, "ti81xx_vram_alloc" },
	{ 0xe8929aa8, "kmem_cache_alloc" },
	{ 0x40d53c4b, "framebuffer_alloc" },
	{ 0x37a0cba, "kfree" },
	{ 0x20e55f67, "remap_pfn_range" },
	{ 0xcd882c7, "cfb_copyarea" },
	{ 0x19a7dcf8, "platform_driver_unregister" },
	{ 0x72993c39, "dev_get_drvdata" },
	{ 0xa11c21fc, "unregister_framebuffer" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=vpss";

