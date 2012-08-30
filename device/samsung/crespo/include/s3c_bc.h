/*!****************************************************************************
@File           s3c_bc.h

@Title          s3c_bc kernel driver parameters

@Author         Imagination Technologies
		Samsung Electronics Co. LTD

@Date           03/03/2010

@Copyright      Licensed under the Apache License, Version 2.0 (the "License");
		you may not use this file except in compliance with the License.
		You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

		Unless required by applicable law or agreed to in writing, software
		distributed under the License is distributed on an "AS IS" BASIS,
		WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
		See the License for the specific language governing permissions and
		limitations under the License.

@Platform       Generic

@Description    s3c_bc kernel driver parameters

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-
$Log: s3c_bc.h $
******************************************************************************/
#ifndef __S3C_BC_H__
#define __S3C_BC_H__

#include <linux/ioctl.h>

#define S3C_BC_DEVICE_NAME					"s3c_bc"

#define S3C_BC_DEVICE_ID					0
#define S3C_BC_DEVICE_BUFFER_COUNT			4								/* TODO: Modify this accordingly. */

#define S3C_BC_DEVICE_PHYS_PAGE_SIZE		0x1000							/* 4KB */

typedef struct S3C_BC_ioctl_package_TAG
{
	int inputparam;
	int outputparam;
} S3C_BC_ioctl_package, *PS3C_BC_ioctl_package;

/*!< Nov 2006: according to ioctl-number.txt 'g' wasn't in use. */
#define S3C_BC_IOC_GID      'g'

#define S3C_BC_IOWR(INDEX)  _IOWR(S3C_BC_IOC_GID, INDEX, S3C_BC_ioctl_package)

#define S3C_BC_ioctl_get_physical_base_address		S3C_BC_IOWR(0)

#endif /* __S3C_BC__H__ */
/******************************************************************************
 End of file (s3c_bc.h)
******************************************************************************/
