/*
   droid vnc server - Android VNC server
   Copyright (C) 2009 Jose Pereira <onaips@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
   */

#include "flinger.h"
#include "screenFormat.h"

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>

#include <binder/IMemory.h>

#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>

#include <ui/DisplayInfo.h>
#include <ui/PixelFormat.h>

using namespace android;

static ScreenshotClient *screenshotClient = NULL;
uint8_t displayOrientation;
sp<IBinder> display;

// Maps orientations from DisplayInfo to ISurfaceComposer
static const uint32_t ORIENTATION_MAP[] = {
	ISurfaceComposer::eRotateNone, // 0 == DISPLAY_ORIENTATION_0
	ISurfaceComposer::eRotate270, // 1 == DISPLAY_ORIENTATION_90
	ISurfaceComposer::eRotate180, // 2 == DISPLAY_ORIENTATION_180
	ISurfaceComposer::eRotate90, // 3 == DISPLAY_ORIENTATION_270
};

extern "C" screenFormat getscreenformat_flinger()
{
	//get format on PixelFormat struct
	//PixelFormat f=screenshotClient->getFormat();

	//PixelFormatInfo pf;
	//getPixelFormatInfo(f,&pf);

	screenFormat format;

	format.bitsPerPixel = bitsPerPixel(screenshotClient->getFormat());
	format.width = screenshotClient->getWidth();
	format.height = screenshotClient->getHeight();
	format.size = format.bitsPerPixel*format.width*format.height/CHAR_BIT;
	// for RGBA8888
	format.redShift = 0;
	format.redMax = 8;
	format.greenShift = 8;
	format.greenMax = 8;
	format.blueShift = 16;
	format.blueMax = 8;
	format.alphaShift = 24;
	format.alphaMax = 8;

	return format;
}


extern "C" int init_flinger()
{
	int errno;

	L("--Initializing gingerbread access method--\n");
	display = SurfaceComposerClient::getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain);
	if (display == NULL) {
		fprintf(stderr, "Unable to get handle for display main\n");
		return -1;
	}

	Vector<DisplayInfo> configs;
	SurfaceComposerClient::getDisplayConfigs(display, &configs);
	int activeConfig = SurfaceComposerClient::getActiveConfig(display);
	if (static_cast<size_t>(activeConfig) >= configs.size()) {
		fprintf(stderr, "Active config %d not inside configs (size %zu)\n",
				activeConfig, configs.size());
		return -1;
	}

	displayOrientation = configs[activeConfig].orientation;

	screenshotClient = new ScreenshotClient();

	status_t ret = screenshotClient->update(display, Rect(), 0, 0, 0, -1U,
			false, ORIENTATION_MAP[displayOrientation]);
	if (ret != NO_ERROR) {
		return -1;
	}

	if (!screenshotClient->getPixels())
		return -1;

	return 0;
}

extern "C" unsigned int *readfb_flinger()
{
	screenshotClient->update(display, Rect(), 0, 0, 0, -1U,
			false, ORIENTATION_MAP[displayOrientation]);
	return (unsigned int*)screenshotClient->getPixels();
}

extern "C" void close_flinger()
{
	free(screenshotClient);
}
