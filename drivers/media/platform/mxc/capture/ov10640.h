/*****************************************************************************
*
* Freescale Confidential Proprietary
*
* Copyright (c) 2014 Freescale Semiconductor;
* All Rights Reserved
*
*****************************************************************************
*
* THIS SOFTWARE IS PROVIDED BY FREESCALE "AS IS" AND ANY EXPRESSED OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL FREESCALE OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*
****************************************************************************/
/**
* \file     ov10640.h
* \brief    declarations for ov10640 driver
* \author   Tomas Babinec
* \version  0.1
* \date     26.1.2015
* \note
****************************************************************************/

#ifndef OV10640_H
#define OV10640_H

#include <linux/ioctl.h>

static int max9271_dep(void);
static int max9271_initial_setup(void);
static int max9271_enable_serial_links(void);
static int max9271_disable_serial_links(void);
static int max9271_enable_csi_output(void);

/*****************************************************************************
* MACRO definitions
*****************************************************************************/

#define OV10640_DRV_SUCCESS 0
#define OV10640_DRV_FAILURE -1

// name of the device file
#define OV10640_DEVICE_NAME "ov10640"

// magic number for fDMA driver
#define OV10640_IOC_MAGIC 'h' + 'l'

//#include "ov10640_ioctl.h"

#endif
