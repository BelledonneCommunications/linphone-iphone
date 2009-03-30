/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006  Simon MORLAT (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef webcam_h
#define webcam_h

#include "mscommon.h"

/**
 * @file mswebcam.h
 * @brief mediastreamer2 mswebcam.h include file
 *
 * This file provide the API needed to manage
 * soundcard filters.
 *
 */

/**
 * @defgroup mediastreamer2_webcam Camera API - manage video capture devices
 * @ingroup mediastreamer2_api
 * @{
 */

struct _MSWebCamManager{
	MSList *cams;
};

/**
 * Structure for webcam manager object.
 * @var MSWebCamManager
 */
typedef struct _MSWebCamManager MSWebCamManager;


struct _MSWebCam;

typedef void (*MSWebCamDetectFunc)(MSWebCamManager *obj);
typedef void (*MSWebCamInitFunc)(struct _MSWebCam *obj);
typedef void (*MSWebCamUninitFunc)(struct _MSWebCam *obj);
typedef struct _MSFilter * (*MSWebCamCreateReaderFunc)(struct _MSWebCam *obj);

struct _MSWebCamDesc{
	const char *driver_type;
	MSWebCamDetectFunc detect;
	MSWebCamInitFunc init;
	MSWebCamCreateReaderFunc create_reader;
	MSWebCamUninitFunc uninit;
};

/**
 * Structure for sound card description object.
 * @var MSWebCamDesc
 */
typedef struct _MSWebCamDesc MSWebCamDesc;

struct _MSWebCam{
	MSWebCamDesc *desc;
	char *name;
	char *id;
	void *data;
};

/**
 * Structure for sound card object.
 * @var MSWebCam
 */
typedef struct _MSWebCam MSWebCam;

#ifdef __cplusplus
extern "C"{
#endif

/**
 * Retreive a webcam manager object.
 *
 * Returns: MSWebCamManager if successfull, NULL otherwise.
 */
MSWebCamManager * ms_web_cam_manager_get(void);

/**
 * Destroy a sound card manager object.
 *
 */
void ms_web_cam_manager_destroy(void);

/**
 * Retreive a sound card object based on its name.
 *
 * @param m    A sound card manager containing sound cards.
 * @param id   A name for card to search.
 *
 * Returns: MSWebCam if successfull, NULL otherwise.
 */
MSWebCam * ms_web_cam_manager_get_cam(MSWebCamManager *m, const char *id);

/**
 * Retreive the default sound card object.
 *
 * @param m    A sound card manager containing sound cards.
 *
 * Returns: MSWebCam if successfull, NULL otherwise.
 */
MSWebCam * ms_web_cam_manager_get_default_cam(MSWebCamManager *m);

/**
 * Retreive the list of webcam objects.
 *
 * @param m    A webcam manager containing webcams.
 *
 * Returns: MSList of cards if successfull, NULL otherwise.
 */
const MSList * ms_web_cam_manager_get_list(MSWebCamManager *m);

/**
 * Add a sound card object in a webcam  manager's list.
 *
 * @param m    A webcam  manager containing webcams
 * @param c    A web cam object.
 *
 */
void ms_web_cam_manager_add_cam(MSWebCamManager *m, MSWebCam *c);

/**
 * Add a sound card object on top of list of the webcam  manager's list.
 *
 * @param m    A webcam  manager containing webcams
 * @param c    A web cam object.
 *
 */
void ms_web_cam_manager_prepend_cam(MSWebCamManager *m, MSWebCam *c);


/**
 * Register a sound card description in a sound card manager.
 *
 * @param m      A sound card manager containing sound cards.
 * @param desc   A sound card description object.
 *
 */
void ms_web_cam_manager_register_desc(MSWebCamManager *m, MSWebCamDesc *desc);

/**
 * Create an INPUT filter based on the selected camera.
 *
 * @param obj      A sound card object.
 *
 * Returns: A MSFilter if successfull, NULL otherwise.
 */
struct _MSFilter * ms_web_cam_create_reader(MSWebCam *obj);

/**
 * Create a new webcam object.
 *
 * @param desc   A webcam description object.
 *
 * Returns: MSWebCam if successfull, NULL otherwise.
 */
MSWebCam * ms_web_cam_new(MSWebCamDesc *desc);

/**
 * Destroy webcam object.
 *
 * @param obj   A MSWebCam object.
 */
void ms_web_cam_destroy(MSWebCam *obj);


/**
 * Retreive a webcam's driver type string.
 *
 * Internal driver types are either: "V4L V4LV2"
 *
 * @param obj   A webcam object.
 *
 * Returns: a string if successfull, NULL otherwise.
 */
const char *ms_web_cam_get_driver_type(const MSWebCam *obj);

/**
 * Retreive a webcam's name.
 *
 * @param obj   A webcam object.
 *
 * Returns: a string if successfull, NULL otherwise.
 */
const char *ms_web_cam_get_name(const MSWebCam *obj);

/**
 * Retreive webcam's id: ($driver_type: $name).
 *
 * @param obj    A webcam object.
 *
 * Returns: A string if successfull, NULL otherwise.
 */
const char *ms_web_cam_get_string_id(MSWebCam *obj);


/*specific methods for static image:*/

void ms_static_image_set_default_image(const char *path);

#ifdef __cplusplus
}
#endif

/** @} */

#endif
