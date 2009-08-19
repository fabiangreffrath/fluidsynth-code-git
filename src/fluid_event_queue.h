/* FluidSynth - A Software Synthesizer
 *
 * Copyright (C) 2003  Peter Hanappe and others.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 */

#ifndef _FLUID_EVENT_QUEUE_H
#define _FLUID_EVENT_QUEUE_H

#include "fluid_sys.h"
#include "fluid_midi.h"

/**
 * Type of queued event.
 */
enum fluid_event_queue_elem
{
  FLUID_EVENT_QUEUE_ELEM_MIDI,          /** Uses midi field of event value */
  FLUID_EVENT_QUEUE_ELEM_GAIN,          /** Uses dval field of event value */
  FLUID_EVENT_QUEUE_ELEM_POLYPHONY,     /** Uses ival field of event value */
  FLUID_EVENT_QUEUE_ELEM_GEN            /** Uses gen field of event value */
};

/**
 * SoundFont generator set event structure.
 */
typedef struct
{
  int channel;          /** MIDI channel number */
  int param;            /** FluidSynth generator ID */
  float value;          /** Value for the generator (absolute or relative) */
  int absolute;         /** 1 if value is absolute, 0 if relative */
} fluid_event_gen_t;

/**
 * Event queue element structure.
 */
typedef struct
{
  char type;            /** #fluid_event_queue_elem */

  union
  {
    fluid_midi_event_t midi;    /** If type == #FLUID_EVENT_QUEUE_ELEM_MIDI */
    fluid_event_gen_t gen;      /** If type == #FLUID_EVENT_QUEUE_ELEM_GEN */
    double dval;                /** A floating point payload value */
    int ival;                   /** An integer payload value */
  };
} fluid_event_queue_elem_t;

/**
 * Lockless event queue instance.
 */
typedef struct
{
  fluid_event_queue_elem_t *array;  /** Queue array of arbitrary size elements */
  int totalcount;       /** Total count of elements in array */
  int count;            /** Current count of elements */
  int in;               /** Index in queue to store next pushed element */
  int out;              /** Index in queue of next popped element */
  void *synth;          /** Owning fluid_synth_t instance */
} fluid_event_queue_t;


fluid_event_queue_t *fluid_event_queue_new (int count);
void fluid_event_queue_free (fluid_event_queue_t *queue);
fluid_event_queue_elem_t *fluid_event_queue_get_inptr (fluid_event_queue_t *queue);
void fluid_event_queue_next_inptr (fluid_event_queue_t *queue);
fluid_event_queue_elem_t *fluid_event_queue_get_outptr (fluid_event_queue_t *queue);
void fluid_event_queue_next_outptr (fluid_event_queue_t *queue);

#endif /* _FLUID_EVENT_QUEUE_H */
