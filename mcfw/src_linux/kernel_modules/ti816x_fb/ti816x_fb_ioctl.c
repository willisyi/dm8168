/*
 * linux/drivers/video/ti81xx/ti81xxfb/ti81xxfb_ioctl.c
 *
 * Copyright (C) 2009 Texas Instruments
 * Author: Yihe Hu <yihehu@ti.com>
 * Modify: phoong(UDWorks)
 *
 * Some codes and ideals are from TI OMAP2 by Tomi Valkeinen
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*----------------------------------------------------------------------------
 Defines referenced header files
-----------------------------------------------------------------------------*/
#include <linux/string.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/ti81xxfb.h>
#include <linux/omapfb.h>
#include <plat/ti81xx_ram.h>

#include "fbpriv.h"

/*----------------------------------------------------------------------------
 Definitions and macro
-----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 Declares variables
-----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 Declares a function prototype
-----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 Local function
-----------------------------------------------------------------------------*/
static void set_trans_key(struct fb_info *fbi,
			  struct ti81xxfb_region_params *rp,
			  struct vps_grpxregionparams *regp)
{
	regp->transenable = 1;
	if (rp->transen == TI81XXFB_FEATURE_DISABLE)
		regp->transenable = 0;

	if (rp->transen) {
		if (rp->transcolor > 0xFFFFFF)
			regp->transcolorrgb24 = 0xFFFFFF;
		else
			regp->transcolorrgb24 = rp->transcolor;

		regp->transtype = rp->transtype;
	}
}

static void set_boundbox(struct fb_info *fbi,
			 struct ti81xxfb_region_params *rp,
			 struct vps_grpxregionparams *regp)
{
	if (rp->bben)
		regp->bbalpha = rp->bbalpha;

	regp->bbenable = 1;
	if (rp->bben == TI81XXFB_FEATURE_DISABLE)
		regp->bbenable = 0;
}

static void set_blend(struct fb_info *fbi,
		      struct ti81xxfb_region_params *rp,
		      struct vps_grpxregionparams *regp)
{

	if (rp->blendtype == TI81XXFB_BLENDING_GLOBAL)
		regp->blendalpha = rp->blendalpha;

	regp->blendtype = (u32)rp->blendtype;

}

static int ti81xxfb_set_region_params(struct fb_info *fbi,
				     struct ti81xxfb_region_params *regparam)
{
	struct ti81xxfb_info *tfbi = FB2TFB(fbi);
	struct ti81xxfb_device *fbdev = tfbi->fbdev;
	struct vps_grpx_ctrl *gctrl;
	struct vps_grpxregionparams regp;
	//struct fb_var_screeninfo *var = &fbi->var;
	int i, r = 0;

	dprintk("--- ti81xxfb set_regparams\n");

	if (regparam->priority >= TI81XXFB_MAX_PRIORITY) {
		dev_err(fbdev->dev, "priority out of range");
		return -EINVAL;
	}

	ti81xxfb_lock(tfbi);

	for(i=0; i<tfbi->num_use_grpx; i++)
	{
		gctrl = tfbi->gctrl[i];
		gctrl->get_regparams(gctrl, &regp);

		/* only update if it are not same*/
		regp.disppriority = regparam->priority;
		regp.firstregion = 1;
		if (regparam->firstregion == TI81XXFB_FEATURE_DISABLE)
			regp.firstregion = 0;

		regp.lastregion = 1;
		if (regparam->lastregion == TI81XXFB_FEATURE_DISABLE)
			regp.lastregion = 0;

		regp.scenable = 1;
		if (regparam->scalaren == TI81XXFB_FEATURE_DISABLE)
			regp.scenable = 0;

		regp.stencilingenable = 1;
		if (regparam->stencilingen == TI81XXFB_FEATURE_DISABLE)
			regp.stencilingenable = 0;

		set_boundbox(fbi, regparam, &regp);
		set_blend(fbi, regparam, &regp);
		set_trans_key(fbi, regparam, &regp);

		r = gctrl->check_params(gctrl, &regp, regparam->ridx);

		if (r == 0)
			r = gctrl->set_regparams(gctrl, &regp);

		if (0 == r) {
			dprintk("set params handle %x\n", (u32)gctrl->handle);
			if (gctrl->gstate.isstarted)
				r = vps_fvid2_queue(gctrl->handle,
						    (struct fvid2_framelist *)
							gctrl->frmls_phy,
						    0);
		}

		//# phoong - for apply : to do - need update when only not same
		gctrl->stop(gctrl);
		gctrl->start(gctrl);
	}
	ti81xxfb_unlock(tfbi);

	if (r)
		dev_err(fbdev->dev, "setup region failed %d\n", r);

	return r;
}

static int ti81xxfb_get_region_params(struct fb_info *fbi,
				     struct ti81xxfb_region_params *regparam)
{

	struct ti81xxfb_info *tfbi = FB2TFB(fbi);
	struct vps_grpx_ctrl *gctrl;
	struct vps_grpxregionparams regp;
	int i, r = 0;

	ti81xxfb_lock(tfbi);

	//for(i=0; i<tfbi->num_use_grpx; i++)
	i=0;
	{
		gctrl = tfbi->gctrl[i];
		r = gctrl->get_regparams(gctrl, &regp);

		if (0 == r)
		{
			regparam->ridx = 0;
			regparam->bbalpha = regp.bbalpha;

			regparam->bben = TI81XXFB_FEATURE_DISABLE;
			if (regp.bbenable == 1)
				regparam->bben = TI81XXFB_FEATURE_ENABLE;
			regparam->blendalpha = regp.blendalpha;
			regparam->blendtype = regp.blendtype;
			regparam->pos_x = regp.regionposx;
			regparam->pos_y = regp.regionposy;
			regparam->priority = regp.disppriority;

			regparam->firstregion = TI81XXFB_FEATURE_DISABLE;
			if (regp.firstregion == 1)
				regparam->firstregion = TI81XXFB_FEATURE_ENABLE;

			regparam->lastregion = TI81XXFB_FEATURE_DISABLE;
			if (regp.lastregion == 1)
				regparam->lastregion = TI81XXFB_FEATURE_ENABLE;

			regparam->scalaren = TI81XXFB_FEATURE_DISABLE;
			if (regp.scenable == 1)
				regparam->scalaren = TI81XXFB_FEATURE_ENABLE;

			regparam->stencilingen = TI81XXFB_FEATURE_DISABLE;
			if (regp.stencilingenable == 1)
				regparam->stencilingen = TI81XXFB_FEATURE_ENABLE;

			regparam->transcolor = regp.transcolorrgb24;
			regparam->transtype = regp.transtype;
			regparam->transen = TI81XXFB_FEATURE_DISABLE;
			if (regp.transenable == 1)
				regparam->transen = TI81XXFB_FEATURE_ENABLE;
		}
	}

    ti81xxfb_unlock(tfbi);

	return r;
}

static int ti81xxfb_set_scparams(struct fb_info *fbi,
				struct ti81xxfb_scparams *scp)
{
	int r = 0;
	struct ti81xxfb_info *tfbi = FB2TFB(fbi);
	struct vps_grpx_ctrl *gctrl = tfbi->gctrl;
	struct vps_grpxscparams  gscp;

	dprintk("ti81xxfb_set_scparams\n");

	ti81xxfb_lock(tfbi);
	/* check the scparamter first */

	gscp.horfineoffset = 0;
	gscp.verfineoffset = 0;
	gscp.sccoeff = NULL;
	gscp.inwidth = scp->inwidth;
	gscp.inheight = scp->inheight;
	gscp.outwidth = scp->outwidth;
	gscp.outheight = scp->outheight;
	gscp.sccoeff = scp->coeff;
	r = gctrl->set_scparams(gctrl, &gscp);
	if (0 == r)
		r = gctrl->apply_changes(gctrl);

	ti81xxfb_unlock(tfbi);

	return r;
}

static int ti81xxfb_get_scparams(struct fb_info *fbi,
				struct ti81xxfb_scparams *scp)
{
	int r = 0;
	struct ti81xxfb_info *tfbi = FB2TFB(fbi);
	struct vps_grpx_ctrl *gctrl = tfbi->gctrl;
	struct vps_grpxscparams gscp;

	ti81xxfb_lock(tfbi);
	r = gctrl->get_scparams(gctrl, &gscp);
	if (r == 0) {
		scp->inwidth = gscp.inwidth;
		scp->inheight = gscp.inheight;
		scp->outwidth = gscp.outwidth;
		scp->outheight = gscp.outheight;
	}
	ti81xxfb_unlock(tfbi);

	return r;
}

/*****************************************************************************
* @brief	ti81xxfb ioctl function
* @section	DESC Description
*	- desc
*****************************************************************************/
int ti81xxfb_ioctl(struct fb_info *fbi, unsigned int cmd,
		  unsigned long arg)
{
	union {
		struct ti81xxfb_region_params regparams;
		struct ti81xxfb_scparams scparams;
		struct ti81xxfb_mem_info   minfo;
		struct ti81xxfb_stenciling_params stparams;
		int mirror;
		int size;
		int offset;
	} param;
	struct ti81xxfb_info *tfbi = FB2TFB(fbi);
	struct ti81xxfb_device *fbdev = tfbi->fbdev;
	int r = 0;

	dprintk("--- ti81xxfb ioctl (cmd:0x%x)\n", cmd);

	if (!tfbi->mreg.size) {
		dev_err(fbdev->dev, "alloce fb memory first\n");
		return -EINVAL;
	}

	switch (cmd)
	{
		case TIFB_SET_PARAMS:
			dprintk("ioctl SET_PARAMS\n");

			if (copy_from_user(&param.regparams, (void __user *)arg,
					   sizeof(struct ti81xxfb_region_params)))
				r = -EFAULT;
			else
				r = ti81xxfb_set_region_params(fbi, &param.regparams);

			break;

		case TIFB_GET_PARAMS:
			dprintk("ioctl GET_PARAMS\n");
			r = ti81xxfb_get_region_params(fbi, &param.regparams);

			if (r < 0)
				break;

			if (copy_to_user((void __user *)arg, &param.regparams,
					 sizeof(param.regparams)))
				r = -EFAULT;
			break;

		case TIFB_SET_SCINFO:
			dprintk("ioctl SET_SCINFO\n");
			if (copy_from_user(&param.scparams, (void __user *)arg,
					   sizeof(param.scparams))) {
				r = -EFAULT;
				break;
			}
			if (param.scparams.coeff)
			{
				struct ti81xxfb_coeff *coeff =
					kzalloc(sizeof(struct ti81xxfb_coeff),
						GFP_KERNEL);

				dprintk("loading app's coeff\n");
				if (copy_from_user(coeff,
						   (void __user *)param.scparams.coeff,
						    sizeof(struct ti81xxfb_coeff))) {
					kfree(coeff);
					r = -EFAULT;
					break;
				}
				param.scparams.coeff = coeff;

			}
			r = ti81xxfb_set_scparams(fbi, &param.scparams);
			kfree(param.scparams.coeff);
			break;

		case  TIFB_GET_SCINFO:
			dprintk("ioctl GET_SCINFO");
			r = ti81xxfb_get_scparams(fbi, &param.scparams);
			if (r == 0) {
				struct ti81xxfb_scparams scp;
				/*keep the coeff pointer in the strucutre and
				  do not change it*/
				if (copy_from_user(&scp,
						(void __user *)arg,
						sizeof(param.scparams))) {
					r = -EFAULT;
					break;
				}
				/*store the coeff back to app*/
				param.scparams.coeff = scp.coeff;

				if (copy_to_user((void __user *)arg, &param.scparams,
						 sizeof(param.scparams)))
					r = -EFAULT;
			}
			break;

		case TIFB_CLOSE:
			dprintk("ioctl TIFB_CLOSE\n");
			ti81xxfb_release(fbi, 0);
			break;

		default:
			dev_err(FB2TFB(fbi)->fbdev->dev, "Unknown ioctl 0x%x\n", cmd);
			r = -EFAULT;
			break;
	}

	if (r < 0)
		dprintk("ioctl 0x%x failed: %d\n", cmd , r);

	return 0;
}
