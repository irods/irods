/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file	sdssImgCutoutMS.hMS.h
 *
 * @brief	Declarations for the SDSS Image Cutout microservices.
 */



#ifndef SDSSIMGCUTOUTMS.hMS_H
#define SDSSIMGCUTOUTMS.hMS_H

#include "rods.h"
#include "reGlobalsExtern.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"



int
msiSdssImgCutout_GetJpeg(msParam_t* inRaParam, 
		 msParam_t* inDecParam, 
		 msParam_t* inScaleParam,
		 msParam_t* inWidthParam,
		 msParam_t* inHeightParam,
		 msParam_t* inOptParam,
		 msParam_t* outImgParam, 
		 ruleExecInfo_t* rei );

#endif	/* SDSSIMGCUTOUTMS.hMS_H */
