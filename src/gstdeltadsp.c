/*
 * GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2003> David Schleef <ds@schleef.org>
 * Copyright (C) <2013> Robert Yang <decatf@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-plugin
 *
 * Noise sharpening DSP.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m audiotestsrc ! delta_dsp gain=120 ! autoaudiosink
 * ]|
 * </refsect2>
 */
 
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>

#include "gstdeltadsp.h"
#include "delta.h"

GST_DEBUG_CATEGORY_STATIC (gst_delta_dsp_debug);
#define GST_CAT_DEFAULT gst_delta_dsp_debug

enum
{
  ARG_0,
  PROP_GAIN,
  PROP_SILENT
};

/* debug category for fltering log messages */
#define DEBUG_INIT(bla) \
  GST_DEBUG_CATEGORY_INIT (gst_delta_dsp_debug, "delta_dsp", 0, "Delta Dsp");

#define gst_delta_dsp_parent_class parent_class

G_DEFINE_TYPE (GstDeltaDsp, gst_delta_dsp, GST_TYPE_AUDIO_FILTER);

static void gst_delta_dsp_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_delta_dsp_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static gboolean gst_delta_dsp_setup (GstAudioFilter * filter, 
		const GstAudioInfo * info);
static GstFlowReturn gst_delta_dsp_filter (GstBaseTransform * bt,
    GstBuffer * outbuf, GstBuffer * inbuf);
static GstFlowReturn gst_delta_dsp_filter_inplace (GstBaseTransform * base_transform,
    GstBuffer * buf);
static gboolean
		setup_delta_dsp_caps(GstAudioInfo * info, GstDeltaDsp* delta_dsp);
static gboolean 
		set_delta_filter_function (GstDeltaDsp *filter);
static void 
		delta_dsp_tostring(GstDeltaDsp *filter);

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define ALLOWED_CAPS \
    GST_AUDIO_CAPS_MAKE ("{ F32LE, F64LE, S8, S16LE, S32LE }") \
    ", layout = (string) interleaved"
#else
#define ALLOWED_CAPS \
    GST_AUDIO_CAPS_MAKE ("{ F32BE, F64BE, S8, S16BE, S32BE }") \
    ", layout = (string) { interleaved }"
#endif

/* GObject vmethod implementations */

static void
gst_delta_dsp_class_init (GstDeltaDspClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseTransformClass *btrans_class;
  GstAudioFilterClass *audio_filter_class;

  gobject_class = (GObjectClass *) klass;
  btrans_class = (GstBaseTransformClass *) klass;
  audio_filter_class = (GstAudioFilterClass *) klass;

  gobject_class->set_property = gst_delta_dsp_set_property;
  gobject_class->get_property = gst_delta_dsp_get_property;

  g_object_class_install_property (gobject_class, PROP_GAIN,
      g_param_spec_int ("gain", "Gain", "Delta gain to apply",
          0, 200, 100, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));


  /* this function will be called whenever the format changes */
  audio_filter_class->setup = gst_delta_dsp_setup;

  /* here you set up functions to process data (either in place, or from
   * one input buffer to another output buffer); only one is required */
	btrans_class->transform = gst_delta_dsp_filter;
  btrans_class->transform_ip = gst_delta_dsp_filter_inplace;

  GstElementClass *element_class = (GstElementClass*) klass;
  GstAudioFilterClass *audiofilter_class = (GstAudioFilterClass *) klass;
  GstCaps *caps;

  caps = gst_caps_from_string (ALLOWED_CAPS);
  gst_audio_filter_class_add_pad_templates (audiofilter_class, caps);
  gst_caps_unref (caps);
  
  gst_element_class_set_details_simple (element_class,
    "delta",
    "Filter/Effect/Audio",
    "Delta audio filter. Noise sharpening dsp.",
    "Robert Yang decatf@gmail.com");
}

static void
gst_delta_dsp_init (GstDeltaDsp * filter)
{
  GST_DEBUG ("init");

  /* initialize default filter settings */
	filter->negotiated = FALSE;
	filter->gain = 1.00f;
	filter->silent = TRUE;
}

static void
gst_delta_dsp_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstDeltaDsp *filter = GST_DELTA_DSP (object);

  GST_OBJECT_LOCK (filter);	
  switch (prop_id) {
    case PROP_GAIN:
      filter->gain = (gfloat)(g_value_get_int (value)/100.f);
      break;
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (filter);
}

static void
gst_delta_dsp_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstDeltaDsp *filter = GST_DELTA_DSP (object);

  GST_OBJECT_LOCK (filter);
  switch (prop_id) {
    case PROP_GAIN:
      g_value_set_int (value, (gint)(filter->gain*100));
      break;
    case PROP_SILENT:
      g_value_set_boolean (value, (gboolean)filter->silent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (filter);
}

static gboolean
gst_delta_dsp_setup (GstAudioFilter * filter,
    const GstAudioInfo * info)
{
  GstDeltaDsp *delta_dsp;

  delta_dsp = GST_DELTA_DSP (filter);

  /* if any setup needs to be done, do it here */
	GST_OBJECT_LOCK(delta_dsp);
	gboolean res = setup_delta_dsp_caps(info, delta_dsp);
	GST_OBJECT_UNLOCK(delta_dsp);
	if (res == TRUE) {
		GST_OBJECT_LOCK(delta_dsp);
  	res = set_delta_filter_function(delta_dsp);
		GST_OBJECT_UNLOCK(delta_dsp);
	}
	else {
    GST_ELEMENT_ERROR (filter, CORE, NEGOTIATION,
        ("Invalid incoming format"), (NULL));
	}

	if (delta_dsp->silent == FALSE)
		delta_dsp_tostring(delta_dsp);	

	delta_dsp->negotiated = res;

  return delta_dsp->negotiated;
}

/*
 * Set the filter caps
 */
static gboolean
setup_delta_dsp_caps(GstAudioInfo * info, GstDeltaDsp* delta_dsp)
{
	GstAudioFormatInfo *finfo = info->finfo;

	if (finfo != NULL)
	{
		delta_dsp->sign = finfo->flags & GST_AUDIO_FORMAT_FLAG_SIGNED;
		delta_dsp->channels = info->channels;

    delta_dsp->width = finfo->width;
		delta_dsp->datatype_nbytes = delta_dsp->width / 8;

    const gchar* format = finfo->name;

		if (format == NULL)
			return FALSE;
		if (format[0] == 'F')
			delta_dsp->is_int = FALSE;
		else if (format[0] == 'S' || format[0] == 'U')
			delta_dsp->is_int = TRUE;				
		else
			return FALSE;			

		const gchar* be = "BE";
		const gchar* le = "LE";
		if (g_strrstr(format, le) != NULL)
			delta_dsp->little_endian = TRUE;
		else if (g_strrstr(format, be) != NULL)
			delta_dsp->little_endian = FALSE;

		return TRUE;
	}
	return FALSE;
}

/* You may choose to implement either a copying filter or an
 * in-place filter (or both).  Implementing only one will give
 * full functionality, however, implementing both will cause
 * audiofilter to use the optimal function in every situation,
 * with a minimum of memory copies. */

static GstFlowReturn
gst_delta_dsp_filter (GstBaseTransform * base_transform,
    GstBuffer * inbuf, GstBuffer * outbuf)
{
  GstDeltaDsp *delta_dsp;
  delta_dsp = GST_DELTA_DSP (base_transform);

  /* copy the source buffer to destination buffer */
	GstMapInfo src_map_info, dest_map_info;
	gboolean res;
	res = gst_buffer_map(inbuf, &src_map_info, GST_MAP_READ);
	if (res == FALSE) {
		GST_ERROR("inbuf map failed.\n");
		return GST_FLOW_ERROR;
	}
	res = gst_buffer_map(outbuf, &dest_map_info, GST_MAP_WRITE);
	if (res == FALSE) {
		GST_ERROR("outbuf map failed.\n");
		return GST_FLOW_ERROR;
	}

	memcpy (dest_map_info.data, src_map_info.data,
		src_map_info.size);

  /* Apply the filter function */
	if (delta_dsp->process != NULL)
		delta_dsp->process (dest_map_info.data, 
				dest_map_info.size / delta_dsp->datatype_nbytes, 
				delta_dsp->channels, 
				delta_dsp->gain);

	gst_buffer_unmap(inbuf, &src_map_info);
	gst_buffer_unmap(outbuf, &dest_map_info);

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_delta_dsp_filter_inplace (GstBaseTransform * base_transform,
    GstBuffer * buf)
{
  GstDeltaDsp *delta_dsp;
  delta_dsp = GST_DELTA_DSP (base_transform);

  if (G_UNLIKELY (!delta_dsp->negotiated)) {
		GST_ELEMENT_ERROR (delta_dsp, CORE, NEGOTIATION,
				("No format was negotiated"), (NULL));
		return GST_FLOW_NOT_NEGOTIATED;
  }

  /* Apply the filter function */
	GstMapInfo map_info;
	gboolean res = gst_buffer_map(buf, &map_info,  GST_MAP_READ | GST_MAP_WRITE);
	if (res == FALSE) {
		GST_ERROR ("buffer map failed.\n");
		return GST_FLOW_ERROR;
	}

	if (delta_dsp->process != NULL)
		delta_dsp->process (map_info.data, 
				map_info.size / delta_dsp->datatype_nbytes, 
				delta_dsp->channels, 
				delta_dsp->gain);

	gst_buffer_unmap(buf, &map_info);

  return GST_FLOW_OK;
}


static gboolean set_delta_filter_function (GstDeltaDsp *filter) {

	filter->process = NULL;

  if (filter->is_int) {
    if (filter->width == 8) {
      if (filter->sign)
        filter->process = (void*)process8;
      else
        filter->process = (void*)process8u;
    } else if (filter->width == 16) {
      if (filter->sign)
        filter->process = (void*)process16;
      else
        filter->process = (void*)process16u;
    } else if (filter->width == 32) {
      if (filter->sign)
        filter->process = (void*)process32;
      else
        filter->process = (void*)process32u;
    } else if (filter->width == 64) {
      if (filter->sign)
        filter->process = (void*)process64;
      else
        filter->process = (void*)process64u;
    }
  } else {
		if (filter->width == 32)
	    filter->process = (void*)processf;
		else if (filter->width == 64)
	    filter->process = (void*)processd;
  }

	return filter->process != NULL;
}

static void 
delta_dsp_tostring(GstDeltaDsp *filter)
{
	g_print("--------\n");
	g_print("Delta Dsp\n");
	g_print("--------\n");
	g_print("is_int: %s\n", filter->is_int ? "int" : "float");
	g_print("channels: %d\n", filter->channels);
	g_print("little_endian %s\n", filter->little_endian ? "LE" : "BE");
	g_print("signed: %s\n", filter->sign ? "signed" : "unsigned");
	g_print("width %d\n", filter->width);
	g_print("datatype_nbytes %d\n", filter->datatype_nbytes);
	g_print("--------\n");
	g_print("gain %f\n", filter->gain);
	g_print("silent %d\n", filter->silent);
	g_print("--------\n");
}

static gboolean
plugin_init (GstPlugin * delta)
{
  GST_DEBUG_CATEGORY_INIT (gst_delta_dsp_debug, "delta",
      0, "Plugin Template.");

  return gst_element_register (delta, "delta", GST_RANK_NONE,
      GST_TYPE_DELTA_DSP);
}

/* gstreamer looks for this structure to register plugins */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    delta,
    "Delta Dsp",
    plugin_init,
    VERSION, "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
);
